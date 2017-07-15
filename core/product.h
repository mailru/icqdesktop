#ifndef __PRODUCT_CORE_H_
#define __PRODUCT_CORE_H_



namespace build
{
    extern int32_t is_core_icq;

    inline bool is_icq()
    {
        return !!is_core_icq;
    }

    inline bool is_agent()
    {
        return !is_core_icq;
    }

    inline void set_core(const int32_t _is_core_icq)
    {
        is_core_icq = _is_core_icq;
    }
}

#endif // __PRODUCT_CORE_H_
