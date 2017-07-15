#include "stdafx.h"
#include "get_chat_home.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


get_chat_home::get_chat_home(const wim_packet_params& _params, const std::string& _new_tag)
    :	robusto_packet(_params)
    ,   new_tag_(_new_tag)
    ,   need_restart_(false)
    ,   finished_(false)
{
}


get_chat_home::~get_chat_home()
{

}

int32_t get_chat_home::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "getChatHome", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    if (!new_tag_.empty())
        node_params.AddMember("tag", new_tag_, a);
    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), std::string());

    return 0;
}

int32_t get_chat_home::parse_results(const rapidjson::Value& _node_results)
{
    auto iter_restart = _node_results.FindMember("restart");
    if (iter_restart != _node_results.MemberEnd() && iter_restart->value.IsBool())
        need_restart_ = iter_restart->value.GetBool();

    if (need_restart_)
        return 0;

    auto iter_finished = _node_results.FindMember("finish");
    if (iter_finished != _node_results.MemberEnd() && iter_finished->value.IsBool())
        finished_ = iter_finished->value.GetBool();

    auto iter_new_tag = _node_results.FindMember("newTag");
    if (iter_new_tag != _node_results.MemberEnd() && iter_new_tag->value.IsString())
        result_tag_ = iter_new_tag->value.GetString();

    auto iter_chats = _node_results.FindMember("chats");
    if (iter_chats != _node_results.MemberEnd() && iter_chats->value.IsArray())
    {
        for (auto iter_chat = iter_chats->value.Begin(); iter_chat != iter_chats->value.End(); ++iter_chat)
        {
            const auto &node = *iter_chat;
            chat_info info;
            if (info.unserialize(node) != 0)
                return wpie_http_parse_response;
            result_.push_back(info);
        }
    }

    return 0;
}