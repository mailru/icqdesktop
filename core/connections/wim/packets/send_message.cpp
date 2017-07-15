#include "stdafx.h"
#include "send_message.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../archive/history_message.h"

using namespace core;
using namespace wim;

send_message::send_message(
    const wim_packet_params& _params,
    const message_type _type,
    const std::string& _internal_id,
    const std::string& _aimid,
    const std::string& _message_text,
    const core::archive::quotes_vec& _quotes)
    :
        wim_packet(_params),
        type_(_type),
        internal_id_(_internal_id),
        aimid_(_aimid),
        message_text_(_message_text),
        sms_error_(0),
        sms_count_(0),
        duplicate_(false),
        hist_msg_id_(-1),
        before_hist_msg_id(-1),
        quotes_(_quotes)
{
    assert(!internal_id_.empty());
}


send_message::~send_message()
{

}

int32_t send_message::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::string method;

    const auto is_sticker = (type_ == message_type::sticker);
    const auto is_sms = (type_ == message_type::sms);

    if (is_sticker && quotes_.empty())
        method = "sendSticker";
    else
        method = "sendIM";

    std::stringstream ss_url;
    ss_url << c_wim_host << "im/" << method;

    _request->set_url(ss_url.str());
    _request->set_keep_alive();
    _request->set_priority(top_priority);
    _request->push_post_parameter("f", "json");
    _request->push_post_parameter("aimsid", escape_symbols(get_params().aimsid_));
    _request->push_post_parameter("t", escape_symbols(aimid_));
    _request->push_post_parameter("r", internal_id_);

    if (!quotes_.empty())
    {
        rapidjson::Document doc(rapidjson::Type::kArrayType);
        auto& a = doc.GetAllocator();
        if (quotes_.empty())
        doc.Reserve(quotes_.size() + 1, a);

        for (auto quote : quotes_)
        {
            rapidjson::Value quote_params(rapidjson::Type::kObjectType);
            quote_params.AddMember("mediaType", quote.get_type(), a);
            auto sticker = quote.get_sticker();
            if (!sticker.empty())
                quote_params.AddMember("stickerId", sticker, a);
            else
                quote_params.AddMember("text", quote.get_text(), a);
            quote_params.AddMember("sn", quote.get_sender(), a);
            quote_params.AddMember("msgId", quote.get_msg_id(), a);
            quote_params.AddMember("time", quote.get_time(), a);
            doc.PushBack(quote_params, a);
        }

        if (!message_text_.empty())
        {
            rapidjson::Value text_params(rapidjson::Type::kObjectType);
            if (is_sticker)
            {
                text_params.AddMember("mediaType", "sticker", a);
                text_params.AddMember("stickerId", message_text_, a);
                doc.PushBack(text_params, a);
            }
            else
            {
                text_params.AddMember("mediaType", "text", a);
                text_params.AddMember("text", message_text_, a);
                doc.PushBack(text_params, a);
            }
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        _request->push_post_parameter("parts", escape_symbols(buffer.GetString()));
    }

    std::string message_text = is_sticker ? message_text_ : escape_symbols(message_text_);

    if (quotes_.empty())
        _request->push_post_parameter((is_sticker ?  "stickerId" : "message"), message_text);

    if (is_sms)
    {
        _request->push_post_parameter("displaySMSSegmentData", "true");
    }
    else
    {
        _request->push_post_parameter("offlineIM", "1");
        _request->push_post_parameter("notifyDelivery", "true");
    }
    
    if (!params_.full_log_)
    {
        set_replace_log_function(is_sticker, _request, message_text);
    }

    return 0;
}

void send_message::set_replace_log_function(bool _is_sticker, std::shared_ptr<core::http_request_simple> _request, const std::string& _message_text)
{
    if (!quotes_.empty())
    {
        std::vector<std::string> vtext;
        vtext.reserve(quotes_.size() + 1);
        vtext.push_back(_message_text);

        for (const auto& quote : quotes_)
            vtext.push_back(wim_packet::escape_symbols(quote.get_text()));

        _request->set_replace_log_function([vtext](tools::binary_stream& _bs)
        {
            for (const auto& _text : vtext)
            {
                uint32_t sz = _bs.available();
                char* logdata = _bs.get_data();

                if (!logdata || !sz)
                    return;

                //"text":"some text"
                const std::string marker(wim_packet::escape_symbols("\"text\":\""));

                std::string search_text = marker + _text + wim_packet::escape_symbols("\"");

                char* cursor = std::search(logdata, logdata + sz, search_text.c_str(), search_text.c_str() + search_text.length());

                if (cursor >= logdata + sz)
                {
                    assert(false);
                    return;
                }

                cursor += marker.length();
                if (cursor >= logdata + sz)
                {
                    assert(false);
                    return;
                }

                for (uint32_t i  = 0; i < _text.length() && cursor < logdata + sz; ++i)
                {
                    *cursor++ = '*';
                }
            }
        });
    }
    else
    {
        if (!_is_sticker && !_message_text.empty())
        {
            _request->set_replace_log_function([_message_text](tools::binary_stream& _bs)
            {
                uint32_t sz = _bs.available();
                char* logdata = _bs.get_data();

                if (!logdata || !sz)
                    return;

                std::string marker("&message=");

                std::string search_text = marker + _message_text;

                char* cursor = std::search(logdata, logdata + sz, search_text.c_str(), search_text.c_str() + search_text.length());

                if (cursor >= logdata + sz)
                {
                    assert(false);
                    return;
                }

                cursor += marker.length();
                if (cursor >= logdata + sz)
                {
                    assert(false);
                    return;
                }

                for (uint32_t i  = 0; i < _message_text.length() && cursor < logdata + sz; ++i)
                {
                    *cursor++ = '*';
                }
            });
        }
    }

    
}

int32_t send_message::parse_response_data(const rapidjson::Value& _data)
{
    auto iter_msgid = _data.FindMember("msgId");
    if (iter_msgid != _data.MemberEnd() && iter_msgid->value.IsString())
        wim_msg_id_ = iter_msgid->value.GetString();

    auto iter_hist_msgid = _data.FindMember("histMsgId");
    if (iter_hist_msgid != _data.MemberEnd() && iter_hist_msgid->value.IsInt64())
        hist_msg_id_ = iter_hist_msgid->value.GetInt64();

    auto iter_before_hist_msgid = _data.FindMember("beforeHistMsgId");
    if (iter_before_hist_msgid != _data.MemberEnd() && iter_before_hist_msgid->value.IsInt64())
        before_hist_msg_id = iter_before_hist_msgid->value.GetInt64();

    auto iter_state = _data.FindMember("state");
    if (iter_state != _data.MemberEnd() && iter_state->value.IsString())
    {
        std::string state = iter_state->value.GetString();
        if (state == "duplicate")
        {
            duplicate_ = true;
        }
    }

    return 0;
}

int32_t send_message::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}
