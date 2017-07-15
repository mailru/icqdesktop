#include "stdafx.h"
#include "scheduler.h"
#include "async_task.h"
#include "core.h"

using namespace core;


scheduler::scheduler()
    :	is_stop_(false)
{
    thread_.reset(new std::thread([this]
    {
        const uint32_t tick_timeout = 1000;

        for(;;)
        {
            std::unique_lock<std::mutex> lock(this->mutex_);
            condition_.wait_for(lock, std::chrono::milliseconds(tick_timeout));
            if (this->is_stop_)
                return;

            auto current_time = std::chrono::system_clock::now();

            for (auto iter_task = timed_tasks_.begin(); iter_task != timed_tasks_.end(); iter_task++)
            {
                auto timer_task = (*iter_task);
                if (current_time - timer_task->last_execute_time_ >= std::chrono::milliseconds(timer_task->timeout_msec_))
                {
                    timer_task->last_execute_time_ = current_time;
                    g_core->execute_core_context(timer_task->function_);
                }
            }
        }
    }));
}


scheduler::~scheduler()
{
    is_stop_ = true;
    condition_.notify_all();
    thread_->join();
}

uint32_t core::scheduler::push_timer(std::function<void()> _function, uint32_t _timeout_msec)
{
    std::unique_lock<std::mutex> lock(this->mutex_);

    static uint32_t id = 0;

    auto timer_task = std::make_shared<scheduler_timer_task>();
    timer_task->function_ = _function;
    timer_task->timeout_msec_ = _timeout_msec;
    timer_task->last_execute_time_ = std::chrono::system_clock::now();
    timer_task->id_ = ++id;

    timed_tasks_.push_back(timer_task);

    return id;
}

void core::scheduler::stop_timer(uint32_t _id)
{
    std::unique_lock<std::mutex> lock(this->mutex_);
    for (auto iter_task = timed_tasks_.begin(); iter_task != timed_tasks_.end(); ++iter_task)
    {
        if ((*iter_task)->id_ == _id)
        {
            iter_task = timed_tasks_.erase(iter_task);
            if (iter_task == timed_tasks_.end())
                break;
        }
    }
}