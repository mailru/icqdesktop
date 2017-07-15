#include "stdafx.h"
#include "async_task.h"
#include "core.h"

using namespace core;

//////////////////////////////////////////////////////////////////////////
// async_task
//////////////////////////////////////////////////////////////////////////
async_task::async_task()
{
}


async_task::~async_task()
{
}




//////////////////////////////////////////////////////////////////////////
// async_executer
//////////////////////////////////////////////////////////////////////////
async_executer::async_executer(unsigned long _count)
    :	threadpool((uint32_t)_count, []()
{
    g_core->on_thread_finish();
})
{

}

async_executer::~async_executer()
{

}

std::shared_ptr<async_task_handlers> async_executer::run_async_task(std::shared_ptr<async_task> task)
{
    return run_async_function(std::bind(&async_task::execute, task));
}

std::shared_ptr<async_task_handlers> core::async_executer::run_async_function( std::function<int32_t()> func )
{
    auto handler = std::make_shared<async_task_handlers>();

    push_back([func, handler]
    {
        int32_t result = func();

        g_core->execute_core_context([handler, result]
        {
            if (handler->on_result_)
                handler->on_result_(result);
        });
    });

    return handler;
}
