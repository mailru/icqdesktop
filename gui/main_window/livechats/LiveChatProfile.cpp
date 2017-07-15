#include "stdafx.h"
#include "LiveChatProfile.h"

#include "LiveChatMembersControl.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../../core_dispatcher.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/PictureWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../fonts.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"

namespace Ui
{
    const int membersLimit = 20;
    const int maxNameTextSize = 40;
    const int maxAboutTextSize = 200;
    const int horMargins = 48;
    const int textMargin = 4;

    LiveChats::LiveChats(QWidget* _parent)
        :   parent_(_parent),
            seq_(-1),
            connected_(false),
            activeDialog_(nullptr)
    {
        connect(Logic::getContactListModel(), SIGNAL(needJoinLiveChat(QString)), this, SLOT(needJoinLiveChat(QString)), Qt::QueuedConnection);
    }

    LiveChats::~LiveChats()
    {

    }

    void LiveChats::needJoinLiveChat(QString _stamp)
    {
        if (!connected_)
        {
            connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), this, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::UniqueConnection);
            connect(Ui::GetDispatcher(), SIGNAL(chatInfoFailed(qint64, core::group_chat_info_errors)), this, SLOT(chatInfoFailed(qint64, core::group_chat_info_errors)), Qt::UniqueConnection);
            connect(Logic::getContactListModel(), SIGNAL(liveChatJoined(QString)), this, SLOT(liveChatJoined(QString)), Qt::QueuedConnection);
        }

        connected_ = true;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("stamp", _stamp);
        collection.set_value_as_int("limit", Logic::ChatMembersModel::get_limit(membersLimit));

        seq_ = Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());

        if (activeDialog_)
        {
            activeDialog_->reject();
            return;
        }
    }

    void LiveChats::liveChatJoined(QString _aimId)
    {
        if (joinedLiveChat_ == _aimId)
        {
            Logic::getContactListModel()->setCurrent(_aimId, -1, true);

            joinedLiveChat_.clear();
        }
    }

    void LiveChats::chatInfo(qint64 _seq, std::shared_ptr<Data::ChatInfo> _info)
    {
        if (_seq != seq_)
            return;

        joinedLiveChat_.clear();

        if (Logic::getContactListModel()->getContactItem(_info->AimId_) && _info->YouMember_)
        {
            Logic::getContactListModel()->setCurrent(_info->AimId_, -1, true);
            return;
        }

        auto liveChatProfileWidget = new LiveChatProfileWidget(nullptr, _info->Name_);
        liveChatProfileWidget->setFixedSize(Utils::scale_value(400), Utils::scale_value(500));
        liveChatProfileWidget->viewChat(_info);

        GeneralDialog containerDialog(liveChatProfileWidget, parent_);
        activeDialog_ = &containerDialog;

        containerDialog.addHead();
        bool pending = _info->YouPending_;
        auto pendingText = QT_TRANSLATE_NOOP("livechats", "Waiting for approval");
        containerDialog.addAcceptButton(pending ? pendingText : QT_TRANSLATE_NOOP("livechats", "Join"), Utils::scale_value(20), !pending);
        containerDialog.setKeepCenter(true);

        QString stamp = _info->Stamp_;

        QMetaObject::Connection con;
        QMetaObject::Connection joinedCon;
        if (_info->ApprovedJoin_)
        {
            auto button = containerDialog.takeAcceptButton();
            QMetaObject::Connection clickCon = connect(button, &QPushButton::clicked, [button, pendingText, stamp]()
            {
                Logic::getContactListModel()->joinLiveChat(stamp, true);
                Utils::ApplyStyle(button, Ui::CommonStyle::getDisabledButtonStyle());
                button->setText(pendingText);
                button->setEnabled(false);
            });
            QString id = _info->AimId_;
            joinedCon = connect(Logic::getContactListModel(), &Logic::ContactListModel::liveChatJoined, [id, button, this, clickCon] (QString aimId)
            {
                if (id == aimId)
                {
                    Utils::ApplyStyle(button, Ui::CommonStyle::getGreenButtonStyle());
                    button->setText(QT_TRANSLATE_NOOP("livechats", "Open"));
                    button->setEnabled(true);

                    if (clickCon)
                        disconnect(clickCon);

                    connect(button, &QPushButton::clicked, [id, this]()
                    {
                        Logic::getContactListModel()->setCurrent(id, -1, true, true);
                        activeDialog_->reject();
                    });
                }
            });
        }

        if (containerDialog.showInPosition(-1, -1))
        {
            Logic::getContactListModel()->joinLiveChat(_info->Stamp_, true);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_join_frompopup);
            if (!_info->ApprovedJoin_)
                joinedLiveChat_ = _info->AimId_;
        }
        else
        {
            joinedLiveChat_.clear();
        }

        if (con)
            disconnect(con);

        if (joinedCon)
            disconnect(joinedCon);

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_popup_livechat);
        activeDialog_ = nullptr;
    }

    void LiveChats::chatInfoFailed(qint64 _seq, core::group_chat_info_errors _error)
    {
        if (_seq != seq_)
            return;

        joinedLiveChat_.clear();

        QString errorText;

        switch (_error)
        {
        case core::group_chat_info_errors::network_error:
            errorText = QT_TRANSLATE_NOOP("livechats", "Chat information is unavailable now, please try again later");
            break;
        default:
            errorText = QT_TRANSLATE_NOOP("livechats", "Chat does not exist or it is hidden by privacy settings");
            break;
        }

        auto errorWidget = new LiveChatErrorWidget(nullptr, errorText);
        errorWidget->setFixedSize(Utils::scale_value(400), Utils::scale_value(300));
        errorWidget->show();

        GeneralDialog containerDialog(errorWidget, parent_);
        activeDialog_ = &containerDialog;

        containerDialog.addHead();
        containerDialog.addAcceptButton(QT_TRANSLATE_NOOP("livechats", "Close"), Utils::scale_value(20), false);
        containerDialog.setKeepCenter(true);

        if (containerDialog.showInPosition(-1, -1))
        {
        }

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_popup_livechat);
        activeDialog_ = nullptr;
    }

    LiveChatProfileWidget::LiveChatProfileWidget(QWidget* _parent, const QString& _stamp)
        :   QWidget(_parent),
            stamp_(_stamp),
            rootLayout_(nullptr),
            avatar_(nullptr),
            members_(nullptr),
            membersCount_(-1),
            initialiNameHeight_(0)
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/livechats/LiveChats.qss"));

        rootLayout_ = Utils::emptyVLayout();
        rootLayout_->setContentsMargins(Utils::scale_value(horMargins), 0, Utils::scale_value(horMargins), 0);
        rootLayout_->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        requestProfile();

        setLayout(rootLayout_);
    }

    LiveChatProfileWidget::~LiveChatProfileWidget()
    {
    }

    void LiveChatProfileWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void LiveChatProfileWidget::requestProfile()
    {

    }

    void LiveChatProfileWidget::viewChat(std::shared_ptr<Data::ChatInfo> _info)
    {
        int currentTextWidth = width() - Utils::scale_value(horMargins) * 2 - Utils::scale_value(textMargin) * 2;

        int needHeight = 0;

        membersCount_ = _info->MembersCount_;

        QHBoxLayout* avatar_layout = new QHBoxLayout();

        avatar_ = new ContactAvatarWidget(this, _info->AimId_, _info->Name_, Utils::scale_value(180), true);
        avatar_layout->addWidget(avatar_);

        needHeight += Utils::scale_value(180);

        rootLayout_->addLayout(avatar_layout);

        QString nameText = ((_info->Name_.length() > maxNameTextSize) ? (_info->Name_.mid(0, maxNameTextSize) + "...") : _info->Name_);

        name_ = new TextEditEx(this, Fonts::appFontScaled(32), CommonStyle::getTextCommonColor(), false, true);
        connect(name_, SIGNAL(setSize(int, int)), this, SLOT(nameResized(int, int)), Qt::QueuedConnection);
        name_->setPlainText(nameText, false, QTextCharFormat::AlignNormal);
        name_->setAlignment(Qt::AlignHCenter);
        name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        name_->setFrameStyle(QFrame::NoFrame);
        name_->setContentsMargins(0, 0, 0, 0);
        name_->setContextMenuPolicy(Qt::NoContextMenu);
        name_->adjustHeight(currentTextWidth);
        initialiNameHeight_ = name_->height();
        needHeight += initialiNameHeight_;
        rootLayout_->addWidget(name_);

        QString aboutText = ((_info->About_.length() > maxAboutTextSize) ? (_info->About_.mid(0, maxAboutTextSize) + "...") : _info->About_);

        TextEditEx* about = new TextEditEx(this, Fonts::appFontScaled(16), QColor("#767676"), false, false);
        about->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        about->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        about->setFrameStyle(QFrame::NoFrame);
        about->setContextMenuPolicy(Qt::NoContextMenu);

        int height = 0;
        for (int i = 0; i < maxAboutTextSize; ++i)
        {
            about->setPlainText(aboutText, false);
            about->setAlignment(Qt::AlignHCenter);
            height = about->adjustHeight(currentTextWidth);

            if (height < Utils::scale_value(70))
                break;

            int newLength = int(((float) aboutText.length()) * 0.9);
            if (newLength <= 0)
            {
                assert(false);
                break;
            }

            aboutText = aboutText.mid(0, newLength) + "...";
        }

        needHeight += height;
        rootLayout_->addWidget(about);

        QString location_string;

        if (!_info->Location_.isEmpty())
        {
            location_string = _info->Location_;
        }

        if (_info->FriendsCount)
        {
            if (!location_string.isEmpty())
                location_string += QString(" - ");

            location_string += QString::number(_info->FriendsCount) + " " +
                Utils::GetTranslator()->getNumberString(
                _info->FriendsCount,
                QT_TRANSLATE_NOOP3("livechats", "friend", "1"),
                QT_TRANSLATE_NOOP3("livechats", "friends", "2"),
                QT_TRANSLATE_NOOP3("livechats", "friends", "5"),
                QT_TRANSLATE_NOOP3("livechats", "friends", "21")
                );
        }

        if (!location_string.isEmpty())
        {
            TextEditEx* location = new TextEditEx(this, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), false, false);
            location->setPlainText(location_string, false);
            location->setAlignment(Qt::AlignHCenter);
            location->setFrameStyle(QFrame::NoFrame);
            height = location->adjustHeight(currentTextWidth);
            needHeight += height;
            rootLayout_->addWidget(location);
        }

        rootLayout_->addSpacing(Utils::scale_value(16));
        needHeight += Utils::scale_value(16);

        QHBoxLayout* avatarsLayout = new QHBoxLayout();
        members_ = new LiveChatMembersControl(this, _info);
        avatarsLayout->addWidget(members_);
        members_->adjustWidth();
        avatarsLayout->setAlignment(Qt::AlignHCenter);

        avatarsLayout->addSpacing(Utils::scale_value(6));

        int realAvatarsCount = members_->getRealCount();

        QLabel* count = new QLabel(QString("+") + QString::number(membersCount_ - realAvatarsCount));
        count->setObjectName("LiveChatMembersCount");
        avatarsLayout->addWidget(count);

        if (realAvatarsCount >= membersCount_)
            count->hide();

        needHeight += members_->height();

        rootLayout_->addLayout(avatarsLayout);

        setFixedHeight(needHeight);
    }

    void LiveChatProfileWidget::nameResized(int, int)
    {
        auto needHeight = height();
        needHeight -= initialiNameHeight_;
        needHeight += name_->height();
        setFixedHeight(needHeight);
    }


    void LiveChatProfileWidget::setStamp(const QString& _stamp)
    {
        stamp_ = _stamp;
    }



    LiveChatErrorWidget::LiveChatErrorWidget(QWidget* _parent, const QString& _errorText)
        :   QWidget(_parent),
            errorText_(_errorText)
    {

    }

    LiveChatErrorWidget::~LiveChatErrorWidget()
    {
    }

    void LiveChatErrorWidget::show()
    {
        int height = 0;
        int currentWidth = width() - Utils::scale_value(horMargins) * 2;

        setStyleSheet(Utils::LoadStyle(":/main_window/livechats/LiveChats.qss"));

        QVBoxLayout* rootLayout = Utils::emptyVLayout();
        rootLayout->setContentsMargins(Utils::scale_value(horMargins), 0, Utils::scale_value(horMargins), 0);
        rootLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        QHBoxLayout* pictureLayout = Utils::emptyHLayout();
        PictureWidget* picture = new PictureWidget(this, ":/resources/placeholders/content_illustration_emptylivechat_100.png");
        picture->setFixedSize(Utils::scale_value(136), Utils::scale_value(216));
        pictureLayout->addWidget(picture);

        height += picture->height();

        rootLayout->addLayout(pictureLayout);

        rootLayout->addSpacing(Utils::scale_value(26));
        height += Utils::scale_value(26);

        TextEditEx* errorText = new TextEditEx(this, Fonts::appFontScaled(16), QColor("#767676"), false, false);
        errorText->setText(errorText_);
        errorText->setStyleSheet("* { border: none; }");
        errorText->setAlignment(Qt::AlignHCenter);
        height += errorText->adjustHeight(currentWidth);
        rootLayout->addWidget(errorText);

        setLayout(rootLayout);

        setFixedHeight(height);
    }

    void LiveChatErrorWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
