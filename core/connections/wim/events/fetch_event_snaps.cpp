#include "stdafx.h"

#include "fetch_event_snaps.h"
#include "../wim_im.h"

using namespace core;
using namespace wim;

fetch_event_snaps::fetch_event_snaps()
{
}

fetch_event_snaps::~fetch_event_snaps()
{
}

int32_t fetch_event_snaps::parse(const rapidjson::Value& _node_event_data)
{
    auto iter_aimid = _node_event_data.FindMember("sn");
    if (iter_aimid != _node_event_data.MemberEnd() && iter_aimid->value.IsString())
        aimid_ = iter_aimid->value.GetString();

    auto iter_state = _node_event_data.FindMember("state");
    if (iter_state != _node_event_data.MemberEnd() && iter_state->value.IsObject())
        state_.unserialize(iter_state->value);

    return 0;
}

void fetch_event_snaps::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_snaps(this, _on_complete);
}
