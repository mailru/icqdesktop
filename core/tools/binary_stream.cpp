#include "stdafx.h"
#include "binary_stream.h"
#include "strings.h"
#include "system.h"

using namespace core;
using namespace tools;

void binary_stream::write_stream(std::istream& _source)
{
    const auto size = buffer_.size();
    std::copy(
        std::istreambuf_iterator<typename std::istream::char_type>(_source),
        std::istreambuf_iterator<char>(),
        std::back_inserter(buffer_));
    input_cursor_ += (buffer_.size() - size);
}

bool binary_stream::save_2_file(const std::wstring& _file_name) const
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

        uint32_t size = available();

        if (size > 0)
        {
            outfile.write(read(size), size);
        }
        outfile.flush();

        if (!outfile.good())
        {
            assert(!"save stream to file error");
            return false;
        }

        outfile.close();
    }

    boost::filesystem::path from(temp_file_name);
    boost::filesystem::path target(_file_name);
    boost::system::error_code error;
    boost::filesystem::rename(from, target, error);

    return (!error);
}

bool binary_stream::load_from_file(const std::wstring& _file_name)
{
    if (!core::tools::system::is_exist(_file_name))
        return false;

    auto infile = tools::system::open_file_for_read(_file_name, std::ifstream::in | std::ifstream::binary |std::ifstream::ate);
    if (!infile.is_open())
        return false;

    const int32_t mem_block_size = 1024*64;

    uint64_t size = infile.tellg();
    infile.seekg (0, std::ifstream::beg);
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

template <> void binary_stream::write<std::string>(const std::string& _val)
{
    if (!_val.empty())
        write(_val.c_str(), (uint32_t)_val.size());
}

template <> std::string core::tools::binary_stream::read<std::string>() const
{
    std::string val;

    uint32_t sz = available();
    if (!sz)
        return val;

    val.assign((const char*) read(sz), sz);

    return val;
}
