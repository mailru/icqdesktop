#include "stdafx.h"
#include "fast_binary_stream.h"
#include "system.h"

using namespace core;
using namespace tools;

fast_binary_stream::fast_binary_stream()
    :	size_(0), cursor_(0)
{
}

fast_binary_stream::fast_binary_stream(const fast_binary_stream& _stream)
{
    copy(_stream);
}

fast_binary_stream& core::tools::fast_binary_stream::operator=( const fast_binary_stream& _stream)
{
    copy(_stream);

    return *this;
}


void fast_binary_stream::copy(const fast_binary_stream& _stream)
{
    size_ = _stream.size_;
    cursor_ = _stream.cursor_;

    for (auto iter = _stream.data_.begin(); iter != _stream.data_.end(); iter++)
    {
        data_.push_back(data_buffer_pointer(new data_buffer(*iter->get())));
    }
}

fast_binary_stream::~fast_binary_stream()
{
}



uint32_t fast_binary_stream::available() const
{
    assert(size_ >= cursor_);
    return (size_ - cursor_);
}

void fast_binary_stream::seek(uint32_t _pos)
{
    assert(_pos < size_);
    cursor_ = _pos;
}

void fast_binary_stream::write(char* _lpData, uint32_t _size)
{
    if (!_size || !_lpData)
        return;

    data_.push_back(data_buffer_pointer(new data_buffer(_lpData, _lpData + _size)));

    size_ += _size;
}


char* fast_binary_stream::read(uint32_t _size)
{
    assert(_size <= available());
    assert(!(_size == 0 && available() == 0));

    data_buffer_pointer return_data = get_return_data_buffer(_size);

    uint32_t skip_size = 0;
    uint32_t size_need = _size;
    uint32_t output_cursor = 0;

    data_buffer_list::iterator iter_l = data_.begin();
    while (iter_l != data_.end())
    {
        data_buffer_pointer ptr_data = (*iter_l);

        if (skip_size + ptr_data->size() > cursor_)
            break;

        skip_size += ptr_data->size();
        iter_l++;
    }

    uint32_t pos_in_block = cursor_ - skip_size;

    for (; iter_l != data_.end(); iter_l++)
    {
        data_buffer_pointer ptr_data = (*iter_l);

        if (size_need <= ptr_data->size() - pos_in_block)
        {
            memcpy(&return_data->operator [](output_cursor), &ptr_data->operator [](pos_in_block), size_need);
            cursor_ += size_need;
            break;
        }
        else
        {
            uint32_t size_current = (uint32_t)ptr_data->size() - pos_in_block;
            memcpy(&return_data->operator [](output_cursor), &ptr_data->operator [](pos_in_block), size_current);
            cursor_ += size_current;
            size_need -= size_current;
            output_cursor += size_current;
            pos_in_block = 0;
        }
    }

    return &return_data->operator [](0);
}

fast_binary_stream::data_buffer_pointer fast_binary_stream::get_return_data_buffer(uint32_t _size)
{
    if (!return_buffer_)
        return_buffer_.reset(new data_buffer(_size));

    if (return_buffer_->size() < _size)
        return_buffer_->resize(_size);

    return return_buffer_;
}

void fast_binary_stream::clear()
{
    data_.clear();
    size_ = 0;
    cursor_ = 0;
}


bool fast_binary_stream::save_2_file(const std::wstring& _file_name) const
{
    boost::filesystem::wpath path_for_file(_file_name);
    std::wstring folder_name = path_for_file.parent_path().wstring();

    if (!core::tools::system::is_exist(folder_name))
    {
        if (!core::tools::system::create_directory(folder_name))
            return false;
    }

    std::wstring temp_file_name = _file_name + L".tmp";

    {
        auto outfile = tools::system::open_file_for_write(temp_file_name, std::ofstream::binary | std::ofstream::trunc);
        if (!outfile.is_open())
            return false;

        auto_scope end_scope_call([&outfile]	{ outfile.close(); });

        for (auto iter_l = data_.begin(); iter_l != data_.end(); iter_l++)
        {
            data_buffer_pointer ptr_data = (*iter_l);

            outfile.write((const char*) &ptr_data->operator [](0), ptr_data->size());

            if (!outfile.good())
            {
                assert(!"save stream to file error");
                return false;
            }
        }
    }

    return core::tools::system::move_file(temp_file_name.c_str(), _file_name.c_str());
}

bool fast_binary_stream::load_from_file(const std::wstring& _file_name)
{
    if (!core::tools::system::is_exist(_file_name))
        return false;

    auto infile = tools::system::open_file_for_read(_file_name, std::ifstream::in | std::ifstream::binary |std::ifstream::ate);
    if (!infile.is_open())
        return false;

    auto_scope end_scope_call([&infile]	{ infile.close(); });

    const int32_t mem_block_size = 1024*64;

    uint64_t size = infile.tellg();
    infile.seekg (0, std::ofstream::beg);
    uint64_t size_read = 0;

    char buffer[mem_block_size];

    while (size_read < size)
    {
        uint64_t bytes_to_read = mem_block_size;
        uint64_t tail = size - size_read;
        if (tail < mem_block_size)
            bytes_to_read = tail;

        infile.read(buffer, bytes_to_read);
        if (!infile.good())
        {
            assert(!"read stream to file error");
            return false;
        }

        write((char*)buffer, (int32_t) bytes_to_read);

        size_read += bytes_to_read;
    }

    return true;
}


