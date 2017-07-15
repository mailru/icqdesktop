#pragma once

#include "snap_info.h"

namespace core
{
    namespace wim
    {
        class snaps_storage
        {
        public:
            snaps_storage();
            ~snaps_storage();

            void set_changed(bool _is_changed) { changed_ = _is_changed; }
            bool is_changed() const { return changed_; }
            bool is_empty() const { return snaps_.empty(); }

            void remove_user(const std::string& _aimId);

            bool is_user_exist(const std::string& _aimId) const;
            void update_snap_state(const std::string& _aimId, const snap_history_state& _state, bool& needUpdateSnaps, icollection* _coll);
            void update_snap_state(const std::string& _aimId, const snap_history_state& _state, icollection* _coll);

            void update_user_snaps(const std::string& _aimId, user_snaps_info _info, icollection* _coll);

            int64_t get_user_patch_version(const std::string& _aimId);

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void serialize(icollection* _coll);

            std::map<std::string, user_snaps_info> get_snaps() const;

        private:
            std::map<std::string, user_snaps_info> snaps_;
            bool changed_;
        };
    }
}