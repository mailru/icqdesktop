#pragma once

#include "semaphore.h"

namespace core
{
    namespace tools
    {
        class threadpool : boost::noncopyable
        {
            boost::thread::id creator_thread_id_;

        public:

            typedef std::function<void()> task;

            explicit threadpool(const unsigned _count, std::function<void()> _on_thread_exit = [](){});

            virtual ~threadpool();

            bool push_back(const task _task);
            bool push_front(const task _task);

            const std::vector<std::thread::id> get_threads_ids() const;

        protected:
            std::vector<std::thread> threads_;
            std::vector<std::thread::id> threads_ids_;
            boost::mutex queue_mutex_;
            boost::condition_variable condition_;
            std::deque<task> tasks_;
            std::atomic<bool> stop_;

            bool run_task_impl();
            bool run_task();

        };
    }

}