#include "stdafx.h"
#include "snap_info.h"

using namespace core;
using namespace wim;

void snap_history_state::unserialize(const rapidjson::Value& _node)
{
    auto iter_views = _node.FindMember("views");
    if (iter_views != _node.MemberEnd() && iter_views->value.IsInt())
        views_ = iter_views->value.GetInt();

    auto iter_views_current = _node.FindMember("viewsCurrent");
    if (iter_views_current != _node.MemberEnd() && iter_views_current->value.IsInt())
        viewsCurrent_ = iter_views_current->value.GetInt();

    auto iter_view_next_snap_id = _node.FindMember("viewNextSnapId");
    if (iter_view_next_snap_id != _node.MemberEnd() && iter_view_next_snap_id->value.IsInt64())
        viewNextSnapId_ = iter_view_next_snap_id->value.GetInt64();

    auto iter_patch_version = _node.FindMember("patchVersion");
    if (iter_patch_version != _node.MemberEnd() && iter_patch_version->value.IsInt64())
        patchVersion_ = iter_patch_version->value.GetInt64();

    auto iter_view_lastest_snap_id = _node.FindMember("latestSnapId");
    if (iter_view_lastest_snap_id != _node.MemberEnd() && iter_view_lastest_snap_id->value.IsInt64())
        latestSnapId_ = iter_view_lastest_snap_id->value.GetInt64();

    auto iter_expires = _node.FindMember("expires");
    if (iter_expires != _node.MemberEnd() && iter_expires->value.IsInt())
    {
        expires_ = iter_expires->value.GetInt();
    }
    else
    {
        auto iter_valid_thru = _node.FindMember("validThru");
        if (iter_valid_thru != _node.MemberEnd() && iter_valid_thru->value.IsInt())
            expires_ = iter_valid_thru->value.GetInt();
    }

    auto iter_view_last_viewed_snap_id = _node.FindMember("lastViewedSnapId");
    if (iter_view_last_viewed_snap_id != _node.MemberEnd() && iter_view_last_viewed_snap_id->value.IsInt64())
        lastViewedSnapId_ = iter_view_last_viewed_snap_id->value.GetInt64();
}

void snap_history_state::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    _node.AddMember("views", views_, _a);
    _node.AddMember("viewsCurrent", viewsCurrent_, _a);
    _node.AddMember("viewNextSnapId", viewNextSnapId_, _a);
    _node.AddMember("patchVersion", patchVersion_, _a);
    _node.AddMember("latestSnapId", latestSnapId_, _a);
    _node.AddMember("expires", expires_, _a);
    _node.AddMember("lastViewedSnapId", lastViewedSnapId_, _a);
}

void snap_history_state::serialize(icollection* _coll)
{
    coll_helper cl(_coll, false);
    cl.set_value_as_int64("viewNextSnapId", viewNextSnapId_);
    cl.set_value_as_int64("latestSnapId", latestSnapId_);
    cl.set_value_as_int64("lastViewedSnapId", lastViewedSnapId_);
    cl.set_value_as_int("views", views_);
    cl.set_value_as_int("viewsCurrent", viewsCurrent_);
}

void snap_object::unserialize(const rapidjson::Value& _node)
{
    auto iter_type_ = _node.FindMember("type");
    if (iter_type_ != _node.MemberEnd() && iter_type_->value.IsString())
    {
        type_ = iter_type_->value.GetString();
    }

    auto iter_id_ = _node.FindMember("id");
    if (iter_id_ != _node.MemberEnd() && iter_id_->value.IsString())
    {
        id_ = iter_id_->value.GetString();
    }
}

void snap_object::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    _node.AddMember("type", type_, _a);
    _node.AddMember("id", id_, _a);
}

void snap_info::unserialize(const rapidjson::Value& _node)
{
    auto iter_snapid = _node.FindMember("snapId");
    if (iter_snapid != _node.MemberEnd() && iter_snapid->value.IsInt64())
        snapId_ = iter_snapid->value.GetInt64();

    auto iter_original = _node.FindMember("original");
    if (iter_original != _node.MemberEnd() && iter_original->value.IsObject())
    {
        auto iter_creator = iter_original->value.FindMember("creator");
        if (iter_creator != iter_original->value.MemberEnd() && iter_creator->value.IsString())
        {
            originalCreator_ = iter_creator->value.GetString();
        }

        auto iter_friendly = iter_original->value.FindMember("friendly");
        if (iter_friendly != iter_original->value.MemberEnd() && iter_friendly->value.IsString())
        {
            originalFriendly_ = iter_friendly->value.GetString();
        }

        auto iter_original_snap_id = iter_original->value.FindMember("snapId");
        if (iter_original_snap_id != iter_original->value.MemberEnd() && iter_original_snap_id->value.IsInt64())
        {
            originalSnapId_ = iter_original_snap_id->value.GetInt64();
        }
    }

    auto iter_url = _node.FindMember("url");
    if (iter_url != _node.MemberEnd() && iter_url->value.IsString())
    {
        url_ = iter_url->value.GetString();
    }

    auto iter_link_code = _node.FindMember("linkCode");
    if (iter_link_code != _node.MemberEnd() && iter_link_code->value.IsString())
    {
        std::string linkCode = iter_link_code->value.GetString();
        url_ = "https://files.icq.net/get/" + linkCode;
    }

    auto iter_time_stamp = _node.FindMember("timestamp");
    if (iter_time_stamp != _node.MemberEnd() && iter_time_stamp->value.IsInt())
    {
        timestamp_ = iter_time_stamp->value.GetInt();
    }

    auto iter_ttl = _node.FindMember("ttl");
    if (iter_ttl != _node.MemberEnd() && iter_ttl->value.IsInt())
    {
        ttl_ = iter_ttl->value.GetInt();
    }

    auto iter_content_type = _node.FindMember("contentType");
    if (iter_content_type != _node.MemberEnd() && iter_content_type->value.IsString())
    {
        contentType_ = iter_content_type->value.GetString();
    }

    auto iter_duration = _node.FindMember("duration");
    if (iter_duration != _node.MemberEnd() && iter_duration->value.IsInt())
    {
        duration_ = iter_duration->value.GetInt();
    }

    auto iter_views = _node.FindMember("views");
    if (iter_views != _node.MemberEnd() && iter_views->value.IsInt())
    {
        views_ = iter_views->value.GetInt();
    }

    auto iter_req_id = _node.FindMember("reqid");
    if (iter_req_id != _node.MemberEnd() && iter_req_id->value.IsString())
    {
        reqId_ = iter_req_id->value.GetString();
    }

    auto iter_abused = _node.FindMember("abused");
    if (iter_abused != _node.MemberEnd() && iter_abused->value.IsInt())
    {
        abused_ = iter_abused->value.GetInt();
    }

    auto iter_source = _node.FindMember("source");
    if (iter_source != _node.MemberEnd() && iter_source->value.IsString())
    {
        source_ = iter_source->value.GetString();
    }

    auto iter_objects = _node.FindMember("objects");
    if (iter_objects != _node.MemberEnd() && iter_objects->value.IsArray())
    {
        for (auto object = iter_objects->value.Begin(); object != iter_objects->value.End(); ++object)
        {
            snap_object obj;
            obj.unserialize(*object);
            objects_.push_back(obj);
        }
    }

    auto iter_type = _node.FindMember("type");
    if (iter_type != _node.MemberEnd() && iter_type->value.IsString())
    {
        std::string type = iter_type->value.GetString();
        isAvatar_ = type == "avatar";
    }
}

void snap_info::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    _node.AddMember("snapId", snapId_, _a);

    rapidjson::Value node_original(rapidjson::Type::kObjectType);
    node_original.AddMember("creator", originalCreator_, _a);
    node_original.AddMember("friendly", originalFriendly_, _a);
    node_original.AddMember("snapId", originalSnapId_, _a);
    _node.AddMember("original", node_original, _a);

    _node.AddMember("url", url_, _a);
    _node.AddMember("timestamp", timestamp_, _a);
    _node.AddMember("ttl", ttl_, _a);
    _node.AddMember("contentType", contentType_, _a);
    _node.AddMember("duration", duration_, _a);
    _node.AddMember("views", views_, _a);
    _node.AddMember("reqid", reqId_, _a);
    _node.AddMember("abused", abused_, _a);
    _node.AddMember("source", source_, _a);

    rapidjson::Value node_objects(rapidjson::Type::kArrayType);
    for (auto obj : objects_)
    {
        rapidjson::Value node_object(rapidjson::Type::kObjectType);
        obj.serialize(node_object, _a);
        node_objects.PushBack(node_object, _a);
    }
    _node.AddMember("objects", node_objects, _a);

    if (isAvatar_)
    {
        std::string type = "avatar";
        _node.AddMember("type", type, _a);
    }
}

void snap_info::serialize(icollection* _coll)
{
    coll_helper cl(_coll, false);
    cl.set_value_as_int64("snapId", snapId_);
    cl.set_value_as_int64("originalSnapId", originalSnapId_);
    cl.set_value_as_string("originalAimId", originalCreator_);
    cl.set_value_as_string("originalFriendly", originalFriendly_);
    cl.set_value_as_string("url", url_);
    cl.set_value_as_string("contentType", contentType_);
    cl.set_value_as_int("duration", duration_);
    cl.set_value_as_int("views", views_);
    cl.set_value_as_int("timestamp", timestamp_);
    cl.set_value_as_int("ttl", ttl_);
}

int32_t user_snaps_info::unserialize(const rapidjson::Value& _node)
{
    auto iter_aimid = _node.FindMember("sn");
    if (iter_aimid != _node.MemberEnd() && iter_aimid->value.IsString())
    {
        aimId_ =  iter_aimid->value.GetString();
    }

    auto iter_labels = _node.FindMember("labels");
    if (iter_labels != _node.MemberEnd() && iter_labels->value.IsArray())
    {
        for (auto label = iter_labels->value.Begin(); label != iter_labels->value.End(); ++label)
        {
            labels_.push_back(label->GetString());
        }
    }

    auto iter_state = _node.FindMember("state");
    if (iter_state != _node.MemberEnd() && iter_state->value.IsObject())
    {
        state_.unserialize(iter_state->value);
    }

    auto iter_official = _node.FindMember("official");
    if (iter_official != _node.MemberEnd() && iter_official->value.IsBool())
    {
        official_ = iter_official->value.GetBool();
    }

    auto iter_friendly = _node.FindMember("friendly");
    if (iter_friendly != _node.MemberEnd() && iter_friendly->value.IsString())
    {
        friendly_ = iter_friendly->value.GetString();
    }

    auto iter_snaps = _node.FindMember("snaps");
    if (iter_snaps != _node.MemberEnd() && iter_snaps->value.IsArray())
    {
        for (auto snap = iter_snaps->value.Begin(); snap != iter_snaps->value.End(); ++snap)
        {
            snap_info info;
            info.unserialize(*snap);
            snaps_.push_back(info);
        }
    }

    auto iter_persons = _node.FindMember("persons");
    if (iter_persons != _node.MemberEnd() && iter_persons->value.IsArray())
    {
        for (auto person = iter_persons->value.Begin(); person != iter_persons->value.End(); ++person)
        {
            std::string aimId;
            auto iter_aimid = person->FindMember("sn");
            if (iter_aimid != person->MemberEnd() && iter_aimid->value.IsString())
                aimId = iter_aimid->value.GetString();

            std::string friendly;
            auto iter_friendly = person->FindMember("friendly");
            if (iter_friendly != person->MemberEnd() && iter_friendly->value.IsString())
            {
                friendly = iter_friendly->value.GetString();
            }

            if (aimId == aimId_)
            {
                friendly_ = friendly;
            }

            for (std::vector<snap_info>::iterator iter = snaps_.begin(); iter != snaps_.end(); ++iter)
            {
                if (iter->originalCreator_ == aimId)
                {
                    iter->originalFriendly_ = friendly;
                }
            }
        }
    }

    return 0;
}

void user_snaps_info::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    _node.AddMember("sn", aimId_, _a);

    rapidjson::Value node_labels(rapidjson::Type::kArrayType);
    for (auto label : labels_)
    {
        rapidjson::Value node_label;
        node_label.SetString(label, _a);
        node_labels.PushBack(node_label, _a);
    }

    _node.AddMember("labels", node_labels, _a);

    rapidjson::Value node_state(rapidjson::Type::kObjectType);
    state_.serialize(node_state, _a);
    _node.AddMember("state", node_state, _a);

    if (official_)
        _node.AddMember("official", official_, _a);

    _node.AddMember("friendly", friendly_, _a);

    rapidjson::Value node_snaps(rapidjson::Type::kArrayType);
    for (auto snap : snaps_)
    {
        rapidjson::Value node_snap(rapidjson::Type::kObjectType);
        snap.serialize(node_snap, _a);
        node_snaps.PushBack(node_snap, _a);
    }
    _node.AddMember("snaps", node_snaps, _a);
}

void user_snaps_info::serialize(icollection* _coll)
{
    coll_helper cl(_coll, false);
    cl.set_value_as_string("aimId", aimId_);
    cl.set_value_as_bool("official", official_);
    cl.set_value_as_string("friendly", friendly_);
    coll_helper coll_state(_coll->create_collection(), true);
    state_.serialize(coll_state.get());
    cl.set_value_as_collection("state", coll_state.get());
    cl.set_value_as_bool("buddy", std::find(labels_.begin(), labels_.end(), "buddy") != labels_.end());
    ifptr<iarray> snaps_array(_coll->create_array());
    snaps_array->reserve((int32_t)snaps_.size());
    for (auto snap : snaps_)
    {
        coll_helper coll_snap(_coll->create_collection(), true);
        snap.serialize(coll_snap.get());
        ifptr<ivalue> val(_coll->create_value());
        val->set_as_collection(coll_snap.get());
        snaps_array->push_back(val.get());
    }
    cl.set_value_as_array("snaps", snaps_array.get());
}