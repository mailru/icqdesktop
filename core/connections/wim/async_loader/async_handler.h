#pragma once

#include "../../../common.shared/loader_errors.h"

#include "transferred_data.h"

namespace core
{
    namespace wim
    {
        template <typename T>
        struct async_handler
        {
            typedef std::function<void (loader_errors _error, const transferred_data<T>& _data)> completion_callback_t;
            completion_callback_t completion_callback_;

            typedef std::function<void (int64_t _total, int64_t _transferred, int32_t _completion_percent)> progress_callback_t;
            progress_callback_t progress_callback_;

            async_handler()
                : completion_callback_(nullptr)
                , progress_callback_(nullptr)
            {
            }

            async_handler(completion_callback_t _completion_callback, progress_callback_t _progress_callback = nullptr)
                : completion_callback_(_completion_callback)
                , progress_callback_(_progress_callback)
            {
            }

            std::string to_string() const
            {
                std::stringstream buf;

                buf << this
                    << " [ "
                    << completion_callback_.target_type().name()
                    << " : "
                    << progress_callback_.target_type().name()
                    << " ]";

                return buf.str();
            }
        };
    }
}
