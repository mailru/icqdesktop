#include "stdafx.h"

#include "RecentsItemRenderer.h"
#include "Common.h"
#include "RecentsModel.h"
#include "ContactList.h"
#include "../../gui_settings.h"
#include "../../controls/CommonStyle.h"
#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"
#include "../../utils/Text2DocConverter.h"
#include "../../controls/TextEditEx.h"
#include "../../utils/utils.h"

namespace ContactList
{
    RecentItemVisualData::RecentItemVisualData(
        const QString &_aimId,
        const QPixmap &_avatar,
        const QString &_state,
        const QString &_status,
        const bool _isHovered,
        const bool _isSelected,
        const QString &_contactName,
        const bool _hasLastSeen,
        const QDateTime &_lastSeen,
        const int _unreadsCounter,
        const bool _muted,
        const QString &_senderNick,
        const bool _isOfficial,
        const bool _drawLastRead,
        const QPixmap& _lastReadAvatar,
        const bool _isTyping,
        const QString _term,
        const bool _hasLastMsg,
        qint64 _msgId)
        : VisualDataBase(_aimId, _avatar, _state, _status, _isHovered, _isSelected, _contactName, _hasLastSeen, _lastSeen, false /*_isWithCheckBox*/
            , false /* _isChatMember */, _isOfficial, _drawLastRead, _lastReadAvatar, QString() /* role */, _unreadsCounter, _term)
        , Muted_(_muted)
        , senderNick_(_senderNick)
        , IsTyping_(_isTyping)
        , HasLastMsg_(_hasLastMsg)
        , msgId_(_msgId)
        , IsMailStatus_(false)
    {
        assert(_unreadsCounter >= 0);
    }

    void RenderRecentsItem(QPainter &_painter, const RecentItemVisualData &_item, const ViewParams& _viewParams)
    {
        auto contactListInRecents = GetRecentsParams(_viewParams.regim_);
        _painter.save();

        _painter.setBrush(Qt::NoBrush);
        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        RenderMouseState(_painter, _item.IsHovered_, _item.IsSelected_, contactListInRecents, _viewParams);
        RenderAvatar(_painter, contactListInRecents.avatarX(), _item.Avatar_, contactListInRecents);

        int rightMargin = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_) - contactListInRecents.itemHorPadding();

        if (_viewParams.regim_ != ::Logic::MembersWidgetRegim::FROM_ALERT && _viewParams.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            if (_viewParams.regim_ == ::Logic::MembersWidgetRegim::UNKNOWN)
            {
                if (!_viewParams.pictOnly_)
                    rightMargin = RenderRemove(_painter, _item.IsSelected_, contactListInRecents, _viewParams);

                if (!_item.unreadsCounter_)
                {
                    if (!_viewParams.pictOnly_)
                        rightMargin = RenderAddContact(_painter, rightMargin, _item.IsSelected_, contactListInRecents);
                }
                else
                {
                    rightMargin = RenderNotifications(_painter, _item.unreadsCounter_, false /* muted */, _viewParams, contactListInRecents, false /* isUnknownHeader */, _item.IsSelected_, _item.IsHovered_);
                }
            }
            else
            {
                if (!_item.drawLastRead_ || _item.Muted_)
                {
                    rightMargin = RenderNotifications(_painter, _item.unreadsCounter_, _item.Muted_, _viewParams, contactListInRecents, false /* isUnknownHeader */, _item.IsSelected_, _item.IsHovered_);
                }
            }
        }

        if (_viewParams.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH && _item.HasLastMsg_)
            rightMargin = RenderDate(_painter, _item.LastSeen_, _item, contactListInRecents, _viewParams);

        if (_viewParams.pictOnly_)
        {
            _painter.restore();
            return;
        }

        if (_item.IsMailStatus_)
        {
            RenderContactName(_painter, _item, contactListInRecents.nameYForMailStatus(), rightMargin, _viewParams, contactListInRecents);
            return;
        }

        RenderContactName(_painter, _item, contactListInRecents.nameY(), rightMargin, _viewParams, contactListInRecents);

        const int lastReadWidth =
            contactListInRecents.getLastReadAvatarSize() + contactListInRecents.getlastReadRightMargin() + contactListInRecents.getlastReadLeftMargin();

        rightMargin -= (_item.drawLastRead_ ? lastReadWidth : 0);

        const int messageWidth = RenderContactMessage(_painter, _item, rightMargin, _viewParams, contactListInRecents);

        const auto showLastMessage = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);

        if (_item.drawLastRead_ && !_item.IsTyping_ && showLastMessage)
        {
            RenderLastReadAvatar(_painter, _item.lastReadAvatar_, contactListInRecents.messageX() + messageWidth + contactListInRecents.getlastReadLeftMargin(), contactListInRecents);
        }

        _painter.restore();
    }

    void RenderRecentsDragOverlay(QPainter &_painter, const ViewParams& _viewParams)
    {
        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);
        auto recentParams = GetRecentsParams(_viewParams.regim_);

        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        QColor overlayColor("#ffffff");
        overlayColor.setAlphaF(0.9);
        _painter.fillRect(0, 0, width, recentParams.itemHeight(), QBrush(overlayColor));
        _painter.setBrush(QBrush(Qt::transparent));
        QPen pen (QColor("#579e1c"), recentParams.dragOverlayBorderWidth(), Qt::DashLine, Qt::RoundCap);
        _painter.setPen(pen);
        _painter.drawRoundedRect(
            recentParams.dragOverlayPadding(),
            recentParams.dragOverlayVerPadding(),
            width - recentParams.itemHorPadding(),
            recentParams.itemHeight() - recentParams.dragOverlayVerPadding(),
            recentParams.dragOverlayBorderRadius(),
            recentParams.dragOverlayBorderRadius()
            );

        QPixmap p(Utils::parse_image_name(":/resources/file_sharing/content_upload_cl_100.png"));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = width / 2 - p.width() / 2. / ratio;
        int y = (recentParams.itemHeight() / 2) - (p.height() / 2. / ratio);
        _painter.drawPixmap(x, y, p);

        _painter.restore();
    }

    void RenderServiceItem(QPainter &_painter, const QString& _text, bool _renderState, bool _isFavorites, const ViewParams& _viewParams)
    {
        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        QPen pen;
        pen.setColor(QColor("#579e1c"));
        _painter.setPen(pen);

        auto font = Fonts::appFontScaled(12, Fonts::FontWeight::Medium);
        _painter.setFont(font);

        auto recentParams = GetRecentsParams(_viewParams.regim_);
        double ratio = Utils::scale_bitmap(1);

        if (!_viewParams.pictOnly_)
        {
            Utils::drawText(_painter, QPointF(recentParams.itemHorPadding(), recentParams.serviceItemHeight() / 2), Qt::AlignVCenter, _text);
        }
        else
        {
            _painter.save();
            QPen line_pen;
            line_pen.setColor(QColor("#d7d7d7"));
            _painter.setPen(line_pen);
            auto p = QPixmap(Utils::parse_image_name(_isFavorites ? ":/resources/icon_favorites_100.png" : ":/resources/icon_recents_100.png"));
            Utils::check_pixel_ratio(p);
            int y = recentParams.serviceItemHeight() / 2;
            int xp = ItemWidth(_viewParams) / 2 - (p.width() / 2. / ratio);
            int yp = recentParams.serviceItemHeight() / 2 - (p.height() / 2. / ratio);
            _painter.drawLine(0, y, xp - recentParams.serviceItemIconPadding(), y);
            _painter.drawLine(xp + p.width() / ratio + recentParams.serviceItemIconPadding(), y, ItemWidth(_viewParams), y);
            _painter.drawPixmap(xp, yp, p);
            _painter.restore();
        }

        if (_renderState && !_viewParams.pictOnly_)
        {
            QPixmap p(Utils::parse_image_name(Logic::getRecentsModel()->isFavoritesVisible() ? ":/resources/basic_elements/arrow_small_green_100.png" : ":/resources/basic_elements/arrow_small_up_green_100.png"));
            Utils::check_pixel_ratio(p);
            QFontMetrics m(font);
            int x = recentParams.itemHorPadding() + m.width(_text) + recentParams.favoritesStatusPadding();
            int y = recentParams.serviceItemHeight() / 2 - (p.height() / 2. / ratio);
            _painter.drawPixmap(x, y, p);
        }

        _painter.restore();
    }

    void RenderUnknownsHeader(QPainter &_painter, const QString& _title, const int _count, const ViewParams& _viewParams)
    {
        auto recentParams = GetRecentsParams(_viewParams.regim_);
        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        if (!_viewParams.pictOnly_)
        {
            QPen pen;
            pen.setColor(Ui::CommonStyle::getTextCommonColor());
            _painter.setPen(pen);

            QFont f;
            if (_count)
            {
                f = Fonts::appFontScaled(16, Fonts::FontWeight::Medium);
            }
            else
            {
                f = Fonts::appFontScaled(16);
            }
            _painter.setFont(f);
            QFontMetrics metrics(f);
            Utils::drawText(_painter, QPointF(recentParams.itemHorPadding(), recentParams.unknownsItemHeight() / 2.), Qt::AlignVCenter, _title);
        }
        else
        {
            QPixmap pict(Utils::parse_image_name(":/resources/unknown_100.png"));
            Utils::check_pixel_ratio(pict);
            _painter.drawPixmap(recentParams.avatarX() + recentParams.avatarSize() / 2 - pict.width() / (2 * Utils::scale_bitmap(1)), recentParams.avatarY(), pict);
        }

        _painter.restore();

        if (_count)
        {
            _painter.save();

            _painter.setBrush(Qt::NoBrush);
            _painter.setPen(Qt::NoPen);
            _painter.setRenderHint(QPainter::Antialiasing);

            RenderNotifications(_painter, _count, false, _viewParams, recentParams, true /* isUnknownHeader */, false, false);
            _painter.restore();
        }
    }
}

namespace ContactList
{
    Emoji::EmojiSizePx GetEmojiSize()
    {
        static std::unordered_map<int32_t, Emoji::EmojiSizePx> info;

        if (info.empty())
        {
            info.emplace(100, Emoji::EmojiSizePx::_16);
            info.emplace(125, Emoji::EmojiSizePx::_22);
            info.emplace(150, Emoji::EmojiSizePx::_27);
            info.emplace(200, Emoji::EmojiSizePx::_32);
        }

        return info[(int32_t)(Utils::getScaleCoefficient() * 100)];
    }


    int RenderContactMessage(QPainter &_painter, const RecentItemVisualData &_visData, const int _rightMargin, const ViewParams& _viewParams, ContactListParams& _recentParams)
    {
        if (!_visData.HasStatus())
        {
            return 0;
        }

        static auto plainTextControl = CreateTextBrowser("message", _recentParams.getMessageStylesheet(false, false), _recentParams.messageHeight());
        static auto unreadsTextControl = CreateTextBrowser("messageUnreads", _recentParams.getMessageStylesheet(true, false), _recentParams.messageHeight());
        static auto selectedTextControl = CreateTextBrowser("message", _recentParams.getMessageStylesheet(false, true), _recentParams.messageHeight());

        const auto hasUnreads = (_viewParams.regim_ != ::Logic::MembersWidgetRegim::FROM_ALERT
            && _viewParams.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH
            && (_visData.unreadsCounter_ > 0));
        auto &textControl = _visData.IsSelected_ ? selectedTextControl : (hasUnreads ? unreadsTextControl : plainTextControl);

        const auto maxWidth = (_rightMargin - _recentParams.messageX());
        textControl->setFixedWidth(maxWidth);
        textControl->setWordWrapMode(QTextOption::WrapMode::NoWrap);

        auto messageTextMaxWidth = maxWidth;

        const auto font = textControl->font();

        auto &doc = *textControl->document();
        doc.clear();

        auto cursor = textControl->textCursor();

        const auto &senderNick = _visData.senderNick_;
        const auto showLastMessage = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);

        if (!showLastMessage && _viewParams.regim_ != Logic::MembersWidgetRegim::FROM_ALERT && _viewParams.regim_ != Logic::MembersWidgetRegim::HISTORY_SEARCH)
            return -1;

        if (!senderNick.isEmpty() && !_visData.IsTyping_)
        {
            static const QString fix(": ");

            QString fixedNick;
            fixedNick.reserve(senderNick.size() + fix.length());
            fixedNick.append(senderNick.simplified());

            fixedNick.append(fix);

            const auto charFormat = cursor.charFormat();

            auto boldCharFormat = charFormat;

            boldCharFormat.setFont(Fonts::appFont(_recentParams.messageFontSize(), Fonts::FontWeight::Normal));

            cursor.setCharFormat(boldCharFormat);

            Logic::Text4Edit(fixedNick, *textControl, cursor, false, GetEmojiSize());

            cursor.setCharFormat(charFormat);

            messageTextMaxWidth -= doc.idealWidth();
        }

        const auto text = _visData.GetStatus().trimmed().simplified();

        auto base_font = Fonts::appFont(_recentParams.messageFontSize(), Fonts::FontWeight::Medium);
        auto term_font = Fonts::appFont(_recentParams.messageFontSize(), Fonts::FontWeight::Medium);

        QFontMetrics m(font);
        QFontMetrics term_m(term_font);

        auto term = _visData.searchTerm_;
        auto elidedText = text;
        int begin_term = term == "" ? 0 : text.indexOf(term, 0, Qt::CaseInsensitive);

        QString left_part;
        QString right_part;
        QString term_visible_part;

        if (term != "" && begin_term != -1)
        {
            static QString stat_term = "";
            static QHash<qint64, std::pair<int, std::vector<QString>>> words_parts_and_width;

            if (stat_term != term)
            {
                stat_term = term;
                words_parts_and_width.clear();
            }

            auto need_update = false;
            if (words_parts_and_width.count(_visData.msgId_) == 0)
            {
                need_update = true;
            }
            else if (words_parts_and_width[_visData.msgId_].first != messageTextMaxWidth)
            {
                words_parts_and_width.remove(_visData.msgId_);
                need_update = true;
            }

            if (need_update)
            {
                Logic::CutText(text, term, messageTextMaxWidth, m, term_m, cursor, Logic::Text2DocHtmlMode::Pass, false, nullptr, left_part, right_part, term_visible_part);

                std::vector<QString> parts;
                parts.push_back(left_part);
                parts.push_back(right_part);
                parts.push_back(term_visible_part);
                words_parts_and_width.insert(_visData.msgId_, std::make_pair(messageTextMaxWidth, parts));
            }
            else
            {
                auto width_and_parts = words_parts_and_width[_visData.msgId_];
                left_part = width_and_parts.second[0];
                right_part = width_and_parts.second[1];
                term_visible_part = width_and_parts.second[2];
            }

        }
        else
        {
            left_part = m.elidedText(text, Qt::ElideRight, messageTextMaxWidth);
        }

        {
            const auto charFormat = cursor.charFormat();
            auto boldCharFormat = charFormat;
            boldCharFormat.setFont(hasUnreads ? base_font : term_font);
            boldCharFormat.setForeground(QBrush(QColor("#579e1c")));

            Logic::Text4Edit(left_part, *textControl, cursor, false, GetEmojiSize());
            
            cursor.setCharFormat(boldCharFormat);
            Logic::Text4Edit(term_visible_part, *textControl, cursor, false, GetEmojiSize());
            
            cursor.setCharFormat(hasUnreads ? boldCharFormat : charFormat);
            Logic::Text4Edit(right_part, *textControl, cursor, false, GetEmojiSize());
        }

        Logic::FormatDocument(doc, _recentParams.messageHeight());
        textControl->render(&_painter, QPoint(_recentParams.messageX(), _recentParams.messageY()));

        return doc.idealWidth();
    }

    void RenderLastReadAvatar(QPainter &_painter, const QPixmap& _avatar, const int _xOffset, ContactListParams& _recentParams)
    {
        _painter.setOpacity(0.7);
        _painter.drawPixmap(_xOffset, _recentParams.lastReadY(), _recentParams.getLastReadAvatarSize(), _recentParams.getLastReadAvatarSize(), _avatar);
    }

    int RenderNotifications(QPainter &_painter, const int _unreads, bool _muted, const ViewParams& _viewParams, ContactListParams& _recentParams, bool _isUnknownHeader, bool _isActive, bool _isHover)
    {
        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        const auto unreadsSize = Utils::scale_value(20);
        assert(_unreads >= 0);

        QPixmap p;
        if (_isActive)
        {
            p = Utils::parse_image_name(":/resources/mute_100_active.png");
        }
        else if (_unreads == 0)
        {
            if (_isHover)
                p = Utils::parse_image_name(":/resources/mute_100_hover.png");
            else
                p = Utils::parse_image_name(":/resources/mute_100.png");
        }
        else
        {
            if (_isHover)
                p = Utils::parse_image_name(":/resources/mute_unread_100_hover.png");
            else
                p = Utils::parse_image_name(":/resources/mute_unread_100.png");
        }
        Utils::check_pixel_ratio(p);

        if (_muted)
        {
            if (!_viewParams.pictOnly_)
            {
                auto mutedX = (width - _recentParams.itemHorPadding()) - p.width() / Utils::scale_bitmap(1);
                _painter.drawPixmap(
                    mutedX,
                    (_recentParams.itemHeight() - unreadsSize) / 2,
                    p);
                return ((width - _recentParams.itemHorPadding()) - p.width() / Utils::scale_bitmap(1) - _recentParams.itemContentPadding());
            }
            else
            {
                auto x = Utils::scale_value(12);
                auto y = Utils::scale_value(4);
                _painter.drawPixmap(x, y, p);
                return width - _recentParams.itemHorPadding();
            }
        }

        if (_unreads <= 0)
        {
            return width - _recentParams.itemHorPadding();
        }

        static QFontMetrics m(_recentParams.unreadsFont());

        const auto text = Utils::getUnreadsBadgeStr(_unreads);
        const auto unreadsRect = m.tightBoundingRect(text);
        const auto firstChar = text[0];
        const auto lastChar = text[text.size() - 1];
        const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));

        auto balloonWidth = unreadsWidth;
        const auto isLongText = (text.length() > 1);
        if (isLongText)
        {
            balloonWidth += (_recentParams.unreadsPadding() * 2);
        }
        else
        {
            balloonWidth = unreadsSize;
        }

        auto unreadsX = width - 
            ((_viewParams.regim_ != Logic::MembersWidgetRegim::UNKNOWN) ?
            _recentParams.itemHorPadding()
            : _recentParams.itemHorPadding()) - balloonWidth;
        auto unreadsY = _isUnknownHeader ?
            _recentParams.unknownsUnreadsY(_viewParams.pictOnly_)
            : (_recentParams.itemHeight() - unreadsSize) / 2;

        if (_viewParams.pictOnly_)
        {
            unreadsX = _recentParams.itemHorPadding();
            if (!_isUnknownHeader)
                unreadsY = (_recentParams.itemHeight() - _recentParams.avatarSize()) / 2;
        }
        else if (_viewParams.regim_ == Logic::MembersWidgetRegim::UNKNOWN)
        {
            unreadsX -= _recentParams.removeSize().width();
        }

        auto borderColor = Ui::CommonStyle::getFrameColor();
        if (_isHover)
        {
            borderColor = Ui::CommonStyle::getContactListHoveredColor();
        }
        else if (_isActive)
        {
            borderColor = (_viewParams.pictOnly_) ? Ui::CommonStyle::getContactListSelectedColor() : Qt::transparent;
        }
        const auto bgColor = _isActive ? QColor("#ffffff") : QColor("#579e1c");
        const auto textColor = _isActive ? QColor("#579e1c") : QColor("#ffffff");
        Utils::drawUnreads(&_painter, _recentParams.unreadsFont(), &bgColor, &textColor, &borderColor, _unreads, unreadsSize, unreadsX, unreadsY);

        _painter.restore();

        return (unreadsX - _recentParams.itemContentPadding());
    }

    void RenderDeleteAllItem(QPainter &_painter, const QString& _title, bool _isMouseOver, const ViewParams& _viewParams)
    {
        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPen pen;
        pen.setColor(_isMouseOver ? Ui::CommonStyle::getRedLinkColorHovered() : Ui::CommonStyle::getRedLinkColor());
        _painter.setPen(pen);
        const auto font_size = Utils::scale_value(16);
        QFont f = Fonts::appFont(font_size);
        _painter.setFont(f);
        QFontMetrics metrics(f);
        auto titleRect = metrics.boundingRect(_title);
        auto recentParams = GetRecentsParams(_viewParams.regim_);

        recentParams.deleteAllFrame().setHeight(recentParams.unknownsItemHeight());
        recentParams.deleteAllFrame().setX(
            CorrectItemWidth(ItemWidth(false, false, false),_viewParams.fixedWidth_)
            - recentParams.itemHorPadding()
            - titleRect.width()
        );
        recentParams.deleteAllFrame().setY(recentParams.unknownsItemHeight() / 2.);
        recentParams.deleteAllFrame().setWidth(ItemWidth(false, false, false) - recentParams.deleteAllFrame().x());
        Utils::drawText(_painter, QPointF(recentParams.deleteAllFrame().x(), recentParams.deleteAllFrame().y()), Qt::AlignVCenter, _title);

        _painter.restore();
    }

    QRect DeleteAllFrame()
    {
        auto recentParams = GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST);
        return recentParams.deleteAllFrame();
    }
}
