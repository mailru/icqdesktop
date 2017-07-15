// corelib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#ifdef ICQ_CORELIB_STATIC_LINKING
    #include "corelib.h"
#else
    #include "core_instance.h"
#endif

#ifdef __APPLE__
    #ifndef ICQ_CORELIB_STATIC_LINKING
        extern "C"
    #endif
#endif



bool get_core_instance(core::icore_interface** _core)
{
	*_core = new core::core_instance();

    return true;
}

