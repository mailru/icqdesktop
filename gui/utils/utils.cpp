#include "stdafx.h"

#include "exif.h"
#include "utils.h"

#include "gui_coll_helper.h"
#include "InterConnector.h"
#include "SChar.h"
#include "profiling/auto_stop_watch.h"
#include "translit.h"
#include "../gui_settings.h"
#include "../core_dispatcher.h"
#include "../cache/countries.h"
#include "../cache/emoji/Emoji.h"
#include "../controls/CommonStyle.h"
#include "../controls/DpiAwareImage.h"
#include "../controls/GeneralDialog.h"
#include "../controls/TextEditEx.h"
#include "../main_window/ContactDialog.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/contact_list/Common.h"
#include "../main_window/history_control/MessageStyle.h"
#include "../theme_settings.h"
#include "../../common.shared/version_info.h"


#ifdef _WIN32
    #include <windows.h>
#else

#ifdef __APPLE__
    #include "macos/mac_support.h"
    typedef unsigned char byte;
#endif

#endif

#ifdef __linux__
#define byte uint8_t
#endif //__linux__

namespace
{
    const int mobile_rect_width = 8;
    const int mobile_rect_height = 12;

    const int mobile_rect_width_mini = 5;
    const int mobile_rect_height_mini = 8;

    const int drag_preview_max_width = 320;
    const int drag_preview_max_height = 240;

	const QColor ColorTable[] = {
		"#FF0000",
		"#FF7373",
		"#CA4242",
		"#F47B5A",
		"#990000",
		"#FF6665",
		"#E69200",
		"#FF9010",
		"#E1CB00",
		"#1CD920",
		"#5DB82A",
		"#088DA5",
		"#008000",
		"#0092D0",
		"#389BFF",
		"#0099CC",
		"#3E687A",
		"#0166CC",
		"#0000FF",
		"#46C5FC",
		"#4356BC",
		"#73799B",
		"#FC62B4",
		"#800180",
		"#B682D8",
		"#B44FF2",
		"#D852AE",
		"#E8A2C4"
	};

#ifdef _WIN32
	const auto ColorTableSize = _countof(ColorTable);
#else
    const auto ColorTableSize = sizeof(ColorTable)/sizeof(ColorTable[0]);
#endif

	const quint32 CRC32Table[ 256 ] =
	{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
		0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
		0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
		0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
		0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
		0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
		0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
		0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
		0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
		0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
		0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
		0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
		0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
		0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
		0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
		0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
		0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
		0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
		0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
		0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
		0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
		0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
		0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
		0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
		0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
		0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
		0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
		0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
		0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
		0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
		0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
		0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
		0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
		0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
		0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
		0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
		0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
		0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
		0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
		0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
		0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
		0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
		0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};

	quint32 crc32FromIODevice(QIODevice* _device)
	{
		quint32 crc32 = 0xffffffff;
		char* buf = new char[256];
		qint64 n;

		while((n = _device->read(buf, 256)) > 0)
			for (qint64 i = 0; i < n; i++)
				crc32 = (crc32 >> 8) ^ CRC32Table[(crc32 ^ buf[i]) & 0xff];
		delete[] buf;

		crc32 ^= 0xffffffff;
		return crc32;
	}

	quint32 crc32FromByteArray(const QByteArray& _array)
	{
		QBuffer buffer;
		buffer.setData(_array);
		if (!buffer.open(QIODevice::ReadOnly))
			return 0;

		return crc32FromIODevice(&buffer);
	}

	quint32 crc32FromString(const QString& _text)
	{
		return crc32FromByteArray(_text.toLatin1());
	}

#ifdef _WIN32
	QChar VKeyToChar( short _vkey, HKL _layout)
	{
		DWORD dwScan=MapVirtualKeyEx(_vkey & 0x00ff, 0, _layout);
		byte KeyStates[256] = {0};
		KeyStates[_vkey & 0x00ff] = 0x80;
		DWORD dwFlag=0;
		if(_vkey & 0x0100)
		{
			KeyStates[VK_SHIFT] = 0x80;
		}
		if(_vkey & 0x0200)
		{
			KeyStates[VK_CONTROL] = 0x80;
		}
		if(_vkey & 0x0400)
		{
			KeyStates[VK_MENU] = 0x80;
			dwFlag = 1;
		}
		wchar_t Result = L' ';
		if (!ToUnicodeEx(_vkey & 0x00ff, dwScan, KeyStates, &Result, 1, dwFlag, _layout) == 1)
		{
			return QChar(' ');
		}
		return QChar(Result);
	}
#endif //_WIN32

    QString msgIdFromUidl(const QString& uidl)
    {
        if (uidl.size() != 16)
        {
            assert(!"wrong uidl");
            return QString();
        }

        byte b[8] = {0};
        bool ok;
        b[0] = uidl.mid(0, 2).toUInt(&ok, 16);
        b[1] = uidl.mid(2, 2).toUInt(&ok, 16);
        b[2] = uidl.mid(4, 2).toUInt(&ok, 16);
        b[3] = uidl.mid(6, 2).toUInt(&ok, 16);
        b[4] = uidl.mid(8, 2).toUInt(&ok, 16);
        b[5] = uidl.mid(10, 2).toUInt(&ok, 16);
        b[6] = uidl.mid(12, 2).toUInt(&ok, 16);
        b[7] = uidl.mid(14, 2).toUInt(&ok, 16);

        QString result;
        result.sprintf("%u%010u", *(int*)(b), *(int*)(b + 4*sizeof(byte)));

        return result;
    }

    const QString redirect = "&noredirecttologin=1";
    const QString page = "https://r.mail.ru:443/cls3564/win.mail.ru/cgi-bin/auth";
    const QString fail_page = "https://r.mail.ru:443/cls3564/win.mail.ru/cgi-bin/auth";
    const QString base_mail_url = "https://auth.mail.ru/cgi-bin/auth?Login=%1&agent=%2&ver=%3&agentlang=%4";
    const QString mail_url = base_mail_url + redirect + "&page=" + page + "&FailPage=" + fail_page;

    const QString read_msg_page = "https://r.mail.ru:443/cln8791/mra-mail.mail.ru/cgi-bin/readmsg?id=";
    const QString mail_open_mail_url = base_mail_url + redirect + "&page=" + read_msg_page + "%5&lang=%4" + "&FailPage=" + read_msg_page + "%5&lang=%4";

}

namespace Utils
{
    void drawText(QPainter& _painter, const QPointF& _point, int _flags,
        const QString& _text, QRectF* _boundingRect)
    {
        const qreal size = 32767.0;
        QPointF corner(_point.x(), _point.y() - size);

        if (_flags & Qt::AlignHCenter)
            corner.rx() -= size/2.0;
        else if (_flags & Qt::AlignRight)
            corner.rx() -= size;

        if (_flags & Qt::AlignVCenter)
            corner.ry() += size/2.0;
        else if (_flags & Qt::AlignTop)
            corner.ry() += size;
        else _flags |= Qt::AlignBottom;

        QRectF rect(corner, QSizeF(size, size));
        _painter.drawText(rect, _flags, _text, _boundingRect);
    }

	ShadowWidgetEventFilter::ShadowWidgetEventFilter(int _shadowWidth)
		: QObject(0)
		, ShadowWidth_(_shadowWidth)
	{

	}

	bool ShadowWidgetEventFilter::eventFilter(QObject* obj, QEvent* _event)
	{
		if (_event->type() == QEvent::Paint)
		{
			QWidget* w = qobject_cast<QWidget*>(obj);

			QRect origin = w->rect();

			QRect right = QRect(
                QPoint(origin.width() - ShadowWidth_, origin.y() + ShadowWidth_ + 1),
                QPoint(origin.width(), origin.height() - ShadowWidth_ - 1)
            );

			QRect left = QRect(
                QPoint(origin.x(), origin.y() + ShadowWidth_),
                QPoint(origin.x() + ShadowWidth_ - 1, origin.height() - ShadowWidth_ - 1)
            );

			QRect top = QRect(
                QPoint(origin.x() + ShadowWidth_ + 1, origin.y()),
                QPoint(origin.width() - ShadowWidth_ - 1, origin.y() + ShadowWidth_)
            );

			QRect bottom = QRect(
                QPoint(origin.x() + ShadowWidth_ + 1, origin.height() - ShadowWidth_),
                QPoint(origin.width() - ShadowWidth_ - 1, origin.height())
            );

			QRect topLeft = QRect(
                origin.topLeft(),
                QPoint(origin.x() + ShadowWidth_, origin.y() + ShadowWidth_)
            );

			QRect topRight = QRect(
                QPoint(origin.width() - ShadowWidth_, origin.y()),
                QPoint(origin.width(), origin.y() + ShadowWidth_)
            );

			QRect bottomLeft = QRect(
                QPoint(origin.x(), origin.height() - ShadowWidth_),
                QPoint(origin.x() + ShadowWidth_, origin.height())
            );

			QRect bottomRight = QRect(
                QPoint(origin.width() - ShadowWidth_, origin.height() - ShadowWidth_),
                origin.bottomRight()
            );

			QPainter p(w);

			bool isActive = w->isActiveWindow();

			QLinearGradient lg = QLinearGradient(right.topLeft(), right.topRight());
			setGradientColor(lg, isActive);
			p.fillRect(right, QBrush(lg));

			lg = QLinearGradient(left.topRight(), left.topLeft());
			setGradientColor(lg, isActive);
			p.fillRect(left, QBrush(lg));

			lg = QLinearGradient(top.bottomLeft(), top.topLeft());
			setGradientColor(lg, isActive);
			p.fillRect(top, QBrush(lg));

			lg = QLinearGradient(bottom.topLeft(), bottom.bottomLeft());
			setGradientColor(lg, isActive);
			p.fillRect(bottom, QBrush(lg));

			QRadialGradient g = QRadialGradient(topLeft.bottomRight(), ShadowWidth_);
			setGradientColor(g, isActive);
			p.fillRect(topLeft, QBrush(g));

			g = QRadialGradient(topRight.bottomLeft(), ShadowWidth_);
			setGradientColor(g, isActive);
			p.fillRect(topRight, QBrush(g));

			g = QRadialGradient(bottomLeft.topRight(), ShadowWidth_);
			setGradientColor(g, isActive);
			p.fillRect(bottomLeft, QBrush(g));

			g = QRadialGradient(bottomRight.topLeft(), ShadowWidth_);
			setGradientColor(g, isActive);
			p.fillRect(bottomRight, QBrush(g));
		}

		return QObject::eventFilter(obj, _event);
	}

	void ShadowWidgetEventFilter::setGradientColor(QGradient& _gradient, bool _isActive)
	{
        QColor windowGradientColor("#000000");
        windowGradientColor.setAlphaF(0.2);
        _gradient.setColorAt(0, windowGradientColor);
        windowGradientColor.setAlphaF(_isActive ? 0.08 : 0.04);
		_gradient.setColorAt(0.2, windowGradientColor);
        windowGradientColor.setAlphaF(0.02);
		_gradient.setColorAt(0.6, _isActive ? windowGradientColor : Qt::transparent);
		_gradient.setColorAt(1, Qt::transparent);
	}

    QString getCountryNameByCode(const QString& _isoCode)
    {
        const auto &countries = Ui::countries::get();
        const auto i = std::find_if(
            countries.begin(), countries.end(), [_isoCode](const Ui::countries::country &v)
        {
            return v.iso_code_.toLower() == _isoCode.toLower();
        });
        if (i != countries.end())
            return i->name_;
        return "";
    }

	QMap<QString, QString> getCountryCodes()
	{
		auto countries = Ui::countries::get();
		QMap<QString, QString> result;
		for (const auto& country : countries)
			result.insert(country.name_, QString("+") + QVariant(country.phone_code_).toString());
		return result;
	}

	QString ScaleStyle(const QString& _style, double _scale)
	{
		QString outString;
		QTextStream result(&outString);

		auto tokens =  _style.split(QRegExp("\\;"));

		for (auto iterLine = tokens.begin(); iterLine != tokens.end(); iterLine++)
		{
			if (iterLine != tokens.begin())
				result << ";";

			int pos = iterLine->indexOf(QRegExp("[\\-\\d]\\d*dip"));

			if (pos != -1)
			{
				result << iterLine->left(pos);
				QString tmp = iterLine->mid(pos, iterLine->right(pos).length());
				int size = QVariant(tmp.left(tmp.indexOf("dip"))).toInt();
                size *= _scale;
				result << QVariant(size).toString();
				result << "px";
			}
			else
			{
				pos = iterLine->indexOf("_100");
				if (pos != -1)
				{
					result << iterLine->left(pos);
					result << "_";

                    if (Utils::is_mac_retina())
                    {
                        result << QVariant(2 * 100).toString();
                    }
                    else
                    {
                        result << QVariant(_scale * 100).toString();
                    }
					result << iterLine->mid(pos + 4, iterLine->length());
				}
				else
				{
					pos = iterLine->indexOf("/100/");
					if (pos != -1)
					{
						result << iterLine->left(pos);
						result << "/";
                        result << QVariant(Utils::scale_bitmap(_scale) * 100).toString();
						result << "/";
						result << iterLine->mid(pos + 5, iterLine->length());
					}
					else
					{
						result << *iterLine;
					}
				}
			}
		}

		return outString;
	}

    void ApplyPropertyParameter(QWidget* _widget, const char* _property, QVariant _parameter)
    {
        if (_widget)
        {
            _widget->setProperty(_property, _parameter);
            _widget->style()->unpolish(_widget);
            _widget->style()->polish(_widget);
            _widget->update();
        }
    }

    void ApplyStyle(QWidget* _widget, QString _style)
    {
        if (_widget)
        {
            QString newStyle = Fonts::SetFont(Utils::ScaleStyle(_style, Utils::getScaleCoefficient()));
            if (newStyle != _widget->styleSheet())
                _widget->setStyleSheet(newStyle);
        }
    }

	QString LoadStyle(const QString& _qssFile)
	{
        QFile file(_qssFile);

		if (!file.open(QIODevice::ReadOnly))
		{
			assert(!"open style file error");
			return "";
		}

		QString qss = file.readAll();
		if (qss.isEmpty())
		{
			return "";
		}

		return ScaleStyle(Fonts::SetFont(qss), Utils::getScaleCoefficient());
	}


	QPixmap getDefaultAvatar(const QString& _uin, const QString& _displayName, const int _sizePx, const bool _isFilled)
	{
		const auto antialiasSizeMult = 8;
		const auto bigSizePx = (_sizePx * antialiasSizeMult);
		const auto bigSize = QSize(bigSizePx, bigSizePx);

		QImage bigResult(bigSize, QImage::Format_ARGB32);

		// evaluate colors

        const auto &str = _uin.isEmpty() ? QString("0") : _uin;
        const auto crc32 = crc32FromString(str);
        const auto colorIndex = (crc32 % ColorTableSize);
        QColor color = ColorTable[colorIndex];

        if (_uin.isEmpty() && _displayName.isEmpty())
            color = QColor("#ebebeb");

        if (_uin == "mail")
            color = QColor("#579e1c");

		QPainter painter(&bigResult);

		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		// set pen

		QPen hollowAvatarPen(color);

		const auto hollowPenWidth = scale_value(2 * antialiasSizeMult);
		hollowAvatarPen.setWidth(hollowPenWidth);

		QPen filledAvatarPen(QColor("#ffffff"));

		const QPen &avatarPen = (_isFilled ? filledAvatarPen : hollowAvatarPen);
		painter.setPen(avatarPen);

		// draw

		if (_isFilled)
		{
			bigResult.fill(color);
		}
		else
		{
			bigResult.fill(Qt::transparent);
			painter.setBrush(QColor("#ffffff"));

			const auto correction = ((hollowPenWidth / 2) + 1);
			const auto ellipseRadius = (bigSizePx - (correction * 2));
			painter.drawEllipse(correction, correction, ellipseRadius, ellipseRadius);
		}

		auto scaledBigResult = bigResult.scaled(QSize(_sizePx, _sizePx), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        if (_uin == "mail")
        {
            QPainter scaled(&scaledBigResult);
            scaled.setRenderHint(QPainter::Antialiasing);
            scaled.setRenderHint(QPainter::TextAntialiasing);
            scaled.setRenderHint(QPainter::SmoothPixmapTransform);

            QPixmap p(parse_image_name(":/resources/alert_magent_100.png"));
            p = p.scaled(QSize(_sizePx, _sizePx), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            scaled.drawPixmap(0, 0, p);
            return QPixmap::fromImage(scaledBigResult);
        }

		const auto trimmedDisplayName = _displayName.trimmed();
        if (trimmedDisplayName.isEmpty())
        {
            QPainter scaled(&scaledBigResult);
            scaled.setRenderHint(QPainter::Antialiasing);
            scaled.setRenderHint(QPainter::TextAntialiasing);
            scaled.setRenderHint(QPainter::SmoothPixmapTransform);
            QPixmap img(Utils::parse_image_name(":/resources/photo_100.png"));
            Utils::check_pixel_ratio(img);
            const auto off = (float)scaledBigResult.width() * 0.25;
            scaled.drawPixmap(QRectF(off, off, scaledBigResult.width() - off * 2., scaledBigResult.height() - off * 2.), img, img.rect());
            return QPixmap::fromImage(scaledBigResult);
        }

		const auto firstChar = Utils::PeekNextSuperChar(trimmedDisplayName);
		if (firstChar.IsSimple())
		{
            QFont font = Fonts::appFont(_sizePx / 1.5);

			QPainter letterPainter(&scaledBigResult);
			letterPainter.setRenderHint(QPainter::Antialiasing);
			letterPainter.setFont(font);
			letterPainter.setPen(avatarPen);

			const auto toDisplay = trimmedDisplayName[0].toUpper();

			auto rawFont = QRawFont::fromFont(font);
            if (platform::is_apple())
            {
                if (!rawFont.supportsCharacter(toDisplay))
                    rawFont = QRawFont::fromFont(QFont(QStringLiteral("AppleColorEmoji"), _sizePx / 1.5));
                assert(rawFont.supportsCharacter(toDisplay));
            }

			quint32 glyphIndex = 0;
			int numGlyphs = 1;
			const auto glyphSearchSucceed = rawFont.glyphIndexesForChars(&toDisplay, 1, &glyphIndex, &numGlyphs);
			if (!glyphSearchSucceed)
			{
				return QPixmap::fromImage(scaledBigResult);
			}

			assert(numGlyphs == 1);

			auto glyphPath = rawFont.pathForGlyph(glyphIndex);

			const auto rawGlyphBounds = glyphPath.boundingRect();
			glyphPath.translate(-rawGlyphBounds.x(), -rawGlyphBounds.y());
			const auto glyphBounds = glyphPath.boundingRect();
			const auto glyphHeight = glyphBounds.height();
			const auto glyphWidth = glyphBounds.width();

			QFontMetrics m(font);
			QRect pseudoRect(0, 0, _sizePx, _sizePx);
			auto centeredRect = m.boundingRect(pseudoRect, Qt::AlignCenter, QString(toDisplay));

			qreal y = (_sizePx / 2.0);
			y -= glyphHeight / 2.0;

			qreal x = centeredRect.x();
			x += (centeredRect.width() / 2.0);
			x -= (glyphWidth / 2.0);

			letterPainter.translate(x, y);
			letterPainter.fillPath(glyphPath, avatarPen.brush());
		}
		else if (firstChar.IsEmoji())
		{
            QPainter emojiPainter(&scaledBigResult);
            const auto nearestSizeAvailable = Emoji::GetNearestSizeAvailable(_sizePx / 2);
            const auto &emoji = Ui::DpiAwareImage(Emoji::GetEmoji(firstChar.Main(), firstChar.Ext(), nearestSizeAvailable));
            emoji.draw(emojiPainter, (_sizePx - emoji.width()) / 2, (_sizePx - emoji.height()) / 2 - (platform::is_apple() ? 0 : 1));
		}

		return QPixmap::fromImage(scaledBigResult);
	}

    std::vector<QStringList> GetPossibleStrings(const QString& _text, unsigned& _count)
	{
        _count = 0;
		std::vector<QStringList> result;

        if (_text.isEmpty())
            return result;

#if defined _WIN32
		HKL aLayouts[8] = {0};
		int nCount = ::GetKeyboardLayoutList(8, aLayouts);
		HKL hCurrent = ::GetKeyboardLayout(0);

		for (int i = 0; i < _text.length(); ++i)
		{
            result.push_back(QStringList());
            result[i].push_back(_text.at(i));
		}

        _count = nCount;

		for (int i = 0; i < _text.length(); i++)
		{
            auto scanEx = ::VkKeyScanEx(_text.at(i).unicode(), hCurrent);

            if (scanEx == -1)
            {
                bool found = false;
                for (int j = 0; j < nCount; ++j)
                {
                    if (::VkKeyScanEx(_text.at(i).unicode(), aLayouts[j]) != -1)
                    {
                        found = true;
                        break;
                    }

                    if (!found)
                    {
                        return std::vector<QStringList>();
                    }
                }
            }

            for (int j = 0; j < nCount; ++j)
            {
                if (hCurrent == aLayouts[j])
                    continue;

                result[i].push_back(scanEx == -1 ? QString() : VKeyToChar(scanEx, aLayouts[j]));
            }
		}

#elif defined __APPLE__
        MacSupport::getPossibleStrings(_text, result, _count);
#endif //_WIN32

#ifdef __linux__

        for (int i = 0; i < _text.length(); ++i)
		{
            result.push_back(QStringList());
            result[i].push_back(_text.at(i));
        }

        _count = 1;
#endif //__Linux__
        auto translit = Translit::getPossibleStrings(_text);

        if (translit.size() > 0)
        {
            for (int i = 0; i < std::min(Translit::getMaxSearchTextLength(), _text.length()); ++i)
		    {
                for (auto pattern : translit[i])
                    result[i].push_back(pattern);
            }
        }

		return result;
	}

	QPixmap roundImage(const QPixmap& _img, const QString& _state, bool /*isDefault*/, bool _miniIcons)
	{
		int scale = std::min(_img.height(), _img.width());
		QImage imageOut(QSize(scale, scale), QImage::Format_ARGB32);
		imageOut.fill(Qt::transparent);

		QPainter painter(&imageOut);

		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);

		painter.setPen(Qt::NoPen);
		painter.setBrush(Qt::NoBrush);
		QPainterPath path(QPointF(0, 0));
		path.addEllipse(0, 0, scale, scale);

        const auto xRnd = 50;
        const auto yRnd = 36;

        if (_state == "mobile" || _state == "mobile_active")
        {
            QPainterPath stPath(QPointF(0, 0));
            if (_miniIcons)
                stPath.addRoundRect(
                    QRect(
                        scale - Utils::scale_bitmap(Utils::scale_value(mobile_rect_width_mini)),
                        scale - Utils::scale_bitmap(Utils::scale_value(mobile_rect_height_mini)),
                        Utils::scale_bitmap(Utils::scale_value(mobile_rect_width_mini)),
                        Utils::scale_bitmap(Utils::scale_value(mobile_rect_height_mini))
                    ),
                    xRnd,
                    yRnd
                );
            else
                stPath.addRoundRect(
                    QRect(
                        scale - Utils::scale_bitmap(Utils::scale_value(mobile_rect_width)),
                        scale - Utils::scale_bitmap(Utils::scale_value(mobile_rect_height)),
                        Utils::scale_bitmap(Utils::scale_value(mobile_rect_width)),
                        Utils::scale_bitmap(Utils::scale_value(mobile_rect_height))
                    ),
                    xRnd,
                    yRnd
                );
            path -= stPath;
        }

        auto addedRadius = Utils::scale_value(8);
        if (_state == "photo enter" || _state == "photo leave")
        {
            QPixmap p(Utils::parse_image_name(":/resources/content_addphoto_100.png"));
            int x = (scale - p.width());
            int y = (scale - p.height());
            QPainterPath stPath(QPointF(0, 0));
            stPath.addEllipse(x - addedRadius / 2, y - addedRadius / 2, p.width() + addedRadius, p.height() + addedRadius);
            path -= stPath;
        }

		painter.setClipPath(path);
		painter.drawPixmap(0, 0, _img);

        if (_state == "photo enter")
        {
            QColor photoEnterColor("#000000");
            photoEnterColor.setAlphaF(0.3);
            painter.setBrush(QBrush(photoEnterColor));
            painter.drawEllipse(0, 0, scale, scale);
            painter.setBrush(Qt::NoBrush);

            const auto fontSize = Utils::scale_bitmap(18);
            painter.setFont(Fonts::appFontScaled(fontSize));
            painter.setPen(QPen(QColor("#ffffff")));

            painter.drawText(QRectF(0, 0, scale, scale), Qt::AlignCenter, QT_TRANSLATE_NOOP("avatar_upload", "Edit\nphoto"));
            painter.setPen(Qt::NoPen);
        }

        if (_state == "online" || _state == "dnd" || _state == "online_active")
        {
            QPainterPath stPath(QPointF(0,0));
            stPath.addRect(0, 0, scale, scale);
            painter.setClipPath(stPath);
            QPixmap p;
            if (_state == "online_active")
                p = QPixmap(Utils::parse_image_name(_miniIcons ? ":/resources/cl_status_online_mini_100_active.png" : ":/resources/cl_status_online_100_active.png"));
            else
                p = QPixmap(Utils::parse_image_name(_miniIcons ? ":/resources/cl_status_online_mini_100.png" : ":/resources/cl_status_online_100.png"));
            int x = (scale - p.width());
            int y = (scale - p.height());
            painter.drawPixmap(x, y, p);
        }
        else if (_state == "mobile" || _state == "mobile_active")
        {
            QPainterPath stPath(QPointF(0,0));
            stPath.addRect(0, 0, scale, scale);
            painter.setClipPath(stPath);
            QPixmap p;
            if (_state == "mobile")
                p = QPixmap(Utils::parse_image_name(_miniIcons ? ":/resources/cl_status_mobile_mini_100.png" : ":/resources/cl_status_mobile_100.png"));
            else
                p = QPixmap(Utils::parse_image_name(_miniIcons ? ":/resources/cl_status_mobile_mini_100_active.png" : ":/resources/cl_status_mobile_100_active.png"));
            int x = (scale - p.width());
            int y = (scale - p.height());
            painter.drawPixmap(x, y, p);
        }
        else if (_state == "photo enter" || _state == "photo leave")
        {
            QPixmap p(Utils::parse_image_name(":/resources/content_addphoto_100.png"));
            int x = (scale - p.width());
            int y = (scale - p.height());

            painter.setPen(Qt::NoPen);
    		painter.setBrush(Qt::NoBrush);
            QPainterPath stPath(QPointF(0, 0));
            stPath.addRect(0, 0, scale, scale);
            painter.setClipPath(stPath);

    		painter.setBrush(Qt::transparent);
            painter.drawEllipse(x - addedRadius / 2, y - addedRadius / 2, p.width() + addedRadius, p.height() + addedRadius);
            painter.drawPixmap(x, y, p);
        }

        QPixmap pixmap = QPixmap::fromImage(imageOut);
        Utils::check_pixel_ratio(pixmap);
		return pixmap;
	}

    bool isValidEmailAddress(const QString& _email)
    {
        QRegExp r("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b");
        r.setCaseSensitivity(Qt::CaseInsensitive);
        r.setPatternSyntax(QRegExp::RegExp);
        return r.exactMatch(_email);
    }

    bool isProbablyPhoneNumber(const QString& _number)
    {
        QRegExp r("/^\\s*(?:\\+?(\\d{1,3}))?([-. (]*(\\d{3})[-. )]*)?((\\d{3})[-. ]*(\\d{2,4})(?:[-.x ]*(\\d+))?)\\s*$/gm");
        r.setCaseSensitivity(Qt::CaseInsensitive);
        r.setPatternSyntax(QRegExp::RegExp);
        return r.exactMatch(_number);
    }

    double fscale_value(const double _px)
    {
        return (Utils::getScaleCoefficient() * _px);
    }

	int scale_value(const int _px)
	{
		return (int)(Utils::getScaleCoefficient() * (double)_px);
	}

    QSize scale_value(const QSize _px)
    {
        return QSize(
            Utils::scale_value(_px.width()),
            Utils::scale_value(_px.height()));
    }

    QSizeF scale_value(const QSizeF _px)
    {
        return QSizeF(
            Utils::scale_value(_px.width()),
            Utils::scale_value(_px.height()));
    }

    QRect scale_value(const QRect _px)
    {
        return QRect(
            _px.left(),
            _px.top(),
            Utils::scale_value(_px.width()),
            Utils::scale_value(_px.height()));
    }

    int unscale_value(const int _px)
    {
        double scale = Utils::getScaleCoefficient();
        return (int)((double) _px / (scale == 0 ? 1.0 : scale));
    }

    QSize unscale_value(const QSize& _px)
    {
        return QSize(
            unscale_value(_px.width()),
            unscale_value(_px.height()));
    }

    bool foregroundWndIsFullscreened()
    {
#ifdef _WIN32
        const HWND foregroundWindow = ::GetForegroundWindow();

        RECT rcScreen;
        ::GetWindowRect(GetDesktopWindow(), &rcScreen);

        RECT rcForegroundApp;
        GetWindowRect(foregroundWindow, &rcForegroundApp);

        if (foregroundWindow != ::GetDesktopWindow() && foregroundWindow != ::GetShellWindow())
        {
            return rcScreen.left == rcForegroundApp.left &&
                rcScreen.top == rcForegroundApp.top &&
                rcScreen.right == rcForegroundApp.right &&
                rcScreen.bottom == rcForegroundApp.bottom;
        }
#elif __APPLE__
        return MacSupport::isFullScreen();
#endif
        return false;
	}

	int scale_bitmap(const int _px)
	{
		return (_px * (is_mac_retina() ? 2 : 1));
	}

    int unscale_bitmap(const int _px)
    {
        return (_px / (is_mac_retina() ? 2 : 1));
    }

	QSize scale_bitmap(const QSize& _px)
	{
		return QSize(
			scale_bitmap(_px.width()),
			scale_bitmap(_px.height())
		);
	}

    QSize unscale_bitmap(const QSize& _px)
    {
        return QSize(
                     unscale_bitmap(_px.width()),
                     unscale_bitmap(_px.height())
                     );
    }

    QRect scale_bitmap(const QRect& _px)
	{
		return QRect(
			_px.left(),
			_px.top(),
			scale_bitmap(_px.width()),
			scale_bitmap(_px.height())
		);
	}

	int scale_bitmap_with_value(const int _px)
	{
		return scale_value(scale_bitmap(_px));
	}

	QSize scale_bitmap_with_value(const QSize& _px)
	{
		return QSize(
			scale_bitmap_with_value(_px.width()),
			scale_bitmap_with_value(_px.height())
		);
	}

	QRect scale_bitmap_with_value(const QRect& _px)
	{
		return QRect(
			_px.left(),
			_px.top(),
			scale_bitmap_with_value(_px.width()),
			scale_bitmap_with_value(_px.height())
		);
	}

    template <typename _T>
    void check_pixel_ratio(_T& _image)
    {
        if (is_mac_retina())
        {
            _image.setDevicePixelRatio(2);
        }
    }
    template void check_pixel_ratio<QImage>(QImage& _pixmap);
    template void check_pixel_ratio<QPixmap>(QPixmap& _pixmap);

    QString parse_image_name(const QString& _imageName)
    {
        if (is_mac_retina())
        {
            QString result(_imageName);
            result.replace("/100/", "/200/");
            result.replace("_100", "_200");
            return result;
        }

        QString result(_imageName);
        double scale = Utils::getScaleCoefficient();
        QString scaleString = QString::number(scale * 100);
        scaleString.prepend("_");
        result.replace("_100", scaleString);

        scaleString = QString::number(scale * 100);
        scaleString.prepend("/");
        scaleString.append("/");
        result.replace("/100/", scaleString);
        return result;
    }

	void addShadowToWidget(QWidget* _target)
	{
		QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(_target);
        QColor widgetShadowColor("#000000");
        widgetShadowColor.setAlphaF(0.3);
		shadow->setColor(widgetShadowColor);
		shadow->setBlurRadius(scale_value(16));
		shadow->setXOffset(scale_value(0));
		shadow->setYOffset(scale_value(2));
		_target->setGraphicsEffect(shadow);
	}

	void addShadowToWindow(QWidget* _target, bool _enabled)
	{
		int shadowWidth = _enabled ? Ui::get_gui_settings()->get_shadow_width() : 0;
		if (_enabled && !_target->testAttribute(Qt::WA_TranslucentBackground))
			_target->setAttribute(Qt::WA_TranslucentBackground);

        auto oldMargins = _target->contentsMargins();
		_target->setContentsMargins(QMargins(oldMargins.left() + shadowWidth, oldMargins.top() + shadowWidth,
            oldMargins.right() + shadowWidth, oldMargins.bottom() + shadowWidth));

		static QPointer<QObject> eventFilter(new ShadowWidgetEventFilter(Ui::get_gui_settings()->get_shadow_width()));

		if (_enabled)
			_target->installEventFilter(eventFilter);
		else
			_target->removeEventFilter(eventFilter);
	}

	void grabTouchWidget(QWidget* _target, bool _topWidget)
	{
#ifdef _WIN32
		if (_topWidget)
		{
			QScrollerProperties sp;
			sp.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.6);
			sp.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
			sp.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.5);
			sp.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.4);
			sp.setScrollMetric(QScrollerProperties::AcceleratingFlickSpeedupFactor, 1.2);
			sp.setScrollMetric(QScrollerProperties::SnapPositionRatio, 0.2);
			sp.setScrollMetric(QScrollerProperties::MaximumClickThroughVelocity, 0);
			sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.01);
			sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0);
            QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
            sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, overshootPolicy);
            sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, overshootPolicy);

			QScroller* clScroller = QScroller::scroller(_target);
			clScroller->grabGesture(_target);
			clScroller->setScrollerProperties(sp);
		}
		else
		{
			QScroller::grabGesture(_target);
		}
#endif //WIN32
	}

	void removeLineBreaks(QString& _source)
	{
		if (_source.isEmpty())
			return;

		bool spaceAtEnd = _source.at(_source.length() - 1) == QChar::Space;
		_source.replace('\n', QChar::Space);
		_source.remove('\r');
		if (!spaceAtEnd && _source.at(_source.length() - 1) == QChar::Space)
			_source = _source.left(_source.length() - 1);
	}

	QColor getSelectionColor()
	{
		QColor color("#579e1c");
		color.setAlphaF(0.5);
		return color;
	}

    QString rgbaStringFromColor(const QColor& _color)
    {
        QString textColorString = QString("rgba(%1, %2, %3, %4%)")
            .arg(_color.red())
            .arg(_color.green())
            .arg(_color.blue())
            .arg(_color.alpha()*100/255);
        return textColorString;
    }

	bool macRetina = false;

	bool is_mac_retina()
	{
		return macRetina;
	}

	void set_mac_retina(bool _val)
	{
        macRetina = _val;
	}

	double scaleCoefficient = 1.0;

	double getScaleCoefficient()
    {
		return scaleCoefficient;
	}

	void setScaleCoefficient(double _coefficient)
	{
        if (platform::is_apple())
        {
            scaleCoefficient = 1.0;
            return;
        }

        if ((_coefficient == 1.0) ||
            (_coefficient == 1.25) ||
            (_coefficient == 1.5) ||
            (_coefficient == 2.0))
        {
            scaleCoefficient = _coefficient;
            return;
        }

        assert(!"unexpected scale value");
        scaleCoefficient = 1.0;
	}

    namespace { double basicScaleCoefficient = 1.0; }

    double getBasicScaleCoefficient()
    {
        return basicScaleCoefficient;
    }

    void initBasicScaleCoefficient(double _coefficient)
    {
        static bool isInitBasicScaleCoefficient = false;
        if (!isInitBasicScaleCoefficient)
            basicScaleCoefficient = _coefficient;
        else
            assert(!"initBasicScaleCoefficient should be called once.");
    }

    void groupTaskbarIcon(bool _enabled)
    {
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_WINDOWS7)
        {
            HMODULE libShell32 = ::GetModuleHandle(L"shell32.dll");
            typedef HRESULT  (__stdcall * SetCurrentProcessExplicitAppUserModelID_Type)(PCWSTR);
            SetCurrentProcessExplicitAppUserModelID_Type SetCurrentProcessExplicitAppUserModelID_Func;
            SetCurrentProcessExplicitAppUserModelID_Func = (SetCurrentProcessExplicitAppUserModelID_Type)::GetProcAddress(libShell32,"SetCurrentProcessExplicitAppUserModelID");
            SetCurrentProcessExplicitAppUserModelID_Func(_enabled ? (build::is_icq() ? application_user_model_id_icq : application_user_model_id_agent) : L"");
        }
#endif //_WIN32
    }

    bool isStartOnStartup()
    {
#ifdef _WIN32
        CRegKey keySoftwareRun;
        if (ERROR_SUCCESS != keySoftwareRun.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_READ))
            return false;

        wchar_t bufferPath[1025];
        ULONG len = 1024;
        auto productName = getProductName();
        if (keySoftwareRun.QueryStringValue((const wchar_t*) productName.utf16(), bufferPath, &len) != ERROR_SUCCESS)
            return false;

#endif //_WIN32
        return true;
    }

    void setStartOnStartup(bool _start)
    {
#ifdef _WIN32

        bool currentState = isStartOnStartup();
        if (currentState == _start)
            return;

        if (_start)
        {
            CRegKey keySoftwareRun;
            if (ERROR_SUCCESS != keySoftwareRun.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_SET_VALUE))
                return;

            wchar_t buffer[1025];
            if (!::GetModuleFileName(0, buffer, 1024))
                return;

            CAtlString exePath = buffer;
            auto productName = getProductName();
            if (ERROR_SUCCESS != keySoftwareRun.SetStringValue((const wchar_t*) productName.utf16(), CAtlString("\"") + exePath + "\"" + " /startup"))
                return;
        }
        else
        {
            auto productName = getProductName();
            ::SHDeleteValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", (const wchar_t*) productName.utf16());
        }
#endif //_WIN32
    }

#ifdef _WIN32
    HWND createFakeParentWindow()
    {
        HINSTANCE instance = (HINSTANCE) ::GetModuleHandle(0);
        HWND hwnd = 0;

        CAtlString className = L"fake_parent_window";
        WNDCLASSEX wc = {0};

        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = instance;
        wc.lpszClassName = (LPCWSTR)className;
        if (!::RegisterClassEx(&wc))
            return hwnd;

        hwnd = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, (LPCWSTR)className, 0, WS_POPUP, 0, 0, 0, 0, 0, 0, instance, 0);
        if (!hwnd)
            return hwnd;

        ::SetLayeredWindowAttributes(
            hwnd,
            0,
            0,
            LWA_ALPHA
            );

        return hwnd;
    }
#endif //WIN32

    const uint getInputMaximumChars()
    {
        return 10000;
    }

    int calcAge(const QDateTime& _birthdate)
    {
        QDate thisdate = QDateTime::currentDateTime().date();
        QDate birthdate = _birthdate.date();

        int age = thisdate.year() - birthdate.year();
        if (age < 0)
            return 0;

        if ((birthdate.month() > thisdate.month()) || (birthdate.month() == thisdate.month() && birthdate.day() > thisdate.day()))
            return (--age);

        return age;
    }

    QString DefaultDownloadsPath()
    {
        QString defaultDownloadsPath = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        if (platform::is_apple())
            defaultDownloadsPath += build::is_agent() ? "/Mail.ru Agent" : "/ICQ";
        return defaultDownloadsPath;
    }
    
    QString UserDownloadsPath()
    {
        QString result = QDir::toNativeSeparators(Ui::get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath()));
        if (platform::is_windows())
            result += build::is_agent() ? "/Mail.ru Agent" : "/ICQ";

        return result;
    }

    bool is_image_extension(const QString& _ext)
    {
        static QStringList imagesExtensions;

        if (imagesExtensions.isEmpty())
        {
            imagesExtensions << "jpg" << "png" << "jpeg" << "gif" << "bmp" << "tif" << "tiff" << "jpeg";
        }

        return imagesExtensions.contains(_ext.toLower());
    }

    bool is_image_extension_not_gif(const QString& _ext)
    {
        static QStringList imagesExtensions;

        if (imagesExtensions.isEmpty())
        {
            imagesExtensions << "jpg" << "png" << "jpeg" << "bmp" << "tif" << "tiff" << "jpeg";
        }

        return imagesExtensions.contains(_ext.toLower());
    }

    bool is_video_extension(const QString& _ext)
    {
        static QStringList videoExtensions;

        if (videoExtensions.isEmpty())
        {
            videoExtensions << "avi" << "mkv" << "wmv" << "flv" << "3gp" << "mpeg4" << "webm" << "mov";
        }

        return videoExtensions.contains(_ext.toLower());
    }

    void copyFileToClipboard(const QString& _path)
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setData("text/uri-list", QUrl::fromLocalFile(_path).toString().toStdString().c_str());
        QApplication::clipboard()->setMimeData(mimeData);
    }

    void saveAs(const QString& _inputFilename, std::function<void (QString& _filename, QString& _directory)> _callback, std::function<void ()> _cancel_callback, bool asSheet)
    {
        static auto lastDirectory = UserDownloadsPath();

        int dot = _inputFilename.lastIndexOf('.');
        QString ext = dot != -1 ? _inputFilename.mid(dot, _inputFilename.length()) : QString();
        QString name = (_inputFilename.contains(QRegExp("\\/:*?\"<>\\|\"")) || _inputFilename.length() >= 128) ? QT_TRANSLATE_NOOP("chat_page", "File") : _inputFilename;
        QString fullName = QDir::toNativeSeparators(QDir(lastDirectory).filePath(name));

#ifdef __APPLE__
        if (asSheet)
        {
            MacSupport::saveFileName(QT_TRANSLATE_NOOP("context_menu", "Save as..."), fullName, "*" + ext, _callback, ext, lastDirectory, _cancel_callback);
            return;
        }
#endif //__APPLE__
        QString destination = QFileDialog::getSaveFileName(0, QT_TRANSLATE_NOOP("context_menu", "Save as..."), fullName, "*" + ext);
        if (!destination.isEmpty())
        {
            QFileInfo info(destination);
            lastDirectory = info.dir().absolutePath();
            QString directory = info.dir().absolutePath();
            QString filename = info.fileName();
            if (info.suffix().isEmpty() && !ext.isEmpty())
            {
                filename += ext;
            }
            _callback(filename, directory);
        }
        else if (_cancel_callback)
        {
            _cancel_callback();
        }
    }

    const SendKeysIndex& getSendKeysIndex()
    {
        static SendKeysIndex index;

        if (index.empty())
        {
            index.emplace_back(QT_TRANSLATE_NOOP("config_dialog", "Enter"), Ui::KeyToSendMessage::Enter);
            index.emplace_back((platform::is_apple() ? QT_TRANSLATE_NOOP("config_dialog", "Cmd+Enter") : QT_TRANSLATE_NOOP("config_dialog", "Ctrl+Enter")), Ui::KeyToSendMessage::Ctrl_Enter);
            index.emplace_back(QT_TRANSLATE_NOOP("config_dialog", "Shift+Enter"), Ui::KeyToSendMessage::Shift_Enter);
        }

        return index;
    }

    void post_stats_with_settings()
    {
        core::stats::event_props_type props;
        props.emplace_back("size", std::to_string(qApp->desktop()->screenGeometry().width()) + "x" + std::to_string(qApp->desktop()->screenGeometry().height()));

        auto env = QProcessEnvironment::systemEnvironment();
        QString processorInfo = QString("Architecture:%1 \nNumber of Processors:%2 \nProcessor Identifier:%3")
            .arg(env.value("PROCESSOR_ARCHITECTURE", "")).arg(env.value("NUMBER_OF_PROCESSORS", "")).arg(env.value("PROCESSOR_IDENTIFIER", ""));
        props.emplace_back("Sys_CPU", processorInfo.toStdString());
        props.emplace_back("Sys_CPU_Qt", QSysInfo::currentCpuArchitecture().toStdString());

        props.emplace_back(std::make_pair("Settings_Startup", std::to_string(Utils::isStartOnStartup())));
        props.emplace_back(std::make_pair("Settings_Taskbar", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))));
        props.emplace_back(std::make_pair("Settings_Sounds", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_sounds_enabled, true))));

        auto currentDownloadDir = UserDownloadsPath();
        props.emplace_back(std::make_pair("Settings_Download_Folder", std::to_string(currentDownloadDir == Utils::DefaultDownloadsPath())));

        QString keyToSend;
        int currentKey = Ui::get_gui_settings()->get_value<int>(settings_key1_to_send_message, Ui::KeyToSendMessage::Enter);
        for (const auto& key : Utils::getSendKeysIndex())
        {
            if (key.second == currentKey)
                keyToSend = key.first;
        }
        props.emplace_back(std::make_pair("Settings_Send_By", keyToSend.toStdString()));
        props.emplace_back(std::make_pair("Settings_Show_Last_Message", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true))));
        props.emplace_back(std::make_pair("Settings_Previews", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true))));
        props.emplace_back(std::make_pair("Settings_Scale", std::to_string(Utils::getScaleCoefficient())));
        props.emplace_back(std::make_pair("Settings_Language", Ui::get_gui_settings()->get_value(settings_language, QString("")).toUpper().toStdString()));

        if (Ui::get_qt_theme_settings()->getDefaultTheme())
            props.emplace_back(std::make_pair("Settings_Wallpaper_Global", std::to_string(Ui::get_qt_theme_settings()->getDefaultTheme()->get_id())));
        else
            props.emplace_back(std::make_pair("Settings_Wallpaper_Global", std::to_string(-1)));

        const auto& themeCounts = Ui::get_qt_theme_settings()->getThemeCounts();
        for (auto iter = themeCounts.cbegin(); iter != themeCounts.cend(); ++iter)
        {
            props.emplace_back(std::make_pair(std::string("Settings_Wallpaper_Local") + " " + std::to_string(iter->first), std::to_string(iter->second)));
        }

        props.emplace_back(std::make_pair("Settings_Sounds_Outgoing", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_outgoing_message_sound_enabled, true))));
        props.emplace_back(std::make_pair("Settings_Alerts", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_notify_new_messages, true))));

        auto contactDialogWidth = Ui::MainPage::getContactDialogWidth(Ui::get_gui_settings()->get_value(settings_main_window_rect, QRect()).width());
        props.emplace_back(std::make_pair("Sidebar_Type", Ui::ContactDialog::getSideBarPolicy(contactDialogWidth)));

        std::stringstream stream;
        stream << Utils::get_proxy_settings()->type_;
        props.emplace_back(std::make_pair("Proxy_Type", stream.str()));

		props.emplace_back(std::make_pair("Settings_Scroll_To_New", std::to_string(Ui::get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, false))));

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::client_settings, props);
    }

    QRect GetMainRect()
    {
        assert(!!Utils::InterConnector::instance().getMainWindow() && "Common.cpp (ItemLength)");
        QRect mainRect = GetWindowRect(Utils::InterConnector::instance().getMainWindow());
        assert("Couldn't get rect: Common.cpp (ItemLength)");
        return mainRect;
    }

    QRect GetWindowRect(QWidget* window)
    {
        QRect mainRect(0, 0, 0, 0);
        if (window)
        {
            mainRect = window->geometry();
        }
        else if (auto window = qApp->activeWindow())
        {
            mainRect = window->geometry();
        }
        return mainRect;
    }

    QPoint GetMainWindowCenter()
    {
        auto mainRect = Utils::GetMainRect();
        auto mainWidth = mainRect.width();
        auto mainHeight = mainRect.height();

        auto x = mainRect.x() + mainWidth / 2;
        auto y = mainRect.y() + mainHeight / 2;
        return QPoint(x, y);
    }

    void UpdateProfile(const std::vector<std::pair<std::string, QString>>& _fields)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        core::ifptr<core::iarray> fieldArray(collection->create_array());
        fieldArray->reserve((int)_fields.size());
        for (unsigned int i = 0; i < _fields.size(); ++i)
        {
            Ui::gui_coll_helper coll(Ui::GetDispatcher()->create_collection(), true);
            coll.set_value_as_string("field_name", _fields[i].first);
            auto value = _fields[i].second;
            coll.set_value_as_string("field_value", value.toUtf8().data(), value.toUtf8().size());

           core::ifptr<core::ivalue> valField(coll->create_value());
           valField->set_as_collection(coll.get());
           fieldArray->push_back(valField.get());
        }

        collection.set_value_as_array("fields", fieldArray.get());
        Ui::GetDispatcher()->post_message_to_core("update_profile", collection.get());
    }

    QString getItemSafe(const std::vector<QString>& _values, size_t _selected, QString _default)
    {
        return _selected < _values.size() ? _values[_selected] : _default;
    }

    Ui::GeneralDialog *NameEditorDialog(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& _resultChatName,
        bool acceptEnter)
    {
        auto mainWidget = new QWidget(_parent);
        mainWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        auto layout = Utils::emptyVLayout(mainWidget);
        layout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(15), Utils::scale_value(24), 0);

        auto textEdit = new Ui::TextEditEx(mainWidget, Fonts::appFontScaled(18), Ui::MessageStyle::getTextColor(), true, true);
        Utils::ApplyStyle(textEdit, Ui::CommonStyle::getLineEditStyle());
        textEdit->setObjectName("input_edit_control");
        textEdit->setPlaceholderText(_chatName);
        textEdit->setPlainText(_chatName);
        textEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        textEdit->setAutoFillBackground(false);
        textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        textEdit->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        textEdit->setCatchEnter(acceptEnter);
        textEdit->setMinimumWidth(Utils::scale_value(300));
        Utils::ApplyStyle(textEdit, "padding: 0; margin: 0;");
        layout->addWidget(textEdit);

        auto horizontalLineWidget = new QWidget(_parent);
        horizontalLineWidget->setFixedHeight(Utils::scale_value(1));
        horizontalLineWidget->setStyleSheet("background-color: #579e1c;");
        layout->addWidget(horizontalLineWidget);

        auto generalDialog = new Ui::GeneralDialog(mainWidget, Utils::InterConnector::instance().getMainWindow());
        generalDialog->addHead();
        generalDialog->addLabel(_headerText);
        generalDialog->addAcceptButton(_buttonText, Utils::scale_value(24), true);

        if (acceptEnter)
            QObject::connect(textEdit, SIGNAL(enter()), generalDialog, SLOT(accept()));

        QTextCursor cursor = textEdit->textCursor();
        cursor.select(QTextCursor::Document);
        textEdit->setTextCursor(cursor);
        textEdit->setFrameStyle(QFrame::NoFrame);

        textEdit->setFocus();

        return generalDialog;
    }

    bool NameEditor(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& _resultChatName,
        bool acceptEnter)
    {
        auto dialog = std::unique_ptr<Ui::GeneralDialog>(NameEditorDialog(_parent, _chatName, _buttonText, _headerText, _resultChatName, acceptEnter));
        auto result = dialog->showInPosition(-1, -1);
        auto textEdit = dialog->findChild<Ui::TextEditEx *>("input_edit_control");
        if (textEdit)
        {
            _resultChatName = textEdit->getPlainText();
        }
        return result;
    }

    bool GetConfirmationWithTwoButtons(
        const QString& _buttonLeftText,
        const QString& _buttonRightText,
        const QString& _messageText,
        const QString& _labelText,
        QWidget* _parent,
        QWidget* _mainWindow)
    {
        auto mainWidget = new QWidget(_parent);

        auto layout = Utils::emptyVLayout(mainWidget);
        mainWidget->setLayout(layout);

        auto generalDialog = new Ui::GeneralDialog(mainWidget, _mainWindow ? _mainWindow : Utils::InterConnector::instance().getMainWindow());
        generalDialog->addHead();
        generalDialog->addLabel(_labelText);
        generalDialog->addText(_messageText, Utils::scale_value(12));
        generalDialog->addButtonsPair(_buttonLeftText, _buttonRightText, Utils::scale_value(24), Utils::scale_value(12));

        auto result = generalDialog->showInPosition(-1, -1);
        delete generalDialog;
        return result;
    }

    bool GetErrorWithTwoButtons(
        const QString& _buttonLeftText,
        const QString& _buttonRightText,
        const QString& /*_messageText*/,
        const QString& _labelText,
        const QString& _errorText,
        QWidget* _parent)
    {
        std::unique_ptr<Ui::GeneralDialog> generalDialog(new Ui::GeneralDialog(nullptr, Utils::InterConnector::instance().getMainWindow()));
        generalDialog->addHead();
        generalDialog->addLabel(_labelText);
        generalDialog->addError(_errorText);
        generalDialog->addButtonsPair(_buttonLeftText, _buttonRightText, Utils::scale_value(24), Utils::scale_value(12));
        return generalDialog->showInPosition(-1, -1);
    }

    ProxySettings::ProxySettings(core::proxy_types _type, QString _username, QString _password,
        QString _proxyServer, int _port, bool _needAuth)
        : type_(_type)
        , username_(_username)
        , password_(_password)
        , proxyServer_(_proxyServer)
        , port_(_port)
        , needAuth_(_needAuth)
    {}

    ProxySettings::ProxySettings()
        : type_(core::proxy_types::auto_proxy)
        , username_("")
        , password_("")
        , proxyServer_("")
        , port_(invalidPort)
        , needAuth_(false)
    {}

    void ProxySettings::postToCore()
    {
        Ui::gui_coll_helper coll(Ui::GetDispatcher()->create_collection(), true);

        coll.set_value_as_enum("settings_proxy_type", type_);
        coll.set_value_as_string("settings_proxy_server", QStringToString(proxyServer_));
        coll.set_value_as_int("settings_proxy_port", port_);
        coll.set_value_as_string("settings_proxy_username", QStringToString(username_));
        coll.set_value_as_string("settings_proxy_password", QStringToString(password_));
        coll.set_value_as_bool("settings_proxy_need_auth", needAuth_);

        Ui::GetDispatcher()->post_message_to_core("set_user_proxy_settings", coll.get());
    }

    ProxySettings* get_proxy_settings()
    {
        static std::shared_ptr<ProxySettings> proxySetting(new ProxySettings());
        return proxySetting.get();
    }

    bool loadPixmap(const QString& _path, Out QPixmap& _pixmap)
    {
        assert(!_path.isEmpty());

        if (!QFile::exists(_path))
        {
            assert(!"file does not exist");
            return false;
        }

        QFile file(_path);
        if (!file.open(QIODevice::ReadOnly))
            return false;

        return loadPixmap(file.readAll(), _pixmap);
    }

    bool loadPixmap(const QByteArray& _data, Out QPixmap& _pixmap)
    {
        assert(!_data.isEmpty());

        static const char *availableFormats[] = { nullptr, "PNG", "JPG" };

        for (auto fmt : availableFormats)
        {
            _pixmap.loadFromData(_data, fmt);

            if (!_pixmap.isNull())
            {
                const auto orientation = Exif::getExifOrientation(_data.data(), _data.size());
                Exif::applyExifOrientation(orientation, InOut _pixmap);
                return true;
            }
        }

        return false;
    }

    bool dragUrl(QWidget* _parent, const QPixmap& _preview, const QString& _url)
    {
        QDrag *drag = new QDrag(_parent);
        QMimeData *mimeData = new QMimeData();
        mimeData->setProperty("icq", true);

        QList<QUrl> list;
        list << _url;
        mimeData->setUrls(list);
        drag->setMimeData(mimeData);

        QPixmap p(_preview);
        if (!p.isNull())
        {
            if (p.width() > Utils::scale_value(drag_preview_max_width))
                p = p.scaledToWidth(Utils::scale_value(drag_preview_max_width), Qt::SmoothTransformation);
            if (p.height() > Utils::scale_value(drag_preview_max_height))
                p = p.scaledToHeight(Utils::scale_value(drag_preview_max_height), Qt::SmoothTransformation);

            QPainter painter;
            painter.begin(&p);
            painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            QColor dragPreviewColor("#000000");
            dragPreviewColor.setAlphaF(0.5);
            painter.fillRect(p.rect(), dragPreviewColor);
            painter.end();

            drag->setPixmap(p);
        }

        return drag->exec(Qt::CopyAction) == Qt::CopyAction;
    }

    bool extractUinFromIcqLink(const QString &_uri, Out QString &_uin)
    {
        auto cleanUri = _uri;
        cleanUri = cleanUri.trimmed();
        cleanUri.remove(QChar::SoftHyphen);
        cleanUri.remove('-');

        QRegularExpression profileLinkRe("^http(s?)://icq.com/(people/)?(?P<uin>\\d{5,9})$");

        auto match = profileLinkRe.match(cleanUri);
        if (!match.hasMatch())
        {
            Out _uin.resize(0);

            return false;
        }

        Out _uin = match.captured("uin");

        return true;
    }

    StatsSender::StatsSender()
        : guiSettingsReceived_(false)
        , themeSettingsReceived_(false)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::guiSettings, this, &StatsSender::recvGuiSettings, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::themeSettings, this, &StatsSender::recvThemeSettings, Qt::QueuedConnection);
    }

    StatsSender* getStatsSender()
    {
        static std::shared_ptr<StatsSender> statsSender(new StatsSender());
        return statsSender.get();
    }

    void StatsSender::trySendStats() const
    {
        if ( (guiSettingsReceived_ || Ui::get_gui_settings()->getIsLoaded() )
            && (themeSettingsReceived_ || Ui::get_qt_theme_settings()->getIsLoaded() ) )
        {
            Utils::post_stats_with_settings();
        }
    }

    bool haveText(const QMimeData * mimedata)
    {
        if (!mimedata)
            return false;

        if (mimedata->hasFormat("application/x-qt-windows-mime;value=\"Csv\""))
            return true;

        if (mimedata->hasUrls())
        {
            QList<QUrl> urlList = mimedata->urls();
            for (auto url : urlList)
            {
                if (url.isValid())
                    return false;
            }
        }

        auto text = mimedata->text();
        QUrl url(text);

        return !text.isEmpty() && (!url.isValid() || url.host().isEmpty());
    }

    QString normalizeLink(const QString& _link)
    {
        QString link = _link;
        while (!link.isEmpty() && (link.endsWith(QChar::Space) ||
                                   link.endsWith(QChar::LineFeed) ||
                                   link.endsWith(QChar::CarriageReturn)))
        {
            link.truncate(link.length() - 1);
        }

        return link;
    }

    const wchar_t* get_crossprocess_mutex_name()
    {
        return (build::is_icq() ? crossprocess_mutex_name_icq : crossprocess_mutex_name_agent);
    }

    const char* get_crossprocess_pipe_name()
    {
        return (build::is_icq() ? crossprocess_pipe_name_icq : crossprocess_pipe_name_agent);
    }

    QHBoxLayout* emptyHLayout(QWidget* parent)
    {
        auto layout = new QHBoxLayout(parent);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        return layout;
    }

    QVBoxLayout* emptyVLayout(QWidget* parent)
    {
        auto layout = new QVBoxLayout(parent);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        return layout;
    }

    void openMailBox(const QString& email, const QString& mrimKey, const QString& mailId)
    {
        core::tools::version_info infoCurrent;
        QString url;
        if (!mailId.isEmpty())
            url = mail_open_mail_url.arg(email).arg(mrimKey).arg(infoCurrent.get_build()).arg(Utils::GetTranslator()->getCurrentLang()).arg(msgIdFromUidl(mailId));
        else
            url = mail_url.arg(email).arg(mrimKey).arg(infoCurrent.get_build()).arg(Utils::GetTranslator()->getCurrentLang());

        QDesktopServices::openUrl(url);

        emit Utils::InterConnector::instance().mailBoxOpened();
    }

    void openAgentUrl(
        const QString& _url,
        const QString& _fail_url,
        const QString& _email,
        const QString& _mrimKey)
    {
        core::tools::version_info infoCurrent;

        const QString signed_url = base_mail_url + redirect + "&page=" + _url + "&FailPage=" + _fail_url;

        const QString final_url = signed_url.arg(_email).arg(_mrimKey).arg(infoCurrent.get_build()).arg(Utils::GetTranslator()->getCurrentLang());

        QDesktopServices::openUrl(final_url);
    }


    QString getProductName()
    {
        return (build::is_icq() ? product_name_icq : product_name_agent);
    }

    QString getInstallerName()
    {
        return (build::is_icq() ? installer_exe_name_icq : installer_exe_name_agent);
    }

    QString getUnreadsBadgeStr(int _unreads)
    {
        QString cnt;
        if (_unreads < 1000)
            cnt = QString::number(_unreads);
        else if (_unreads < 10000)
            cnt = QString("%1k").arg(_unreads / 1000);
        else
            cnt = "9k+";

        return cnt;
    }
    
    void drawUnreads(QPainter *p, const QFont &font, const QColor *bgColor, const QColor *textColor, const QColor *borderColor, int unreads, int balloonSize, int x, int y)
    {
        if (p && unreads > 0)
        {
            QFontMetrics m(font);

            const auto text = getUnreadsBadgeStr(unreads);
            const auto unreadsRect = m.tightBoundingRect(text);
            const auto firstChar = text[0];
            const auto lastChar = text[text.size() - 1];
            const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
            const auto unreadsHeight = unreadsRect.height();

            auto balloonWidth = unreadsWidth;
            const auto isLongText = (text.length() > 1);
            if (isLongText)
            {
                balloonWidth += (Utils::scale_value(16));
            }
            else
            {
                balloonWidth = balloonSize;
            }

            const auto balloonRadius = (balloonSize / 2);

            p->save();
            p->setPen(Qt::NoPen);
            p->setRenderHint(QPainter::Antialiasing);

            int borderWidth = 0;
            if (borderColor)
            {
                p->setBrush(*borderColor);
                p->setPen(*borderColor);
                borderWidth = Utils::scale_value(1);
                p->drawRoundedRect(x - borderWidth, y - borderWidth, balloonWidth + borderWidth * 2, balloonSize + borderWidth * 2, balloonRadius, balloonRadius);
            }

            p->setBrush(*bgColor);
            p->drawRoundedRect(x, y, balloonWidth, balloonSize, balloonRadius, balloonRadius);

            p->setFont(font);
            p->setPen(*textColor);
            if (platform::is_apple())
            {
                p->drawText(QRectF(x, y, balloonWidth, balloonSize), text, QTextOption(Qt::AlignCenter));
            }
            else
            {
                const float textX = floorf((float)x + ((float)balloonWidth - (float)unreadsWidth) / 2.);
                const float textY = ceilf((float)y + ((float)balloonSize + (float)unreadsHeight) / 2.);
                p->drawText(textX, textY, text);
            }
        }
    }

	QPoint getUnreadsSize(QPainter *p, const QFont &font, bool bBorder, int unreads, int balloonSize)
    {
		QPoint ret;
        if (p)
        {
            QFontMetrics m(font);

            const auto text = getUnreadsBadgeStr(unreads);
            const auto unreadsRect = m.tightBoundingRect(text);
            const auto firstChar = text[0];
            const auto lastChar = text[text.size() - 1];
            const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
            const auto unreadsHeight = unreadsRect.height();

            auto balloonWidth = unreadsWidth;
            const auto isLongText = (text.length() > 1);
            if (isLongText)
            {
                balloonWidth += (Utils::scale_value(16));
            }
            else
            {
                balloonWidth = balloonSize;
            }

            const auto balloonRadius = (balloonSize / 2);

            int borderWidth = 0;
            if (bBorder)
            {
                borderWidth = Utils::scale_value(1);
				ret.setX(balloonWidth + borderWidth * 2);
				ret.setY(balloonSize + borderWidth * 2);
            }
			else
			{
				ret.setX(balloonWidth);
				ret.setY(balloonSize);
			}
		}
		return ret;
	}

    QImage iconWithCounter(int size, int count, QColor bg, QColor fg, QImage back)
    {
        QString cnt = getUnreadsBadgeStr(count);

        QImage result = back.isNull() ? QImage(size, size, QImage::Format_ARGB32) : back;
        int32_t cntSize = cnt.size();
        if (back.isNull())
            result.fill(Qt::transparent);

        {
            QPainter p(&result);
            p.setBrush(bg);
            p.setPen(Qt::NoPen);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            p.setRenderHint(QPainter::HighQualityAntialiasing);
            int32_t fontSize;
            if (back.isNull())
            {
                if (size == 16) 
                    fontSize = (cntSize < 3) ? 11 : 8;
                else 
                    fontSize = (cntSize < 3) ? 22 : 16;
            }
            else
            {
                if (size == 16) 
                    fontSize = 8;
                else 
                    fontSize = 16;
            }
            
            auto f = Fonts::appFont(fontSize, Fonts::FontWeight::Normal);
            f.setStyleStrategy(QFont::PreferQuality);

            //don't try to understand it. just walk away. really.
            QFontMetrics m(f);
            int32_t d = 0, r = 0, l = 0, k = 0, o = 0;
            if (back.isNull())
            {
                if (size == 16) 
                {
                    l = cntSize > 2 ? 2 : 0;
                    d = cntSize > 2 ? 4 : 0;
                    k = count > 10000 ? 1 : 0;
                    r = cntSize > 2 ? 6 : 8;
                }
                else
                {
                    l = cntSize > 2 ? 4 : 0;
                    d = cntSize > 2 ? 8 : 0;
                    k = count > 10000 ? 2 : 0;
                    r = cntSize > 2 ? 12 : 16;
                }
            }
            else
            {
                if (size == 16) 
                {
                    o = cntSize > 2 ? 0 : 4;
                    l = cntSize > 2 ? 2 : 2;
                    d = 4;
                    k = count > 10000 ? 1 : 0;
                    r = cntSize > 2 ? 6 : 8;
                }
                else
                {
                    o = cntSize > 2 ? 0 : 8;
                    l = cntSize > 2 ? 4 : 4;
                    d = 8;
                    k = count > 10000 ? 2 : 0;
                    r = cntSize > 2 ? 12 : 16;
                }
            }
            
            auto rect = QRect(o, d, size - o, size - d);
            p.drawRoundedRect(rect, r, r);
            p.setFont(f);
            p.setPen(fg);

            const auto unreadsRect = m.tightBoundingRect(cnt);
            const auto firstChar = cnt[0];
            const auto lastChar = cnt[cnt.size() - 1];
            const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
            const auto unreadsHeight = unreadsRect.height();
            const float textX = floorf(((float)size - (float)unreadsWidth) / 2. + k + o / 2.);
            const float textY = ceilf(((float)size + (float)unreadsHeight) / 2. + l);
            p.drawText(textX, textY, cnt);
        }
        return result;
    }
}
