#ifndef __MAINTHREAD_H__
#define __MAINTHREAD_H__

#pragma once

#include "tools/threadpool.h"

namespace core
{
    class main_thread : protected core::tools::threadpool
    {
    public:

        main_thread();
        virtual ~main_thread();

        void execute_core_context(std::function<void()> task);
        std::thread::id get_core_thread_id() const;
    };
}


#endif //__MAINTHREAD_H__