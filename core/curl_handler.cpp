#include "stdafx.h"

#include <event2/thread.h>

#include "core.h"
#include "curl_context.h"
#include "network_log.h"

#include "curl_handler.h"

namespace
{
    const int MAX_NORMAL_TRANSMISSIONS = 4;
    const int MAX_HIGH_TRANSMISSIONS = 6;
    const int MAX_HIGHEST_TRANSMISSIONS = 8;
}

namespace core
{
    void start_new_job(curl_handler* _curl_handler)
    {
        if (_curl_handler->keep_working_)
        {
            event_active(_curl_handler->start_task_event_, 0, 0);
        }
    }

    void finish_job(curl_handler* _curl_handler, CURL* handle, CURLcode _result)
    {
        auto it = _curl_handler->connections_.find(handle);
        assert(it != _curl_handler->connections_.end());

        if (it != _curl_handler->connections_.end())
        {
            auto connection = it->second.get();

            boost::apply_visitor(curl_handler::completion_visitor(_result), connection->completion_handler_);

            _curl_handler->connections_.erase(it);
        }
    }

    void check_multi_info(curl_handler* _curl_handler)
    {
        int messages_left = 0;
        while (auto message = curl_multi_info_read(_curl_handler->multi_handle_, &messages_left))
        {
            if (message->msg == CURLMSG_DONE)
            {
                const auto easy_handle = message->easy_handle;
                const auto result = message->data.result;

                const auto error = curl_multi_remove_handle(_curl_handler->multi_handle_, easy_handle);
                assert(!error);
                (void*) error; // supress warning

                finish_job(_curl_handler, easy_handle, result);

                start_new_job(_curl_handler);
            }
        }
    }

    void add_socket(curl_socket_t _socket, CURL* _easy_handle, int _action, curl_handler* _curl_handler)
    {
        const auto connection_it = _curl_handler->connections_.find(_easy_handle);
        assert(connection_it != _curl_handler->connections_.end());

        const auto connection = connection_it->second.get();

        connection->update(_socket, _action);

        const auto error = curl_multi_assign(_curl_handler->multi_handle_, _socket, connection);
        assert(!error);
        (void*) error; // supress warning
    }

    int socket_callback(CURL* _easy_handle, curl_socket_t _socket, int _what, void* _curl_handler_ptr, void* _connection_ptr)
    {
        const auto handler = static_cast<curl_handler*>(_curl_handler_ptr);

        const auto connection = static_cast<curl_handler::connection_context*>(_connection_ptr);

        if (_what == CURL_POLL_REMOVE)
        {
            connection->free_event();
        }
        else if (connection == NULL)
        {
            add_socket(_socket, _easy_handle, _what, handler);
        }
        else
        {
            connection->update(_socket, _what);
        }

        return 0;
    }

    int timer_callback(CURLM* _multi_handle, milliseconds_t _timeout, void* _curl_handler_ptr)
    {
        const auto handler = static_cast<curl_handler*>(_curl_handler_ptr);

        if (_timeout == -1)
        {
            evtimer_del(handler->timer_event_);
        }
        else if (_timeout == 0)
        {
            const auto error = curl_multi_socket_action(handler->multi_handle_, CURL_SOCKET_TIMEOUT, 0, &handler->running_);
            assert(!error);
            (void*) error; // supress warning
        }
        else
        {
            const auto tv = curl_handler::make_timeval(_timeout);
            evtimer_add(handler->timer_event_, &tv);
        }

        return 0;
    }

    void event_callback(int _socket, short _kind, void* _connection_ptr)
    {
        const auto connection = static_cast<curl_handler::connection_context*>(_connection_ptr);
        const auto handler = connection->curl_handler_;

        const auto tv = curl_handler::make_timeval(connection->timeout_);
        event_del(connection->timeout_event_);
        event_add(connection->timeout_event_, &tv);

        int action = 0;
        if (_kind & EV_READ)
            action |= CURL_CSELECT_IN;
        if (_kind & EV_WRITE)
            action |= CURL_CSELECT_OUT;

        const auto error = curl_multi_socket_action(handler->multi_handle_, _socket, action, &handler->running_);
        assert(!error);
        (void*) error; // supress warning

        check_multi_info(handler);

        if (handler->running_ <= 0)
        {
            if (evtimer_pending(handler->timer_event_, NULL))
                evtimer_del(handler->timer_event_);
        }
    }

    void event_timer_callback(evutil_socket_t /*_descriptor*/, short /*_flags*/, void* _curl_handler_ptr)
    {
        const auto handler = static_cast<curl_handler*>(_curl_handler_ptr);

        const auto error = curl_multi_socket_action(handler->multi_handle_, CURL_SOCKET_TIMEOUT, 0, &handler->running_);
        assert(!error);
        (void*) error; // supress warning

        check_multi_info(handler);
    }

    void event_timeout_callback(evutil_socket_t /*_descriptor*/, short /*_flags*/, void* _connection_ptr)
    {
        const auto connection = static_cast<curl_handler::connection_context*>(_connection_ptr);

        const auto error = curl_multi_remove_handle(connection->curl_handler_->multi_handle_, connection->easy_handle_);
        assert(!error);
        (void*) error; // supress warning

        const auto handler = connection->curl_handler_;

        finish_job(handler, connection->easy_handle_, CURLE_OPERATION_TIMEDOUT);

        start_new_job(handler);
    }

    void start_task_callback(evutil_socket_t /*_descriptor*/, short /*_flags*/, void* _curl_handler_ptr)
    {
        const auto handler = static_cast<curl_handler*>(_curl_handler_ptr);

        typedef std::unique_ptr<curl_handler::connection_context> connection_ptr;
        std::vector<connection_ptr> to_process;

        {
            std::lock_guard<std::mutex> lock(handler->jobs_mutex_);

            auto transmissions = handler->connections_.size();

            while (!handler->pending_jobs_.empty())
            {
                const auto& job = handler->pending_jobs_.top();

                if (job.priority_ >= highest_priority && transmissions > MAX_HIGHEST_TRANSMISSIONS)
                    break;

                if (job.priority_ >= high_priority && transmissions > MAX_HIGH_TRANSMISSIONS)
                    break;

                if (job.priority_ >= default_priority && transmissions > MAX_NORMAL_TRANSMISSIONS)
                    break;

                ++transmissions;

                const auto completion_handler = job.completion_;
                const auto timeout = job.timeout_;
                const auto easy_handle = job.handle_;

                auto connection = std::unique_ptr<curl_handler::connection_context>(
                    new curl_handler::connection_context(timeout, handler, easy_handle, completion_handler));
                to_process.push_back(std::move(connection));

                handler->pending_jobs_.pop();
            }
        }

        for (auto&& connection : to_process)
        {
            const auto easy_handle = connection->easy_handle_;

            handler->connections_[easy_handle] = std::move(connection);

            const auto result = curl_multi_add_handle(handler->multi_handle_, easy_handle);

            if (result != CURLM_OK)
            {
                finish_job(handler, easy_handle, CURLE_FAILED_INIT);
            }
        }
    }
}

timeval core::curl_handler::make_timeval(milliseconds_t _timeout)
{
    struct timeval tv;
    tv.tv_sec = _timeout / 1000;
    tv.tv_usec = _timeout - tv.tv_sec * 1000;
    return tv;
}

core::curl_handler& core::curl_handler::instance()
{
    // §6.7 [stmt.dcl] p4
    // If control enters the declaration concurrently while the variable is being initialized,
    // the concurrent execution shall wait for completion of the initialization.
    static curl_handler handler;
    return handler;
}

core::curl_handler::curl_handler()
{
#ifdef _WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif
}

void core::curl_handler::init()
{
#ifdef _WIN32
    curl_global_init(CURL_GLOBAL_ALL);
#else
    curl_global_init(CURL_GLOBAL_SSL);
#endif
}

void core::curl_handler::cleanup()
{
    curl_multi_cleanup(multi_handle_);

    curl_global_cleanup();
}

void core::curl_handler::start()
{
    multi_handle_ = curl_multi_init();

    curl_multi_setopt(multi_handle_, CURLMOPT_SOCKETFUNCTION, socket_callback);
    curl_multi_setopt(multi_handle_, CURLMOPT_SOCKETDATA, this);

    curl_multi_setopt(multi_handle_, CURLMOPT_TIMERFUNCTION, timer_callback);
    curl_multi_setopt(multi_handle_, CURLMOPT_TIMERDATA, this);

    event_base_ = event_base_new();
    timer_event_ = evtimer_new(event_base_, event_timer_callback, this);
    start_task_event_ = event_new(event_base_, -1, EV_PERSIST, start_task_callback, this);

    keep_working_ = true;

    event_loop_thread_ = std::thread([this]()
    {
        event_base_loop(event_base_, EVLOOP_NO_EXIT_ON_EMPTY);
    });
}

void core::curl_handler::stop()
{
    keep_working_ = false;

    event_base_loopbreak(event_base_);

    event_loop_thread_.join();

    for (auto& it : connections_)
    {
        const auto error = curl_multi_remove_handle(multi_handle_, it.first);
        assert(!error);
        (void*) error; // supress warning

        const auto& connection = it.second;
        boost::apply_visitor(completion_visitor(CURLE_ABORTED_BY_CALLBACK), connection->completion_handler_);
    }

    connections_.clear();

    while (!pending_jobs_.empty())
    {
        auto& job = pending_jobs_.top();
        boost::apply_visitor(completion_visitor(CURLE_ABORTED_BY_CALLBACK), job.completion_);
        pending_jobs_.pop();
    }
}

CURL* core::curl_handler::get_handle()
{
    return curl_easy_init();
}

void core::curl_handler::release_handle(CURL* _handle)
{
    curl_easy_cleanup(_handle);
}

core::curl_handler::future_t core::curl_handler::perform(priority_t _priority, milliseconds_t _timeout, CURL* _handle)
{
    auto promise = promise_t();
    auto future = promise.get_future();

    if (keep_working_)
    {
        auto completion_handler = completion_handler_t(promise_wrapper(std::move(promise)));
        add_task(_priority, _timeout, _handle, completion_handler);
    }
    else
    {
        promise.set_value(CURLE_FAILED_INIT);
    }

    return future;
}

void core::curl_handler::perform_async(priority_t _priority, milliseconds_t _timeout, CURL* _handle, completion_callback_t _completion_callback)
{
    if (keep_working_)
    {
        auto completion_handler = completion_handler_t(_completion_callback);
        add_task(_priority, _timeout, _handle, completion_handler);
    }
    else
    {
        _completion_callback(false);
    }
}

void core::curl_handler::add_task(priority_t _priority, milliseconds_t _timeout, CURL* _handle, const completion_handler_t& _completion_handler)
{
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);

        pending_jobs_.emplace(_priority, _timeout, _handle, _completion_handler);
    }

    event_active(start_task_event_, 0, 0);
}

core::curl_handler::connection_context::connection_context(milliseconds_t _timeout, curl_handler* _curl_handler, CURL* _easy_handle, const completion_handler_t& _completion_handler)
    : timeout_(_timeout)
    , curl_handler_(_curl_handler)
    , easy_handle_(_easy_handle)
    , completion_handler_(_completion_handler)
    , socket_(0)
    , event_(nullptr)
{
    const auto tv = make_timeval(timeout_);
    timeout_event_ = evtimer_new(curl_handler_->event_base_, event_timeout_callback, this);
    evtimer_add(timeout_event_, &tv);
}

core::curl_handler::connection_context::~connection_context()
{
    free_event();
    event_free(timeout_event_);
}

void core::curl_handler::connection_context::update(curl_socket_t _socket, int _action)
{
    socket_ = _socket;

    if (event_)
        free_event();

    auto kind = EV_PERSIST;
    if (_action & CURL_POLL_IN)
        kind |= EV_READ;
    if (_action & CURL_POLL_OUT)
        kind |= EV_WRITE;

    event_ = event_new(curl_handler_->event_base_, socket_, kind, event_callback, this);
    event_add(event_, NULL);
}

void core::curl_handler::connection_context::free_event()
{
    if (event_)
    {
        event_free(event_);
        event_ = nullptr;
    }
}

core::curl_handler::completion_visitor::completion_visitor(CURLcode _result)
    : result_(_result)
{
}

void core::curl_handler::completion_visitor::operator()(const promise_wrapper& _promise) const
{
    const_cast<promise_wrapper&>(_promise).set_value(result_);
}

void core::curl_handler::completion_visitor::operator()(completion_callback_t _callback) const
{
    if (_callback)
        _callback(result_ == CURLE_OK);
}

core::curl_handler::promise_wrapper::promise_wrapper(promise_t&& _promise)
    : promise_(std::move(_promise))
{
}

core::curl_handler::promise_wrapper::promise_wrapper(const promise_wrapper& _right)
{
    promise_ = std::move(const_cast<promise_wrapper&>(_right).promise_);
}

core::curl_handler::promise_wrapper& core::curl_handler::promise_wrapper::operator=(const promise_wrapper& _right)
{
    promise_ = std::move(const_cast<promise_wrapper&>(_right).promise_);
    return *this;
}

void core::curl_handler::promise_wrapper::set_value(CURLcode _result)
{
    promise_.set_value(_result);
}

core::curl_handler::job::job(priority_t _priority, milliseconds_t _timeout, CURL* _handle, const completion_handler_t& _completion)
    : priority_(_priority)
    , timeout_(_timeout)
    , handle_(_handle)
    , completion_(_completion)
{
}

bool core::curl_handler::job_priority_comparer::operator()(const job& _left, const job& _right)
{
    return _left.priority_ > _right.priority_;
}
