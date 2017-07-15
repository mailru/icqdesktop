#ifndef __PRODUCT_H_
#define __PRODUCT_H_

namespace build
{
    extern int is_build_icq;

    inline bool is_icq()
    {
        return !!is_build_icq;
    }

    inline bool is_agent()
    {
        return !is_build_icq;
    }
}

#endif // __PRODUCT_H_
