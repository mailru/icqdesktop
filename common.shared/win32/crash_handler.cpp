#include "stdafx.h"

#ifdef _WIN32

//#include "../core/utils.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <new.h>
#include <signal.h>
#include <exception>
#include <sys/stat.h>
#include <psapi.h>
#include <rtcapi.h>
#include <Shellapi.h>
#include <dbghelp.h>

#include <ctime>
#include <fstream>

#include "crash_handler.h"
#include "../version_info.h"
#include "../common_defs.h"
#include "common_crash_sender.h"

#ifndef _AddressOfReturnAddress

// Taken from: http://msdn.microsoft.com/en-us/library/s975zw7k(VS.71).aspx
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

// _ReturnAddress and _AddressOfReturnAddress should be prototyped before use 
EXTERNC void * _AddressOfReturnAddress(void);
EXTERNC void * _ReturnAddress(void);

#endif 

namespace core
{
    namespace dump
    {
        std::string crash_handler::product_bundle_;
        std::wstring crash_handler::product_path_;
        bool crash_handler::is_sending_after_crash_ = false;

        crash_handler::crash_handler(
            const std::string& _bundle, 
            const std::wstring& _product_path, 
            const bool _is_sending_after_crash)
        {
            crash_handler::is_sending_after_crash_ = _is_sending_after_crash;
            crash_handler::product_bundle_ = _bundle;
            crash_handler::product_path_ = _product_path;
        }

        crash_handler::~crash_handler()
        {
        }

        std::wstring crash_handler::get_product_path()
        {
            return crash_handler::product_path_;
        }

        void crash_handler::set_product_bundle(const std::string& _product_bundle)
        {
            product_bundle_ = _product_bundle;
        }

        void crash_handler::set_is_sending_after_crash(bool _is_sending_after_crash)
        {
            is_sending_after_crash_ = _is_sending_after_crash;
        }

        std::wstring get_report_path()
        {
            return crash_handler::get_product_path() + L"\\reports";
        }

        void set_os_version(const std::string& _os_version)
        {
            os_version = _os_version;
        }

        std::string get_os_version()
        {
            return os_version;
        }

        std::wstring get_report_log_path()
        {
            return get_report_path() + L"\\crash.log";
        }

        std::wstring get_report_mini_dump_path()
        {
            return get_report_path() + L"\\crashdump.dmp";
        }

        int32_t crash_handler::get_dump_type()
        {
            static int32_t dump_type = -1;

            if (dump_type == -1)
            {
                std::ifstream dump_type_file(crash_handler::get_product_path() + L"/settings/dump_type.txt");

                dump_type = 1;
                if (dump_type_file.good())
                {
                    dump_type_file >> dump_type;
                }
            }

            switch (dump_type)
            {
            case 0:
                return -1;
            case 2:
                return MiniDumpWithFullMemory;
            case 1:
            default:
                return MiniDumpNormal;
            }
        }

        bool crash_handler::need_write_dump()
        {
            if (!is_crash_handle_enabled())
                return false;
            if (get_dump_type() == -1)
                return false;
            return true;
        }

        void crash_handler::set_process_exception_handlers()
        {
            if (!is_crash_handle_enabled())
                return;

            if (!need_write_dump())
                return;

            // Install top-level SEH handler
            SetUnhandledExceptionFilter(seh_handler);    

            // Catch pure virtual function calls.
            // Because there is one _purecall_handler for the whole process, 
            // calling this function immediately impacts all threads. The last 
            // caller on any thread sets the handler. 
            // http://msdn.microsoft.com/en-us/library/t296ys27.aspx

            //return;

            _set_purecall_handler(pure_call_handler);    

            // Catch new operator memory allocation exceptions
            _set_new_handler(new_handler);

            // Catch invalid parameter exceptions.
            _set_invalid_parameter_handler(invalid_parameter_handler); 

            // Set up C++ signal handlers

            _set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);

            // Catch an abnormal program termination
            signal(SIGABRT, sigabrt_handler);  

            // Catch illegal instruction handler
            signal(SIGINT, sigint_handler);     

            // Catch a termination request
            signal(SIGTERM, sigterm_handler);          

        }

        void crash_handler::set_thread_exception_handlers()
        {
            if (!is_crash_handle_enabled())
                return;

            if (!need_write_dump())
                return;

            // Catch terminate() calls. 
            // In a multithreaded environment, terminate functions are maintained 
            // separately for each thread. Each new thread needs to install its own 
            // terminate function. Thus, each thread is in charge of its own termination handling.
            // http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
            
            // NOTE : turn off terminate handle
            // set_terminate(terminate_handler);       

            // Catch unexpected() calls.
            // In a multithreaded environment, unexpected functions are maintained 
            // separately for each thread. Each new thread needs to install its own 
            // unexpected function. Thus, each thread is in charge of its own unexpected handling.
            // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
            set_unexpected(unexpected_handler);    

            // Catch a floating point error
            typedef void (*sigh)(int);
            signal(SIGFPE, (sigh)sigfpe_handler);     

            // Catch an illegal instruction
            signal(SIGILL, sigill_handler);     

            // Catch illegal storage access errors
            signal(SIGSEGV, sigsegv_handler);   

        }

        // The following code gets exception pointers using a workaround found in CRT code.
        void crash_handler::get_exception_pointers(DWORD dwExceptionCode, 
            EXCEPTION_POINTERS** ppExceptionPointers)
        {
            // The following code was taken from VC++ 8.0 CRT (invarg.c: line 104)

            EXCEPTION_RECORD ExceptionRecord;
            CONTEXT ContextRecord;
            memset(&ContextRecord, 0, sizeof(CONTEXT));

#ifdef _X86_

            __asm {
                mov dword ptr [ContextRecord.Eax], eax
                    mov dword ptr [ContextRecord.Ecx], ecx
                    mov dword ptr [ContextRecord.Edx], edx
                    mov dword ptr [ContextRecord.Ebx], ebx
                    mov dword ptr [ContextRecord.Esi], esi
                    mov dword ptr [ContextRecord.Edi], edi
                    mov word ptr [ContextRecord.SegSs], ss
                    mov word ptr [ContextRecord.SegCs], cs
                    mov word ptr [ContextRecord.SegDs], ds
                    mov word ptr [ContextRecord.SegEs], es
                    mov word ptr [ContextRecord.SegFs], fs
                    mov word ptr [ContextRecord.SegGs], gs
                    pushfd
                    pop [ContextRecord.EFlags]
            }

            ContextRecord.ContextFlags = CONTEXT_CONTROL;
#pragma warning(push)
#pragma warning(disable:4311)
            ContextRecord.Eip = (ULONG)_ReturnAddress();
            ContextRecord.Esp = (ULONG)_AddressOfReturnAddress();
#pragma warning(pop)
            ContextRecord.Ebp = *((ULONG *)_AddressOfReturnAddress()-1);


#elif defined (_IA64_) || defined (_AMD64_)

            /* Need to fill up the Context in IA64 and AMD64. */
            RtlCaptureContext(&ContextRecord);

#else  /* defined (_IA64_) || defined (_AMD64_) */

            ZeroMemory(&ContextRecord, sizeof(ContextRecord));

#endif  /* defined (_IA64_) || defined (_AMD64_) */

            ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));

            ExceptionRecord.ExceptionCode = dwExceptionCode;
            ExceptionRecord.ExceptionAddress = _ReturnAddress();

            ///

            EXCEPTION_RECORD* pExceptionRecord = new EXCEPTION_RECORD;
            memcpy(pExceptionRecord, &ExceptionRecord, sizeof(EXCEPTION_RECORD));
            CONTEXT* pContextRecord = new CONTEXT;
            memcpy(pContextRecord, &ContextRecord, sizeof(CONTEXT));

            *ppExceptionPointers = new EXCEPTION_POINTERS;
            (*ppExceptionPointers)->ExceptionRecord = pExceptionRecord;
            (*ppExceptionPointers)->ContextRecord = pContextRecord;  
        }
        
         const std::string from_utf16(const std::wstring& _source_16)
        {
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(_source_16);
        }

        bool is_dir_existed(const std::wstring& path)
        {
            struct stat info;
            if (stat(from_utf16(path).c_str(), &info) != 0)
                return false;
            else if(info.st_mode & S_IFDIR)
                return true;
            else
                return false;
        }

        // This method creates minidump of the process
        void crash_handler::create_mini_dump(EXCEPTION_POINTERS* pExcPtrs)
        {   
            MINIDUMP_EXCEPTION_INFORMATION mei;
            MINIDUMP_CALLBACK_INFORMATION mci;

            // Load dbghelp.dll
            HMODULE hDbgHelp = LoadLibrary(_T("dbghelp.dll"));
            if(hDbgHelp == NULL)
            {
                // Error - couldn't load dbghelp.dll
                return;
            }

            if (!is_dir_existed(crash_handler::get_product_path()))
            {
                 if (!CreateDirectory((crash_handler::get_product_path()).c_str(), NULL) &&
                    ERROR_ALREADY_EXISTS != GetLastError())
                {
                    return;
                }
            }

            // create folder if not existed
            if (!CreateDirectory((get_report_path() + std::wstring(L"\\")).c_str(), NULL) &&
                ERROR_ALREADY_EXISTS != GetLastError())
            {
                return;
            }

            create_log_file_for_hockey_app(pExcPtrs);

            // Create the minidump file
            HANDLE handleFile (CreateFile(
                get_report_mini_dump_path().c_str(),
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL));

            // https://msdn.microsoft.com/ru-ru/library/windows/desktop/5fc6ft2t(v=vs.80).aspx
            if(handleFile==INVALID_HANDLE_VALUE)
            {
                return;
            }

            CHandle hFile(handleFile);

            // Write minidump to the file
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = pExcPtrs;
            mei.ClientPointers = FALSE;
            mci.CallbackRoutine = NULL;
            mci.CallbackParam = NULL;

            typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(
                HANDLE hProcess, 
                DWORD ProcessId, 
                HANDLE hFile, 
                MINIDUMP_TYPE DumpType, 
                CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, 
                CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, 
                CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

            LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump = 
                (LPMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
            if(!pfnMiniDumpWriteDump)
            {    
                // Bad MiniDumpWriteDump function
                return;
            }

            HANDLE hProcess = GetCurrentProcess();
            DWORD dwProcessId = GetCurrentProcessId();
            auto dump_type = (_MINIDUMP_TYPE)get_dump_type();
            if (dump_type == -1)
            {
                assert(!"dump_type must be positive");
                dump_type = MiniDumpNormal;
            }

            BOOL bWriteDump = pfnMiniDumpWriteDump(
                hProcess,
                dwProcessId,
                hFile,
                dump_type,
                &mei,
                NULL,
                &mci);

            if(!bWriteDump)
            {    
                // Error writing dump.
                return;
            }

            // Unload dbghelp.dll
            FreeLibrary(hDbgHelp);
        }

        void crash_handler::create_log_file_for_hockey_app(EXCEPTION_POINTERS* /* pExcPtrs */)
        {   
            const int max_stack_trace_depth = 48;
            void *stack[max_stack_trace_depth];
            USHORT count = CaptureStackBackTrace(0, max_stack_trace_depth, stack, NULL);

            std::stringstream stack_trace_stream;
            for (USHORT c = 0; c < count; c++)
            {
                stack_trace_stream << stack[c] << ";";
            }

            std::string stack_trace = stack_trace_stream.str();
            std::wstring log_name = get_report_log_path();

            time_t now = time(0);
            std::stringstream log_stream;
            // TODO (*) : use real info about app

            assert(product_bundle_ != "");

            log_stream << "Package: " << product_bundle_ << std::endl;
            log_stream << "Version: " << VERSION_INFO_STR << std::endl;
            log_stream << "OS: " << get_os_version() << std::endl;
            log_stream << "Manufacturer: MS" << std::endl;
            log_stream << "Model: " << tools::version_info().get_version() << std::endl;

            char str[26];
            ctime_s(str, sizeof str, &now);
            log_stream << "Date: " << str << std::endl;
            log_stream << stack_trace << std::endl;

            auto log_text = log_stream.str();
            DWORD dwWritten;
            HANDLE handleFile (CreateFile(log_name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 
                0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0));

            // https://msdn.microsoft.com/ru-ru/library/windows/desktop/5fc6ft2t(v=vs.80).aspx
            if(handleFile == INVALID_HANDLE_VALUE)
            {
                return;
            }

            CHandle hFile(handleFile);
            WriteFile(hFile, log_text.c_str(), log_text.size(), &dwWritten, 0);
        }

        void crash_handler::process_exception_pointers(EXCEPTION_POINTERS* pExceptionPtrs)
        {
             // Write minidump file
            create_mini_dump(pExceptionPtrs);

            if (is_sending_after_crash_)
            {
                ::common_crash_sender::start_sending_process();
            }

            // Terminate process
            TerminateProcess(GetCurrentProcess(), 1);
        }

        // Structured exception handler
        LONG WINAPI crash_handler::seh_handler(PEXCEPTION_POINTERS pExceptionPtrs)
        { 
            process_exception_pointers(pExceptionPtrs);

            // Unreacheable code  
            return EXCEPTION_EXECUTE_HANDLER;
        }

        // CRT terminate() call handler
        void __cdecl crash_handler::terminate_handler()
        {
            // Abnormal program termination (terminate() function was called)

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);
        }

        // CRT unexpected() call handler
        void __cdecl crash_handler::unexpected_handler()
        {
            // Unexpected error (unexpected() function was called)

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);
        }

        // CRT Pure virtual method call handler
        void __cdecl crash_handler::pure_call_handler()
        {
            // Pure virtual function call

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);
        }

        // CRT invalid parameter handler
        void __cdecl crash_handler::invalid_parameter_handler(
            const wchar_t* /* expression */, 
            const wchar_t* /* function */, 
            const wchar_t* /* file */, 
            unsigned int /* line */, 
            uintptr_t pReserved)
        {
            pReserved;

            // Invalid parameter exception

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);

        }

        // CRT new operator fault handler
        int __cdecl crash_handler::new_handler(size_t)
        {
            // 'new' operator memory allocation exception

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);

            // Unreacheable code
            return 0;
        }

        // CRT SIGABRT signal handler
        void crash_handler::sigabrt_handler(int)
        {
            // Caught SIGABRT C++ signal

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);

        }

        // CRT SIGFPE signal handler
        void crash_handler::sigfpe_handler(int /*code*/, int /* subcode */)
        {
            // Floating point exception (SIGFPE)

            EXCEPTION_POINTERS* pExceptionPtrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

            process_exception_pointers(pExceptionPtrs);

        }

        // CRT sigill signal handler
        void crash_handler::sigill_handler(int)
        {
            // Illegal instruction (SIGILL)

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);

        }

        // CRT sigint signal handler
        void crash_handler::sigint_handler(int)
        {
            // Interruption (SIGINT)

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);

        }

        // CRT SIGSEGV signal handler
        void crash_handler::sigsegv_handler(int)
        {
            // Invalid storage access (SIGSEGV)

            PEXCEPTION_POINTERS pExceptionPtrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

            process_exception_pointers(pExceptionPtrs);
        }

        // CRT SIGTERM signal handler
        void crash_handler::sigterm_handler(int)
        {
            // Termination request (SIGTERM)

            // Retrieve exception information
            EXCEPTION_POINTERS* pExceptionPtrs = NULL;
            get_exception_pointers(0, &pExceptionPtrs);

            process_exception_pointers(pExceptionPtrs);
        }
    }
}

#endif // _WIN32

