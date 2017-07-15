#include <boost/test/unit_test.hpp>

#include <common.shared/url_parser/url_parser.h>

namespace
{
    bool check(const std::string& _source, const common::tools::url_vector_t& _expected)
    {
        const auto actual = common::tools::url_parser::parse_urls(_source);

        if (actual.size() != _expected.size())
        {
            std::cout << "  size mismatch: " << actual.size() << " != " << _expected.size() << " (expected)\n";
            for (auto& s : actual)
                std::cout << "  " << s.url_ << '\n';
            return false;
        }

        bool failed = false;
        for (size_t i = 0, size = _expected.size(); i != size; ++i)
        {
            if (actual[i].url_ != _expected[i].url_)
            {
                std::cout << "  \"" << actual[i].url_ << "\" != \"" << _expected[i].url_ << "\" (expected)\n";
                failed = true;
            }

            if (actual[i].type_ != _expected[i].type_)
            {
                std::cout << "  \"" << to_string(actual[i].type_) << "\" != \"" << to_string(_expected[i].type_) << "\" (expected)\n";
                failed = true;
            }

            if (_expected[i].type_ == common::tools::url::type::email)
                continue;

            if (actual[i].protocol_ != _expected[i].protocol_)
            {
                std::cout << "  \"" << to_string(actual[i].protocol_) << "\" != \"" << to_string(_expected[i].protocol_) << "\" (expected)\n";
                failed = true;
            }

            if (actual[i].extension_ != _expected[i].extension_)
            {
                std::cout << "  \"" << to_string(actual[i].extension_) << "\" != \"" << to_string(_expected[i].extension_) << "\" (expected)\n";
                failed = true;
            }
        }

        if (failed)
        {
            return false;
        }

        return true;
    }

    struct DataWrapper
    {
        DataWrapper& clear()
        {
            data_.clear();
            return *this;
        }

        DataWrapper& replace(const std::string& _url, common::tools::url::type _type = common::tools::url::type::image)
        {
            data_.clear();
            return append(_url, _type);
        }

        DataWrapper& append(const std::string& _url, common::tools::url::type _type = common::tools::url::type::image)
        {
            data_.emplace_back(_url, _type, common::tools::url::protocol::http, common::tools::url::extension::undefined);
            return *this;
        }

        DataWrapper& replace(const std::string& _url, common::tools::url::type _type, common::tools::url::extension _extension)
        {
            data_.clear();
            return append(_url, _type, _extension);
        }

        DataWrapper& append(const std::string& _url, common::tools::url::type _type, common::tools::url::extension _extension)
        {
            data_.emplace_back(_url, _type, common::tools::url::protocol::http, _extension);
            return *this;
        }

        DataWrapper& replace(const std::string& _url, common::tools::url::type _type, common::tools::url::protocol _protocol)
        {
            data_.clear();
            return append(_url, _type, _protocol);
        }

        DataWrapper& append(const std::string& _url, common::tools::url::type _type, common::tools::url::protocol _protocol)
        {
            data_.emplace_back(_url, _type, _protocol, common::tools::url::extension::undefined);
            return *this;
        }

        DataWrapper& replace(const std::string& _url, common::tools::url::type _type, common::tools::url::protocol _protocol, common::tools::url::extension _extension)
        {
            data_.clear();
            return append(_url, _type, _protocol, _extension);
        }

        DataWrapper& append(const std::string& _url, common::tools::url::type _type, common::tools::url::protocol _protocol, common::tools::url::extension _extension)
        {
            data_.emplace_back(_url, _type, _protocol, _extension);
            return *this;
        }

        operator const common::tools::url_vector_t&() const
        {
            return data_;
        }

        common::tools::url_vector_t data_;
    };
}

BOOST_AUTO_TEST_SUITE(common)

BOOST_AUTO_TEST_SUITE(tools)

BOOST_AUTO_TEST_SUITE(test_url_parser)

BOOST_AUTO_TEST_CASE(all_tests)
{
    using namespace common::tools;

    DataWrapper data;

    BOOST_CHECK(check("«12.127.17.72/test»", data.replace("http://12.127.17.72/test", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("«12.127.17.72»", data.replace("http://12.127.17.72", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("https://jira.mail.ru/browse/SWA-3529. hello", data.replace("https://jira.mail.ru/browse/SWA-3529", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("https://jira.mail.ru/browse/SWA-3529\" hello", data.replace("https://jira.mail.ru/browse/SWA-3529", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("https://jira.mail.ru/browse/SWA-3529> hello", data.replace("https://jira.mail.ru/browse/SWA-3529", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("https://jira.mail.ru/browse/SWA-3529` hello", data.replace("https://jira.mail.ru/browse/SWA-3529", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("http://mail.ru]", data.replace("http://mail.ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("http://mail.ru? текст", data.replace("http://mail.ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("cleric@bk.ru?", data.replace("cleric@bk.ru", url::type::email)));
    BOOST_CHECK(check("получить https://files.icq.net/get/ ->> первая часть", data.replace("https://files.icq.net/get/", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("https://o.life/бука💀", data.replace("https://o.life/бука💀", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("кто-нибудь вживую? https://sys.mail.ru/users/k.kashirina/ ",
        data.replace("https://sys.mail.ru/users/k.kashirina/", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("krybachuk@bk.ru\nvsras@inbox.ru\natachenko@bk.ru",
        data.replace("krybachuk@bk.ru", url::type::email).append("vsras@inbox.ru", url::type::email).append("atachenko@bk.ru", url::type::email)));
    BOOST_CHECK(check("http://img-fotki.yandex.ru/get/6707/139667729.b/0_16e81a_4aa200b1_M.jpg",
        data.replace("http://img-fotki.yandex.ru/get/6707/139667729.b/0_16e81a_4aa200b1_M.jpg", url::type::image, url::protocol::http, url::extension::jpg)));
    BOOST_CHECK(check("https://ae01.alicdn.com/kf/HTB18EXkIFXXXXc2XpXXq6xXFXXXJ/NECA-Чужой-ПРОТИВ-хищника-Фигурки-Набор-2-В-1-ПВХ-Рис-Игрушка-Установить-Классический-Хищник-Фильм.jpg_640x640.jpg",
        data.replace("https://ae01.alicdn.com/kf/HTB18EXkIFXXXXc2XpXXq6xXFXXXJ/NECA-Чужой-ПРОТИВ-хищника-Фигурки-Набор-2-В-1-ПВХ-Рис-Игрушка-Установить-Классический-Хищник-Фильм.jpg_640x640.jpg", url::type::image, url::protocol::https, url::extension::jpg)));
    BOOST_CHECK(check("h Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("ht Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("htt Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("http Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("https Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("f Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("ft Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("ftp Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("https://mail.ru@124 ", data.replace("https://mail.ru@124", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("https://mail.ru@124", data.replace("https://mail.ru@124", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("chat.my.com", data.replace("http://chat.my.com", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("files.icq.net", data.replace("http://files.icq.net", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("урлы будут icq.com", data.replace("http://icq.com", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("Завтра Hi-Tech Mail.Ru приглашает", data.replace("http://Mail.Ru", url::type::site, url::protocol::http)));
    BOOST_CHECK(check("file.zip", data.clear()));
    BOOST_CHECK(check("file.host", data.clear()));
    BOOST_CHECK(check("https://medium.com/@geometrieva/tutorial-for-that-space-illustration-you-keep-seeing-around-9c97214b4d92#.bceysleoc",
        data.replace("https://medium.com/@geometrieva/tutorial-for-that-space-illustration-you-keep-seeing-around-9c97214b4d92#.bceysleoc", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("http://i.imgur.com/Ospf9qn.gifv", data.replace("http://i.imgur.com/Ospf9qn.gifv", url::type::site)));
    BOOST_CHECK(check("http://media.mnn.com/assets/images/2015/08/forest-waterfall-thailand.jpg.838x0_q80.jpg",
        data.replace("http://media.mnn.com/assets/images/2015/08/forest-waterfall-thailand.jpg.838x0_q80.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("https://pbs.twimg.com/media/CwzDLi5XcAAmX6C.jpg:large",
        data.replace("https://pbs.twimg.com/media/CwzDLi5XcAAmX6C.jpg:large", url::type::image, url::protocol::https, url::extension::jpg)));
    BOOST_CHECK(check("i.com/f.bmp", data.replace("http://i.com/f.bmp", url::type::image, url::extension::bmp)));
    BOOST_CHECK(check("i.com/f.jpg", data.replace("http://i.com/f.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("i.com/f.jpeg", data.replace("http://i.com/f.jpeg", url::type::image, url::extension::jpeg)));
    BOOST_CHECK(check("i.com/f.png", data.replace("http://i.com/f.png", url::type::image, url::extension::png)));
    BOOST_CHECK(check("i.com/f.tiff", data.replace("http://i.com/f.tiff", url::type::image, url::extension::tiff)));
    BOOST_CHECK(check("i.com/f.gif", data.replace("http://i.com/f.gif", url::type::image, url::extension::gif)));
    BOOST_CHECK(check("i.com/f.avi", data.replace("http://i.com/f.avi", url::type::video, url::extension::avi)));
    BOOST_CHECK(check("i.com/f.mkv", data.replace("http://i.com/f.mkv", url::type::video, url::extension::mkv)));
    BOOST_CHECK(check("i.com/f.wmv", data.replace("http://i.com/f.wmv", url::type::video, url::extension::wmv)));
    BOOST_CHECK(check("i.com/f.flv", data.replace("http://i.com/f.flv", url::type::video, url::extension::flv)));
    BOOST_CHECK(check("i.com/f.3gp", data.replace("http://i.com/f.3gp", url::type::video, url::extension::_3gp)));
    BOOST_CHECK(check("i.com/f.mpeg4", data.replace("http://i.com/f.mpeg4", url::type::video, url::extension::mpeg4)));
    BOOST_CHECK(check("i.com/f.webm", data.replace("http://i.com/f.webm", url::type::video, url::extension::webm)));
    BOOST_CHECK(check("i.com/f.mov", data.replace("http://i.com/f.mov", url::type::video, url::extension::mov)));
    BOOST_CHECK(check("http://vignette2.wikia.nocookie.net/himym/images/5/59/Robin_dogs.jpg/revision/latest?cb=20120219232609",
        data.replace("http://vignette2.wikia.nocookie.net/himym/images/5/59/Robin_dogs.jpg/revision/latest?cb=20120219232609", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("10.0.12134 dacl73@yahoo.com", data.replace("dacl73@yahoo.com", url::type::email)));
    BOOST_CHECK(check("http://icqsnip.go.mail.ru/snippet/?", data.replace("http://icqsnip.go.mail.ru/snippet/", url::type::site)));
    BOOST_CHECK(check("http://icqsnip.go.mail.ru/snippet/?v=1&favicon=1&force_reload=0&layer=1&url=https://www.dropbox.com/s/du9mmc4yt48juix/settings.jpg?dl=0",
        data.replace("http://icqsnip.go.mail.ru/snippet/?v=1&favicon=1&force_reload=0&layer=1&url=https://www.dropbox.com/s/du9mmc4yt48juix/settings.jpg?dl=0", url::type::site)));
    BOOST_CHECK(check("ftp://ftp.mrunix.net/ ()", data.replace("ftp://ftp.mrunix.net/", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("прекрасно). 9 минут. theblueprint.ru/culture/unusual-magazines", data.replace("http://theblueprint.ru/culture/unusual-magazines", url::type::site)));
    BOOST_CHECK(check("hTO1DY", data.clear()));
    BOOST_CHECK(check("fTO1DY", data.clear()));
    BOOST_CHECK(check("hTp1DY", data.clear()));
    BOOST_CHECK(check("fTp1DY", data.clear()));
    BOOST_CHECK(check("hTtpDY", data.clear()));
    BOOST_CHECK(check("hTtpsY", data.clear()));
    BOOST_CHECK(check("fTpsDY", data.clear()));
    BOOST_CHECK(check("ftp://user@host/", data.replace("ftp://user@host/", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("http://user@host.ru/", data.replace("http://user@host.ru/", url::type::site)));
    BOOST_CHECK(check("https://user@host.ru/", data.replace("https://user@host.ru/", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("ftp://user@host.ru/", data.replace("ftp://user@host.ru/", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("ftps://user@host.ru/", data.replace("ftps://user@host.ru/", url::type::ftp, url::protocol::ftps)));
    BOOST_CHECK(check("ftp://user:pass@host.ru/", data.replace("ftp://user:pass@host.ru/", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("example.com#fooBaR", data.replace("http://example.com#fooBaR", url::type::site)));
    BOOST_CHECK(check("example.com/foo/捦挺挎/bar", data.replace("http://example.com/foo/捦挺挎/bar", url::type::site)));
    BOOST_CHECK(check("ne. ", data.clear()));
    BOOST_CHECK(check("fd.dmp", data.clear()));
    BOOST_CHECK(check("hd.dmp", data.clear()));
    BOOST_CHECK(check("%appdata%\\ICQ\\reports\\crashdump.dmp", data.clear()));
    BOOST_CHECK(check("mpmulti-l6wd.slack-msgs.com ", data.replace("http://mpmulti-l6wd.slack-msgs.com", url::type::site)));
    BOOST_CHECK(check("http://img0.joyreactor.cc/pics/post/%20ифки-болливуд-Индийское-кино-фильм-ужасов-3460286.gif",
        data.replace("http://img0.joyreactor.cc/pics/post/%20ифки-болливуд-Индийское-кино-фильм-ужасов-3460286.gif", url::type::image, url::extension::gif)));
    BOOST_CHECK(check("http://img0.joyreactor.cc/pics/post/гифки-болливуд-Индийское-кино-фильм-ужасов-3460286.gif",
        data.replace("http://img0.joyreactor.cc/pics/post/гифки-болливуд-Индийское-кино-фильм-ужасов-3460286.gif", url::type::image, url::extension::gif)));
    BOOST_CHECK(check("1. ", data.clear()));
    BOOST_CHECK(check("1.", data.clear()));
    BOOST_CHECK(check("https://1l.mail.ru/report/view/?uidReport=58186a939b58f2.82089113&idOwner=200265&graph=countFirstLoginDaily#data",
        data.replace("https://1l.mail.ru/report/view/?uidReport=58186a939b58f2.82089113&idOwner=200265&graph=countFirstLoginDaily#data", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("media3.giphy.com/media/zZQe7wglGKCEE/200w.gif#5", data.replace("http://media3.giphy.com/media/zZQe7wglGKCEE/200w.gif#5", url::type::image, url::extension::gif)));
    BOOST_CHECK(check("ftp.ru", data.replace("http://ftp.ru", url::type::site)));
    BOOST_CHECK(check("ftps.ru", data.replace("http://ftps.ru", url::type::site)));
    BOOST_CHECK(check("http.ru", data.replace("http://http.ru", url::type::site)));
    BOOST_CHECK(check("https.ru", data.replace("http://https.ru", url::type::site)));
    BOOST_CHECK(check("http.msk", data.clear()));
    BOOST_CHECK(check("http://odri.mail.msk:8111/viewType.html?buildTypeId=ImOsx_IcqQtDevelopXcode7",
        data.replace("http://odri.mail.msk:8111/viewType.html?buildTypeId=ImOsx_IcqQtDevelopXcode7", url::type::site)));
    BOOST_CHECK(check("...", data.clear()));
    BOOST_CHECK(check("master_10.0", data.clear()));
    BOOST_CHECK(check("lenta.ru?", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("lenta.ru!", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("lenta.ru,", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("lenta.ru.", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("lenta.ru? ", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("lenta.ru! ", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("lenta.ru, ", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("lenta.ru. ", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("http://icqsnip.go.mail.ru/snippet/?v=1&force_reload=1&favicon=1&layer=1&url=rb.mail.ru",
        data.replace("http://icqsnip.go.mail.ru/snippet/?v=1&force_reload=1&favicon=1&layer=1&url=rb.mail.ru", url::type::site)));
    BOOST_CHECK(check("wwwcom.ru", data.replace("http://wwwcom.ru", url::type::site)));
    BOOST_CHECK(check("ftpcom.ru", data.replace("http://ftpcom.ru", url::type::site)));
    BOOST_CHECK(check("ftp://i.com", data.replace("ftp://i.com", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("ftp://i.com/f.zip", data.replace("ftp://i.com/f.zip", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("ftp://i.com:8080/f.zip", data.replace("ftp://i.com:8080/f.zip", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("ftp://1.2.0.100/f.zip", data.replace("ftp://1.2.0.100/f.zip", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("ftp://1.2.0.100:8080/f.zip", data.replace("ftp://1.2.0.100:8080/f.zip", url::type::ftp, url::protocol::ftp)));
    BOOST_CHECK(check("ProblemBook.NET · GitBook",  data.replace("http://ProblemBook.NET", url::type::site)));
    BOOST_CHECK(check("<http://images.com/foto.jpg>", data.replace("http://images.com/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("w", data.clear()));
    BOOST_CHECK(check("hi", data.clear()));
    BOOST_CHECK(check("wat?", data.clear()));
    BOOST_CHECK(check("123.com", data.replace("http://123.com", url::type::site)));
    BOOST_CHECK(check("123@mail.com", data.replace("123@mail.com", url::type::email)));
    BOOST_CHECK(check("my@mail.com", data.replace("my@mail.com", url::type::email)));
    BOOST_CHECK(check("my.com@mail.com", data.replace("my.com@mail.com", url::type::email)));
    BOOST_CHECK(check("my@mail.com/some", data.clear()));
    BOOST_CHECK(check("my@mail.com?some", data.replace("my@mail.com", url::type::email)));
    BOOST_CHECK(check("my@mail.com:1010", data.clear()));
    BOOST_CHECK(check("my@mail", data.clear()));
    BOOST_CHECK(check("Https://icq.com/androidbeta", data.replace("Https://icq.com/androidbeta", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("Https://ICQ.com/androidbeta", data.replace("Https://ICQ.com/androidbeta", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("https://ICQ.com/androidbeta", data.replace("https://ICQ.com/androidbeta", url::type::site, url::protocol::https)));
    BOOST_CHECK(check("https://wtf.jpg.wtf/e8/31/1474222035-e83114ddc66ee30550fa27f26d9e9433.jpeg", 
        data.replace("https://wtf.jpg.wtf/e8/31/1474222035-e83114ddc66ee30550fa27f26d9e9433.jpeg", url::type::image, url::protocol::https, url::extension::jpeg)));
    BOOST_CHECK(check("я.", data.clear()));
    BOOST_CHECK(check("ext:119:sticker:1", data.clear()));
    BOOST_CHECK(check("sticker:10", data.clear()));
    BOOST_CHECK(check("http://images.com/foto.jpg", data.replace("http://images.com/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("https://images.com/foto.jpg", data.replace("https://images.com/foto.jpg", url::type::image, url::protocol::https, url::extension::jpg)));
    BOOST_CHECK(check("http://i.com/f.png", data.replace("http://i.com/f.png", url::type::image, url::extension::png)));
    BOOST_CHECK(check("http://i.com/f1.png http://i.com/f2.jpg ,https://i.com/f3.png",
        data.replace("http://i.com/f1.png", url::type::image, url::extension::png).
            append("http://i.com/f2.jpg", url::type::image, url::extension::jpg).
            append("https://i.com/f3.png", url::type::image, url::protocol::https, url::extension::png)));
    BOOST_CHECK(check("did you see this: http://images.com/foto.jpg?", data.replace("http://images.com/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://сайт.рф/foto.jpg?", data.replace("http://сайт.рф/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://САЙТ.РФ/foto.jpg", data.replace("http://САЙТ.РФ/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("http://images.com:8080/foto.jpg", data.replace("http://images.com:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://сайт.рф:8080/foto.jpg?", data.replace("http://сайт.рф:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://сайт.рф:8080/foto.jpg.", data.replace("http://сайт.рф:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://сайт.рф:8080/foto.jpg,", data.replace("http://сайт.рф:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://сайт.рф:8080/foto.jpg? ", data.replace("http://сайт.рф:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://сайт.рф:8080/foto.jpg! ", data.replace("http://сайт.рф:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://сайт.рф:8080/foto.jpg. ", data.replace("http://сайт.рф:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check(" http://сайт.рф:8080/foto.jpg, ", data.replace("http://сайт.рф:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("http://example.com/%E5%BC%95%E3%81%8D%E5%89%B2%E3%82%8A.jpg",
        data.replace("http://example.com/%E5%BC%95%E3%81%8D%E5%89%B2%E3%82%8A.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("http://127.0.0.1:8080/foto.jpg", data.replace("http://127.0.0.1:8080/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("http://127.0.0.1/foto.jpg", data.replace("http://127.0.0.1/foto.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("HTTP://I.com/f.png FTPS://I.COM/f.png ;HTTP://I.COM/F.png , FTP://I.COM/F.PNG  HTTPS://I.COM/F.JPG",
        data.replace("HTTP://I.com/f.png", url::type::image, url::extension::png).
            append("FTPS://I.COM/f.png", url::type::image, url::protocol::ftps, url::extension::png).
            append("HTTP://I.COM/F.png", url::type::image, url::extension::png).
            append("FTP://I.COM/F.PNG", url::type::image, url::protocol::ftp, url::extension::png).
            append("HTTPS://I.COM/F.JPG", url::type::image, url::protocol::https, url::extension::jpg)));
    BOOST_CHECK(check("Привет! http://floristics.info/images/stati_photo/convallaria/convallaria4_3.jpg",
        data.replace("http://floristics.info/images/stati_photo/convallaria/convallaria4_3.jpg", url::type::image, url::extension::jpg)));
    BOOST_CHECK(check("http://i.com/f.jpeg", data.replace("http://i.com/f.jpeg", url::type::image, url::extension::jpeg)));
    BOOST_CHECK(check("http://i.com/f.gif", data.replace("http://i.com/f.gif", url::type::image, url::extension::gif)));
    BOOST_CHECK(check("https://www.dropbox.com/s/sxqsy9qqevmfqku/%D0%A1%D0%BA%D1%80%D0%B8%D0%BD%D1%88%D0%BE%D1%82%202016-06-27%2013.25.37.png?dl=0",
        data.replace("https://www.dropbox.com/s/sxqsy9qqevmfqku/%D0%A1%D0%BA%D1%80%D0%B8%D0%BD%D1%88%D0%BE%D1%82%202016-06-27%2013.25.37.png?dl=0", url::type::image, url::protocol::https, url::extension::png)));
    BOOST_CHECK(check("https://scontent-ams3-1.xx.fbcdn.net/v/t1.0-9/13516151_1781129492105796_4673404754537260997_n.jpg?oh=9ed7eb6120966f8dc03c029e7d6f850b&oe=57FFB21D",
        data.replace("https://scontent-ams3-1.xx.fbcdn.net/v/t1.0-9/13516151_1781129492105796_4673404754537260997_n.jpg?oh=9ed7eb6120966f8dc03c029e7d6f850b&oe=57FFB21D", url::type::image, url::protocol::https, url::extension::jpg)));
    BOOST_CHECK(check("http://files.icq.net/get/0zcm41ngURshP8sQ74B4eN578502541ba",
        data.replace("http://files.icq.net/get/0zcm41ngURshP8sQ74B4eN578502541ba", url::type::filesharing)));
    BOOST_CHECK(check("http://files.icq.net/get/0zcm41ngURshP8sQ74B4eN578502541ba http://files.icq.net/get/0fu9GNuiHmI5q733TuDiUb57861ccf1ai",
        data.replace("http://files.icq.net/get/0zcm41ngURshP8sQ74B4eN578502541ba", url::type::filesharing)
        .append("http://files.icq.net/get/0fu9GNuiHmI5q733TuDiUb57861ccf1ai", url::type::filesharing)));
    BOOST_CHECK(check("https://icq.com/files/0ocg8iAzD7xbSTJ5UT42z45788a26b1bb",
        data.replace("https://icq.com/files/0ocg8iAzD7xbSTJ5UT42z45788a26b1bb", url::type::filesharing, url::protocol::https)));
    BOOST_CHECK(check("https://files.icq.net/get/1fu8IH8lD0to91d7puYGH8to91AdbsmicDBiG1NT413i2HZ48S1Ny1wSq0beD1i5UbS14VhUi1GY2f811INoH1tfFVLSnuMinScT1wV1RwxQUSBp9WO1nu63FsyRzEDGi5KFLSZbXLaGFuxGyImTVzl194bsmi",
        data.replace("https://files.icq.net/get/1fu8IH8lD0to91d7puYGH8to91AdbsmicDBiG1NT413i2HZ48S1Ny1wSq0beD1i5UbS14VhUi1GY2f811INoH1tfFVLSnuMinScT1wV1RwxQUSBp9WO1nu63FsyRzEDGi5KFLSZbXLaGFuxGyImTVzl194bsmi", url::type::filesharing, url::protocol::https)));
    BOOST_CHECK(check("https://files.icq.net/get/0kE2t0tNZvcOhm3YTn3rElwKjc7UrsbBB ",
        data.replace("https://files.icq.net/get/0kE2t0tNZvcOhm3YTn3rElwKjc7UrsbBB", url::type::filesharing, url::protocol::https)));
    BOOST_CHECK(check("www.icq.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ",
        data.replace("http://www.icq.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ", url::type::filesharing)));
    BOOST_CHECK(check("chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ",
        data.replace("http://chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ", url::type::filesharing)));
    BOOST_CHECK(check("www.chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ",
        data.replace("http://www.chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ", url::type::filesharing)));
    BOOST_CHECK(check("http://www.chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ",
        data.replace("http://www.chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ", url::type::filesharing)));
    BOOST_CHECK(check("http://chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ",
        data.replace("http://chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ", url::type::filesharing)));
    BOOST_CHECK(check(" chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ",
        data.replace("http://chat.my.com/files/0hquY5yMzTW2ePH1O2542iqfr0McfiSAJ", url::type::filesharing)));
    BOOST_CHECK(check("http://img6a.flixcart.com//image/keyboard/9/q/u/ttesports-ttesports-meka-g-unit-gaming-keyboard-meka-original-imad2658r8rqvmrz.jpeg",
        data.replace("http://img6a.flixcart.com//image/keyboard/9/q/u/ttesports-ttesports-meka-g-unit-gaming-keyboard-meka-original-imad2658r8rqvmrz.jpeg", url::type::image, url::extension::jpeg)));
    BOOST_CHECK(check("http://images.com/foto.bmp", data.replace("http://images.com/foto.bmp", url::type::image, url::extension::bmp)));
    BOOST_CHECK(check("https://scontent-arn2-1.xx.fbcdn.net/v/t1.0-9/14022305_1600680923562681_6123020614678369175_n.jpg?oh=910166fdcaf14be8f1ad7e623f24b974&oe=584D970E",
        data.replace("https://scontent-arn2-1.xx.fbcdn.net/v/t1.0-9/14022305_1600680923562681_6123020614678369175_n.jpg?oh=910166fdcaf14be8f1ad7e623f24b974&oe=584D970E", url::type::image, url::protocol::https, url::extension::jpg)));
    BOOST_CHECK(check("life1", data.clear()));
    BOOST_CHECK(check("i.org", data.replace("http://i.org", url::type::site)));
    BOOST_CHECK(check("i.org/", data.replace("http://i.org/", url::type::site)));
    BOOST_CHECK(check("ico.org", data.replace("http://ico.org", url::type::site)));
    BOOST_CHECK(check("ico.test.org", data.replace("http://ico.test.org", url::type::site)));
    BOOST_CHECK(check("img2?", data.clear()));
    BOOST_CHECK(check("img2/", data.clear()));
    BOOST_CHECK(check("img2:", data.clear()));
    BOOST_CHECK(check("sqlite?", data.clear()));
    BOOST_CHECK(check("lenta.sru", data.clear()));
    BOOST_CHECK(check("lenta.ru", data.replace("http://lenta.ru", url::type::site)));
    BOOST_CHECK(check("com", data.clear()));
    BOOST_CHECK(check("Уф.ф", data.clear()));
    BOOST_CHECK(check("batenka.ru/resource/med/dementia/", data.replace("http://batenka.ru/resource/med/dementia/", url::type::site)));
    BOOST_CHECK(check("\nbatenka.ru/resource/med/dementia/", data.replace("http://batenka.ru/resource/med/dementia/", url::type::site)));
    BOOST_CHECK(check("\nbatenka.ru/resource/med/dementia/\n", data.replace("http://batenka.ru/resource/med/dementia/", url::type::site)));
    BOOST_CHECK(check("http://graphite14.mail.ru/render?width=1800&from=-14h&until=&height=900&target=movingAverage(smsapi.sum.operator_ALL_MOROCCO.{send,action},30)&hideLegend=false",
        data.replace("http://graphite14.mail.ru/render?width=1800&from=-14h&until=&height=900&target=movingAverage(smsapi.sum.operator_ALL_MOROCCO.{send,action},30)&hideLegend=false", url::type::site)));
}

BOOST_AUTO_TEST_CASE(periods)
{
    using namespace common::tools;

    DataWrapper data;

    BOOST_CHECK(check("www.images.com/.x.jpg", data.replace("http://www.images.com/.x.jpg", url::type::image, url::extension::jpg)));
}

BOOST_AUTO_TEST_CASE(mamba0)
{
    using namespace common::tools;

    DataWrapper data;

    BOOST_CHECK(
        check("http://a.org?cd=pic_256d722a1075bfb9",
        data.replace("http://a.org?cd=pic_256d722a1075bfb9", url::type::site)));
}

BOOST_AUTO_TEST_CASE(mamba1)
{
    using namespace common::tools;

    DataWrapper data;

    BOOST_CHECK(
        check("http://go.imgsmail.ru/imgpreview?mb=mamba&key=pic_256d722a1075bfb9",
        data.replace("http://go.imgsmail.ru/imgpreview?mb=mamba&key=pic_256d722a1075bfb9", url::type::site)));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
