#include <boost/test/unit_test.hpp>

#include <core/tools/url.h>

BOOST_AUTO_TEST_SUITE(core)

BOOST_AUTO_TEST_SUITE(tools)

BOOST_AUTO_TEST_SUITE(test_url)

BOOST_AUTO_TEST_CASE(test_encode_url)
{
    using namespace core::tools;

    BOOST_CHECK_EQUAL(
        "http://img0.joyreactor.cc/pics/post/%D0%B3%D0%B8%D1%84%D0%BA%D0%B8-%D1%81%D0%BB%D0%BE%D0%B2-%D0%BD%D0%B5%D1%82-%D1%82%D0%BE%D0%BB%D1%8C%D0%BA%D0%BE-%D0%B1%D0%BE%D0%BB%D1%8C-3735838.gif",
        encode_url("http://img0.joyreactor.cc/pics/post/гифки-слов-нет-только-боль-3735838.gif"));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
