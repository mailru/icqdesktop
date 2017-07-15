#include "stdafx.h"

#ifdef __APPLE__
    #include "../../core/tools/strings.h"
#else
    #include "../core/tools/strings.h"
#endif

#include "url_parser.h"

namespace
{
    #include "domain_parser.in"

    const char* FILES_ICQ_NET = "files.icq.net/get/";
    static const int FILES_ICQ_NET_SAFE_POS = strlen("files.icq.net") - 1;

    const char* ICQ_COM = "icq.com/files/";
    static const int ICQ_COM_SAFE_POS = strlen("icq.com") - 1;

    const char* CHAT_MY_COM = "chat.my.com/files/";
    static const int CHAT_MY_COM_SAFE_POS = strlen("chat.my.com") - 1;

    const char* LEFT_DOUBLE_QUOTE = "«";
    const char* RIGHT_DOUBLE_QUOTE = "»";
}

const char* to_string(common::tools::url::type _value)
{
    switch (_value)
    {
        case common::tools::url::type::undefined:   return "undefined";
        case common::tools::url::type::image:       return "image";
        case common::tools::url::type::video:       return "video";
        case common::tools::url::type::filesharing: return "filesharing";
        case common::tools::url::type::site:        return "site";
        case common::tools::url::type::email:       return "email";
        case common::tools::url::type::ftp:         return "ftp";
        default:                                    return "#unknown";
    }
}

const char* to_string(common::tools::url::protocol _value)
{
    switch (_value)
    {
        case common::tools::url::protocol::undefined:   return "undefined";
        case common::tools::url::protocol::http:        return "http";
        case common::tools::url::protocol::ftp:         return "ftp";
        case common::tools::url::protocol::https:       return "https";
        case common::tools::url::protocol::ftps:        return "ftps";
        default:                                        return "#unknown";
    }
}

const char* to_string(common::tools::url::extension _value)
{
    switch (_value)
    {
        case common::tools::url::extension::undefined:  return "undefined";
        case common::tools::url::extension::avi:        return "avi";
        case common::tools::url::extension::bmp:        return "bmp";
        case common::tools::url::extension::flv:        return "flv";
        case common::tools::url::extension::gif:        return "gif";
        case common::tools::url::extension::jpeg:       return "jpeg";
        case common::tools::url::extension::jpg:        return "jpg";
        case common::tools::url::extension::mkv:        return "mkv";
        case common::tools::url::extension::mov:        return "mov";
        case common::tools::url::extension::mpeg4:       return "mpeg4";
        case common::tools::url::extension::png:        return "png";
        case common::tools::url::extension::tiff:       return "tiff";
        case common::tools::url::extension::webm:       return "webm";
        case common::tools::url::extension::wmv:        return "wmv";
        default:                                        return "#unknown";
    }
}

common::tools::url::url()
    : type_(type::undefined)
{
}

common::tools::url::url(const std::string& _url, type _type, protocol _protocol, extension _extension)
    : original_(_url)
    , url_(_url)
    , type_(_type)
    , protocol_(_protocol)
    , extension_(_extension)
{
}

common::tools::url::url(const std::string& _original, const std::string& _url, type _type, protocol _protocol, extension _extension)
    : original_(_original)
    , url_(_url)
    , type_(_type)
    , protocol_(_protocol)
    , extension_(_extension)
{
}

bool common::tools::url::is_filesharing() const
{
    return type_ == type::filesharing;
}

bool common::tools::url::is_image() const
{
    return type_ == type::image;
}

bool common::tools::url::is_video() const
{
    return type_ == type::video;
}

bool common::tools::url::is_site() const
{
    return type_ == type::site;
}

bool common::tools::url::is_email() const
{
    return type_ == type::email;
}

bool common::tools::url::is_ftp() const
{
    return type_ == type::ftp;
}

bool common::tools::url::has_prtocol() const
{
    if (original_.length() < 4)
        return false;

    return (original_.compare(0, 4, "http", 4) == 0);
}

bool common::tools::url::operator==(const url& _right) const
{
    return url_ == _right.url_
        && type_ == _right.type_
        && protocol_== _right.protocol_
        && extension_ == _right.extension_;
}

bool common::tools::url::operator!=(const url& _right) const
{
    return !(*this == _right);
}

std::ostream& operator<<(std::ostream& _out, const common::tools::url& _url)
{
    _out << "['"
        << _url.url_
        << "', "
        << to_string(_url.type_)
        << ", "
        << to_string(_url.protocol_)
        << ", "
        << to_string(_url.extension_)
        << "]";
    return _out;
}

common::tools::url_parser::url_parser()
{
    reset();
}

void common::tools::url_parser::process(char c)
{
    char_buf_[char_pos_] = c;

    if (char_pos_ > 0)
    {
        ++char_pos_;

        if (char_pos_ < char_size_)
            return;

        char_pos_ = 0;
        process();
        return;
    }

    char_size_ = core::tools::utf8_char_size(c);

    is_utf8_ = char_size_ > 1;

    if (!is_utf8_)
        process();
    else
        ++char_pos_;
}

bool common::tools::url_parser::has_url() const
{
    return url_.type_ != url::type::undefined;
}

const common::tools::url& common::tools::url_parser::get_url() const
{
    return url_;
}

bool common::tools::url_parser::skipping_chars() const
{
    return state_ == states::lookup;
}

int32_t common::tools::url_parser::raw_url_length() const
{
    return buf_.size();
}

int32_t common::tools::url_parser::tail_size() const
{
    return tail_size_;
}

void common::tools::url_parser::finish()
{
    save_url();
}

common::tools::url_vector_t common::tools::url_parser::parse_urls(const std::string& _source)
{
    url_vector_t urls;

    url_parser parser;

    for (char c : _source)
    {
        parser.process(c);
        if (parser.has_url())
        {
            urls.push_back(parser.get_url());
            parser.reset();
        }
    }

    parser.finish();
    if (parser.has_url())
    {
        urls.push_back(parser.get_url());
    }

    return urls;
}

#ifdef URL_PARSER_RESET_PARSER
#error Rename the macros
#endif
#define URL_PARSER_RESET_PARSER { reset(); return; }

#ifdef URL_PARSER_URI_FOUND
#error Rename the macros
#endif
#define URL_PARSER_URI_FOUND { if (!save_url()) reset(); return; }

void common::tools::url_parser::process()
{
    const char c = char_buf_[0];
    switch (state_)
    {
    case states::lookup:
        if (c == 'w' || c == 'W') { state_ = states::www_2; break; }
        else if (c == 'h' || c == 'H') { protocol_ = url::protocol::http; state_ = states::protocol_t1; break; }
        else if (c == 'f' || c == 'F') { protocol_ = url::protocol::ftp; state_ = states::protocol_t1; break; }
        else if (c == 'f' || c == 'F') { compare(FILES_ICQ_NET, states::filesharing_id, states::host, FILES_ICQ_NET_SAFE_POS); id_length_ = 0; break; }
        else if (c == 'i' || c == 'I') { compare(ICQ_COM, states::filesharing_id, states::host, ICQ_COM_SAFE_POS); id_length_ = 0; break; }
        else if (c == 'c' || c == 'C') { compare(CHAT_MY_COM, states::filesharing_id, states::host, CHAT_MY_COM_SAFE_POS); id_length_ = 0; break; }
        else if (c == LEFT_DOUBLE_QUOTE[0] && char_size_ == 2 && char_buf_[1] == LEFT_DOUBLE_QUOTE[1]) return;
        else if (is_letter_or_digit(c, is_utf8_)) { save_char_buf(domain_); state_ = states::host; break; }
        return;
    case states::protocol_t1:
        if (c == 't' || c == 'T') state_ = protocol_ == url::protocol::ftp ? states::protocol_p : states::protocol_t2;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; state_ = states::host; }
        break;
    case states::protocol_t2:
        if (c == 't' || c == 'T') state_ = states::protocol_p;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; state_ = states::host; }
        break;
    case states::protocol_p:
        if (c == 'p' || c == 'P') state_ = states::protocol_s;
        else if (is_space(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else { protocol_ = url::protocol::undefined; state_ = states::host; }
        break;
    case states::protocol_s:
        if (c == 's' || c == 'S') { if (protocol_ == url::protocol::http) protocol_ = url::protocol::https; else protocol_ = url::protocol::ftps; state_ = states::delimeter_colon; }
        else if (c == '.') { protocol_ = url::protocol::undefined; state_ = states::host_dot; }
        else if (c == ':') state_ = states::delimeter_slash1;
        else if (protocol_ == url::protocol::ftp && is_allowable_char(c, is_utf8_)) { protocol_ = url::protocol::undefined; state_ = states::host; }
        else URL_PARSER_RESET_PARSER
        break;
    case states::check_www:
        if (c == 'w' || c == 'W') state_ = states::www_2;
        else if (c == 'f' || c == 'F') { compare(FILES_ICQ_NET, states::filesharing_id, states::host, FILES_ICQ_NET_SAFE_POS); id_length_ = 0; break; }
        else if (c == 'i' || c == 'I') { compare(ICQ_COM, states::filesharing_id, states::host, ICQ_COM_SAFE_POS); id_length_ = 0; break; }
        else if (c == 'c' || c == 'C') { compare(CHAT_MY_COM, states::filesharing_id, states::host, CHAT_MY_COM_SAFE_POS); id_length_ = 0; break; }
        else if (is_allowable_char(c, is_utf8_)) { save_char_buf(domain_); state_ = states::host; }
        else URL_PARSER_RESET_PARSER
        break;
    case states::www_2:
        if (c == 'w' || c == 'W') state_ = states::www_3;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::host;
        else URL_PARSER_RESET_PARSER
        break;
    case states::www_3:
        if (c == 'w' || c == 'W') state_ = states::www_4;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::host;
        else URL_PARSER_RESET_PARSER
        break;
    case states::www_4:
        if (c == '.') state_ = states::check_filesharing;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::host;
        else URL_PARSER_RESET_PARSER
        break;
    case states::compare:
        if (compare_pos_ >= safe_position_) state_ = states::compare_safe;
    case states::compare_safe:
        if (!is_equal(to_compare_ + compare_pos_)) { fallback(*char_buf_); return; }
        save_char_buf(compare_buf_);
        save_to_buf(to_compare_[compare_pos_]);
        compare_pos_ += char_size_;
        if (!to_compare_[compare_pos_]) state_ = ok_state_;
        return;
    case states::delimeter_colon:
        if (c == ':') state_ = states::delimeter_slash1;
        else if (c == '.') { protocol_ = url::protocol::undefined; state_ = states::host_dot; }
        else URL_PARSER_RESET_PARSER
        break;
    case states::delimeter_slash1:
        if (c == '/') state_ = states::delimeter_slash2;
        else URL_PARSER_RESET_PARSER
        break;
    case states::delimeter_slash2:
        if (c == '/') { has_protocol_prefix_ = true; state_ = states::check_www; }
        else URL_PARSER_RESET_PARSER
        break;
    case states::filesharing_id:
        ++id_length_;
        if (is_space(c, is_utf8_)) { if (id_length_ >= min_filesharing_id_length) URL_PARSER_URI_FOUND else { state_ = states::host; need_to_check_domain_ = false; URL_PARSER_URI_FOUND } }
        else if (!is_letter_or_digit(c, is_utf8_)) URL_PARSER_RESET_PARSER
        break;
    case states::check_filesharing:
        if (c == 'f' || c == 'F') { compare(FILES_ICQ_NET, states::filesharing_id, states::host, FILES_ICQ_NET_SAFE_POS); id_length_ = 0; break; }
        else if (c == 'i' || c == 'I') { compare(ICQ_COM, states::filesharing_id, states::host, ICQ_COM_SAFE_POS); id_length_ = 0; break; }
        else if (c == 'c' || c == 'C') { compare(CHAT_MY_COM, states::filesharing_id, states::host, CHAT_MY_COM_SAFE_POS); id_length_ = 0; break; }
        else if (is_allowable_char(c, is_utf8_)) { save_char_buf(domain_); state_ = states::host; }
        else URL_PARSER_RESET_PARSER
        break;
    case states::password:
        if (c == '@') state_ = states::host;
        else if (!is_allowable_char(c, is_utf8_)) URL_PARSER_RESET_PARSER
        break;
    case states::host:
        if (c == '.') state_ = states::host_dot;
        else if (c == ':') state_ = states::host_colon;
        else if (c == '@') state_ = states::host_at;
        else if (c == '/') { if (protocol_ == url::protocol::undefined) URL_PARSER_RESET_PARSER }
        else if (c == '?') URL_PARSER_RESET_PARSER
        else if (!is_allowable_char(c, is_utf8_)) { if (protocol_ != url::protocol::undefined) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else save_char_buf(domain_);
        break;
    case states::host_n:
        if (c == '.') state_ = states::host_dot;
        else if (c == ':') { if (!is_email_ && is_valid_top_level_domain(domain_)) state_ = states::host_colon; else URL_PARSER_RESET_PARSER }
        else if (c == '@') { if (is_email_ || is_not_email_) URL_PARSER_RESET_PARSER else { state_ = states::host; is_email_ = true; } }
        else if (c == '/') { if (!is_email_ && is_valid_top_level_domain(domain_)) state_ = states::host_slash; else URL_PARSER_RESET_PARSER }
        else if (c == '?') { if (is_email_) URL_PARSER_URI_FOUND else if (is_valid_top_level_domain(domain_)) state_ = states::query; else URL_PARSER_RESET_PARSER }
        else if (c == '#') { if (!is_email_ && is_valid_top_level_domain(domain_)) state_ = states::query; else URL_PARSER_RESET_PARSER }
        else if (is_space(c, is_utf8_)) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER; }
        else if (is_ending_char(c)) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (c == RIGHT_DOUBLE_QUOTE[0] && char_size_ == 2 && char_buf_[1] == RIGHT_DOUBLE_QUOTE[1]) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (!is_letter_or_digit(c, is_utf8_) && c != '-') URL_PARSER_RESET_PARSER
        else save_char_buf(domain_);
        break;
    case states::host_at:
        if (!is_allowable_char(c, is_utf8_)) URL_PARSER_RESET_PARSER
        else if (protocol_ != url::protocol::undefined) state_ = states::host;
        else if (is_email_ || is_not_email_) URL_PARSER_RESET_PARSER 
        else { state_ = states::host; is_email_ = true; }
        break;
    case states::host_dot:
        ++domain_segments_;
        if (is_ipv4_segment(domain_))
            ++ipv4_segnents_; 
        if (is_letter_or_digit(c, is_utf8_)) { domain_.clear(); save_char_buf(domain_); state_ = states::host_n; }
        else if (is_space(c, is_utf8_)) { --domain_segments_; URL_PARSER_URI_FOUND }
        else URL_PARSER_RESET_PARSER
        break;
    case states::host_colon:
        if (is_digit(c, is_utf8_)) { if (is_valid_top_level_domain(domain_)) state_ = states::port; else URL_PARSER_RESET_PARSER }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::password;
        else URL_PARSER_RESET_PARSER
        break;
    case states::host_slash:
        if (c == '/') break;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else URL_PARSER_RESET_PARSER
        break;
    case states::port:
        if (c == '/') state_ = states::port_slash;
        else if (!is_digit(c, is_utf8_)) URL_PARSER_RESET_PARSER
        break;
    case states::port_dot:
        if (is_digit(c, is_utf8_)) state_ = states::port;
        else URL_PARSER_RESET_PARSER
        break;
    case states::port_slash:
        if (is_letter_or_digit(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::path:
        if (c == '.') state_ = states::path_dot;
        else if (c == '/') state_ = states::path_slash;
        else if (c == '?') state_ = states::query;
        else if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else if (is_ending_char(c)) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (c == RIGHT_DOUBLE_QUOTE[0] && char_size_ == 2 && char_buf_[1] == RIGHT_DOUBLE_QUOTE[1]) { if (is_valid_top_level_domain(domain_)) URL_PARSER_URI_FOUND else URL_PARSER_RESET_PARSER }
        else if (!is_allowable_char(c, is_utf8_)) URL_PARSER_RESET_PARSER
        break;
    case states::path_dot:
        if (c == 'j' || c == 'J') state_ = states::jpg_p;
        else if (c == 'p' || c == 'P') state_ = states::png_n;
        else if (c == 'g' || c == 'G') state_ = states::gif_i;
        else if (c == 'b' || c == 'B') state_ = states::bmp_m;
        else if (c == 't' || c == 'T') state_ = states::tiff_i;
        else if (c == 'a' || c == 'A') state_ = states::avi_v;
        else if (c == 'm' || c == 'M') state_ = states::mkv_k;
        else if (c == 'f' || c == 'F') state_ = states::flv_l;
        else if (c == '3') state_ = states::_3gp_g;
        else if (c == 'w' || c == 'W') state_ = states::webm_e;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else URL_PARSER_RESET_PARSER
        break;
    case states::path_slash:
        if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else if (c == '?') state_ = states::query;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::jpg_p:
        if (c == 'p' || c == 'P') state_ = states::jpg_g;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::jpg_g:
        if (c == 'g' || c == 'G') { extension_ = url::extension::jpg; state_ = states::query_or_end; }
        else if (c == 'e' || c == 'E') state_ = states::jpg_e;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::jpg_e:
        if (c == 'g' || c == 'G') { extension_ = url::extension::jpeg; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::png_n:
        if (c == 'n' || c == 'N') state_ = states::png_g;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::png_g:
        if (c == 'g' || c == 'G') { extension_ = url::extension::png; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::gif_i:
        if (c == 'i' || c == 'I') state_ = states::gif_f;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::gif_f:
        if (c == 'f' || c == 'F') { extension_ = url::extension::gif; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::bmp_m:
        if (c == 'm' || c == 'M') state_ = states::bmp_p;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::bmp_p:
        if (c == 'p' || c == 'P') { extension_ = url::extension::bmp; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::tiff_i:
        if (c == 'i' || c == 'I') state_ = states::tiff_f1;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::tiff_f1:
        if (c == 'f' || c == 'F') state_ = states::tiff_f2;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::tiff_f2:
        if (c == 'f' || c == 'F') { extension_ = url::extension::tiff; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::avi_v:
        if (c == 'v' || c == 'V') state_ = states::avi_i;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::avi_i:
        if (c == 'i' || c == 'I') { extension_ = url::extension::avi; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::mkv_k:
        if (c == 'k' || c == 'K') state_ = states::mkv_v;
        else if (c == 'o' || c == 'O') state_ = states::mov_v;
        else if (c == 'p' || c == 'P') state_ = states::mpeg4_e;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::mkv_v:
        if (c == 'v' || c == 'V') { extension_ = url::extension::mkv; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::mov_v:
        if (c == 'v' || c == 'V') { extension_ = url::extension::mov; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::mpeg4_e:
        if (c == 'e' || c == 'E') state_ = states::mpeg4_g;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::mpeg4_g:
        if (c == 'g' || c == 'G') state_ = states::mpeg4_4;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::mpeg4_4:
        if (c == '4') { extension_ = url::extension::mpeg4; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::flv_l:
        if (c == 'l' || c == 'L') state_ = states::flv_v;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::flv_v:
        if (c == 'v' || c == 'V') { extension_ = url::extension::flv; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::_3gp_g:
        if (c == 'g' || c == 'G') state_ = states::_3gp_p;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::_3gp_p:
        if (c == 'p' || c == 'P') { extension_ = url::extension::_3gp; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::webm_e:
        if (c == 'e' || c == 'E') state_ = states::webm_b;
        else if (c == 'm' || c == 'M') state_ = states::wmv_v;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::webm_b:
        if (c == 'b' || c == 'B') state_ = states::webm_m;
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::webm_m:
        if (c == 'm' || c == 'M') { extension_ = url::extension::webm; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::wmv_v:
        if (c == 'v' || c == 'V') { extension_ = url::extension::wmv; state_ = states::query_or_end; }
        else if (is_allowable_char(c, is_utf8_)) state_ = states::path;
        else URL_PARSER_RESET_PARSER
        break;
    case states::query_or_end:
        if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else if (c == '>') URL_PARSER_URI_FOUND
        else if (c == '?' || c == '!' || c == ',' || c == '#' || c == '/' || c == ':' || c == '.') state_ = states::query;
        else if (c == '_') state_ = states::path;
        else if (is_letter_or_digit(c, is_utf8_)) { extension_ = url::extension::undefined; state_ = states::query; }
        else URL_PARSER_RESET_PARSER
        break; 
    case states::query:
        if (is_space(c, is_utf8_)) URL_PARSER_URI_FOUND
        else if (!is_allowable_query_char(c, is_utf8_)) URL_PARSER_RESET_PARSER
        break;
    default:
        assert(!"invalid state_!");
    };

    save_char_buf(buf_);
}

#undef URL_PARSER_RESET_PARSER

#undef URL_PARSER_URI_FOUND

void common::tools::url_parser::reset()
{
    state_ = states::lookup;

    protocol_ = url::protocol::undefined;

    buf_.clear();
    domain_.clear();

    to_compare_ = nullptr;
    compare_pos_ = 0;

    extension_ = url::extension::undefined;

    is_not_email_ = false;
    is_email_ = false;

    has_protocol_prefix_ = false;

    domain_segments_ = 1;
    ipv4_segnents_ = 1;

    id_length_ = 0;

    char_pos_ = 0;
    char_size_ = 0;

    is_utf8_ = false;

    url_.type_ = url::type::undefined;

    need_to_check_domain_ = true;

    tail_size_ = 0;
}

bool common::tools::url_parser::save_url()
{
    switch (state_)
    {
    case states::lookup:
        return false;
    case states::protocol_t1:
        return false;
    case states::protocol_t2:
        return false;
    case states::protocol_p:
        return false;
    case states::protocol_s:
        return false;
    case states::check_www:
        return false;
    case states::www_2:
        return false;
    case states::www_3:
        return false;
    case states::www_4:
        return false;
    case states::compare:
        return false;
    case states::compare_safe:
        url_ = make_url(protocol_ == url::protocol::ftp || protocol_ == url::protocol::ftps ? common::tools::url::type::ftp : common::tools::url::type::site);
        return true;
    case states::delimeter_colon:
        return false;
    case states::delimeter_slash1:
        return false;
    case states::delimeter_slash2:
        return false;
    case states::filesharing_id:
        break;
    case states::check_filesharing:
        return false;
    case states::password:
        return false;
    case states::host:
        if (protocol_ != url::protocol::undefined)
            break;
        return false;
    case states::host_n:
        break;
    case states::host_at:
        break;
    case states::host_dot:
        break;
    case states::host_colon:
        return false;
    case states::host_slash:
        break;
    case states::port:
        break;
    case states::port_dot:
        break;
    case states::port_slash:
        break;
    case states::path:
        break;
    case states::path_dot:
        break;
    case states::path_slash:
        break;
    case states::jpg_p:
        break;
    case states::jpg_g:
        break;
    case states::jpg_e:
        break;
    case states::png_n:
        break;
    case states::png_g:
        break;
    case states::gif_i:
        break;
    case states::gif_f:
        break;
    case states::bmp_m:
        break;
    case states::bmp_p:
        break;
    case states::tiff_i:
        break;
    case states::tiff_f1:
        break;
    case states::tiff_f2:
        break;
    case states::avi_v:
        break;
    case states::avi_i:
        break;
    case states::mkv_k:
        break;
    case states::mkv_v:
        break;
    case states::mov_v:
        break;
    case states::mpeg4_e:
        break;
    case states::mpeg4_g:
        break;
    case states::mpeg4_4:
        break;
    case states::flv_l:
        break;
    case states::flv_v:
        break;
    case states::_3gp_g:
        break;
    case states::_3gp_p:
        break;
    case states::webm_e:
        break;
    case states::webm_b:
        break;
    case states::webm_m:
        break;
    case states::wmv_v:
        break;
    case states::query_or_end:
        break;
    case states::query:
        break;
    default:
        assert(!"you must handle all enumerations");
    };

    if (domain_segments_ == 1 && state_ != states::filesharing_id && protocol_ == url::protocol::undefined)
        return false;

    while (is_ending_char(buf_.back()))
    {
        buf_.pop_back();
        ++tail_size_;
    }

    if (is_email_ && protocol_ == url::protocol::undefined)
    {
        url_ = make_url(common::tools::url::type::email);
    }
    else if (extension_ == url::extension::bmp
        || extension_ == url::extension::gif
        || extension_ == url::extension::jpeg
        || extension_ == url::extension::jpg
        || extension_ == url::extension::png
        || extension_ == url::extension::tiff)
    {
        url_ = make_url(common::tools::url::type::image);
    }
    else if (extension_ == url::extension::_3gp
        || extension_ == url::extension::avi
        || extension_ == url::extension::flv
        || extension_ == url::extension::mkv
        || extension_ == url::extension::mov
        || extension_ == url::extension::mpeg4
        || extension_ == url::extension::webm
        || extension_ == url::extension::wmv)
    {
        url_ = make_url(common::tools::url::type::video);
    }
    else
    {
        const auto is_filesharing_link =
            (state_ == states::filesharing_id) && (id_length_ >= min_filesharing_id_length);
        if (is_filesharing_link)
        {
            url_ = make_url(common::tools::url::type::filesharing);
        }
        else
        {
            if (need_to_check_domain_ && !is_valid_top_level_domain(domain_))
                return false;
            url_ = make_url(protocol_ == url::protocol::ftp || protocol_ == url::protocol::ftps ? common::tools::url::type::ftp : common::tools::url::type::site);
        }
    }

    return true;
}

common::tools::url common::tools::url_parser::make_url(url::type _type) const
{
    if (!has_protocol_prefix_ && _type != url::type::email)
    {
        switch (protocol_)
        {
        case url::protocol::undefined:
            return url(buf_, "http://" + buf_, _type, url::protocol::http, extension_);
        case url::protocol::http:
            return url(buf_, "http://" + buf_, _type, protocol_, extension_);
        case url::protocol::ftp:
            return url(buf_, "ftp://" + buf_, _type, protocol_, extension_);
        case url::protocol::https:
            return url(buf_, "https://" + buf_, _type, protocol_, extension_);
        case url::protocol::ftps:
            return url(buf_, "ftps://" + buf_, _type, protocol_, extension_);
        }
    }

    return url(buf_, buf_, _type, protocol_, extension_);
}

void common::tools::url_parser::compare(const char* _text, states _ok_state, states _fallback_state, int _safe_position)
{
    if (!is_equal(_text))
    {
        state_ = _fallback_state;
        return;
    }

    compare_buf_.clear();
    save_char_buf(compare_buf_);
    to_compare_ = _text;
    compare_pos_ = 1;
    state_ = states::compare;
    ok_state_ = _ok_state;
    fallback_state_ = _fallback_state;
    safe_position_ = _safe_position;
}

void common::tools::url_parser::fallback(char _last_char)
{
    state_ = fallback_state_;

    buf_.resize(buf_.size() - compare_pos_);

    for (char c : compare_buf_)
        process(c);

    process(_last_char);
}

bool common::tools::url_parser::is_equal(const char* _text) const
{
    if (!is_utf8_)
        return char_buf_[0] == *_text || tolower(char_buf_[0]) == *_text;

    for (int i = 0; i < char_size_; ++i)
        if (char_buf_[i] != _text[i])
            return false;

    return true;
}

void common::tools::url_parser::save_char_buf(std::string& _to)
{
    std::copy(char_buf_, char_buf_ + char_size_, std::back_inserter(_to));
}

void common::tools::url_parser::save_to_buf(char c)
{
    buf_.push_back(c);
}

bool common::tools::url_parser::is_space(char _c, bool _is_utf8) const
{
    return !_is_utf8 && isspace(_c);
}

bool common::tools::url_parser::is_digit(char _c) const
{
    return _c >= '0' && _c <= '9';
}

bool common::tools::url_parser::is_digit(char _c, bool _is_utf8) const
{
     return !_is_utf8 && is_digit(_c);
}

bool common::tools::url_parser::is_letter(char _c, bool _is_utf8) const
{
    return _is_utf8 || (_c >= 0x41 && _c <= 0x5A) || (_c >= 0x61 && _c <= 0x7A);
}

bool common::tools::url_parser::is_letter_or_digit(char _c, bool _is_utf8) const
{
    return is_letter(_c, _is_utf8) || is_digit(_c);
}

bool common::tools::url_parser::is_allowable_char(char _c, bool _is_utf8) const
{
    if (is_letter_or_digit(_c, _is_utf8))
        return true;

    return _c == '-' || _c == '.' || _c == '_' || _c == '~' || _c == '!' || _c == '$' || _c == '&'
        || _c == '\'' || _c == '(' || _c == ')'|| _c == '*' || _c == '+' || _c == ',' || _c == ';'
        || _c == '=' || _c == ':' || _c == '%' || _c == '?' || _c == '#' || _c == '@'
        || _c == '{' || _c == '}' || _c == '/';

    /*
        https://www.w3.org/Addressing/URL/uri-spec.html

        national
            { | } | vline | [ | ] | \ | ^ | ~

        The "national" and "punctuation" characters do not appear in any productions and therefore may not appear in URIs.

        But we must support the links with "national" characters therefore we process { }
    */
}

bool common::tools::url_parser::is_allowable_query_char(char _c, bool _is_utf8) const
{
    if (is_allowable_char(_c, _is_utf8))
        return true;

    return _c == '/' || _c == '?';
}

bool common::tools::url_parser::is_ending_char(char _c) const
{
    switch (_c)
    {
    case '.': return true;
    case ',': return true;
    case '!': return true;
    case '?': return true;
    case '"': return true;
    case '\'': return true;
    case '`': return true;
    case '>': return true;
    case ']': return true;
    }
    return false;
}

bool common::tools::url_parser::is_valid_top_level_domain(const std::string& _name) const
{
    if (_name.empty())
        return false;

    if (protocol_ != url::protocol::undefined)
        return true;

    if (domain_segments_ == 4 && ipv4_segnents_ == 4)
        return true;

    return is_valid_domain(_name);
}

bool common::tools::url_parser::is_ipv4_segment(const std::string _name) const
{
    if (_name.size() <= 3)
    {
        switch (_name.size()) // ok let this is ip4 address
        {
        case 1:
            if (is_digit(_name[0])) return true;
            break;
        case 2:
            if (is_digit(_name[0]) && is_digit(_name[1])) return true;
            break;
        case 3:
            if (is_digit(_name[0]) && is_digit(_name[1]) && is_digit(_name[2])) return true;
        }
    }

    return false;
}
