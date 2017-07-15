#include "stdafx.h"
#include "LiveChatsHome.h"

#include "LiveChatProfile.h"
#include "LiveChatsModel.h"
#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../controls/CommonStyle.h"
#include "../../utils/utils.h"

namespace
{
    int spacing = 24;
    int profile_height = 500;
    int profile_width = 400;
}

namespace Ui
{
    LiveChatHomeWidget::LiveChatHomeWidget(QWidget* _parent, const Data::ChatInfo& _info)
        :   QWidget(_parent)
        ,   info_(_info)
        ,   joinButton_(new QPushButton(this))
    {
        auto layout = Utils::emptyVLayout(this);
        profile_ = new LiveChatProfileWidget(_parent, _info.Stamp_);
        profile_->setFixedSize(Utils::scale_value(profile_width), Utils::scale_value(profile_height));
        profile_->viewChat(std::make_shared<Data::ChatInfo>(_info));
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(spacing), QSizePolicy::Preferred, QSizePolicy::Fixed));
        layout->addWidget(profile_);
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(spacing), QSizePolicy::Preferred, QSizePolicy::Fixed));
        Utils::ApplyStyle(joinButton_, CommonStyle::getGreenButtonStyle());
        initButtonText();
        auto hLayout = new QHBoxLayout(this);
        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        hLayout->addWidget(joinButton_);
        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        layout->addLayout(hLayout);
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(spacing), QSizePolicy::Preferred, QSizePolicy::Fixed));
        setStyleSheet("background: #ffffff; border-radius: 8px;");
        setFixedWidth(Utils::scale_value(profile_width));
        setFixedHeight(profile_->height() + Utils::scale_value(spacing) * 3 + joinButton_->height());
        Utils::addShadowToWidget(this);

        connect(joinButton_, SIGNAL(clicked()), this, SLOT(joinButtonClicked()), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(liveChatJoined(QString)), this, SLOT(chatJoined(QString)), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(liveChatRemoved(QString)), this, SLOT(chatRemoved(QString)), Qt::QueuedConnection);
    }

    LiveChatHomeWidget::~LiveChatHomeWidget()
    {
        delete profile_;
    }

    void LiveChatHomeWidget::chatJoined(QString _aimId)
    {
        if (info_.AimId_ == _aimId)
            initButtonText();
    }

    void LiveChatHomeWidget::chatRemoved(QString _aimId)
    {
        if (info_.AimId_ == _aimId)
            initButtonText();
    }

    void LiveChatHomeWidget::joinButtonClicked()
    {
        if (Logic::getContactListModel()->getContactItem(info_.AimId_) && info_.YouMember_)
        {
            Logic::getContactListModel()->setCurrent(info_.AimId_, -1, true, true);
        }
        else
        {
            Logic::getContactListModel()->joinLiveChat(info_.Stamp_, true);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_join_fromprofile);
            Logic::GetLiveChatsModel()->joined(info_.AimId_);
            info_.YouMember_ = true;
            if (info_.ApprovedJoin_)
            {
                joinButton_->setText(QT_TRANSLATE_NOOP("livechats", "Waiting for approval"));
                Utils::ApplyStyle(joinButton_, CommonStyle::getDisabledButtonStyle());
                Logic::GetLiveChatsModel()->pending(info_.AimId_);
            }
        }
    }

    void LiveChatHomeWidget::initButtonText()
    {
        if (Logic::getContactListModel()->getContactItem(info_.AimId_))
        {
            joinButton_->setText(QT_TRANSLATE_NOOP("livechats", "Open"));
            Utils::ApplyStyle(joinButton_, CommonStyle::getGreenButtonStyle());
        }
        else
        {
            if (info_.YouPending_)
            {
                joinButton_->setText(QT_TRANSLATE_NOOP("livechats", "Waiting for approval"));
            }
            else
            {
                joinButton_->setText(QT_TRANSLATE_NOOP("livechats", "Join"));
            }
            Utils::ApplyStyle(joinButton_, info_.YouPending_ ? CommonStyle::getDisabledButtonStyle() : CommonStyle::getGreenButtonStyle());
        }

        joinButton_->adjustSize();
    }

    void LiveChatHomeWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    LiveChatHome::LiveChatHome(QWidget* _parent)
        :   QWidget(_parent)
        ,   layout_(new QVBoxLayout(this))
    {
        layout_->setSpacing(0);
        layout_->setContentsMargins(0, 0, 0, 0);
        layout_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        setStyleSheet("background: transparent;");

        connect(Logic::GetLiveChatsModel(), SIGNAL(selected(Data::ChatInfo)), this, SLOT(liveChatSelected(Data::ChatInfo)), Qt::QueuedConnection);
    }


    LiveChatHome::~LiveChatHome()
    {
    }

    void LiveChatHome::liveChatSelected(Data::ChatInfo _info)
    {
        auto item = layout_->takeAt(0);
        if (item)
        {
            item->widget()->hide();
            item->widget()->deleteLater();
        }
        
        layout_->addWidget(new LiveChatHomeWidget(this, _info));
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechat_profile_open);
    }

    void LiveChatHome::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
