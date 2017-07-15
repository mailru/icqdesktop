#ifndef __CORE_MD5_H_
#define __CORE_MD5_H_

#pragma once

namespace core
{
    namespace tools
    {
        std::string md5(const void* _data, int32_t _size);

        uint8_t hextobin(const char * str, uint8_t * bytes, size_t blen);
    }
}


#endif //__CORE_MD5_H_