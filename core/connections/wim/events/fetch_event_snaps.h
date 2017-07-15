#pragma once

#include "fetch_event.h"
#include "../snap_info.h"

namespace core
{
    namespace wim
    {
        class fetch_event_snaps: public fetch_event
        {
            std::string aimid_;

            snap_history_state state_;

        public:

            fetch_event_snaps();

            virtual  ~fetch_event_snaps();

            inline const std::string& aim_id() const { return aimid_; }

            inline const snap_history_state& state() const { return state_; }

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;

            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }
}