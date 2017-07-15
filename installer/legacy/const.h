#pragma once

namespace legacy
{
    static const wchar_t* str_cs_icq_mra_key = L"Software\\ICQ\\ICQ";
    static const wchar_t* str_cs_agent_mra_key = L"Software\\Mail.Ru\\Agent";
    static const wchar_t* cs_mra_key = (build::is_icq() ? legacy::str_cs_icq_mra_key : legacy::str_cs_agent_mra_key);
}

#define STR_REG_INSTALLER_DOWNLOADED L"installer_downloaded"
