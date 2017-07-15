#include "stdafx.h"
#include "imstat.h"

#include "../wim_packet.h"
#include "../packets/send_imstat.h"
#include "../../../utils.h"
#include "../../../external/openssl/openssl/blowfish.h"
#include "../../../core.h"

namespace core
{
    namespace statistic
    {
        enum stat_params
        {
            device_id		= 44,
            referer			= 131,
            first_referer	= 133
        };

        const int32_t send_period = 24;
        const int32_t blowfish_key_length = 56;

        imstat::imstat()
            :	last_start_time_(std::chrono::system_clock::now() - std::chrono::hours(send_period))

        {
        }

        imstat::~imstat()
        {
        }

        bool imstat::need_send()
        {
            return ((std::chrono::system_clock::now() - last_start_time_) > std::chrono::hours(send_period));
        }

        std::string get_referer()
        {
            std::string referer = utils::get_product_name();
#ifdef _WIN32
            CRegKey key;
            if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, CAtlString(L"Software\\") + tools::from_utf8(utils::get_product_name().c_str()).c_str()))
            {
                wchar_t buffer[1025];
                unsigned long len = 1024;
                if (ERROR_SUCCESS == key.QueryStringValue(L"referer", buffer, &len))
                {
                    referer = tools::from_utf16(buffer);
                }

            }
#endif //_WIN32

            return referer;
        }

        std::string get_first_referer()
        {
            std::string referer = utils::get_product_name();
#ifdef _WIN32
            CRegKey key;
            if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, CAtlString(L"Software\\") + tools::from_utf8(utils::get_product_name().c_str()).c_str()))
            {
                wchar_t buffer[1025];
                unsigned long len = 1024;
                if (ERROR_SUCCESS == key.QueryStringValue(L"referer_first", buffer, &len))
                {
                    referer = tools::from_utf16(buffer);
                }

            }
#endif //_WIN32

            return referer;
        }

        std::string encrypt_stat(const std::string& _key, const std::string& _stat)
        {
            std::stringstream out_data;

            std::string key_normalized;
            while (key_normalized.length() + _key.length() <= blowfish_key_length)
                key_normalized += _key;

            key_normalized += _key.substr(0, blowfish_key_length - key_normalized.length());

            BF_KEY bf_key;
            BF_set_key(&bf_key, (uint32_t)key_normalized.length(), (const unsigned char*) key_normalized.c_str());

            uint32_t in_size = (uint32_t)_stat.length() + 1;
            const unsigned char* in = (const unsigned char*) _stat.c_str();

            unsigned char out_buffer[BF_BLOCK];

            while (in_size >= 8)
            {
                BF_ecb_encrypt(in, out_buffer, &bf_key, BF_ENCRYPT);

                in += BF_BLOCK;
                in_size -= BF_BLOCK;

                out_data << wim::wim_packet::escape_symbols_data((const char*) out_buffer, BF_BLOCK);
            }
            if (in_size > 0)
            {
                unsigned char buf8[BF_BLOCK];
                memcpy(buf8, in, in_size);
                for (auto i = in_size; i < BF_BLOCK; i++)
                {
                    buf8[i] = rand();
                }

                BF_ecb_encrypt(buf8, out_buffer, &bf_key, BF_ENCRYPT);
                out_data << wim::wim_packet::escape_symbols_data((const char*) out_buffer, BF_BLOCK);
            }

            return out_data.str();
        }

        std::shared_ptr<wim::wim_packet> imstat::get_statistic(const wim::wim_packet_params& _params)
        {
            if (!_params.aimid_.empty())
            {
                boost::property_tree::ptree xml;
                {
                    boost::property_tree::ptree node_stats;
                    {
                        boost::property_tree::ptree node_useragent;
                        {
                            node_useragent.put("<xmlattr>.name", utils::get_user_agent());
                            node_useragent.put("<xmlattr>.platform", utils::get_platform_string());
                        }
                        node_stats.add_child("useragent", node_useragent);

                        boost::property_tree::ptree node_a;
                        {
                            node_a.put("<xmlattr>.login", _params.aimid_);
                            node_a.put("<xmlattr>.protocol_uid", _params.aimid_);
                            node_a.put("<xmlattr>.server", "Boss");
                            node_a.put("<xmlattr>.account_type", ((build::is_icq()) ? "Icq" : "Agent"));
                            node_a.put("<xmlattr>.main", "1");

                            boost::property_tree::ptree node_l;
                            {
                                node_l.put("<xmlattr>.id", "1");

                                boost::property_tree::ptree node_p_referer;
                                {
                                    node_p_referer.put_value(get_referer());
                                    node_p_referer.put("<xmlattr>.id", (int32_t) stat_params::referer);
                                }
                                node_l.add_child("p", node_p_referer);

                                boost::property_tree::ptree node_p_referer_first;
                                {
                                    node_p_referer_first.put_value(get_first_referer());
                                    node_p_referer_first.put("<xmlattr>.id", (int32_t) stat_params::first_referer);
                                }
                                node_l.add_child("p", node_p_referer_first);

                                boost::property_tree::ptree node_p_device_id;
                                {
                                    node_p_device_id.put_value(g_core->get_uniq_device_id());
                                    node_p_device_id.put("<xmlattr>.id", (int32_t) stat_params::device_id);
                                }
                                node_l.add_child("p", node_p_device_id);
                            }
                            node_a.add_child("l", node_l);
                        }
                        node_stats.add_child("a", node_a);
                    }				
                    xml.add_child("stats", node_stats);
                }


                std::stringstream output_stream;
                boost::property_tree::write_xml(output_stream, xml);

                last_start_time_ = std::chrono::system_clock::now();

                return std::make_shared<wim::send_imstat>(_params, encrypt_stat(_params.aimid_, output_stream.str()));
            }

            return std::shared_ptr<wim::wim_packet>();
        }

    }
}
