#include <boost/test/unit_test.hpp>

#include <common.shared/message_processing/message_tokenizer.h>

#ifdef CHECK_TOKEN_START
#error Rename the macros
#endif
#define CHECK_TOKEN_START common::tools::message_tokenizer tokenizer(_message);

#ifdef CHECK_TOKEN_N
#error Rename the macros
#endif
#define CHECK_TOKEN_N(N) \
    if (!tokenizer.has_token()) { std::cerr << "no token\n"; return false; } \
    if (tokenizer.current().type_ != get_type<T##N>()) { std::cerr << tokenizer.current().type_ << " != " << get_type<T##N>() << '\n'; return false; } \
    if (!equal<T##N>(tokenizer.current().data_, _v##N)) { std::cerr << to_string<T##N>(tokenizer.current().data_) << " != " << _v##N << '\n'; return false; } \
    tokenizer.next();

#ifdef CHECK_TOKEN_FINISH
#error Rename the macros
#endif
#define CHECK_TOKEN_FINISH \
    if (tokenizer.has_token()) { std::cerr << "too many tokens\n"; return false; } \
    return true;

namespace
{
    template <class T>
    common::tools::message_token::type get_type(typename std::enable_if<std::is_convertible<T, std::string>::value>::type* _dummy = nullptr)
    {
        return common::tools::message_token::type::text;
    }

    template <class T>
    common::tools::message_token::type get_type(typename std::enable_if<std::is_same<T, common::tools::url>::value>::type* _dummy = nullptr)
    {
        return common::tools::message_token::type::url;
    }

    template <class T>
    bool equal(const common::tools::message_token::data_t& x, const T& y, typename std::enable_if<std::is_convertible<T, std::string>::value>::type* _dummy = nullptr)
    {
        return boost::get<std::string>(x) == y;
    }

    template <class T>
    bool equal(const common::tools::message_token::data_t& x, const T& y, typename std::enable_if<std::is_same<T, common::tools::url>::value>::type* _dummy = nullptr)
    {
        return boost::get<T>(x) == y;
    }

    template <class T>
    std::string to_string(const common::tools::message_token::data_t& x, typename std::enable_if<std::is_convertible<T, std::string>::value>::type* _dummy = nullptr)
    {
        return boost::get<std::string>(x);
    }

    template <class T>
    std::string to_string(const common::tools::message_token::data_t& x, typename std::enable_if<std::is_same<T, common::tools::url>::value>::type* _dummy = nullptr)
    {
        std::stringstream buf;
        buf << boost::get<T>(x);
        return buf.str();
    }

    template <class T1>
    bool check(const char* _message, const T1& _v1)
    {
        CHECK_TOKEN_START
        CHECK_TOKEN_N(1)
        CHECK_TOKEN_FINISH
    }

    template <class T1, class T2>
    bool check(const char* _message, const T1& _v1, const T2& _v2)
    {
        CHECK_TOKEN_START
        CHECK_TOKEN_N(1)
        CHECK_TOKEN_N(2)
        CHECK_TOKEN_FINISH
    }

    template <class T1, class T2, class T3>
    bool check(const char* _message, const T1& _v1, const T2& _v2, const T3& _v3)
    {
        CHECK_TOKEN_START
        CHECK_TOKEN_N(1)
        CHECK_TOKEN_N(2)
        CHECK_TOKEN_N(3)
        CHECK_TOKEN_FINISH
    }

    template <class T1, class T2, class T3, class T4>
    bool check(const char* _message, const T1& _v1, const T2& _v2, const T3& _v3, const T4& _v4)
    {
        CHECK_TOKEN_START
        CHECK_TOKEN_N(1)
        CHECK_TOKEN_N(2)
        CHECK_TOKEN_N(3)
        CHECK_TOKEN_N(4)
        CHECK_TOKEN_FINISH
    }

    template <class T1, class T2, class T3, class T4, class T5>
    bool check(const char* _message, const T1& _v1, const T2& _v2, const T3& _v3, const T4& _v4, const T5& _v5)
    {
        CHECK_TOKEN_START
        CHECK_TOKEN_N(1)
        CHECK_TOKEN_N(2)
        CHECK_TOKEN_N(3)
        CHECK_TOKEN_N(4)
        CHECK_TOKEN_N(5)
        CHECK_TOKEN_FINISH
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    bool check(const char* _message, const T1& _v1, const T2& _v2, const T3& _v3, const T4& _v4, const T5& _v5, const T6& _v6)
    {
        CHECK_TOKEN_START
        CHECK_TOKEN_N(1)
        CHECK_TOKEN_N(2)
        CHECK_TOKEN_N(3)
        CHECK_TOKEN_N(4)
        CHECK_TOKEN_N(5)
        CHECK_TOKEN_N(6)
        CHECK_TOKEN_FINISH
    }
}

#undef CHECK_TOKEN_START
#undef CHECK_TOKEN_N
#undef CHECK_TOKEN_FINISH


BOOST_AUTO_TEST_SUITE(common)

BOOST_AUTO_TEST_SUITE(tools)

BOOST_AUTO_TEST_SUITE(message_processing)

BOOST_AUTO_TEST_CASE(test_message_tokenizer)
{
    using namespace common::tools;

    BOOST_CHECK(check("текст 8.8.8@ya.ru.", "текст ", url("8.8.8@ya.ru", url::type::email, url::protocol::undefined, url::extension::undefined), "."));
    BOOST_CHECK(check("http://mail.ru? текст", url("http://mail.ru", url::type::site, url::protocol::http, url::extension::undefined), "? текст"));
    BOOST_CHECK(check("O.life", url("http://O.life", url::type::site, url::protocol::http, url::extension::undefined)));
    BOOST_CHECK(check("O.life!", url("http://O.life", url::type::site, url::protocol::http, url::extension::undefined), "!"));
    BOOST_CHECK(check("O.life 18", url("http://O.life", url::type::site, url::protocol::http, url::extension::undefined), " 18"));
    BOOST_CHECK(check("https://ya.ru, Ok!", url("https://ya.ru", url::type::site, url::protocol::https, url::extension::undefined), ", Ok!"));
    BOOST_CHECK(check("test1@mail.ru\ntest2@mail.ru\ntest3@mail.ru", url("test1@mail.ru", url::type::email, url::protocol::undefined, url::extension::undefined), "\n",
        url("test2@mail.ru", url::type::email, url::protocol::undefined, url::extension::undefined), "\n",
        url("test3@mail.ru", url::type::email, url::protocol::undefined, url::extension::undefined)));
    BOOST_CHECK(check("получить https://files.icq.net/get/ ->> первая часть", "получить ", 
        url("https://files.icq.net/get/", url::type::site, url::protocol::https, url::extension::undefined), " ->> первая часть"));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
