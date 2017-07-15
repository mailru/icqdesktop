#include "stdafx.h"

#ifdef _WIN32
#include <Windows.h>
#include <shlobj.h>
#endif // _WIN32

#ifdef _WIN32
namespace common
{
    std::wstring get_user_profile()
    {
        wchar_t buffer[MAX_PATH + 1];

        const auto error = ::SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, buffer);
        if (FAILED(error))
        {
            return std::wstring();
        }

        return buffer;
    }
}
#endif // _WIN32


namespace common
{
    unsigned int get_limit_search_results()
    {
        return 100;
    }
}
