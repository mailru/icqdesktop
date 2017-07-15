#include "stdafx.h"
#include "system.h"

#include "../../external/minizip/unzip.h"

#if defined(_WIN32)
#include <Windows.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#if defined(BSD)
#include <sys/sysctl.h>
#endif

#else
#error "Unable to define getMemorySize( ) for an unknown OS."
#endif

namespace core { namespace tools { namespace system {

    const int32_t mb = 1024 * 1024;

    /**
    * Returns the size of physical memory (RAM) in bytes.
    */
    size_t get_memory_size_mb()
    {
#if defined(_WIN32) && (defined(__CYGWIN__) || defined(__CYGWIN32__))
        /* Cygwin under Windows. ------------------------------------ */
        /* New 64-bit MEMORYSTATUSEX isn't available.  Use old 32.bit */
        MEMORYSTATUS status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatus( &status );
        return (size_t)status.dwTotalPhys;

#elif defined(_WIN32)
        /* Windows. ------------------------------------------------- */
        /* Use new 64-bit MEMORYSTATUSEX, not old 32-bit MEMORYSTATUS */
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx( &status );
        return (size_t) (status.ullTotalPhys / mb);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
        /* UNIX variants. ------------------------------------------- */
        /* Prefer sysctl() over sysconf() except sysctl() HW_REALMEM and HW_PHYSMEM */

#if defined(CTL_HW) && (defined(HW_MEMSIZE) || defined(HW_PHYSMEM64))
        int32_t mib[2];
        mib[0] = CTL_HW;
#if defined(HW_MEMSIZE)
        mib[1] = HW_MEMSIZE;            /* OSX. --------------------- */
#elif defined(HW_PHYSMEM64)
        mib[1] = HW_PHYSMEM64;          /* NetBSD, OpenBSD. --------- */
#endif
        int64_t size = 0;               /* 64-bit */
        size_t len = sizeof( size );
        if ( sysctl( mib, 2, &size, &len, NULL, 0 ) == 0 )
            return (size_t)size / mb;
        return 0L;			/* Failed? */

#elif defined(_SC_AIX_REALMEM)
        /* AIX. ----------------------------------------------------- */
        return (size_t)sysconf( _SC_AIX_REALMEM ) * (size_t)1024L / mb;

#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
        /* FreeBSD, Linux, OpenBSD, and Solaris. -------------------- */
        return (size_t)sysconf( _SC_PHYS_PAGES ) *
            (size_t)sysconf( _SC_PAGESIZE ) / mb;

#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
        /* Legacy. -------------------------------------------------- */
        return (size_t)sysconf( _SC_PHYS_PAGES ) *
            (size_t)sysconf( _SC_PAGE_SIZE ) / mb;

#elif defined(CTL_HW) && (defined(HW_PHYSMEM) || defined(HW_REALMEM))
        /* DragonFly BSD, FreeBSD, NetBSD, OpenBSD, and OSX. -------- */
        int32_t mib[2];
        mib[0] = CTL_HW;
#if defined(HW_REALMEM)
        mib[1] = HW_REALMEM;		/* FreeBSD. ----------------- */
#elif defined(HW_PYSMEM)
        mib[1] = HW_PHYSMEM;		/* Others. ------------------ */
#endif
        unsigned int32_t size = 0;		/* 32-bit */
        size_t len = sizeof( size );
        if ( sysctl( mib, 2, &size, &len, NULL, 0 ) == 0 )
            return (size_t)size / mb;
        return 0L;			/* Failed? */
#endif /* sysctl and sysconf variants */

#else
        return 0L;			/* Unknown OS. */
#endif
    }

    bool is_exist(const std::wstring& _path)
    {
#ifndef _WIN32
        auto p = tools::from_utf16(_path);
        boost::filesystem::path path(p);
#else
        boost::filesystem::wpath path(_path);
#endif //_WIN32
        boost::system::error_code e;
        return boost::filesystem::exists(path, e);
    }

    bool is_exist(const boost::filesystem::wpath & path)
    {
        return is_exist(path.wstring());
    }

    bool create_directory(const std::wstring& path)
    {
        boost::filesystem::wpath pathW(path);
        boost::system::error_code e;
        return boost::filesystem::create_directories(pathW, e);
    }

    bool create_directory(const boost::filesystem::wpath& path)
    {
        boost::system::error_code e;
        return boost::filesystem::create_directories(path, e);
    }

    bool create_directory_if_not_exists(const boost::filesystem::wpath& _path)
    {
        if (!system::is_exist(_path))
            return system::create_directory(_path);
        return true;
    }
    
    std::wstring create_temp_file_path()
    {
        std::wstring result;
        boost::system::error_code e;
        boost::filesystem::wpath p = boost::filesystem::temp_directory_path(e);
        if (boost::filesystem::exists(p, e))
        {
            result += p.wstring();
            auto separator = boost::filesystem::wpath::preferred_separator;
            if (*(result.rbegin()) != separator)
                result += separator;
            auto temp_file = boost::filesystem::unique_path(L"%%%%-%%%%-%%%%-%%%%", e);
            if (!e)
            {
                result += temp_file.wstring();
            }
        }
        return result;
    }

    bool create_empty_file(const std::wstring &_path)
    {
        assert(!_path.empty());

        binary_stream out;
        return out.save_2_file(_path);
    }

    bool read_file(const boost::filesystem::wpath& _path, std::string& _result)
    {
        auto file = std::ifstream(_path.native());
        if (!file)
            return false;

        _result = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        return true;
    }

    bool unzip(const boost::filesystem::path& _archive, const boost::filesystem::path& _target_dir)
    {
#ifdef _WIN32
        unzFile zip = unzOpen(
            is_windows_vista_or_higher()
                ? _archive.string().c_str()
                : get_short_file_name(_archive.native()).c_str());
#else
        unzFile zip = unzOpen(_archive.string().c_str());
#endif

        if (!zip)
            return false;

        if (unzGoToFirstFile(zip) != UNZ_OK)
            return false;

        if (!create_directory_if_not_exists(_target_dir))
        {
            unzClose(zip);
            return false;
        }

        const int32_t buffer_size = 32768;
        std::vector<char> buffer(buffer_size);

        while (true)
        {
            if (unzGetCurrentFileInfo(zip, nullptr, buffer.data(), buffer_size, nullptr, 0, nullptr, 0) != UNZ_OK)
            {
                unzClose(zip);
                return false;
            }

            auto archive_file_path = boost::filesystem::path(buffer.data());
            if (archive_file_path.filename() == ".")
            {
                const auto result = unzGoToNextFile(zip);
                if (result != UNZ_OK)
                {
                    unzClose(zip);
                    return result == UNZ_END_OF_LIST_OF_FILE;
                }
                continue;
            }

            const auto parent = archive_file_path.parent_path();
            if (!parent.empty())
            {
                if (!create_directory_if_not_exists(_target_dir / parent))
                {
                    unzClose(zip);
                    return false;
                }
            }

            if (unzOpenCurrentFile(zip) != UNZ_OK)
            {
                unzClose(zip);
                return false;
            }

            const auto file_path = _target_dir / archive_file_path;

            std::ofstream out(file_path.native(), std::ios::binary);
            if (!out)
            {
                unzCloseCurrentFile(zip);
                unzClose(zip);
                return false;
            }

            int32_t readed = 0;
            while ((readed = unzReadCurrentFile(zip, buffer.data(), buffer_size)) > 0)
            {
                out.write(buffer.data(), readed);
            }

            unzCloseCurrentFile(zip);

            const auto result = unzGoToNextFile(zip);
            if (result != UNZ_OK)
            {
                unzClose(zip);
                return result == UNZ_END_OF_LIST_OF_FILE;
            }
        }
    }

    bool clean_directory(const boost::filesystem::path& _dir)
    {
        for(auto& entry : boost::make_iterator_range(
            boost::filesystem::directory_iterator(_dir), boost::filesystem::directory_iterator()))
        {
            boost::filesystem::remove_all(entry);
        }
        return true;
    }

#ifdef _WIN32
    std::ifstream open_file_for_read(const std::string& _file_name, std::ios_base::openmode _mode)
    {
        return std::ifstream(tools::from_utf8(_file_name), _mode);
    }

    std::ifstream open_file_for_read(const std::wstring& _file_name, std::ios_base::openmode _mode)
    {
        return std::ifstream(_file_name, _mode);
    }

    std::ofstream open_file_for_write(const std::string& _file_name, std::ios_base::openmode _mode)
    {
        return std::ofstream(tools::from_utf8(_file_name), _mode);
    }

    std::ofstream open_file_for_write(const std::wstring& _file_name, std::ios_base::openmode _mode)
    {
        return std::ofstream(_file_name, _mode);
    }
#else
    std::ifstream open_file_for_read(const std::string& _file_name, std::ios_base::openmode _mode)
    {
        return std::ifstream(_file_name, _mode);
    }

    std::ifstream open_file_for_read(const std::wstring& _file_name, std::ios_base::openmode _mode)
    {
        return std::ifstream(tools::from_utf16(_file_name), _mode);
    }

    std::ofstream open_file_for_write(const std::string& _file_name, std::ios_base::openmode _mode)
    {
        return std::ofstream(_file_name, _mode);
    }

    std::ofstream open_file_for_write(const std::wstring& _file_name, std::ios_base::openmode _mode)
    {
        return std::ofstream(tools::from_utf16(_file_name), _mode);
    }
#endif

    size_t get_file_size(const std::string& _file_name)
    {
        auto file = tools::system::open_file_for_read(_file_name, std::ios::binary | std::ios::ate);
        return file.good()
            ? static_cast<size_t>(file.tellg())
            : 0;
    }

    size_t get_file_size(const std::wstring& _file_name)
    {
        auto file = tools::system::open_file_for_read(_file_name, std::ios::binary | std::ios::ate);
        return file.good()
            ? static_cast<size_t>(file.tellg())
            : 0;
    }
}}}
