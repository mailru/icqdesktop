#include "stdafx.h"
#include "my_info.h"


using namespace core;
using namespace wim;

namespace
{
    enum info_fields
    {
        aimId = 1,
        displayId,
        friendlyName,
        state,
        userType,
        phoneNumber,
        flags,
        autoCreated,
        hasMail,
    };
}

my_info::my_info()
    : flags_(0)
    , hasMail_(false)
{

}

int32_t my_info::unserialize(const rapidjson::Value& _node)
{
    auto iter_aimid = _node.FindMember("aimId");
    if (iter_aimid != _node.MemberEnd() && iter_aimid->value.IsString())
        aimId_ = iter_aimid->value.GetString();

    auto iter_displayId = _node.FindMember("displayId");
    if (iter_displayId != _node.MemberEnd() && iter_displayId->value.IsString())
        displayId_ = iter_displayId->value.GetString();

    auto iter_friendlyName = _node.FindMember("friendly");
    if (iter_friendlyName != _node.MemberEnd() && iter_friendlyName->value.IsString())
        friendlyName_ = iter_friendlyName->value.GetString();

    auto iter_state = _node.FindMember("state");
    if (iter_state != _node.MemberEnd() && iter_state->value.IsString())
        state_ = iter_state->value.GetString();

    auto iter_userType = _node.FindMember("userType");
    if (iter_userType != _node.MemberEnd() && iter_userType->value.IsString())
        userType_ = iter_userType->value.GetString();

    auto iter_phoneNumber = _node.FindMember("attachedPhoneNumber");
    if (iter_phoneNumber != _node.MemberEnd() && iter_phoneNumber->value.IsString())
        phoneNumber_ = iter_phoneNumber->value.GetString();

    auto iter_flags = _node.FindMember("globalFlags");
    if (iter_flags != _node.MemberEnd() && iter_flags->value.IsUint())
        flags_ = iter_flags->value.GetUint();

    auto iter_expressions = _node.FindMember("expressions");
    if (iter_expressions != _node.MemberEnd())
    {
        const auto & expression_node = iter_expressions->value;
        auto iter_large_icon = expression_node.FindMember("largeBuddyIcon");
        if (iter_large_icon != expression_node.MemberEnd() && iter_large_icon->value.IsString())
            largeIconId_ = iter_large_icon->value.GetString();
    }

    auto item_has_mail = _node.FindMember("hasMail");
    if (item_has_mail != _node.MemberEnd() && item_has_mail->value.IsUint())
    {
        hasMail_ = item_has_mail->value.GetUint() == 1;
    }

    if (state_ == "occupied" || state_ == "na" || state_ == "busy")
        state_ = "dnd";
    else if (state_ == "away")
        state_ = "away";
    else if (state_ != "offline" && state_ != "invisible")
        state_ = "online";

    return 0;
}

void my_info::serialize(core::coll_helper _coll)
{
    _coll.set_value_as_string("aimId", aimId_);
    _coll.set_value_as_string("displayId", displayId_);
    _coll.set_value_as_string("friendly", friendlyName_);
    _coll.set_value_as_string("state", state_);
    _coll.set_value_as_string("userType", userType_);
    _coll.set_value_as_string("attachedPhoneNumber", phoneNumber_);
    _coll.set_value_as_uint("globalFlags", flags_);
    _coll.set_value_as_string("largeIconId", largeIconId_);
    _coll.set_value_as_bool("hasMail", hasMail_);
}

void my_info::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack info_pack;
    info_pack.push_child(core::tools::tlv(info_fields::aimId, aimId_));
    info_pack.push_child(core::tools::tlv(info_fields::displayId, displayId_));
    info_pack.push_child(core::tools::tlv(info_fields::friendlyName, friendlyName_));
    info_pack.push_child(core::tools::tlv(info_fields::state, state_));
    info_pack.push_child(core::tools::tlv(info_fields::userType, userType_));
    info_pack.push_child(core::tools::tlv(info_fields::phoneNumber, phoneNumber_));
    info_pack.push_child(core::tools::tlv(info_fields::flags, flags_));
    info_pack.push_child(core::tools::tlv(info_fields::hasMail, hasMail_));
    info_pack.serialize(_data);
}

bool my_info::unserialize(core::tools::binary_stream& _data)
{
    core::tools::tlvpack info_pack;
    if (!info_pack.unserialize(_data))
        return false;

    auto item_aimId_ =  info_pack.get_item(info_fields::aimId);
    auto item_displayId_ = info_pack.get_item(info_fields::displayId);
    auto item_friendlyName_ = info_pack.get_item(info_fields::friendlyName);
    auto item_state_ = info_pack.get_item(info_fields::state);
    auto item_userType_ = info_pack.get_item(info_fields::userType);
    auto item_phoneNumber_ = info_pack.get_item(info_fields::phoneNumber);
    auto item_flags_ = info_pack.get_item(info_fields::flags);
    auto item_has_mail_ = info_pack.get_item(info_fields::hasMail);

    if (!item_aimId_ || !item_displayId_  || !item_friendlyName_ || !item_state_ || !item_userType_ || (!item_phoneNumber_ && !item_flags_))
        return false;

    aimId_ =  item_aimId_->get_value<std::string>("");
    displayId_ =  item_displayId_->get_value<std::string>("");
    friendlyName_ =  item_friendlyName_->get_value<std::string>("");
    state_ =  item_state_->get_value<std::string>("");
    userType_ =  item_userType_->get_value<std::string>("");
    phoneNumber_ =  item_phoneNumber_->get_value<std::string>("");
    flags_ = item_flags_->get_value<uint32_t>(0);
    if (item_has_mail_)
        hasMail_ = item_has_mail_->get_value<bool>(false);
    return true;
}

my_info_cache::my_info_cache()
    : changed_(false)
    , info_(new my_info())
{

}

std::shared_ptr<my_info> my_info_cache::get_info() const
{
    return info_;
}

void my_info_cache::set_info(std::shared_ptr<my_info> _info)
{
    *info_ = *_info;
    changed_ = true;
}


bool my_info_cache::is_changed() const
{
    return changed_;
}

int32_t my_info_cache::save(const std::wstring& _filename)
{
    core::archive::storage storage(_filename);

    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;
    if (!storage.open(mode))
        return 1;

    core::tools::binary_stream block_data;
    info_->serialize(block_data);

    int64_t offset = 0;
    if (storage.write_data_block(block_data, offset))
    {
        changed_ = false;
        return 0;
    }

    return 1;
}

int32_t my_info_cache::load(const std::wstring& _filename)
{
    core::archive::storage storage(_filename);

    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!storage.open(mode))
        return 1;

    core::tools::binary_stream state_stream;
    if (!storage.read_data_block(-1, state_stream))
        return 1;

    return info_->unserialize(state_stream) ? 0 : 1;
}
