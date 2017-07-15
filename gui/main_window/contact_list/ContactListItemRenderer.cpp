#include "stdafx.h"

#include "../../core_dispatcher.h"
#include "../../controls/CommonStyle.h"
#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../controls/TextEditEx.h"

#include "Common.h"
#include "ContactListItemRenderer.h"
#include "ContactList.h"

namespace
{
    using namespace ContactList;

    void RenderRole(QPainter &_painter, const int x, const QString& _role, ContactListParams& _contactList);

    int RenderMore(QPainter &_painter, ContactListParams& _contactList, const ViewParams& _viewParams);

    void RenderStatus(QPainter &_painter, const QString &_status, const int _rightMargin, ContactListParams& _contactList, bool _isSelected);
}

namespace ContactList
{
    void RenderServiceContact(QPainter &_painter, const bool _isHovered, const bool _isActive,
        QString _name, Data::ContactType _type, int _leftMargin, const ViewParams& _viewParams)
    {
        auto contactList = GetContactListParams();
        contactList.setLeftMargin(_leftMargin);

        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setBrush(Qt::NoBrush);

        RenderMouseState(_painter, _isHovered, _isActive, contactList, _viewParams);

        if (_type == Data::EMPTY_IGNORE_LIST)
        {
            _painter.setPen(Ui::CommonStyle::getTextCommonColor());
            _painter.setFont(contactList.emptyIgnoreListFont());
        }

        if (_type == Data::ContactType::SEARCH_IN_ALL_CHATS)
        {
            _painter.setPen(contactList.searchInAllChatsColor());
            _painter.setFont(contactList.searchInAllChatsFont());
            static QFontMetrics m(contactList.searchInAllChatsFont());
            const auto textWidth = m.tightBoundingRect(_name).width();

            auto rightX =
                CorrectItemWidth(ItemWidth(_viewParams) - contactList.itemHorPadding(), _viewParams.fixedWidth_)
                - _viewParams.rightMargin_;

            _painter.drawText(rightX - textWidth, contactList.searchInAllChatsY(), _name);
        }
        else
        {
            _painter.translate(contactList.GetContactNameX(), contactList.searchInAllChatsY());
            _painter.drawText(0, 0, _name);
        }

        _painter.restore();

        contactList.resetParams();
    }

    void RenderContactItem(QPainter &_painter, VisualDataBase _item, ViewParams _viewParams)
    {
        const auto regim = _viewParams.regim_;

        auto contactList = GetContactListParams();
        contactList.setLeftMargin(_viewParams.leftMargin_);
        contactList.setWithCheckBox(IsSelectMembers(regim));

        _painter.save();

        _painter.setBrush(Qt::NoBrush);
        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        RenderMouseState(_painter, _item.IsHovered_, _item.IsSelected_, contactList, _viewParams);

        if (IsSelectMembers(regim))
        {
            RenderCheckbox(_painter, _item, contactList);
        }

        RenderAvatar(_painter, contactList.avatarX() + _viewParams.leftMargin_, _item.Avatar_, contactList);

        auto rightMargin = 0;

        if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS)
        {
            rightMargin = RenderRemove(_painter, false, contactList, _viewParams);
            rightMargin = RenderAddContact(_painter, rightMargin, false, contactList);
            RenderContactName(_painter, _item, contactList.contactNameCenterY(), rightMargin, _viewParams, contactList);
            _painter.restore();
            return;
        }
        else if (_item.IsHovered_)
        {
            if (Logic::is_members_regim(regim))
            {
                rightMargin = RenderRemove(_painter, false, contactList, _viewParams);
            }
            else if (Logic::is_admin_members_regim(regim))
            {
                rightMargin = RenderMore(_painter, contactList, _viewParams);
            }
        }


        if (!_item.IsOnline())
        {
            rightMargin = RenderDate(_painter, _item.LastSeen_, _item, contactList, _viewParams);
        }
        else
        {
            rightMargin = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
            rightMargin -= _viewParams.rightMargin_;
        }

        if (_item.HasStatus())
        {
            if (_item.role_ == "admin" || _item.role_ == "moder" || _item.role_ == "readonly")
            {
                RenderRole(_painter, contactList.GetContactNameX(), _item.role_, contactList);
                _viewParams.leftMargin_ += contactList.role_offset();
            }

            RenderContactName(_painter, _item, contactList.contactNameTopY(), rightMargin, _viewParams, contactList);
            RenderStatus(_painter, _item.GetStatus(), rightMargin, contactList, _item.IsSelected_);
        }
        else
        {
            if (_item.role_ == "admin" || _item.role_ == "moder" || _item.role_ == "readonly")
            {
                RenderRole(_painter, contactList.GetContactNameX(), _item.role_, contactList);
                _viewParams.leftMargin_ += contactList.role_offset();
            }

            RenderContactName(_painter, _item, contactList.contactNameCenterY(), rightMargin, _viewParams, contactList);
        }

        _painter.restore();
        contactList.resetParams();
    }

    void RenderGroupItem(QPainter &_painter, const QString &_groupName, const ViewParams& _viewParams)
    {
        auto contactList = GetContactListParams();
        contactList.setLeftMargin(_viewParams.leftMargin_);
        
        _painter.save();
        _painter.setPen(contactList.groupColor());
        _painter.setBrush(Qt::NoBrush);
        _painter.setFont(contactList.groupFont());

        _painter.translate(contactList.GetContactNameX(), contactList.groupY());
        _painter.drawText(0, 0, _groupName);

        _painter.restore();
        contactList.resetParams();
    }

    void RenderContactsDragOverlay(QPainter &_painter)
    {
        auto contactList = GetContactListParams();
        _painter.save();

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        QColor overlayColor("#ffffff");
        overlayColor.setAlphaF(0.9);
        _painter.fillRect(0, 0, ItemWidth(false, false, false), contactList.itemHeight(), QBrush(overlayColor));
        _painter.setBrush(QBrush(Qt::transparent));
        QPen pen (QColor("#579e1c"), contactList.dragOverlayBorderWidth(), Qt::DashLine, Qt::RoundCap);
        _painter.setPen(pen);
        _painter.drawRoundedRect(
            contactList.dragOverlayPadding(),
            contactList.dragOverlayVerPadding(),
            ItemWidth(false, false, false) - contactList.itemHorPadding() - Utils::scale_value(1),
            contactList.itemHeight() - contactList.dragOverlayVerPadding(),
            contactList.dragOverlayBorderRadius(),
            contactList.dragOverlayBorderRadius()
        );

        QPixmap p(Utils::parse_image_name(":/resources/file_sharing/content_upload_cl_100.png"));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = (ItemWidth(false, false, false) / 2) - (p.width() / 2. / ratio);
        int y = (contactList.itemHeight() / 2) - (p.height() / 2. / ratio);
        _painter.drawPixmap(x, y, p);

        _painter.restore();
    }
}

namespace
{
    void RenderRole(QPainter &_painter, const int _x, const QString& _role, ContactListParams& _contactList)
    {
        QPixmap rolePixmap;
        if (_role == "admin")
            rolePixmap = QPixmap(Utils::parse_image_name(":/resources/user_ultraadmin_100.png"));
        else if (_role == "moder")
            rolePixmap = QPixmap(Utils::parse_image_name(":/resources/user_king_100.png"));
        else if (_role == "readonly")
            rolePixmap = QPixmap(Utils::parse_image_name(":/resources/user_onlyread_100.png"));
        else
            return;
        Utils::check_pixel_ratio(rolePixmap);
        double ratio = Utils::scale_bitmap(1);

        _painter.drawPixmap(_x, (_contactList.itemHeight() - rolePixmap.height() / ratio) / 2, rolePixmap);
    }

    int RenderMore(QPainter &_painter, ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        const auto width = _viewParams.fixedWidth_;
        auto optionsImg = QPixmap(Utils::parse_image_name(":/resources/basic_elements/options_100.png"));
        Utils::check_pixel_ratio(optionsImg);
        _painter.save();

        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);

        double ratio = Utils::scale_bitmap(1);
        _painter.drawPixmap(
            CorrectItemWidth(ItemWidth(false, false), width)
            - _contactList.itemHorPadding() - (optionsImg.width() / ratio),
            _contactList.itemHeight() / 2 - (optionsImg.height() / 2. / ratio),
            optionsImg
        );

        _painter.restore();

        const auto xPos =
            CorrectItemWidth(ItemWidth(_viewParams), width)
            - _contactList.itemHorPadding()
            - optionsImg.width();
        return xPos;
    }

    void RenderStatus(QPainter &_painter, const QString &_status, const int _rightMargin, ContactListParams& _contactList, bool _isSelected)
    {
        assert(!_status.isEmpty());

        static auto textControl = CreateTextBrowser("status", _contactList.getStatusStylesheet(false), _contactList.statusHeight());
        static auto activeTextControl = CreateTextBrowser("status", _contactList.getStatusStylesheet(true), _contactList.statusHeight());

        auto control = _isSelected ? activeTextControl.get() : textControl.get();

        const auto maxWidth = _rightMargin - _contactList.GetStatusX();
        control->setFixedWidth(maxWidth);

        QFontMetrics m(control->font());
        const auto elidedString = m.elidedText(_status, Qt::ElideRight, maxWidth);

        auto &doc = *control->document();
        doc.clear();
        QTextCursor cursor = control->textCursor();
        Logic::Text4Edit(elidedString, *control, cursor, false, Emoji::EmojiSizePx::Auto);
        Logic::FormatDocument(doc, _contactList.statusHeight());

        control->render(&_painter, QPoint(_contactList.GetStatusX(), _contactList.statusY()));
    }
}
