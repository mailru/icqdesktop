#ifndef __SETTINGS_H_
#define __SETTINGS_H_

#pragma once


namespace core
{
    namespace tools
    {
        class tlv;
        class binary_stream;

        class settings
        {
            typedef std::map<uint32_t, std::shared_ptr<core::tools::tlv>> values_map;

            values_map	values_;

        public:

            settings();
            virtual ~settings();

            template <class T_>
            void set_value(uint32_t _value_key, T_ _value);

            template <class T_>
            T_ get_value(uint32_t _value_key, T_ _default_value) const;

            template <class T_>
            bool get_value(uint32_t _value_key, /*out*/ T_* _value) const;

            bool value_exists(uint32_t _value_key) const;

            void serialize(binary_stream& bstream) const;
            bool unserialize(binary_stream& bstream);
        };

        template <class T_>
        void settings::set_value(uint32_t _value_key, T_ _value)
        {
            auto value_tlv = std::make_shared<core::tools::tlv>();
            value_tlv->set_value(_value);
            value_tlv->set_type(_value_key);
            values_[_value_key] = value_tlv;
        }

        template <class T_>
        T_ settings::get_value(uint32_t _value_key, T_ _default_value) const
        {
            auto iter = values_.find(_value_key);
            if (iter == values_.end())
                return _default_value;

            return iter->second->get_value(_default_value);
        }

        template <>
        tools::binary_stream settings::get_value<tools::binary_stream>(uint32_t _value_key, tools::binary_stream _default_value) const;

        template <class T_>
        bool settings::get_value(uint32_t _value_key, /*out*/ T_* _value) const
        {
            if (!_value)
            {
                assert(false);
                return false;
            }

            auto iter = values_.find(_value_key);
            if (iter == values_.end())
                return false;

            *_value = iter->second->get_value<T_>();

            return true;
        }

    }


}


#endif //__SETTINGS_H_