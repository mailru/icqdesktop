#pragma once

namespace common
{
    namespace tools
    {
        struct url
        {
            enum class type
            {
                undefined,

                image,
                video,
                filesharing,
                site,
                email,
                ftp
            };

            enum class protocol
            {
                undefined,

                http,
                ftp,
                https,
                ftps
            };

            enum class extension
            {
                undefined,

                _3gp,
                avi,
                bmp,
                flv,
                gif,
                jpeg,
                jpg,
                mkv,
                mov,
                mpeg4,
                png,
                tiff,
                webm,
                wmv
            };

            std::string original_;
            std::string url_;
            type type_;
            protocol protocol_;
            extension extension_;

            url();
            url(const std::string& _url, type _type, protocol _protocol, extension _extension);
            url(const std::string& _original, const std::string& _url, type _type, protocol _protocol, extension _extension);

            bool is_filesharing() const;
            bool is_image() const;
            bool is_video() const;
            bool is_site() const;
            bool is_email() const;
            bool is_ftp() const;
            bool has_prtocol() const;

            bool operator==(const url& _right) const;
            bool operator!=(const url& _right) const;
        };

        typedef std::vector<url> url_vector_t;

        class url_parser
        {
            enum class states
            {
                lookup,
                protocol_t1,
                protocol_t2,
                protocol_p,
                protocol_s,
                check_www,
                www_2,
                www_3,
                www_4,
                compare,
                compare_safe,
                delimeter_colon,
                delimeter_slash1,
                delimeter_slash2,
                filesharing_id,
                check_filesharing,
                password,
                host,
                host_n,
                host_at,
                host_dot,
                host_colon,
                host_slash,
                port,
                port_dot,
                port_slash,
                path,
                path_dot,
                path_slash,
                jpg_p,      // jpg
                jpg_g,
                jpg_e,      // jpeg
                png_n,      // png
                png_g,
                gif_i,      // gif
                gif_f,
                bmp_m,      // bmp
                bmp_p,
                tiff_i,     // tiff
                tiff_f1,
                tiff_f2,
                avi_v,      // avi
                avi_i,
                mkv_k,      // mkv
                mkv_v,
                mov_v,      // mov
                mpeg4_e,    // mpeg
                mpeg4_g,
                mpeg4_4,
                flv_l,      // flv
                flv_v,
                _3gp_g,     // 3gp
                _3gp_p,
                webm_e,     // webm
                webm_b,
                webm_m,
                wmv_v,      // wmv
                query_or_end,
                query
            };

        public:
            url_parser();

            void process(char c);
            void finish();
            void reset();

            bool has_url() const;
            const url& get_url() const;

            bool skipping_chars() const;

            int32_t raw_url_length() const;
            int32_t tail_size() const;

            static url_vector_t parse_urls(const std::string& _source);

        private:
            void process();

            bool save_url();

            void compare(const char* _char, states _ok_state, states _fallback_state, int _safe_position);
            void fallback(char _last_char);

            bool is_equal(const char* _text) const;

            void save_char_buf(std::string& _to);
            void save_to_buf(char c);

            static const int32_t min_filesharing_id_length = 33;

            bool is_space(char _c, bool _is_utf8) const;
            bool is_digit(char _c) const;
            bool is_digit(char _c, bool _is_utf8) const;
            bool is_letter(char _c, bool _is_utf8) const;
            bool is_letter_or_digit(char _c, bool _is_utf8) const;
            bool is_allowable_char(char _c, bool _is_utf8) const;
            bool is_allowable_query_char(char _c, bool _is_utf8) const;
            bool is_ending_char(char _c) const;
            bool is_valid_top_level_domain(const std::string& _name) const;

            url make_url(url::type _type) const;

            bool is_ipv4_segment(const std::string _name) const;

        private:
            states state_;

            url::protocol protocol_;

            std::string buf_;
            std::string domain_;

            std::string compare_buf_;
            const char* to_compare_;
            int32_t compare_pos_;
            states ok_state_;
            states fallback_state_;
            int safe_position_;

            url::extension extension_;

            bool is_not_email_;
            bool is_email_;

            bool has_protocol_prefix_;

            int32_t domain_segments_;
            int32_t ipv4_segnents_;

            int32_t id_length_;

            char char_buf_[6];
            int32_t char_pos_;
            int32_t char_size_;

            bool is_utf8_;

            url url_;

            bool need_to_check_domain_;

            int32_t tail_size_;
        };
    }
}

const char* to_string(common::tools::url::type _value);
const char* to_string(common::tools::url::protocol _value);
const char* to_string(common::tools::url::extension _value);

std::ostream& operator<<(std::ostream& _out, const common::tools::url& _url);
