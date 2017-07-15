#pragma once

#include "fetch_event.h"
#include "../imstate.h"
#include "../mailboxes.h"

namespace core
{
    namespace wim
    {
        class mailbox_storage;

        class fetch_event_notification : public fetch_event
        {
        public:
            fetch_event_notification();
            virtual ~fetch_event_notification();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            mailbox_changes changes_;
        };

    }
}
