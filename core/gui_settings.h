#pragma once

#include "tools/settings.h"

namespace core
{
    class coll_helper;

    class gui_settings : public std::enable_shared_from_this<gui_settings>
    {
    protected:
        std::map<std::string, tools::binary_stream> values_;
        std::wstring file_name_;
        std::wstring file_name_exported_;

        bool changed_;
        uint32_t timer_;

        bool load_exported();

    public:

        gui_settings(
            const std::wstring& _file_name,
            const std::wstring& _file_name_exported);
        virtual ~gui_settings();

        bool load();
        
        void start_save();
        void clear_values();
        
        virtual void set_value(const std::string& _name, const tools::binary_stream& _data);

        void serialize(core::coll_helper _collection) const;
        void serialize(tools::binary_stream& _bs) const;
        bool unserialize(tools::binary_stream& _bs);
        bool unserialize(const rapidjson::Value& _node);

        void save_if_needed();
    };

}