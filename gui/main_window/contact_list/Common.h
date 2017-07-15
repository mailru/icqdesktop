#pragma once

#include "../../namespaces.h"
#include "../../controls/CommonStyle.h"
#include "../../fonts.h"
#include "../../utils/utils.h"

FONTS_NS_BEGIN

enum class FontFamily;
enum class FontWeight;

FONTS_NS_END

namespace Ui
{
    class TextEditEx;
}

namespace ContactList
{
	typedef std::unique_ptr<Ui::TextEditEx> TextEditExUptr;

	struct VisualDataBase
	{
		VisualDataBase(
			const QString& _aimId,
			const QPixmap& _avatar,
			const QString& _state,
			const QString& _status,
			const bool _isHovered,
			const bool _isSelected,
			const QString& _contactName,
			const bool _hasLastSeen,
			const QDateTime& _lastSeen,
			const bool _isWithCheckBox,
            bool _isChatMember,
            bool _official,
            const bool _drawLastRead,
            const QPixmap& _lastReadAvatar,
            const QString& role,
            int _UnreadsCounter,
            const QString _term);

		const QString AimId_;

		const QPixmap &Avatar_;

		const QString State_;

		const QString &ContactName_;

		const bool IsHovered_;

		const bool IsSelected_;

		const bool HasLastSeen_;

		const QDateTime LastSeen_;

		bool IsOnline() const { return HasLastSeen_ && !LastSeen_.isValid(); }

		bool HasLastSeen() const { return HasLastSeen_; }

		const bool isCheckedBox_;

        bool isChatMember_;

        bool isOfficial_;

        const bool drawLastRead_;

        const QPixmap& lastReadAvatar_;

        const QString& GetStatus() const;

        bool HasStatus() const;

        void SetStatus(const QString& _status);

        QString role_;

        const int unreadsCounter_;

        const QString searchTerm_;

    private:
        QString Status_;
	};

	QString FormatTime(const QDateTime &_time);

	TextEditExUptr CreateTextBrowser(const QString& _name, const QString& _stylesheet, const int _textHeight);

    QLineEdit *CreateTextBrowser2(const QString& _name, const QString& _stylesheet, const int _textHeight);
    
	int ItemWidth(bool _fromAlert, bool _isWithCheckBox, bool _isPictureOnlyView = false);

    struct ViewParams;
    int ItemWidth(const ViewParams& _viewParams);

    int CorrectItemWidth(int _itemWidth, int _fixedWidth);

	int ItemLength(bool _isWidth, double _koeff, int _addWidth);
    int ItemLength(bool _isWidth, double _koeff, int _addWidth, QWidget* parent);

    bool IsPictureOnlyView();

    struct ViewParams
    {
        ViewParams(int _regim, int _fixedWidth, int _leftMargin, int _rightMargin)
            : regim_(_regim)
            , fixedWidth_(_fixedWidth)
            , leftMargin_(_leftMargin)
            , rightMargin_(_rightMargin)
            , pictOnly_(false)
        {}

        ViewParams()
            : regim_(-1)
            , fixedWidth_(-1)
            , leftMargin_(0)
            , rightMargin_(0)
            , pictOnly_(false)
        {}

        int regim_;
        int fixedWidth_;
        int leftMargin_;
        int rightMargin_;
        bool pictOnly_;
    };

    class ContactListParams
    {
    public:

        //Common
        const int itemHeight() const { return isCl_ ? Utils::scale_value(44) : Utils::scale_value(64); }
        const int itemWidth() const { return Utils::scale_value(320); }
        const int itemHorPadding() const { return  Utils::scale_value(16); }
        const int itemContentPadding() const { return Utils::scale_value(16); }
        const int getItemMiddleY()
        {
            return avatarSize() / 2 + avatarY();
        };
        const int serviceItemHeight() { return  Utils::scale_value(24); }
        const int serviceItemIconPadding() {return Utils::scale_value(12); }

        //Contact avatar
        int avatarX() const
        {
            if (!isCl_) return itemHorPadding();
            return withCheckBox_ ? itemHorPadding() + checkboxSize() + Utils::scale_value(12) : itemHorPadding();
        }
        const int avatarY() const
        {
            return (itemHeight() - avatarSize()) / 2;
        }
        const int avatarSize() const
        {
            if (!isCl_) return Utils::scale_value(48);
            return Utils::scale_value(32);
        }

        //Contact name
        int GetContactNameX() const
        {
            if (!isCl_) return  avatarX() + avatarSize() + itemContentPadding();
            return (avatarX() + avatarSize() + Utils::scale_value(12) + leftMargin_);
        }
        const int nameY() { return  Utils::scale_value(10); }
        const int nameYForMailStatus() { return  Utils::scale_value(20); }
        const int contactNameFontSize() { return Utils::scale_value(16); }
        const int contactNameHeight() { return  Utils::scale_value(22); }
        const int contactNameCenterY() { return  Utils::scale_value(10); }
        const int contactNameTopY() { return  Utils::scale_value(2); }
        QString getContactNameStylesheet(const QString& _fontColor, const Fonts::FontWeight _fontWeight)
        {
            assert(_fontWeight > Fonts::FontWeight::Min);
            assert(_fontWeight < Fonts::FontWeight::Max);

            const auto fontQss = Fonts::appFontFullQss(contactNameFontSize(), Fonts::defaultAppFontFamily(), _fontWeight);
            const auto result =
                QString("%1; color: %2; background-color: transparent;").arg(fontQss).arg(_fontColor);

            return result;
        };

        const QColor getNameFontColor(bool _isSelected, bool _isMemberChecked) const
        {
            return _isSelected ? QColor("#ffffff") : _isMemberChecked ? QColor("#579e1c") : QColor("#000000");
        }

        //Message
        const int messageFontSize() { return  Utils::scale_value(14); }
        const int messageHeight() { return  Utils::scale_value(24); }
        const int messageX() { return  GetContactNameX(); }
        const int messageY() const { return  Utils::scale_value(30); }
        const QString getRecentsMessageFontColor(const bool _isUnread)
        {
            const auto color = Utils::rgbaStringFromColor(_isUnread ? "#000000" : "#767676");

            return color;
        };

        const QString getMessageStylesheet(const bool _isUnread, const bool _isSelected)
        {
            const auto fontWeight = Fonts::FontWeight::Normal;
            const auto fontQss = Fonts::appFontFullQss(messageFontSize(), Fonts::defaultAppFontFamily(), fontWeight);
            const auto fontColor = _isSelected ? "#ffffff" : getRecentsMessageFontColor(_isUnread);
            const auto result =
                QString("%1; color: %2; background-color: transparent").arg(fontQss).arg(fontColor);

            return result;
        };

        //Unknown contacts page
        const int unknownsUnreadsY(bool _pictureOnly)
        {
            return _pictureOnly ? Utils::scale_value(4) : Utils::scale_value(14);
        }
        const int unknownsItemHeight() { return  Utils::scale_value(40); }

        //Contact status
        int GetStatusX() { return GetContactNameX(); }
        const int statusY() { return  Utils::scale_value(21); }
        const int statusHeight() { return  Utils::scale_value(20); }
        const QString getStatusStylesheet(bool _isSelected)
        {
            return QString("font: %1 %2px \"%3\"; color: %4; background-color: transparent")
                .arg(Fonts::defaultAppFontQssWeight())
                .arg(Utils::scale_value(13))
                .arg(Fonts::defaultAppFontQssName())
                .arg(_isSelected ? "#ffffff" : "#767676");
        };

        //Time
        const int timeY() const
        {
            return !isCl_ ? avatarY() + Utils::scale_value(24) : Utils::scale_value(27);
        }
        const QFont timeFont() const { return Fonts::appFontScaled(12); }
        const QColor timeFontColor(bool _isSelected) const
        {
            return _isSelected ? QColor("#ffffff") : QColor("#767676");
        }

        //Additional options
        const int checkboxSize() const { return Utils::scale_value(20); }
        const QSize removeSize() const { return QSize(Utils::scale_value(28), Utils::scale_value(24)); }
        const int role_offset() const { return  Utils::scale_value(24); }
        const int role_ver_offset() const { return Utils::scale_value(4); }

        //Groups in Contact list
        const int groupY() { return Utils::scale_value(17); }
        const QFont groupFont() { return Fonts::appFontScaled(12); }
        const QColor groupColor() { return QColor("#579e1c"); }

        //Unreads counter
        const QFont unreadsFont() { return Fonts::appFontScaled(13, Fonts::FontWeight::Medium); }
        const int unreadsPadding() { return  Utils::scale_value(8); }

        //Last seen
        int lastReadY() const { return  Utils::scale_value(38); }
        int getLastReadAvatarSize() const { return Utils::scale_value(12); }
        const int getlastReadLeftMargin() const { return Utils::scale_value(4); }
        const int getlastReadRightMargin() const { return Utils::scale_value(4); }


        const QFont emptyIgnoreListFont() { return Fonts::appFontScaled(16); }

        //Search in the dialog
        const QColor searchInAllChatsColor() { return QColor("#579e1c"); }
        const QFont searchInAllChatsFont() { return Fonts::appFontScaled(14, Fonts::FontWeight::Medium); }
        const int searchInAllChatsHeight() { return   Utils::scale_value(48); }
        const int searchInAllChatsY() { return Utils::scale_value(28); }

        const int dragOverlayPadding() { return Utils::scale_value(8); }
        const int dragOverlayBorderWidth() { return Utils::scale_value(2); }
        const int dragOverlayBorderRadius() { return Utils::scale_value(8); }
        const int dragOverlayVerPadding() { return  Utils::scale_value(1); }

        const int official_hor_padding() { return  Utils::scale_value(6); }

        ContactListParams(bool _isCl)
            : isCl_(_isCl)
            , withCheckBox_(false)
            , leftMargin_(0)
        {}

        bool getWithCheckBox() const
        {
            return withCheckBox_;
        }

        void setWithCheckBox(bool _withCheckBox)
        {
            withCheckBox_ = _withCheckBox;
        }

        void setLeftMargin(int _leftMargin)
        {
            leftMargin_ = _leftMargin;
        }

        int getLeftMargin() const
        {
            return leftMargin_;
        }

        bool isCL() const
        {
            return isCl_;
        }

        void setIsCL(bool _isCl)
        {
            isCl_ = _isCl;
        }

        void resetParams()
        {
            leftMargin_ = 0;
            withCheckBox_ = false;
        }

        const int favoritesStatusPadding() { return  Utils::scale_value(4); }
        
        QRect& addContactFrame()
        {
            static QRect addContactFrameRect(0, 0, 0, 0);
            return addContactFrameRect;
        }

        QRect& removeContactFrame()
        {
            static QRect removeContactFrameRect(0, 0, 0, 0);
            return removeContactFrameRect;
        }

        QRect& deleteAllFrame()
        {
            static QRect deleteAllFrameRect(0, 0, 0, 0);
            return deleteAllFrameRect;
        }

        //snaps
        const int snapItemHeight() const { return Utils::scale_value(116); }
        const int snapItemWidth() const { return Utils::scale_value(68); }
        const int snapGradientHeight() const { return Utils::scale_value(40); }
        const int snapPreviewHeight() const { return Utils::scale_value(100); }
        const int snapPreviewWidth() const { return Utils::scale_value(60); }
        const int snapLeftPadding() const { return Utils::scale_value(4); }
        const int snapTopPadding() const { return Utils::scale_value(4); }
        const int snapNameBottomPadding() const { return Utils::scale_value(12); }

    private:
        bool isCl_;
        bool withCheckBox_;
        int leftMargin_;
    };

    ContactListParams& GetContactListParams();

    ContactListParams& GetRecentsParams(int _regim);

    void RenderAvatar(QPainter &_painter, int _x, const QPixmap& _avatar, ContactList::ContactListParams& _contactList);

    void RenderMouseState(QPainter &_painter, const bool isHovered, const bool _isSelected, const ContactList::ContactListParams& _contactList, const ContactList::ViewParams& _viewParams);

    void RenderContactName(QPainter &_painter, const ContactList::VisualDataBase &_visData, const int _y, const int _rightMargin, ContactList::ViewParams _viewParams, ContactList::ContactListParams& _contactList);

    int RenderDate(QPainter &_painter, const QDateTime &_date, const VisualDataBase &_item, ContactListParams& _contactList, const ViewParams& _viewParams);

    int RenderAddContact(QPainter &_painter, int& _rightMargin, bool _isSelected, ContactListParams& _recentParams);
    
    void RenderCheckbox(QPainter &_painter, const VisualDataBase &_visData, ContactListParams& _contactList);

    int RenderRemove(QPainter &_painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams);

    int GetXOfRemoveImg(int width);

    bool IsSelectMembers(int regim);
}

namespace Logic
{
    class AbstractItemDelegateWithRegim : public QItemDelegate
    {
    public:
        AbstractItemDelegateWithRegim(QObject* parent)
            : QItemDelegate(parent)
        {}
        virtual void setRegim(int _regim) = 0;

        virtual void setFixedWidth(int width) = 0;
    };
}
