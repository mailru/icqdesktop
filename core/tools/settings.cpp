#include "stdafx.h"
#include "settings.h"

using namespace core;
using namespace tools;

settings::settings()
{
}


settings::~settings()
{
}

void core::tools::settings::serialize(binary_stream& bstream) const
{
    tlvpack pack;

    for (auto iter = values_.begin(); iter != values_.end(); iter++)
        pack.push_child(iter->second);

    pack.serialize(bstream);
}

bool core::tools::settings::unserialize(binary_stream& bstream)
{
    tlvpack pack;
    if (!pack.unserialize(bstream))
        return false;

    auto val = pack.get_first();
    while (val)
    {
        values_[val->get_type()] = val;
        val = pack.get_next();
    }

    return true;
}

template <>
        tools::binary_stream settings::get_value<tools::binary_stream>(uint32_t _value_key, tools::binary_stream _default_value) const
        {
            auto iter = values_.find(_value_key);
            if (iter == values_.end())
                return _default_value;

            return iter->second->get_value<tools::binary_stream>();
        }

bool core::tools::settings::value_exists(uint32_t _value_key) const
{
    auto iter = values_.find(_value_key);
    return iter != values_.end();
}
