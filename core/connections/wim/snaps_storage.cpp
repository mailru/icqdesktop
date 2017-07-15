#include "stdafx.h"
#include "snaps_storage.h"

namespace core
{
namespace wim
{
    snaps_storage::snaps_storage()
        : changed_(false)
    {
    }

    snaps_storage::~snaps_storage()
    {
    }

    bool snaps_storage::is_user_exist(const std::string& _aimId) const
    {
        return snaps_.find(_aimId) != snaps_.end();
    }

    void snaps_storage::remove_user(const std::string& _aimId)
    {
        snaps_.erase(_aimId);
        changed_ = true;
    }

    void snaps_storage::update_snap_state(const std::string& _aimId, const snap_history_state& _state, bool& needUpdateSnaps, icollection* _coll)
    {
        needUpdateSnaps = false;

        if (snaps_[_aimId].state_.patchVersion_ == _state.patchVersion_ && snaps_[_aimId].state_.latestSnapId_ == _state.latestSnapId_)
        {
            update_snap_state(_aimId, _state, _coll);
        }

        if (snaps_[_aimId].state_.latestSnapId_ != _state.latestSnapId_ || snaps_[_aimId].state_.patchVersion_ != _state.patchVersion_)
            needUpdateSnaps = true;
    }

    void snaps_storage::update_snap_state(const std::string& _aimId, const snap_history_state& _state, icollection* _coll)
    {
        auto& state = snaps_[_aimId].state_;
        auto lastViewed = state.lastViewedSnapId_;
        state = _state;
        if (state.lastViewedSnapId_ == -1 && lastViewed != -1)
            state.lastViewedSnapId_ = lastViewed;
        changed_ = true;

        coll_helper cl(_coll, false);
        cl.set_value_as_string("aimId", _aimId);
        state.serialize(cl.get());
    }

    void snaps_storage::update_user_snaps(const std::string& _aimId, user_snaps_info _info, icollection* _coll)
    {
        if (snaps_.find(_aimId) != snaps_.end())
            _info.labels_ = snaps_[_aimId].labels_;

        snaps_[_aimId] = _info;
        snaps_[_aimId].aimId_ = _aimId;
        changed_ = true;

        snaps_[_aimId].serialize(_coll);
    }

    int64_t snaps_storage::get_user_patch_version(const std::string& _aimId)
    {
        if (!is_user_exist(_aimId))
            return -1;

        return snaps_[_aimId].state_.patchVersion_;
    }

    int32_t snaps_storage::unserialize(const rapidjson::Value& _node)
    {
        auto iter_chats = _node.FindMember("users");
        if (iter_chats != _node.MemberEnd() && iter_chats->value.IsArray())
        {
            for (auto iter_chat = iter_chats->value.Begin(); iter_chat != iter_chats->value.End(); ++iter_chat)
            {
                const auto &node = *iter_chat;
                user_snaps_info info;
                if (info.unserialize(node) != 0)
                    return 1;
                snaps_.insert(std::make_pair(info.aimId_, info));
            }
        }

        std::map<std::string, std::string> persons;
        auto iter_persons = _node.FindMember("persons");
        if (iter_persons != _node.MemberEnd() && iter_persons->value.IsArray())
        {
            for (auto person = iter_persons->value.Begin(); person != iter_persons->value.End(); ++person)
            {
                std::string aimId;
                auto iter_aimid = person->FindMember("sn");
                if (iter_aimid != person->MemberEnd() && iter_aimid->value.IsString())
                    aimId = iter_aimid->value.GetString();

                auto iter_friendly = person->FindMember("friendly");
                if (iter_friendly != person->MemberEnd() && iter_friendly->value.IsString())
                    persons[aimId] = iter_friendly->value.GetString();
            }
        }

        for (std::map<std::string, user_snaps_info>::iterator snap = snaps_.begin(); snap != snaps_.end(); ++snap)
        {
            if (persons.find(snap->first) != persons.end())
            {
                snap->second.friendly_ = persons[snap->first];
            }

            for (std::vector<snap_info>::iterator iter = snap->second.snaps_.begin(); iter != snap->second.snaps_.end(); ++iter)
            {
                if (!iter->originalCreator_.empty() && persons.find(iter->originalCreator_) != persons.end())
                {
                    iter->originalFriendly_ = persons[iter->originalCreator_];
                }
            }
        }

        set_changed(true);

        return 0;
    }

    void snaps_storage::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
    {
        rapidjson::Value users(rapidjson::Type::kArrayType);
        for (auto snap : snaps_)
        {
            rapidjson::Value user(rapidjson::Type::kObjectType);
            snap.second.serialize(user, _a);
            users.PushBack(user, _a);
        }
        _node.AddMember("users", users, _a);
    }

    void snaps_storage::serialize(icollection* _coll)
    {
        coll_helper cl(_coll, false);
        ifptr<iarray> users_array(_coll->create_array());
        users_array->reserve((int32_t)snaps_.size());
        for (auto snap : snaps_)
        {
            coll_helper coll_snap(_coll->create_collection(), true);
            snap.second.serialize(coll_snap.get());
            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(coll_snap.get());
            users_array->push_back(val.get());
        }
        cl.set_value_as_array("users", users_array.get());
    }

    std::map<std::string, user_snaps_info> snaps_storage::get_snaps() const
    {
        return snaps_;
    }
}
}