#pragma once

namespace installer
{
    namespace logic
    {
        installer::error shutdown_8x();
        installer::error export_from_8x();
        installer::error uninstall_8x_from_executable();
        installer::error store_exported_accounts(bool _is_from_8x);
        installer::error store_exported_settings(bool _is_from_8x);
    }
}
