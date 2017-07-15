#include "stdafx.h"

#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../controls/TextEditEx.h"

#include "Common.h"
#include "ContactList.h"
#include "../../gui_settings.h"

namespace ContactList
{
	VisualDataBase::VisualDataBase(
		const QString& _aimId,
		const QPixmap& _avatar,
		const QString& _state,
		const QString& _status,
		const bool _isHovered,
		const bool _isSelected,
		const QString& _contactName,
		const bool _hasLastSeen,
		const QDateTime& _lastSeen,
		bool _isWithCheckBox,
        bool _isChatMember,
        bool _isOfficial,
        const bool _drawLastRead,
        const QPixmap& _lastReadAvatar,
        const QString& _role,
        int _unreadsCounter,
        const QString _term)

		: AimId_(_aimId)
		, Avatar_(_avatar)
		, State_(_state)
		, Status_(_status)
		, IsHovered_(_isHovered)
		, IsSelected_(_isSelected)
		, ContactName_(_contactName)
		, HasLastSeen_(_hasLastSeen)
		, LastSeen_(_lastSeen)
		, isCheckedBox_(_isWithCheckBox)
        , isChatMember_(_isChatMember)
        , isOfficial_(_isOfficial)
        , drawLastRead_(_drawLastRead)
        , lastReadAvatar_(_lastReadAvatar)
        , role_(_role)
        , unreadsCounter_(_unreadsCounter)
        , searchTerm_(_term)
	{
		assert(!AimId_.isEmpty());
		assert(!ContactName_.isEmpty());
	}

    const QString& VisualDataBase::GetStatus() const
    {
        return Status_;
    }

    bool VisualDataBase::HasStatus() const
    {
        return !Status_.isEmpty();
    }

    void VisualDataBase::SetStatus(const QString& _status)
    {
        Status_ = _status;
    }

	QString FormatTime(const QDateTime& _time)
	{
		if (!_time.isValid())
		{
			return QString();
		}

		const auto current = QDateTime::currentDateTime();

		const auto days = _time.daysTo(current);

		if (days == 0)
		{
			const auto minutes = _time.secsTo(current) / 60;
			if (minutes < 1)
			{
				return QT_TRANSLATE_NOOP("contact_list", "now");
			}

			if (minutes < 10)
			{
				return QVariant(minutes).toString() + QT_TRANSLATE_NOOP("contact_list", " min");
			}

			return _time.time().toString(Qt::SystemLocaleShortDate);
		}

		if (days == 1)
		{
			return QT_TRANSLATE_NOOP("contact_list", "yesterday");
		}

		const auto date = _time.date();
		return Utils::GetTranslator()->formatDate(date, date.year() == current.date().year());
	}

	TextEditExUptr CreateTextBrowser(const QString& _name, const QString& _stylesheet, const int _textHeight)
	{
		assert(!_name.isEmpty());
		assert(!_stylesheet.isEmpty());

		TextEditExUptr ctrl(new Ui::TextEditEx(nullptr, QFont(), QColor(), false, false));

		ctrl->setObjectName(_name);
		ctrl->setStyleSheet(_stylesheet);
        if (_textHeight)
		    ctrl->setFixedHeight(_textHeight);

		ctrl->setFrameStyle(QFrame::NoFrame);
		ctrl->setContentsMargins(0, 0, 0, 0);

		ctrl->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		ctrl->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		return ctrl;
	}

    QLineEdit *CreateTextBrowser2(const QString& _name, const QString& _stylesheet, const int _textHeight)
    {
        QLineEdit *ctrl = new QLineEdit();
        
        ctrl->setReadOnly(true);
        ctrl->setFrame(false);
        ctrl->setObjectName(_name);
        ctrl->setStyleSheet(_stylesheet);
        if (_textHeight)
            ctrl->setFixedHeight(_textHeight);
        
        ctrl->setContentsMargins(0, 0, 0, 0);
        
        return ctrl;
    }

	int ItemWidth(bool _fromAlert, bool _isWithCheckBox, bool _isPictureOnlyView)
	{
        if (_isPictureOnlyView)
        {
            return
                (::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).itemHorPadding()
                + ::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).avatarSize()
                + ::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).itemHorPadding()
                );
        }

		if (_fromAlert)
			return ContactList::GetRecentsParams(Logic::MembersWidgetRegim::FROM_ALERT).itemWidth();

		// TODO : use L::checkboxWidth
        return (_isWithCheckBox ? Utils::scale_value(30) : 0) + std::min(Utils::scale_value(400), ItemLength(true, 1. / 3, 0));
	}

    int ItemWidth(const ViewParams& _viewParams)
    {
        return ItemWidth(_viewParams.regim_ == ::Logic::MembersWidgetRegim::FROM_ALERT,
            IsSelectMembers(_viewParams.regim_), _viewParams.pictOnly_);
    }

    int CorrectItemWidth(int _itemWidth, int _fixedWidth)
    {
        return _fixedWidth == -1 ? _itemWidth : _fixedWidth;
    }

	int ItemLength(bool _isWidth, double _koeff, int _addWidth)
	{
        return ItemLength(_isWidth, _koeff, _addWidth, Utils::InterConnector::instance().getMainWindow());
	}

    int ItemLength(bool _isWidth, double _koeff, int _addWidth, QWidget* parent)
    {
        assert(!!parent && "Common.cpp (ItemLength)");
        auto mainRect = Utils::GetWindowRect(parent);
        if (mainRect.width() && mainRect.height())
        {
            auto mainLength = _isWidth ? mainRect.width() : mainRect.height();
            return _addWidth + mainLength * _koeff;
        }
        assert("Couldn't get rect: Common.cpp (ItemLength)");
        return 0;
    }

    bool IsPictureOnlyView()
    {
        auto mainRect = Utils::GetMainRect();

        // TODO : use Utils::MinWidthForMainWindow
        return mainRect.width() <= Utils::scale_value(800);
    }

    ContactListParams& GetContactListParams()
    {
        static ContactListParams params(true);
        return params;
    }

    ContactListParams& GetRecentsParams(int _regim)
    {
        if (_regim == Logic::MembersWidgetRegim::FROM_ALERT || _regim == Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            static ContactListParams params(false);
            return params;
        }
        else
        {
            const auto show_last_message = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);
            static ContactListParams params(!show_last_message);
            params.setIsCL(!show_last_message);
            return params;
        }
    }

    void RenderAvatar(QPainter &_painter, int _x, const QPixmap& _avatar, ContactListParams& _contactList)
    {
        if (_avatar.isNull())
        {
            return;
        }

        _painter.drawPixmap(_x, _contactList.avatarY(), _contactList.avatarSize(), _contactList.avatarSize(), _avatar);
    }

    void RenderMouseState(QPainter &_painter, const bool _isHovered, const bool _isSelected, const ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        if (!_isHovered && !_isSelected)
        {
            return;
        }

        _painter.save();

        if (_isHovered)
        {
            static QBrush hoverBrush(Ui::CommonStyle::getContactListHoveredColor());
            _painter.setBrush(hoverBrush);
        }
        if (_isSelected)
        {
            static QBrush selectedBrush(Ui::CommonStyle::getContactListSelectedColor());
            _painter.setBrush(selectedBrush);
        }

        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        _painter.drawRect(0, 0, width, _contactList.itemHeight());

        _painter.restore();
    }

    int RenderDate(QPainter &_painter, const QDateTime &_ts, const VisualDataBase &_item, ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        const auto regim = _viewParams.regim_;
        const auto isWithCheckBox = IsSelectMembers(regim);
        auto timeXRight =
            CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_)
            - _contactList.itemHorPadding();

        if (!_ts.isValid())
        {
            return timeXRight;
        }

        const auto timeStr = FormatTime(_ts);
        if (timeStr.isEmpty())
        {
            return timeXRight;
        }

        static QFontMetrics m(_contactList.timeFont());
        const auto leftBearing = m.leftBearing(timeStr[0]);
        const auto rightBearing = m.rightBearing(timeStr[timeStr.length() - 1]);
        const auto timeWidth = (m.tightBoundingRect(timeStr).width() + leftBearing + rightBearing);
        const auto timeX = timeXRight - timeWidth;

        if ((!isWithCheckBox && !Logic::is_members_regim(regim) && !Logic::is_admin_members_regim(regim))
            || (Logic::is_members_regim(regim) && !_item.IsHovered_)
            || (!Logic::is_admin_members_regim(regim) && !_item.IsHovered_))
        {
            _painter.save();
            _painter.setFont(_contactList.timeFont());
            _painter.setPen(_contactList.timeFontColor(_item.IsSelected_));
            _painter.drawText(timeX, _contactList.timeY(), timeStr);
            _painter.restore();
        }

        return timeX;
    }

    void RenderContactName(QPainter &_painter, const ContactList::VisualDataBase &_visData, const int _y, const int _rightMargin, ContactList::ViewParams _viewParams, ContactList::ContactListParams& _contactList)
    {
        assert(_y > 0);
        assert(_rightMargin > 0);
        assert(!_visData.ContactName_.isEmpty());

        _contactList.setLeftMargin(_viewParams.leftMargin_);

        QColor color;
        auto weight = Fonts::FontWeight::Normal;
        const auto hasUnreads = (_viewParams.regim_ != ::Logic::MembersWidgetRegim::FROM_ALERT && (_visData.unreadsCounter_ > 0));
        weight = (hasUnreads ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
        QString name;
        int height = 0;
        if (_contactList.isCL())
        {
            auto isMemberSelected = IsSelectMembers(_viewParams.regim_) && (_visData.isChatMember_ || _visData.isCheckedBox_);
            color = _contactList.getNameFontColor(_visData.IsSelected_, isMemberSelected);
            name = "name";
            height = _contactList.contactNameHeight();
        }
        else
        {
            color = _contactList.getNameFontColor(_visData.IsSelected_, false);
            name = hasUnreads ? "nameUnread" : "name";
            height = _contactList.contactNameHeight() + (platform::is_apple() ? 1 : 0);
        }

        const auto styleSheetQss = _contactList.getContactNameStylesheet(Utils::rgbaStringFromColor(color), weight);

#ifdef __APPLE__
        static std::shared_ptr<QLineEdit> textControl(CreateTextBrowser2(name, styleSheetQss, height));
#else
        static auto textControl = CreateTextBrowser(name, styleSheetQss, height);
#endif

        textControl->setStyleSheet(styleSheetQss);

        QPixmap official_mark;
        if (_visData.isOfficial_)
        {
            official_mark = QPixmap(Utils::parse_image_name(":/resources/official_badge_blue_100.png"));
            Utils::check_pixel_ratio(official_mark);
        }

        int maxWidth = _rightMargin - _contactList.GetContactNameX();
        if (_contactList.isCL())
        {
            maxWidth -= _contactList.itemContentPadding();
        }

        if (!official_mark.isNull())
            maxWidth -= official_mark.width();

        textControl->setFixedWidth(maxWidth);

        QFontMetrics m(textControl->font());
        const auto elidedString = m.elidedText(_visData.ContactName_, Qt::ElideRight, maxWidth);

#ifdef __APPLE__
        textControl->setText(elidedString);
#else
        auto &doc = *textControl->document();
        doc.clear();
        QTextCursor cursor = textControl->textCursor();
        Logic::Text4Edit(elidedString, *textControl, cursor, false, Emoji::EmojiSizePx::Auto);
        Logic::FormatDocument(doc, _contactList.contactNameHeight());

        // maybe cut it? ehh.. later ;)
        qreal correction = 0;
        if (platform::is_apple())
        {
            qreal realHeight = doc.documentLayout()->documentSize().toSize().height();

            if (_contactList.isCL())
                correction = (realHeight > 20) ? 0 : 2;
            else
                correction = (realHeight > 21) ? -2 : 2;
            textControl->render(&_painter, QPoint(_contactList.GetContactNameX(), _y + correction));
        }
        else
#endif
        {
            textControl->render(&_painter, QPoint(_contactList.GetContactNameX(), _y));
        }

        double ratio = Utils::scale_bitmap(1);

        int pX = _contactList.GetContactNameX() + m.width(elidedString) + _contactList.official_hor_padding();
        int pY = _y + (_contactList.contactNameHeight() - official_mark.height() / ratio) / 2;
        _painter.drawPixmap(pX, pY, official_mark);
    }

    int RenderAddContact(QPainter &_painter, int& _rightMargin, bool _isSelected, ContactListParams& _recentParams)
    {
        auto img = QPixmap(Utils::parse_image_name(_isSelected ? ":/resources/i_add_100.png" : ":/resources/i_add_100.png"));
        Utils::check_pixel_ratio(img);

        double ratio = Utils::scale_bitmap(1);
        _recentParams.addContactFrame().setX(
            _rightMargin - (img.width() / ratio) - Utils::scale_value(8));
        _recentParams.addContactFrame().setY(
            (_recentParams.itemHeight() / 2) - (img.height() / ratio / 2.));
        _recentParams.addContactFrame().setWidth(img.width() / ratio);
        _recentParams.addContactFrame().setHeight(img.height() / ratio);
        _painter.save();
        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);
        _painter.drawPixmap(_recentParams.addContactFrame().x(),
            _recentParams.addContactFrame().y(),
            img);
        _painter.restore();
        //_rightMargin -= img.width();
        return (_recentParams.addContactFrame().x() - _recentParams.itemHorPadding());
    }

    void RenderCheckbox(QPainter &_painter, const VisualDataBase &_visData, ContactListParams& _contactList)
    {
        auto img = QPixmap(_visData.isChatMember_ ?
            Utils::parse_image_name(":/resources/i_history_100.png")
            : (_visData.isCheckedBox_
                ? Utils::parse_image_name(":/resources/basic_elements/content_check_100.png")
                : Utils::parse_image_name(":/resources/basic_elements/content_uncheck_100.png")));

        Utils::check_pixel_ratio(img);
        double ratio = Utils::scale_bitmap(1);
        _painter.drawPixmap(_contactList.itemHorPadding(), _contactList.getItemMiddleY() - (img.height() / 2. / ratio), img);
    }

    int RenderRemove(QPainter &_painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        auto remove_img = QPixmap(Utils::parse_image_name(_isSelected ? ":/resources/basic_elements/close_b_100.png" : ":/resources/basic_elements/close_a_100.png"));
        Utils::check_pixel_ratio(remove_img);

        double ratio = Utils::scale_bitmap(1);
        _contactList.removeContactFrame().setX(
            CorrectItemWidth(ItemWidth(false, false, false), _viewParams.fixedWidth_)
            - _contactList.itemHorPadding() - (remove_img.width() / ratio) + Utils::scale_value(8));
        _contactList.removeContactFrame().setY(
            (_contactList.itemHeight() / 2) - (remove_img.height() / ratio / 2.));
        _contactList.removeContactFrame().setWidth(remove_img.width());
        _contactList.removeContactFrame().setHeight(remove_img.height());

        _painter.save();
        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);

        _painter.drawPixmap(_contactList.removeContactFrame().x(),
            _contactList.removeContactFrame().y(),
            remove_img);

        _painter.restore();

        return _contactList.removeContactFrame().x();

    }

    int GetXOfRemoveImg(int _width)
    {
        auto _contactList = GetContactListParams();
        return
            CorrectItemWidth(ItemWidth(false, false), _width)
            - _contactList.itemHorPadding()
            - _contactList.removeSize().width()
            + Utils::scale_value(8);
    }

    bool IsSelectMembers(int regim)
    {
        return (regim == Logic::MembersWidgetRegim::SELECT_MEMBERS) || (regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE);
    }
}
