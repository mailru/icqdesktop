#include "stdafx.h"
#include "start_session.h"
#include "client_login.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../../common.shared/version_info.h"
#include "../../../utils.h"


#define WIM_API_START_SESSION_HOST			"https://api.icq.net/aim/startSession"
#define ICQ_APP_IDTYPE						"ICQ"
#define WIM_EVENTS							"myInfo,presence,buddylist,typing,dataIM,userAddedToBuddyList,service,webrtcMsg,mchat,hist,hiddenChat,diff,permitDeny,imState,notification,snapsEvent"
#define WIM_PRESENCEFIELDS					"aimId,buddyIcon,bigBuddyIcon,iconId,bigIconId,largeIconId,displayId,friendly,offlineMsg,state,statusMsg,userType,phoneNumber,cellNumber,smsNumber,workNumber,otherNumber,capabilities,ssl,abPhoneNumber,moodIcon,lastName,abPhones,abContactName,lastseen,mute,livechat,official"
#define WIM_INTERESTCAPS					"8eec67ce70d041009409a7c1602a5c84," WIM_CAP_VOIP_VOICE "," WIM_CAP_VOIP_VIDEO
#define WIM_ASSERTCAPS						WIM_CAP_VOIP_VOICE "," WIM_CAP_VOIP_VIDEO "," WIM_CAP_UNIQ_REQ_ID "," WIM_CAP_EMOJI "," WIM_CAP_MAIL_NOTIFICATIONS "," WIM_CAP_SNAPS
#define WIM_INVISIBLE						"false"


using namespace core;
using namespace wim;

#define SAAB__CODE_SAAB_OPENAUTH_REQUEST_NOT_FRESH 1015

start_session::start_session(const wim_packet_params& params, bool _is_ping, const std::string& _uniq_device_id, const std::string& _locale, time_t _timeout, std::function<bool(int32_t)> _wait_function)
    :	wim_packet(params),
        is_ping_(_is_ping),
        need_relogin_(false),
        correct_ts_(false),
        ts_(0),
        uniq_device_id_(_uniq_device_id),
        locale_(_locale),
        timeout_(_timeout),
        wait_function_(_wait_function)
{
    set_can_change_hosts_scheme(true);
}


start_session::~start_session()
{
}

int32_t start_session::init_request_full_start_session(std::shared_ptr<core::http_request_simple> request)
{
    request->set_url(WIM_API_START_SESSION_HOST);
    request->set_keep_alive();
    request->set_priority(top_priority);

    request->push_post_parameter("f", "json");
    request->push_post_parameter("k", escape_symbols(params_.dev_id_));

    request->push_post_parameter("a", escape_symbols(params_.a_token_));
    request->push_post_parameter("clientName", escape_symbols(utils::get_app_name()));

    request->push_post_parameter("imf", "plain");

    //push valid version info
    request->push_post_parameter("buildNumber", core::tools::version_info().get_build_version());
    request->push_post_parameter("majorVersion", core::tools::version_info().get_major_version() + core::tools::version_info().get_minor_version());
    request->push_post_parameter("minorVersion", core::tools::version_info().get_minor_version());
    request->push_post_parameter("pointVersion", "0");
    request->push_post_parameter("clientVersion", WIM_APP_VER);

    std::string interest_caps = WIM_INTERESTCAPS;
    std::string assert_caps = WIM_ASSERTCAPS;

    request->push_post_parameter("events", escape_symbols(WIM_EVENTS));
    request->push_post_parameter("includePresenceFields", escape_symbols(WIM_PRESENCEFIELDS));
    if (!assert_caps.empty())
        request->push_post_parameter("assertCaps", escape_symbols(assert_caps));

    request->push_post_parameter("interestCaps", escape_symbols(interest_caps));
    request->push_post_parameter("invisible", WIM_INVISIBLE);
    request->push_post_parameter("language", escape_symbols(locale_.empty() ? "en-us" : locale_));
    request->push_post_parameter("mobile", "0");
    request->push_post_parameter("rawMsg", "0");
    request->push_post_parameter("deviceId", escape_symbols(uniq_device_id_));

#ifdef _DEBUG
    request->push_post_parameter("sessionTimeout", "86400"); //1 day
#else
    request->push_post_parameter("sessionTimeout", "1209600"); //two weeks
#endif

    //move instance into na state after activeTimeout time elapsed since last activity
    //provided by calling fetchEvent or validateSid Api methods
    request->push_post_parameter("inactiveView", "offline");
    request->push_post_parameter("activeTimeout", "180"); // seconds

    time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    request->push_post_parameter("ts", tools::from_int64(ts));
    request->push_post_parameter("view", "online");
    request->push_post_parameter("nonce", tools::from_int64(ts) + "-" + tools::from_int64(params_.nonce_));

    std::map<std::string, std::string> post_params;
    request->get_post_parameters(post_params);

    request->push_post_parameter("sig_sha256", escape_symbols(get_url_sign(WIM_API_START_SESSION_HOST, post_params, params_, true)));

    return 0;
}

int32_t start_session::init_request_short_start_session(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;
    ss_url << c_wim_host << "aim/pingSession" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&k=" << escape_symbols(params_.dev_id_);

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}


int32_t start_session::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    return (is_ping_ ? init_request_short_start_session(_request) : init_request_full_start_session(_request));
}

int32_t start_session::on_response_error_code()
{
    need_relogin_ = true;

    switch (get_status_code())
    {
    case 401:
        {
            return wpie_start_session_wrong_credentials;
        }
    case 400:
        {
            if (get_status_detail_code() == SAAB__CODE_SAAB_OPENAUTH_REQUEST_NOT_FRESH)
            {
                correct_ts_ = true;
                need_relogin_ = false;
            }

            return wpie_start_session_invalid_request;
        }
    case 408:
        {
            need_relogin_ = false;
            return wpie_start_session_request_timeout;
        }

    case 607:
        {
            need_relogin_ = false;
            return  wpie_start_session_rate_limit;
        }
    case 440:
        return wpie_start_session_wrong_devid;
    default:
        return wpie_start_session_unknown_error;
    }
}

int32_t start_session::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    wait_function_(timeout_);

    bool res = (is_ping_ ? _request->get() : _request->post());
    if (!res)
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}


int32_t start_session::parse_response_data(const rapidjson::Value& _data)
{
    auto iter_ts = _data.FindMember("ts");
    if (iter_ts == _data.MemberEnd() || !iter_ts ->value.IsUint())
        return wpie_error_parse_response;

    ts_ = iter_ts->value.GetUint();

    auto iter_aimsid = _data.FindMember("aimsid");
    if (iter_aimsid == _data.MemberEnd() || !iter_aimsid->value.IsString())
        return wpie_error_parse_response;

    aimsid_ = iter_aimsid->value.GetString();

    auto iter_fetch_url = _data.FindMember("fetchBaseURL");
    if (iter_fetch_url == _data.MemberEnd() || !iter_fetch_url->value.IsString())
        return wpie_error_parse_response;

    fetch_url_ = iter_fetch_url->value.GetString();

    auto iter_my_info = _data.FindMember("myInfo");
    if (iter_my_info != _data.MemberEnd() && iter_my_info->value.IsObject())
    {
        auto iter_aimid = iter_my_info->value.FindMember("aimId");
        if (iter_aimid != iter_my_info->value.MemberEnd() || !iter_aimid->value.IsString())
            aimid_ = iter_aimid->value.GetString();
    }

    return 0;
}

void start_session::parse_response_data_on_error(const rapidjson::Value& _data)
{
    auto iter_ts = _data.FindMember("ts");
    if (iter_ts == _data.MemberEnd() || !iter_ts ->value.IsUint())
        return;

    ts_ = iter_ts->value.GetUint();
}

