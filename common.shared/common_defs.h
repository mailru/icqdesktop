#ifndef __COMMON_DEFS_H_
#define __COMMON_DEFS_H_

namespace common
{
#ifdef _WIN32
    std::wstring get_user_profile();
#endif //_WIN32



    struct core_gui_settings
    {
        core_gui_settings()
            :   recents_avatars_size_(-1),
                is_build_icq_(1)
        {
        }

        core_gui_settings(
            int32_t _is_build_icq,
            int32_t _recents_avatars_size)
            :   is_build_icq_(_is_build_icq),
                recents_avatars_size_(_recents_avatars_size)
        {}

        int32_t is_build_icq_;
        int32_t recents_avatars_size_;
    };
}

namespace common
{
    uint32_t get_limit_search_results();
}

#endif // __COMMON_DEFS_H_