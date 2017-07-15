#include "stdafx.h"
#include "LiveChatItemDelegate.h"

#include "LiveChatMembersControl.h"
#include "../contact_list/Common.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../types/chat.h"
#include "../../utils/utils.h"
#include "../../utils/Text2DocConverter.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace
{
    const int top_margin = 10;
    const int bottom_margin = 8;
    const int right_margin = 24;
    const int avatar_size = 80;
    const int avatar_left_margin = 24;
    const int avatar_right_margin = 8;

#ifdef __APPLE__
    const int spacing = 4;
    const int desc_spacing = 8;
    const int friends_spacing = 9;
    const int k_count_spacing = 9;
    const int avatars_spacing = 6;
#else
    const int spacing = 6;
    const int desc_spacing = 13;
    const int friends_spacing = 15;
    const int k_count_spacing = 12;
    const int avatars_spacing = 12;
#endif //__APPLE__

    const int max_desc_chars = 60;


    QString getChatNameStylesheet(const QString& _color)
    {
        return QString("font: %1 %2px \"%3\"; color: %4; background-color: transparent")
            .arg(Fonts::appFontWeightQss(Fonts::FontWeight::Normal))
            .arg(Utils::scale_value(18))
            .arg(Fonts::defaultAppFontQssName())
            .arg(_color);
    };

    QString getChatDescriptionStylesheet(const QString& _color)
    {
        return QString("font: %1 %2px \"%3\"; color: %4; background-color: transparent")
            .arg(Fonts::appFontWeightQss(Fonts::FontWeight::Normal))
            .arg(Utils::scale_value(14))
            .arg(Fonts::defaultAppFontQssName())
            .arg(_color);
    };

    QString getFriendsStylesheet(const QString& _color)
    {
        return QString("font: %1 %2px \"%3\"; color: %4; background-color: transparent")
            .arg(Fonts::appFontWeightQss(Fonts::FontWeight::Medium))
            .arg(Utils::scale_value(12))
            .arg(Fonts::defaultAppFontQssName())
            .arg(_color);
    };

    QString formatMembersCount(int _members, int _avatarsVisible)
    {
        int resultInt = _members - _avatarsVisible;
        if (resultInt <= 0)
            return QString();

        QString result = "+";
        if (resultInt > 999)
        {
            float resultFloat = (float)resultInt / float(1000);
            result += QString::number(resultFloat, 'f', 1);
            result += "k";
        }
        else
        {
            result += QVariant(resultInt).toString();
        }

        return result;
    }
}

namespace Logic
{
    LiveChatItemDelegate::LiveChatItemDelegate(QWidget* _parent)
        : QItemDelegate(_parent)
        , parent_(_parent)
        , stateBlocked_(false)
    {
    }


    void LiveChatItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex & _index) const
    {
        QVariant data = _index.model()->data(_index);
        if (data.isValid())
        {
            _painter->save();
            _painter->setRenderHint(QPainter::Antialiasing);
            _painter->translate(_option.rect.topLeft());

            QSize size = sizeHint(_option, _index);
            if (_option.state & QStyle::State_MouseOver && !stateBlocked_)
            {
                static QBrush hoverBrush(Ui::CommonStyle::getContactListHoveredColor());
                _painter->fillRect(0, 0, size.width(), size.height(), hoverBrush);
            }

            if (_option.state & QStyle::State_Selected && !stateBlocked_)
            {
                static QBrush selectedBrush(Ui::CommonStyle::getContactListSelectedColor());
                _painter->fillRect(0, 0, size.width(), size.height(), selectedBrush);
            }

            Data::ChatInfo info = data.value<Data::ChatInfo>();
            static Ui::ContactAvatarWidget avatar(0, info.AimId_, info.Name_, Utils::scale_value(avatar_size), false);
            avatar.UpdateParams(info.AimId_, info.Name_);
            avatar.render(_painter, QPoint(Utils::scale_value(avatar_left_margin), Utils::scale_value(top_margin)), QRegion(), QWidget::DrawChildren);

            bool isSelected = (_option.state & QStyle::State_Selected);

            static auto name = ContactList::CreateTextBrowser(
                "chat_name",
                getChatNameStylesheet(Utils::rgbaStringFromColor(Ui::CommonStyle::getTextCommonColor())),
                0
            );

            static auto name_active = ContactList::CreateTextBrowser(
                "chat_name",
                getChatNameStylesheet("#ffffff"),
                0
                );

            auto name_control = isSelected ? name_active.get() : name.get();

            name_active->clear();
            name->clear();

            QTextCursor cursorName = name_control->textCursor();
            Logic::Text2Doc(info.Name_, cursorName, Logic::Text2DocHtmlMode::Pass, false);
            int margin = Utils::scale_value(avatar_size) + Utils::scale_value(avatar_left_margin) + Utils::scale_value(avatar_right_margin);
            int maxWidth = ContactList::ItemWidth(false, false, false) - margin - Utils::scale_value(right_margin);
            name_control->setFixedWidth(maxWidth);
            name_control->render(_painter, QPoint(margin, Utils::scale_value(top_margin - spacing)));

            static auto description = ContactList::CreateTextBrowser("chat_info", getChatDescriptionStylesheet("#767676"), 0);
            static auto description_active = ContactList::CreateTextBrowser("chat_info", getChatDescriptionStylesheet("#ffffff"), 0);

            auto desc_control = isSelected ? description_active.get() : description.get();

            description_active->clear();
            description->clear();

            QTextCursor cursorDescription = desc_control->textCursor();
            QString about = info.About_;
            if (about.length() > max_desc_chars)
                about = about.left(max_desc_chars) + "...";

            int lineCount = 0;
            int eol = about.indexOf(QChar::LineSeparator);
            while (eol != -1 && lineCount <= 2)
            {
                if (++lineCount == 3)
                {
                    about = about.left(eol);
                    break;
                }
            }

            Logic::Text2Doc(about, cursorDescription, Logic::Text2DocHtmlMode::Pass, false);
            desc_control->setFixedWidth(maxWidth);
            desc_control->render(_painter, QPoint(margin, Utils::scale_value(top_margin) + name_control->document()->size().height() - Utils::scale_value(desc_spacing)));

            QString friendsInfoStr;
            if (!info.Location_.isEmpty())
            {
                friendsInfoStr = info.Location_;
            }
            if (info.FriendsCount != 0)
            {
                if (!friendsInfoStr.isEmpty())
                    friendsInfoStr += " - ";

                friendsInfoStr.append(QVariant(info.FriendsCount).toString() + " " +
                    Utils::GetTranslator()->getNumberString(
                    info.FriendsCount,
                    QT_TRANSLATE_NOOP3("livechats", "friend", "1"),
                    QT_TRANSLATE_NOOP3("livechats", "friends", "2"),
                    QT_TRANSLATE_NOOP3("livechats", "friends", "5"),
                    QT_TRANSLATE_NOOP3("livechats", "friends", "21")));
            }

            static auto friends = ContactList::CreateTextBrowser("friends", getFriendsStylesheet("#767676"), 0);
            static auto friends_active = ContactList::CreateTextBrowser("friends", getFriendsStylesheet("#ffffff"), 0);

            auto friends_control = isSelected ? friends_active.get() : friends.get();

            friends->clear();
            friends_active->clear();

            if (!friendsInfoStr.isEmpty())
            {
                QTextCursor cursorFriend = friends_control->textCursor();
                Logic::Text2Doc(friendsInfoStr, cursorFriend, Logic::Text2DocHtmlMode::Pass, false);
                int margin = Utils::scale_value(avatar_size) + Utils::scale_value(avatar_left_margin) + Utils::scale_value(avatar_right_margin);
                int maxWidth = ContactList::ItemWidth(false, false, false) - margin - Utils::scale_value(right_margin);
                friends_control->setFixedWidth(maxWidth);
                friends_control->render(_painter, QPoint(margin, Utils::scale_value(top_margin) + name_control->document()->size().height() + desc_control->document()->size().height() - Utils::scale_value(friends_spacing)));
            }

            Ui::LiveChatMembersControl avatarsCollection_(parent_, std::make_shared<Data::ChatInfo>(info), 4);
            int y = Utils::scale_value(top_margin) + name_control->document()->size().height() + desc_control->document()->size().height();
            if (!friendsInfoStr.isEmpty())
                y += friends_control->document()->size().height();

            y -= Utils::scale_value(avatars_spacing);

            avatarsCollection_.adjustWidth();

            if (_option.state & QStyle::State_Selected)
                avatarsCollection_.setColor(Ui::CommonStyle::getContactListSelectedColor());
            else if (_option.state & QStyle::State_MouseOver)
                avatarsCollection_.setColor(Ui::CommonStyle::getContactListHoveredColor());
            else
                avatarsCollection_.setColor(Ui::CommonStyle::getFrameColor());

            avatarsCollection_.render(_painter, QPoint(margin, y), QRegion(), QWidget::DrawChildren);

            int height = y + avatarsCollection_.height() + Utils::scale_value(bottom_margin);
            if (height_[_index.row()] != height)
            {
                height_[_index.row()] = height;
                emit const_cast<QAbstractItemModel*>(_index.model())->dataChanged(_index, _index);
            }

            y += Utils::scale_value(k_count_spacing);

            static QLabel kCount_;
            kCount_.setText(formatMembersCount(info.MembersCount_, avatarsCollection_.getRealCount()));
            kCount_.setFont(Fonts::appFontScaled(12));
            QPalette p;
            p.setColor(QPalette::Foreground, isSelected ? QColor("#ffffff") : QColor("#767676"));
            kCount_.setPalette(p);
            kCount_.adjustSize();
            kCount_.render(_painter, QPoint(margin + avatarsCollection_.width(), y), QRegion(), QWidget::DrawChildren);

            _painter->restore();
        }
    }

    QSize LiveChatItemDelegate::sizeHint(const QStyleOptionViewItem &/*option*/, const QModelIndex& _index) const
    {
        if (!height_.contains(_index.row()))
            height_[_index.row()] = 200;

        return QSize(ContactList::ItemWidth(false, false, false), height_[_index.row()]);
    }

    void LiveChatItemDelegate::blockState(bool _value)
    {
        stateBlocked_ = _value;
    }
}