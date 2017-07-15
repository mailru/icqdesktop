#include "stdafx.h"
#include "get_chat_pending.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


get_chat_pending::get_chat_pending(const wim_packet_params& _params, const std::string& _aimId)
    :   robusto_packet(_params),
        aimId_(_aimId)
{
}


get_chat_pending::~get_chat_pending()
{

}

int32_t get_chat_pending::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "getPendingList", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    if (!aimId_.empty())
    {
        node_params.AddMember("sn", aimId_, a);
    }

    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), "");

    return 0;
}

int32_t get_chat_pending::parse_results(const rapidjson::Value& _node_results)
{
    auto iter_members = _node_results.FindMember("list");
    if (iter_members != _node_results.MemberEnd() && iter_members->value.IsArray())
    {
        for (auto iter = iter_members->value.Begin(); iter != iter_members->value.End(); iter++)
        {
            chat_member_info member_info;
            auto iter_aimid = iter->FindMember("sn");
            if (iter_aimid != iter->MemberEnd() && iter_aimid->value.IsString())
                member_info.aimid_ =  iter_aimid->value.GetString();

            auto iter_anketa = iter->FindMember("anketa");
            if (iter_anketa != iter->MemberEnd())
            {
                auto iter_first_name = iter_anketa->value.FindMember("firstName");
                if (iter_first_name != iter_anketa->value.MemberEnd() && iter_first_name->value.IsString())
                    member_info.first_name_ = iter_first_name->value.GetString();

                auto iter_last_name = iter_anketa->value.FindMember("lastName");
                if (iter_last_name != iter_anketa->value.MemberEnd() && iter_last_name->value.IsString())
                    member_info.last_name_ = iter_last_name->value.GetString();

                auto iter_nickname = iter_anketa->value.FindMember("nickname");
                if (iter_nickname != iter_anketa->value.MemberEnd() && iter_nickname->value.IsString())
                    member_info.nick_name_ = iter_nickname->value.GetString();
            }

            result_.push_back(member_info);
        }
    }

    return 0;
}

int32_t get_chat_pending::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_robusto_you_are_not_chat_member;
    else if (status_code_ == 40002)
        return wpie_error_robusto_you_are_blocked;

    return robusto_packet::on_response_error_code();
}