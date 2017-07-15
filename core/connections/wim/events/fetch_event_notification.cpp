#include "stdafx.h"

#include "fetch_event_notification.h"
#include "../wim_im.h"
#include "../wim_packet.h"

using namespace core;
using namespace wim;

fetch_event_notification::fetch_event_notification()
{
}


fetch_event_notification::~fetch_event_notification()
{
}

int32_t fetch_event_notification::parse(const rapidjson::Value& _node_event_data)
{
    auto iter_fields = _node_event_data.FindMember("fields");
    if (iter_fields != _node_event_data.MemberEnd() && iter_fields->value.IsArray())
    {
        for (auto iter_f = iter_fields->value.Begin(); iter_f != iter_fields->value.End(); iter_f++)
        {
            mailbox_storage::unserialize(*iter_f, changes_);
        }
    }
    return 0;
}

void fetch_event_notification::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_notification(this, _on_complete);
}
