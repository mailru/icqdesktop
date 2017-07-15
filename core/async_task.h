#ifndef __ASYNC_TASK_H_
#define __ASYNC_TASK_H_

#pragma once

#include "tools/threadpool.h"
#include "core.h"

namespace core
{
    class ithread_callback
    {
    public:
        ithread_callback() {}
        virtual ~ithread_callback() {}
        virtual void on_thread_shutdown() = 0;
    };

    class async_task
    {
    public:
        async_task();
        virtual ~async_task();

        virtual int32_t execute() = 0;
    };

    struct async_task_handlers
    {
        std::function<void(int32_t)>	on_result_;

        async_task_handlers()
        {
            on_result_ = [](int32_t){};
        }
    };

    template<typename T>
    struct t_async_task_handlers
    {
        std::function<void(T)>	on_result_;

        t_async_task_handlers()
        {
            on_result_ = [](T){};
        }
    };

    class auto_callback : private boost::noncopyable
    {
        std::function<void(int32_t)> callback_;

    public:

        auto_callback(std::function<void(int32_t)> _call_back)
            :   callback_(_call_back)
        {

        }

        ~auto_callback()
        {
            assert(!callback_); // callback must be nullptr
            if (callback_)
                callback_(-1);
        }

        void callback(int32_t _error)
        {
            callback_(_error);
            callback_ = nullptr;
        }
    };

    typedef std::shared_ptr<auto_callback> auto_callback_sptr;

    class async_executer : core::tools::threadpool
    {
    public:
        async_executer(unsigned long _count = 1);
        virtual ~async_executer();

        virtual std::shared_ptr<async_task_handlers> run_async_task(std::shared_ptr<async_task> task);

        virtual std::shared_ptr<async_task_handlers> run_async_function(std::function<int32_t()> func);

        template<typename T>
        std::shared_ptr<t_async_task_handlers<T>> run_t_async_function(std::function<T()> func)
        {
            auto handler = std::make_shared<t_async_task_handlers<T>>();

            push_back([func, handler]
            {
                auto result = func();

                g_core->execute_core_context([handler, result]
                {
                    if (handler->on_result_)
                        handler->on_result_(result);
                });
            });

            return handler;
        }
    };

    typedef std::unique_ptr<async_executer> async_executer_uptr;

}


#endif //__ASYNC_TASK_H_