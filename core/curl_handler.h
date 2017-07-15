#pragma once

#include <boost/variant.hpp>

#include <curl.h>

#include <event2/event.h>

namespace core
{
    class curl_handler;

    void start_new_job(curl_handler* _curl_handler);
    void finish_job(curl_handler* _curl_handler, CURL* handle, CURLcode _result);
    void check_multi_info(curl_handler* _curl_handler_ptr);
    void add_socket(curl_socket_t _socket, CURL* _easy_handle, int _action, curl_handler* _curl_handler_ptr);
    int socket_callback(CURL* _easy_handle, curl_socket_t _socket, int _what, void* _curl_handler_ptr, void* _connection_ptr);
    int timer_callback(CURLM* _multi_handle, milliseconds_t _timeout, void* _curl_handler_ptr);
    void event_callback(int _socket, short _kind, void* _connection_ptr);
    void event_timer_callback(evutil_socket_t _descriptor, short _flags, void* _curl_handler_ptr);
    void event_timeout_callback(evutil_socket_t _descriptor, short _flags, void* _connection_ptr);
    void start_task_callback(evutil_socket_t _descriptor, short _flags, void* _curl_handler_ptr);

    class curl_handler final
    {
        friend void start_new_job(curl_handler* _curl_handler);
        friend void finish_job(curl_handler* _curl_handler, CURL* handle, CURLcode _result);
        friend void check_multi_info(curl_handler* _curl_handler_ptr);
        friend void add_socket(curl_socket_t _socket, CURL* _easy_handle, int _action, curl_handler* _curl_handler_ptr);
        friend int socket_callback(CURL* _easy_handle, curl_socket_t _socket, int _what, void* _curl_handler_ptr, void* _connection_ptr);
        friend int timer_callback(CURLM* _multi_handle, milliseconds_t _timeout, void* _curl_handler_ptr);
        friend void event_callback(int _socket, short _kind, void* _curl_handler_ptr);
        friend void event_timer_callback(evutil_socket_t _descriptor, short _flags, void* _curl_handler_ptr);
        friend void event_timeout_callback(evutil_socket_t _descriptor, short _flags, void* _connection_ptr);
        friend void start_task_callback(evutil_socket_t _descriptor, short _flags, void* _curl_handler_ptr);
    public:
        static curl_handler& instance();

        void init();
        void cleanup();

        void start();
        void stop(); // cancel all transfers

        CURL* get_handle();
        void release_handle(CURL* _handle);

        typedef std::future<CURLcode> future_t;
        future_t perform(priority_t _priority, milliseconds_t _timeout, CURL* _handle);

        typedef std::function<void (bool _success)> completion_callback_t;
        void perform_async(priority_t _priority, milliseconds_t _timeout, CURL* _handle, completion_callback_t _completion_callback);

    private:
        static timeval make_timeval(milliseconds_t _timeout);

        curl_handler();

        typedef std::promise<CURLcode> promise_t;

        struct promise_wrapper
        {
            explicit promise_wrapper(promise_t&& _promise);

            promise_wrapper(const promise_wrapper& _right);
            promise_wrapper& operator=(const promise_wrapper& _right);

            void set_value(CURLcode _result);

        private:
            promise_t promise_;
        };

        typedef boost::variant<promise_wrapper, completion_callback_t> completion_handler_t;

        void add_task(priority_t _priority, milliseconds_t _timeout, CURL* _handle, const completion_handler_t& _completion_handler);

        struct connection_context
        {
            connection_context(milliseconds_t _timeout, curl_handler* _curl_handler, CURL* _easy_handle, const completion_handler_t& _completion_handler);
            ~connection_context();

            void update(curl_socket_t _socket, int _action);

            void free_event();

            milliseconds_t timeout_;
            event* timeout_event_;

            curl_handler* curl_handler_;

            CURL* easy_handle_;

            completion_handler_t completion_handler_;

            curl_socket_t socket_;

            event* event_;
        };

        CURLM* multi_handle_;
        int running_;

        event_base* event_base_;
        event* timer_event_;
        event* start_task_event_;

        std::thread event_loop_thread_;

        std::unordered_map<CURL*, std::unique_ptr<connection_context>> connections_;

        struct job
        {
            job(priority_t _priority, milliseconds_t _timeout, CURL* _handle, const completion_handler_t& _completion);

            priority_t priority_; // the lower number is the higher priority
            milliseconds_t timeout_;
            CURL* handle_;
            completion_handler_t completion_;
        };

        struct job_priority_comparer
            : public std::unary_function<bool, const job&>
        {
            bool operator()(const job& _left, const job& _right);
        };

        std::priority_queue<job, std::vector<job>, job_priority_comparer> pending_jobs_;
        std::mutex jobs_mutex_;

        std::atomic<bool> keep_working_;

        struct completion_visitor
             : public boost::static_visitor<void>
        {
            explicit completion_visitor(CURLcode _result);

            void operator()(const promise_wrapper& _promise) const;
            void operator()(completion_callback_t _callback) const;

        private:
            const CURLcode result_;
        };
    };
}
