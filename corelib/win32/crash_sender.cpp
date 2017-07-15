#include "stdafx.h"
#include "../common.shared/win32/crash_handler.h"

#ifndef _WIN32
extern "C"
#endif
    void init_crash_handlers()
    {
        assert(!"need for binary compatability");
    }
