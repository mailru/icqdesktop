#pragma once

#include "../../../corelib/collection_helper.h"

namespace core
{
    namespace wim
    {
        struct snap_history_state
        {
            snap_history_state()
                : viewNextSnapId_(-1)
                , patchVersion_(-1)
                , latestSnapId_(-1)
                , lastViewedSnapId_(-1)
                , views_(0)
                , viewsCurrent_(0)
                , expires_(0)
            {
            }

            int64_t viewNextSnapId_;
            int64_t patchVersion_;
            int64_t latestSnapId_;
            int64_t lastViewedSnapId_;
            int32_t views_;
            int32_t viewsCurrent_;
            int32_t expires_;

            void unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void serialize(icollection* _coll);
        };

        struct snap_object
        {
            std::string type_;
            std::string id_;

            void unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
        };

        struct snap_info
        {
            snap_info()
                : snapId_(-1)
                , originalSnapId_(-1)
                , timestamp_(0)
                , ttl_(0)
                , duration_(0)
                , views_(0)
                , abused_(0)
                , isAvatar_(false)
            {
            }

            int64_t snapId_;
            int64_t originalSnapId_;
            int32_t timestamp_;
            int32_t ttl_;
            int32_t duration_;
            int32_t views_;
            int32_t abused_;
            std::string originalCreator_;
            std::string originalFriendly_;
            std::string url_;
            std::string contentType_;
            std::string reqId_;
            std::string source_;
            bool isAvatar_;

            std::vector<snap_object> objects_;

            void unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void serialize(icollection* _coll);
        };

        struct user_snaps_info
        {
            user_snaps_info()
                : official_(false)
            {
            }

            std::string aimId_;
            std::string friendly_;
            snap_history_state state_;
            bool official_;
            std::vector<std::string> labels_;
            std::vector<snap_info> snaps_;

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
            void serialize(icollection* _coll);
        };

    }
}