#pragma once

#ifdef SOLUTION_DEF_installer_agent
#define  __AGENT
#endif //agent

#ifdef SOLUTION_DEF_installer_icq
#define __ICQ
#endif //icq

namespace build
{
    static bool is_agent()
    {
#ifdef __AGENT
        return true;
#else
        return false;
#endif //__AGENT
    }

    static bool is_icq()
    {
#ifdef __ICQ
        return true;
#else
        return false;
#endif //__ICQ
    }

}