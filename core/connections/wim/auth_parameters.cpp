#include "stdafx.h"
#include "auth_parameters.h"


using namespace core;
using namespace wim;

const auto startup_period = std::chrono::seconds(30);

inline std::string get_dev_id()
{
    if (build::is_icq())
    {
        if (platform::is_apple())
        {
            return "ic18eTwFBO7vAdt9";
        }
        else
        {
            return "ic1nmMjqg7Yu-0hL";
        }
    }
    else
    {
        if (platform::is_apple())
        {
            return "ic1gBBFr7Ir9EOA2";
        }
        else
        {
            return "ic1ReaqsoxgOBHFX";
        }
    }
}

core::wim::auth_parameters::auth_parameters()
    : exipired_in_(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
    , time_offset_(0)
    , dev_id_(get_dev_id())
    , robusto_client_id_(-1)
    , serializable_(true)
    , login_()
    , fetch_url_()
{
}

bool core::wim::auth_parameters::is_valid_agent_token() const
{
    return (!agent_token_.empty() && !login_.empty() && !product_guid_8x_.empty());
}

bool core::wim::auth_parameters::is_valid_token() const
{
    return (!a_token_.empty() && !session_key_.empty() && !dev_id_.empty());
}

bool core::wim::auth_parameters::is_valid_md5() const
{
    return (!password_md5_.empty() && !login_.empty());
}

bool core::wim::auth_parameters::is_valid() const
{
    return (is_valid_token() || is_valid_md5() || is_valid_agent_token());
}

void core::wim::auth_parameters::reset_robusto()
{
    robusto_token_.clear();
    robusto_client_id_ = -1;
}

void core::wim::auth_parameters::clear()
{
    aimid_.clear();
    a_token_.clear();
    session_key_.clear();
    dev_id_.clear();
    aimsid_.clear();
    version_.clear();
    product_guid_8x_.clear();
    agent_token_.clear();

    reset_robusto();
}

bool core::wim::auth_parameters::is_robusto_valid() const
{
    return (
        !robusto_token_.empty() &&
        (robusto_client_id_ > 0));
}

enum auth_param_type
{
    apt_aimid = 1,
    apt_a_token = 2,
    apt_session_key = 3,
    apt_dev_id = 4,
    apt_aimsid = 5,
    apt_exipired_in = 7,
    apt_time_offset = 8,
    apt_version = 11,
    apt_locale = 12,

    apt_robusto_token = 100,
    apt_robusto_client_id = 101
};

enum fetch_param_type
{
    fpt_fetch_url = 1,
    fpt_next_fetch_time = 2,
    fpt_last_successful_fetch = 3
};

void core::wim::auth_parameters::serialize(core::tools::binary_stream& _stream) const
{
    if (!serializable_)
        return;

    core::tools::tlvpack pack;

    core::tools::binary_stream temp_stream;

    pack.push_child(core::tools::tlv(apt_aimid, aimid_));
    pack.push_child(core::tools::tlv(apt_a_token, a_token_));
    pack.push_child(core::tools::tlv(apt_session_key, session_key_));
    pack.push_child(core::tools::tlv(apt_dev_id, dev_id_));
    pack.push_child(core::tools::tlv(apt_aimsid, aimsid_));
    pack.push_child(core::tools::tlv(apt_exipired_in, (int64_t) exipired_in_));
    pack.push_child(core::tools::tlv(apt_time_offset, (int64_t) time_offset_));
    pack.push_child(core::tools::tlv(apt_version, version_));
    pack.push_child(core::tools::tlv(apt_locale, locale_));

    pack.push_child(core::tools::tlv(apt_robusto_token, robusto_token_));
    pack.push_child(core::tools::tlv(apt_robusto_client_id, robusto_client_id_));

    pack.serialize(temp_stream);

    core::tools::tlvpack rootpack;
    rootpack.push_child(core::tools::tlv(0, temp_stream));

    rootpack.serialize(_stream);
}

bool core::wim::auth_parameters::unserialize(core::tools::binary_stream& _stream)
{
    core::tools::tlvpack tlv_pack;
    if (!tlv_pack.unserialize(_stream))
        return false;

    auto root_tlv = tlv_pack.get_item(0);
    if (!root_tlv)
        return false;

    core::tools::tlvpack tlv_pack_childs;
    if (!tlv_pack_childs.unserialize(root_tlv->get_value<core::tools::binary_stream>()))
        return false;

    auto tlv_aimid = tlv_pack_childs.get_item(apt_aimid);
    auto tlv_a_token = tlv_pack_childs.get_item(apt_a_token);
    auto tlv_session_key = tlv_pack_childs.get_item(apt_session_key);
    auto tlv_dev_id = tlv_pack_childs.get_item(apt_dev_id);
    auto tlv_aim_sid = tlv_pack_childs.get_item(apt_aimsid);
    auto tlv_exipired_in = tlv_pack_childs.get_item(apt_exipired_in);
    auto tlv_time_offset = tlv_pack_childs.get_item(apt_time_offset);
    auto tlv_version = tlv_pack_childs.get_item(apt_version);
    auto tlv_locale = tlv_pack_childs.get_item(apt_locale);

    auto tlv_robusto_token = tlv_pack_childs.get_item(apt_robusto_token);
    auto tlv_robusto_client_id = tlv_pack_childs.get_item(apt_robusto_client_id);

    if (
        !tlv_aimid || 
        !tlv_a_token || 
        !tlv_session_key || 
        !tlv_dev_id || 
        !tlv_aim_sid ||  
        !tlv_exipired_in || 
        !tlv_time_offset)
        return false;

    aimid_ = tlv_aimid->get_value<std::string>("");
    a_token_ = tlv_a_token->get_value<std::string>("");
    session_key_ = tlv_session_key->get_value<std::string>("");
    dev_id_ = tlv_dev_id->get_value<std::string>("");
    aimsid_ = tlv_aim_sid->get_value<std::string>("");
    exipired_in_ = tlv_exipired_in->get_value<int64_t>(0);
    time_offset_ = tlv_time_offset->get_value<int64_t>(0);

    if (tlv_robusto_token)
        robusto_token_ = tlv_robusto_token->get_value<std::string>("");

    if (tlv_robusto_client_id)
        robusto_client_id_ = tlv_robusto_client_id->get_value<int32_t>(-1);

    if (tlv_version)
        version_ = tlv_version->get_value<std::string>("");

    if (tlv_locale)
        locale_ = tlv_locale->get_value<std::string>("");

    return true;
}

bool core::wim::auth_parameters::unserialize(const rapidjson::Value& _node)
{
    auto iter_aimid = _node.FindMember("aimid");
    if (iter_aimid == _node.MemberEnd() || !iter_aimid->value.IsString())
        return false;

    aimid_ = iter_aimid->value.GetString();

    auto iter_atoken = _node.FindMember("atoken");
    if (iter_atoken != _node.MemberEnd() && iter_atoken->value.IsString())
        a_token_ = iter_atoken->value.GetString();

    auto iter_agenttoken = _node.FindMember("agenttoken");
    if (iter_agenttoken != _node.MemberEnd() && iter_agenttoken->value.IsString())
        agent_token_ = iter_agenttoken->value.GetString();

    auto iter_guid = _node.FindMember("productguid");
    if (iter_guid != _node.MemberEnd() && iter_guid->value.IsString())
        product_guid_8x_ = iter_guid->value.GetString();

    auto iter_session_key = _node.FindMember("sessionkey");
    if (iter_session_key != _node.MemberEnd() && iter_session_key->value.IsString())
        session_key_ = iter_session_key->value.GetString();

    auto iter_devid = _node.FindMember("devid");
    if (iter_devid != _node.MemberEnd() && iter_devid->value.IsString())
        dev_id_ = iter_devid->value.GetString();

    auto iter_aimsid = _node.FindMember("aimsid");
    if (iter_aimsid != _node.MemberEnd() && iter_aimsid->value.IsString())
        aimsid_ = iter_aimsid->value.GetString();

    // TODO : time_t
    auto iter_expiredin = _node.FindMember("expiredin");
    if (iter_expiredin != _node.MemberEnd() && iter_expiredin->value.IsInt64())
    {
        exipired_in_ = iter_expiredin->value.GetInt64();
    }

    auto iter_timeoffset = _node.FindMember("timeoffset");
    if (iter_timeoffset != _node.MemberEnd() && iter_timeoffset->value.IsInt64())
    {
        time_offset_ = iter_timeoffset->value.GetInt64();
    }

    auto iter_login = _node.FindMember("login");
    if (iter_login != _node.MemberEnd() && iter_login->value.IsString())
    {
        login_ = iter_login->value.GetString();
    }

    auto iter_fetchurl = _node.FindMember("fetchurl");
    if (iter_fetchurl != _node.MemberEnd() && iter_fetchurl->value.IsString())
    {
        fetch_url_ = iter_fetchurl->value.GetString();
    }

    auto iter_password_md5 = _node.FindMember("password_md5");
    if (iter_password_md5 != _node.MemberEnd() && iter_password_md5->value.IsString())
    {
        password_md5_ = iter_password_md5->value.GetString();
    }



    return true;
}

void core::wim::auth_parameters::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("login", login_, _a);
    _node.AddMember("aimid", aimid_, _a);
    _node.AddMember("atoken", a_token_, _a);
    _node.AddMember("sessionkey", session_key_, _a);
    _node.AddMember("devid", dev_id_, _a);
    _node.AddMember("expiredin", (int64_t)exipired_in_, _a);
    _node.AddMember("timeoffset", (int64_t)time_offset_, _a);
    _node.AddMember("aimsid", aimsid_, _a);
    _node.AddMember("fetchurl", fetch_url_, _a);
    _node.AddMember("password_md5", password_md5_, _a);
    _node.AddMember("productguid", product_guid_8x_, _a);
    _node.AddMember("agenttoken", agent_token_, _a);
}



fetch_parameters::fetch_parameters()
    :
    next_fetch_time_(std::chrono::system_clock::now()),
    last_successful_fetch_(0)
{

}

void fetch_parameters::serialize(core::tools::binary_stream& _stream) const
{
    core::tools::tlvpack pack;
    core::tools::binary_stream temp_stream;

    
    pack.push_child(core::tools::tlv(fpt_fetch_url, fetch_url_));
    pack.push_child(core::tools::tlv(fpt_last_successful_fetch, (int64_t) last_successful_fetch_));
    
    pack.serialize(temp_stream);

    core::tools::tlvpack rootpack;
    rootpack.push_child(core::tools::tlv(0, temp_stream));

    rootpack.serialize(_stream);
}

bool fetch_parameters::unserialize(core::tools::binary_stream& _stream)
{
    core::tools::tlvpack tlv_pack;
    if (!tlv_pack.unserialize(_stream))
        return false;

    auto root_tlv = tlv_pack.get_item(0);
    if (!root_tlv)
        return false;

    core::tools::tlvpack tlv_pack_childs;
    if (!tlv_pack_childs.unserialize(root_tlv->get_value<core::tools::binary_stream>()))
        return false;

    auto tlv_fetch_url = tlv_pack_childs.get_item(fpt_fetch_url);
    auto tlv_last_successfull_fetch = tlv_pack_childs.get_item(fpt_last_successful_fetch);
        
    if (
        !tlv_fetch_url || 
        !tlv_last_successfull_fetch)
        return false;

    fetch_url_ = tlv_fetch_url->get_value<std::string>("");
    last_successful_fetch_ = tlv_last_successfull_fetch->get_value<int64_t>(0);

    return true;
}

bool fetch_parameters::is_valid() const
{
    return !fetch_url_.empty();
}
