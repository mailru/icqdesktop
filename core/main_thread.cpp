#include "stdafx.h"
#include "main_thread.h"

using namespace core;

main_thread::main_thread()
    :	threadpool( 1 )
{
}


main_thread::~main_thread()
{
}

void main_thread::execute_core_context(std::function<void()> task)
{
    push_back(task);
}

std::thread::id main_thread::get_core_thread_id() const
{
    assert(get_threads_ids().size() == 1);

    if (get_threads_ids().size() == 1)
    {
        return get_threads_ids()[0];
    }

    return std::thread::id(); // nobody
}
