#pragma once

#include "../../../corelib/collection_helper.h"
#include "../../../corelib/ifptr.h"

#include "../../../tools/system.h"

namespace core
{
    namespace wim
    {
        template <typename T>
        struct transferred_data
        {
            typedef std::shared_ptr<tools::binary_stream> data_stream_ptr;
            typedef std::shared_ptr<T> additional_data_ptr;

            transferred_data()
                : response_code_(0)
            {
            }

            transferred_data(long _response_code, data_stream_ptr _header, data_stream_ptr _content)
                : response_code_(_response_code)
                , header_(_header)
                , content_(_content)
            {
            }

            transferred_data(long _response_code, additional_data_ptr _additional_data)
                : response_code_(_response_code)
                , additional_data_(_additional_data)
            {
            }

            transferred_data(long _response_code, data_stream_ptr _header, data_stream_ptr _content, additional_data_ptr _additional_data)
                : response_code_(_response_code)
                , header_(_header)
                , content_(_content)
                , additional_data_(_additional_data)
            {
            }

            template <typename Y>
            transferred_data(const transferred_data<Y>& _copied, additional_data_ptr _additional_data)
                : response_code_(_copied.response_code_)
                , header_(_copied.header_)
                , content_(_copied.content_)
                , additional_data_(_additional_data)
            {
            }

            explicit transferred_data(additional_data_ptr _additional_data)
                : response_code_(0)
                , additional_data_(_additional_data)
            {
            }

            data_stream_ptr get_content() const
            {
                if (!content_)
                {
                    auto file_info = additional_data_;
                    const auto& local_path = file_info->local_path_;

                    content_.reset(new tools::binary_stream());

                    auto file = tools::system::open_file_for_read(local_path, std::ios::binary);
                    if (file.good())
                        content_->write(file);
                }

                return content_;
            }

            ifptr<istream> get_content_as_core_istream(coll_helper& _helper) const
            {
                auto stream = _helper->create_stream();

                if (!content_)
                    get_content();

                const auto size = content_->available();
                if (size > 0)
                    stream->write(reinterpret_cast<uint8_t*>(content_->read(size)), size);

                return stream;
            }

            long response_code_;

            data_stream_ptr header_;
            mutable data_stream_ptr content_;

            additional_data_ptr additional_data_;
        };
    }
}
