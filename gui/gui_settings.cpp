#include "stdafx.h"
#include "gui_settings.h"
#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"
#include "my_info.h"

namespace Ui
{
    qt_gui_settings::qt_gui_settings()
        : 
        shadowWidth_(0)
    {

    }


    bool qt_gui_settings::contains_value(const QString& _name) const
    {
        return (values_.find(_name) != values_.end());
    }

    template<> void qt_gui_settings::set_value<QString>(const QString& _name, const QString& _value)
    {
        QByteArray arr = _value.toUtf8();
        set_value_simple_data(_name, arr.data(), arr.size() + 1);
    }

    template<> QString qt_gui_settings::get_value<QString>(const QString& _name, const QString& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        return &data[0];
    }

    template<> void qt_gui_settings::set_value<int>(const QString& _name, const int& _value)
    {
        set_value_simple(_name, _value);
    }

    template <> int qt_gui_settings::get_value<int>(const QString& _name, const int& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template<> void qt_gui_settings::set_value<double>(const QString& _name, const double& _value)
    {
        set_value_simple(_name, _value);
    }

    template <> double qt_gui_settings::get_value<double>(const QString& _name, const double& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template<> void qt_gui_settings::set_value<bool>(const QString& _name, const bool& _value)
    {
        set_value_simple(_name, _value);
    }

    template <> bool qt_gui_settings::get_value<bool>(const QString& _name, const bool& _defaultValue) const
    {
        return get_value_simple(_name, _defaultValue);
    }

    template<> void qt_gui_settings::set_value<std::vector<int32_t>>(const QString& _name, const std::vector<int32_t>& _value)
    {
        set_value_simple_data(_name, (const char*) &_value[0], (int)_value.size()*sizeof(int32_t));
    }

    template<> std::vector<int32_t> qt_gui_settings::get_value<std::vector<int32_t>>(const QString& _name, const std::vector<int32_t>& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        if (data.empty())
            return _defaultValue;

        if ((data.size() % sizeof(int32_t)) != 0)
        {
            assert(false);
            return _defaultValue;
        }

        std::vector<int32_t> out_data(data.size()/sizeof(int32_t));
        ::memcpy(&out_data[0], &data[0], data.size());

        return out_data;
    }

    template<> void qt_gui_settings::set_value<QRect>(const QString& _name, const QRect& _value)
    {
        int32_t buffer[4] = {_value.left(), _value.top(), _value.width(), _value.height()};

        set_value_simple_data(_name, (const char*) buffer, sizeof(buffer));
    }

    template<> QRect qt_gui_settings::get_value<QRect>(const QString& _name, const QRect& _defaultValue) const
    {
        std::vector<char> data;
        if (!get_value_simple_data(_name, data))
            return _defaultValue;

        if (data.size() != sizeof(int32_t[4]))
        {
            assert(false);
            return _defaultValue;
        }

        int32_t* buffer = (int32_t*) &data[0];

        return QRect(buffer[0], buffer[1], buffer[2], buffer[3]);
    }


    void qt_gui_settings::set_shadow_width(int _width)
    {
        shadowWidth_ = _width;
    }

    int qt_gui_settings::get_shadow_width() const
    {
        return shadowWidth_;
    }

    int qt_gui_settings::get_current_shadow_width() const
    {
        return (get_value<bool>(settings_window_maximized, false) == true ? 0 : shadowWidth_);
    }

    void qt_gui_settings::unserialize(core::coll_helper _collection)
    {
        core::iarray* values_array = _collection.get_value_as_array("values");
        if (!values_array)
        {
            assert(false);
            return;
        }

        for (int i = 0; i < values_array->size(); ++i)
        {
            const core::ivalue* val = values_array->get_at(i);

            gui_coll_helper coll_val(val->get_as_collection(), false);

            core::istream* idata = coll_val.get_value_as_stream("value");

            int len = idata->size();

            set_value_simple_data(coll_val.get_value_as_string("name"), (const char*) idata->read(len), len, false);
        }

        emit received();
        setIsLoaded(true);
    }



    void qt_gui_settings::post_value_to_core(const QString& _name, const settings_value& _val) const
    {
        Ui::gui_coll_helper cl_coll(GetDispatcher()->create_collection(), true);

        core::ifptr<core::istream> data_stream(cl_coll->create_stream());

        if (_val.data_.size())
            data_stream->write((const uint8_t*) &_val.data_[0], (uint32_t)_val.data_.size());

        cl_coll.set_value_as_qstring("name", _name);
        cl_coll.set_value_as_stream("value", data_stream.get());

        GetDispatcher()->post_message_to_core("settings/value/set", cl_coll.get());
    }

    qt_gui_settings* get_gui_settings()
    {
        static std::unique_ptr<qt_gui_settings> settings(new qt_gui_settings());
        return settings.get();
    }

    const std::string get_account_setting_name(const std::string& _settingName)
    {
        return MyInfo()->aimId().toUtf8().constData() + std::string("/") + _settingName;
    }

    bool qt_common_settings::get_need_show_promo() const
    {
        return need_show_promo_;
    }

    void qt_common_settings::set_need_show_promo(bool _need_show_promo)
    {
        need_show_promo_ = _need_show_promo;
    }

    qt_common_settings* get_common_settings()
    {
        static std::unique_ptr<qt_common_settings> settings(new qt_common_settings());
        return settings.get();
    }
}
