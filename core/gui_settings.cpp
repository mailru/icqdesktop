#include "stdafx.h"
#include "gui_settings.h"

#include "core.h"

#include "tools/system.h"
#include "../../corelib/collection_helper.h"

using namespace core;

enum gui_settings_types
{
    gst_name = 0,
    gst_value = 1
};

gui_settings::gui_settings(
    const std::wstring& _file_name,
    const std::wstring& _file_name_exported)
    :	file_name_(_file_name),
        file_name_exported_(_file_name_exported),
        changed_(false)
{
}


gui_settings::~gui_settings()
{
    if (timer_ > 0)
        g_core->stop_timer(timer_);

    save_if_needed();
}

void gui_settings::start_save()
{
    std::weak_ptr<gui_settings> wr_this = shared_from_this();

    timer_ =  g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->save_if_needed();
    }, 10000);
}

bool gui_settings::load()
{
    core::tools::binary_stream bstream;
    if (bstream.load_from_file(file_name_))
        return unserialize(bstream);
    
    return load_exported();
}

bool gui_settings::load_exported()
{
    core::tools::binary_stream bstream_exported;
    if (bstream_exported.load_from_file(file_name_exported_))
    {
        bstream_exported.write<char>('\0');

        rapidjson::Document doc;
        if (!doc.Parse(bstream_exported.read_available()).HasParseError())
        {
            if (unserialize(doc))
            {
                changed_ = true;
                return true;
            }
        }
    }

    return false;
}

void gui_settings::set_value(const std::string& _name, const tools::binary_stream& _data)
{
    values_[_name] = _data;
    changed_ = true;
}

void gui_settings::serialize(tools::binary_stream& _bs) const
{
    tools::tlvpack pack;

    int32_t counter = 0;

    for (auto iter = values_.begin(); iter != values_.end(); ++iter)
    {
        tools::tlvpack value_tlv;
        value_tlv.push_child(tools::tlv(gui_settings_types::gst_name, iter->first));
        value_tlv.push_child(tools::tlv(gui_settings_types::gst_value, iter->second));

        tools::binary_stream bs_value;
        value_tlv.serialize(bs_value);
        pack.push_child(tools::tlv(++counter, bs_value));
    }

    pack.serialize(_bs);
}

bool gui_settings::unserialize(tools::binary_stream& _bs)
{
    if (!_bs.available())
    {
        return false;
    }

    tools::tlvpack pack;
    if (!pack.unserialize(_bs))
        return false;

    for (auto tlv_val = pack.get_first(); tlv_val; tlv_val = pack.get_next())
    {
        tools::binary_stream val_data = tlv_val->get_value<tools::binary_stream>();

        tools::tlvpack pack_val;
        if (!pack_val.unserialize(val_data))
            return false;

        auto tlv_name = pack_val.get_item(gui_settings_types::gst_name);
        auto tlv_value_data = pack_val.get_item(gui_settings_types::gst_value);

        if (!tlv_name || !tlv_value_data)
        {
            assert(false);
            return false;
        }

        values_[tlv_name->get_value<std::string>()] = tlv_value_data->get_value<tools::binary_stream>();
    }

    return true;
}

bool gui_settings::unserialize(const rapidjson::Value& _node)
{
    auto iter_show_in_taskbar = _node.FindMember(settings_show_in_taskbar);
    if (iter_show_in_taskbar != _node.MemberEnd() && iter_show_in_taskbar->value.IsBool())
    {
        tools::binary_stream bs;
        bs.write<bool>(iter_show_in_taskbar->value.GetBool());

        set_value(settings_show_in_taskbar, bs);
    }

    auto iter_enable_sounds = _node.FindMember(settings_sounds_enabled);
    if (iter_enable_sounds != _node.MemberEnd() && iter_enable_sounds->value.IsBool())
    {
        tools::binary_stream bs;
        bs.write<bool>(iter_enable_sounds->value.GetBool());

        set_value(settings_sounds_enabled, bs);
    }

    auto iter_autosave_files = _node.FindMember(settings_download_files_automatically);
    if (iter_autosave_files != _node.MemberEnd() && iter_autosave_files->value.IsBool())
    {
        tools::binary_stream bs;
        bs.write<bool>(iter_autosave_files->value.GetBool());

        set_value(settings_download_files_automatically, bs);
    }

    auto iter_download_folder = _node.FindMember(settings_download_directory);
    if (iter_download_folder != _node.MemberEnd() && iter_download_folder->value.IsString())
    {
        tools::binary_stream bs;
        std::string folder = iter_download_folder->value.GetString();
        bs.write(folder.c_str(), folder.length() + 1);

        set_value(settings_download_directory, bs);
    }

    auto iter_send_hotkey = _node.FindMember(settings_key1_to_send_message);
    if (iter_send_hotkey != _node.MemberEnd() && iter_send_hotkey->value.IsInt())
    {
        tools::binary_stream bs;
        bs.write<int32_t>(iter_send_hotkey->value.GetInt());

        set_value(settings_key1_to_send_message, bs);
    }
        
    auto iter_enable_preview = _node.FindMember(settings_show_video_and_images);
    if (iter_enable_preview != _node.MemberEnd() && iter_enable_preview->value.IsBool())
    {
        tools::binary_stream bs;
        bs.write<bool>(iter_enable_preview->value.GetBool());

        set_value(settings_show_video_and_images, bs);
    }

    auto iter_language = _node.FindMember(settings_language);
    if (iter_language != _node.MemberEnd() && iter_language->value.IsString())
    {
        tools::binary_stream bs;
        std::string folder = iter_language->value.GetString();
        bs.write(folder.c_str(), folder.length() + 1);

        set_value(settings_language, bs);
    }

    auto iter_notify = _node.FindMember(settings_notify_new_messages);
    if (iter_notify != _node.MemberEnd() && iter_notify->value.IsBool())
    {
        tools::binary_stream bs;
        bs.write<bool>(iter_notify->value.GetBool());

        set_value(settings_notify_new_messages, bs);
    }

    return true;
}

void gui_settings::serialize(core::coll_helper _collection) const
{
    ifptr<iarray> values_array(_collection->create_array(), true);

    for (auto iter = values_.begin(); iter != values_.end(); ++iter)
    {
        coll_helper coll_value(_collection->create_collection(), true);
        
        ifptr<istream> value_data_stream(_collection->create_stream(), true);
        auto bs_value_data = iter->second;
        int32_t len = bs_value_data.available();
        value_data_stream->write((const uint8_t*) bs_value_data.read(len), len);
        
        coll_value.set_value_as_string("name", iter->first);
        coll_value.set_value_as_stream("value", value_data_stream.get());

        ifptr<ivalue> ival(_collection->create_value(), true);
        ival->set_as_collection(coll_value.get());

        values_array->push_back(ival.get());
    }
    _collection.set_value_as_array("values", values_array.get());
}

void gui_settings::save_if_needed()
{
    if (changed_)
    {
        changed_ = false;

        auto bs_data = std::make_shared<tools::binary_stream>();
        serialize(*bs_data);

        std::wstring file_name = file_name_;

        g_core->save_async([bs_data, file_name]
        { 
            bs_data->save_2_file(file_name);

            return 0;
        });
    }
}

void gui_settings::clear_values()
{
    values_.clear();
    changed_ = true;
}