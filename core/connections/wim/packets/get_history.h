#ifndef __ROBUSTO_GET_HISTORY
#define __ROBUSTO_GET_HISTORY

#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace archive
    {
        class history_message;
        class dlg_state;
        class history_patch;

        typedef std::vector<std::shared_ptr<history_message>> history_block;

        typedef std::unique_ptr<history_patch> history_patch_uptr;
    }

    namespace wim
    {
        struct get_history_params
        {
            const std::string	aimid_;
            const int64_t		till_msg_id_;
            const int64_t		from_msg_id_;
            const int32_t		count_;
            const std::string   patch_version_;
            bool                init_;

            get_history_params(
                const std::string &_aimid,
                const int64_t _from_msg_id,
                const int64_t _till_msg_id,
                const int32_t _count,
                const std::string &_patch_version,
                bool _init = false
                );
        };


        class get_history : public robusto_packet
        {
            int64_t older_msgid_;

            get_history_params hist_params_;

            std::shared_ptr<archive::history_block> messages_;
            std::shared_ptr<archive::dlg_state> dlg_state_;
            std::vector<archive::history_patch_uptr> history_patches_;
            std::string patch_version_;
            std::string locale_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

            void apply_patches();

            void parse_patches(const rapidjson::Value& _node_patch);

            void set_last_message();

        public:
            get_history(
                const wim_packet_params& _params,
                const get_history_params& _hist_params,
                const std::string& _locale
                );

            virtual ~get_history();

            std::shared_ptr<archive::history_block> get_messages() { return messages_; }
            std::shared_ptr<archive::dlg_state> get_dlg_state() { return dlg_state_; }
            const get_history_params& get_hist_params() const { return hist_params_; }
            int64_t get_older_msgid() const { return older_msgid_; }
        };

    }

}


#endif// __ROBUSTO_GET_HISTORY
