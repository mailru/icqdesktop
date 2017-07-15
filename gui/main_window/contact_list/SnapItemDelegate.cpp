#include "stdafx.h"

#include "SnapItemDelegate.h"
#include "ContactList.h"
#include "ContactListModel.h"
#include "../../cache/snaps/SnapStorage.h"
#include "../../cache/emoji/Emoji.h"
#include "../../utils/Text2DocConverter.h"
#include "../../controls/TextEditEx.h"

namespace
{
    Emoji::EmojiSizePx getEmojiSize()
    {
        Emoji::EmojiSizePx emojiSize = Emoji::EmojiSizePx::_32;
        int scale = (int) (Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        switch (scale)
        {
            case 100:
                emojiSize = Emoji::EmojiSizePx::_32;
                break;
            case 125:
                emojiSize = Emoji::EmojiSizePx::_40;
                break;
            case 150:
                emojiSize = Emoji::EmojiSizePx::_48;
                break;
            case 200:
                emojiSize = Emoji::EmojiSizePx::_64;
                break;
            default:
                assert(!"invalid scale");
        }
        
        return emojiSize;
    }
    
    const int SMILE_SIZE = 24;
}

namespace Logic
{
    SnapItemDelegate::SnapItemDelegate(QObject* parent)
        : QItemDelegate(parent)
        , fixedWidth_(-1)
    {
    }

    SnapItemDelegate::~SnapItemDelegate()
    {
    }

    void SnapItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        auto snap = index.data(Qt::DisplayRole).value<Logic::SnapItem>();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(option.rect.topLeft());
        painter->setPen(QPen(Qt::transparent, 0));

        auto params = ContactList::GetContactListParams();
        
        QRect r;
        if (index.column() == 0 || (index.row() == Logic::GetSnapStorage()->getFriendsRow() && index.column() == Logic::GetSnapStorage()->getFriendsSnapsCount() - 1) || (index.row() == Logic::GetSnapStorage()->getFeaturedRow() && index.column() == Logic::GetSnapStorage()->getFeaturedSnapsCount() - 1))
            r = QRect(0, 0, params.snapItemWidth() + getFirstAndLastOffset(), params.snapItemHeight());
        else
            r = QRect(0, 0, params.snapItemWidth(), params.snapItemHeight());
        
        painter->fillRect(r, Qt::white);

        auto padding = params.snapLeftPadding();
        if (index.column() == 0)
            padding += getFirstAndLastOffset();

        painter->drawPixmap(padding, params.snapTopPadding(), params.snapPreviewWidth(), params.snapPreviewHeight(), snap.Snap_);

        const auto fontQss = Fonts::appFontFullQss(Utils::scale_value(12), Fonts::defaultAppFontFamily(), Fonts::FontWeight::Normal);
        const auto styleSheetQss =
            QString("%1; color: %2; background-color: transparent;").arg(fontQss).arg(Utils::rgbaStringFromColor(QColor("#ffffff")));
        static auto textControl = ContactList::CreateTextBrowser("name", styleSheetQss, params.contactNameHeight());
        textControl->setWordWrapMode(QTextOption::NoWrap);
        textControl->setStyleSheet(styleSheetQss);
        textControl->setStyle(QApplication::style());
        auto &doc = *textControl->document();
        doc.clear();

        QString friendly = Logic::getContactListModel()->getDisplayName(snap.AimId_);
        if (snap.AimId_ == friendly && !snap.Friendly_.isEmpty())
            friendly = snap.Friendly_;

        auto maxWidth = params.snapItemWidth() - params.snapLeftPadding() * 2;

        QFontMetrics f(textControl->font());
        friendly = f.elidedText(friendly, Qt::ElideRight, maxWidth);
        textControl->setFixedWidth(maxWidth);

        QTextCursor cursor = textControl->textCursor();

        Logic::Text4Edit(friendly, *textControl, cursor, false, Emoji::EmojiSizePx::_16);

        Logic::FormatDocument(doc, params.contactNameHeight());

        auto textBlockFormat = cursor.blockFormat();
        textBlockFormat.setAlignment(Qt::AlignCenter);
        cursor.mergeBlockFormat(textBlockFormat);
        textControl->setTextCursor(cursor);

        textControl->render(painter, QPoint(padding, params.snapItemHeight() - params.snapNameBottomPadding() - textControl->height()));

        if (snap.HaveNewSnap_)
        {
            auto fire = Emoji::GetEmoji(0x1f525, 0x0, getEmojiSize()).scaled(QSize(Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE)), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int x = params.snapPreviewWidth() - Utils::scale_value(SMILE_SIZE) / 2 - Utils::scale_value(platform::is_apple() ? 2 : 4);
            if (index.column() == 0)
                x += getFirstAndLastOffset();

            painter->drawImage(QRect(x, 0, Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE + (platform::is_apple() ? 0 : 2))), fire, fire.rect());
        }

        painter->restore();
    }

    QSize SnapItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex &index) const
    {
        return getSnapItemSize();
    }

    QSize SnapItemDelegate::getSnapItemSize()
    {
        auto params = ContactList::GetContactListParams();
        return QSize(params.snapItemWidth(), params.snapItemHeight());
    }

    QSize SnapItemDelegate::getSnapPreviewItemSize()
    {
        auto params = ContactList::GetContactListParams();
        return QSize(params.snapPreviewWidth(), params.snapPreviewHeight());
    }

    int SnapItemDelegate::getFirstAndLastOffset()
    {
        return Utils::scale_value(12);
    }

    int SnapItemDelegate::getGradientHeight()
    {
        auto params = ContactList::GetContactListParams();
        return params.snapGradientHeight();
    }
}
