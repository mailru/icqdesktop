#include "stdafx.h"
#include "VideoPanel.h"

#include "DetachedVideoWnd.h"
#include "VoipTools.h"
#include "../core_dispatcher.h"
#include "../controls/ToolTipEx.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/utils.h"
#include "../controls/GeneralDialog.h"
#include "SelectionContactsForConference.h"
#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ChatMembersModel.h"

#define internal_spacer_w  (Utils::scale_value(24))
#define internal_spacer_w4 (Utils::scale_value(16))
#define internal_spacer_w_small (Utils::scale_value(12))

#define DISPLAY_ADD_CHAT_BUTTON 0

enum { kFitSpacerWidth = 560, kNormalModeMinWidth = 476};

Ui::VideoPanel::VideoPanel(
    QWidget* _parent, QWidget* _container)
#ifdef __APPLE__
    : BaseBottomVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint)
#else
    : BaseBottomVideoPanel(_parent)
#endif
    , container_(_container)
    , rootWidget_(NULL)
    , parent_(_parent)
    , mouseUnderPanel_(false)
        , fullScreenButton_(NULL)
        , addChatButton_(NULL)
        , settingsButton_(NULL)
        , stopCallButton_(NULL)
        , videoButton_(NULL)
        , micButton_(NULL)
        , conferenceModeButton_(nullptr)
        , minimalBandwidthMode_(nullptr)
        , minimalBandwidthTooltip_(nullptr)
        , isTakling(false)
        , isFadedVisible(false)
{
    setStyleSheet(Utils::LoadStyle(":/voip/video_panel.qss"));
    setProperty("VideoPanel", true);
    //setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    //setAttribute(Qt::WindowDoesNotAcceptFocus, true);
    

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setProperty("VideoPanel", true);
    layout()->addWidget(rootWidget_);
    
    QHBoxLayout* layoutTarget = Utils::emptyHLayout();
    layoutTarget->setAlignment(Qt::AlignVCenter);
    rootWidget_->setLayout(layoutTarget);

    QWidget* parentWidget = rootWidget_;
    auto addButton = [this, parentWidget, layoutTarget] (const char* _propertyName, const char* _slot)->QPushButton*
    {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parentWidget);
        if (_propertyName != NULL)
        {
            btn->setProperty(_propertyName, true);
        }
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);
        layoutTarget->addWidget(btn);
#ifdef __APPLE__
        connect(btn, SIGNAL(clicked()), this, SLOT(activateVideoWindow()), Qt::QueuedConnection);
#endif
        connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);
        
        return btn;
    };
    
    auto addVolumeGroup = [this, parentWidget, layoutTarget] ()->VolumeGroup*
    {
        VolumeGroup* volumeGroup = new VolumeGroup(parentWidget, false, [] (QPushButton& _btn, bool _muted)
        {
            _btn.setProperty("CallPanelEnBtn", !_muted);
            _btn.setProperty("CallPanelDisBtn", _muted);
            _btn.setProperty("CallSoundOn", !_muted);
            _btn.setProperty("CallSoundOff", _muted);
            _btn.setStyle(QApplication::style());
        }, Utils::scale_value(670));
        
        volumeGroup->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        layoutTarget->addWidget(volumeGroup, 0, Qt::AlignVCenter);
        
        connect(volumeGroup, &VolumeGroup::isVideoWindowActive, this, &VideoPanel::isVideoWindowActive,
                Qt::DirectConnection);

#ifdef __APPLE__
        connect(volumeGroup, SIGNAL(clicked()), this, SLOT(activateVideoWindow()), Qt::QueuedConnection);
#endif
        return volumeGroup;
    };

    QPushButton* btn = NULL;
    layoutTarget->addSpacing(internal_spacer_w4);
    btn = addButton("CallGoChat", SLOT(onClickGoChat()));
    layoutTarget->addSpacing(internal_spacer_w);

    minimalBandwidthMode_ = addButton("MinimalBandwithMode", SLOT(onMinimalBandwithMode()));
    minimalBandwidthMode_->setProperty("MinimalBandwithMode", false);
    minimalBandwidthMode_->hide(); // We will show it on call accepted.

	leftSpacer_ = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding);
    layoutTarget->addSpacerItem(leftSpacer_);

    addUsers_ = addButton("AddUser", SLOT(onAddUserClicked()));

    layoutTarget->addSpacing(internal_spacer_w);

    micButton_ = addButton("CallEnableMic", SLOT(onAudioOnOffClicked()));
    micButton_->setProperty("CallDisableMic", false);

    layoutTarget->addSpacing(internal_spacer_w);
    videoButton_ = addButton(NULL, SLOT(onVideoOnOffClicked()));

    layoutTarget->addSpacing(internal_spacer_w);
    stopCallButton_ = addButton("StopCallButton", SLOT(onHangUpButtonClicked()));

    if (DISPLAY_ADD_CHAT_BUTTON)
    {
        layoutTarget->addSpacing(internal_spacer_w);
        addChatButton_ = addButton("CallAddChat", SLOT(onClickAddChat()));
    }

    layoutTarget->addSpacing(internal_spacer_w);
    
    volumeGroup = addVolumeGroup();

	rightSpacer_ = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding);
    layoutTarget->addSpacerItem(rightSpacer_);

    conferenceModeButton_ = addButton("ConferenceAllTheSame", SLOT(onChangeConferenceMode()));

    layoutTarget->addSpacing(internal_spacer_w);
    settingsButton_ = addButton("CallSettings", SLOT(onClickSettings()));

    layoutTarget->addSpacing(internal_spacer_w);
    fullScreenButton_ = addButton(NULL, SLOT(_onFullscreenClicked()));

    layoutTarget->addSpacing(internal_spacer_w4);

    minimalBandwidthTooltip_ = new Ui::ToolTipEx(minimalBandwidthMode_);

    resetHangupText();
    
    connect(volumeGroup, SIGNAL(controlActivated(bool)), this, SLOT(controlActivated(bool)), Qt::QueuedConnection);


    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const voip_manager::ContactsList&)), this, SLOT(onVoipCallNameChanged(const voip_manager::ContactsList&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);

    videoPanelEffect_ = new UIEffects(*this);
    minimalBandwidthTooltipEffect_ = new UIEffects(*minimalBandwidthTooltip_);

    hideBandwidthTooltipTimer = new QTimer();
    hideBandwidthTooltipTimer->setInterval(3000);
    connect(hideBandwidthTooltipTimer, SIGNAL(timeout()), this, SLOT(hideBandwidthTooltip()));

    hideButtonList.push_back(minimalBandwidthMode_);
    hideButtonList.push_back(settingsButton_);
    hideButtonList.push_back(conferenceModeButton_);    
}

Ui::VideoPanel::~VideoPanel()
{

}

void Ui::VideoPanel::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        emit onkeyEscPressed();
    }
}

void Ui::VideoPanel::controlActivated(bool _activated)
{
    mouseUnderPanel_ = _activated;
    
    if (_activated)
    {
        emit onMouseEnter();
    }
    else
    {
        emit onMouseLeave();
    }
}

void Ui::VideoPanel::onClickSettings()
{
    if (MainPage* mainPage = MainPage::instance())
    {
        if (MainWindow* wnd = static_cast<MainWindow*>(mainPage->window()))
        {
            wnd->raise();
            wnd->activate();
        }
        mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
    }
}

void Ui::VideoPanel::onClickGoChat()
{
    if (MainPage* mainPage = MainPage::instance())
    {
        if (MainWindow* wnd = static_cast<MainWindow*>(mainPage->window()))
        {
            wnd->raise();
            wnd->activate();
            wnd->hideMenu();
        }
    }

    if (!activeContact_.empty())
    {
        Logic::getContactListModel()->setCurrent(activeContact_[0].contact.c_str(), -1, true, true);
    }
}

void Ui::VideoPanel::onMinimalBandwithMode()
{
    Ui::GetDispatcher()->getVoipController().switchMinimalBandwithMode();

    minimalBandwidthMode_->setProperty("MinimalBandwithMode", !minimalBandwidthMode_->property("MinimalBandwithMode").toBool());
    minimalBandwidthMode_->setStyle(QApplication::style());
}

void Ui::VideoPanel::onVoipMinimalBandwidthChanged (bool _bEnable)
{
    bool youTurnOn = minimalBandwidthMode_->property("MinimalBandwithMode").toBool() == _bEnable;

    minimalBandwidthMode_->setProperty("MinimalBandwithMode", _bEnable);
    minimalBandwidthMode_->setStyle(QApplication::style());

    bool show = false;
    emit showToolTip(show);
    if (show)
    {
        if (_bEnable)
        {
            QString companionFullName;

            emit companionName(companionFullName);

            QString toolTipText = youTurnOn ? QT_TRANSLATE_NOOP("voip_pages", "Data saving enabled") : companionFullName + QT_TRANSLATE_NOOP("voip_pages", " enabled data saving");
            minimalBandwidthTooltip_->setText(toolTipText);
            minimalBandwidthTooltip_->show();
            if (!youTurnOn)
            {
                emit showPanel();
            }

            bool autoHide = false;
            emit autoHideToolTip(autoHide);

            if (autoHide)
            {
                startToolTipHideTimer();
            }
        }
        else
        {
            minimalBandwidthTooltip_->hide();
            hideBandwidthTooltipTimer->stop();
        }
    }
}

void Ui::VideoPanel::setContacts(const std::vector<voip_manager::Contact>& contacts)
{
    activeContact_ = contacts;

    updateConferenceModeButton();
    updateBandwidthButtonState();
}

void Ui::VideoPanel::onClickAddChat()
{
    assert(false);
}

void Ui::VideoPanel::setFullscreenMode(bool _en)
{
    if (!fullScreenButton_)
    {
        return;
    }

    fullScreenButton_->setProperty("CallFSOff", _en);
    fullScreenButton_->setProperty("CallFSOn", !_en);
    fullScreenButton_->setStyle(QApplication::style());
}

void Ui::VideoPanel::_onFullscreenClicked()
{
    emit onFullscreenClicked();
}

void Ui::VideoPanel::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    if (!mouseUnderPanel_)
    {
        emit onMouseEnter();
    }
}

void Ui::VideoPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);

    const bool focusOnVolumePanel = onUnderMouse(*volumeGroup->verticalVolumeWidget());
    if (!focusOnVolumePanel)
    {
        mouseUnderPanel_ = false;
        emit onMouseLeave();
    }
}

void Ui::VideoPanel::hideEvent(QHideEvent* _e)
{
    QWidget::hideEvent(_e);
    volumeGroup->hideSlider();
}

void Ui::VideoPanel::showEvent(QShowEvent * _e)
{
    QWidget::showEvent(_e);
    // Because we disable this control on hangup.
    volumeGroup->setEnabled(true);
}

void Ui::VideoPanel::moveEvent(QMoveEvent* _e)
{
    QWidget::moveEvent(_e);
    //volumeGroup->hideSlider();
    volumeGroup->updateSliderPosition();

    updateToolTipsPosition();
}

void Ui::VideoPanel::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);

    bool bVisibleButton = isNormalPanelMode();

	auto rootLayout = qobject_cast<QBoxLayout*>(rootWidget_->layout());

    for (QWidget* button : hideButtonList)
    {
        if (button)
        {
            if (button != minimalBandwidthMode_)
            {
                button->setVisible(bVisibleButton);
            }
            else
            {
                updateBandwidthButtonState();
            }
        }
    }

	// Change spacers width, if video panel is too small.
	if (rootLayout)
	{
		for (int i = 0; i < rootLayout->count(); i++)
		{
			QLayoutItem * item = rootLayout->itemAt(i);
			bool isCorner = (i == 0 || i == rootLayout->count() - 1);
			if (item)
			{
				QSpacerItem * spacer = item->spacerItem();
				if (spacer && spacer != leftSpacer_ && spacer != rightSpacer_)
				{
					spacer->changeSize(isFitSpacersPanelMode() ? internal_spacer_w_small :
						(isCorner ? internal_spacer_w4 : internal_spacer_w), 1, QSizePolicy::Fixed);
				}
			}
		}

		rootLayout->update();
	}
    
#ifdef __APPLE__
    assert(parent_);
    if (parent_ && !parent_->isFullScreen())
    {
        auto rc = rect();
        QPainterPath path(QPointF(0, 0));
        path.addRoundedRect(rc.x(), rc.y(), rc.width(), rc.height(), Utils::scale_value(5), Utils::scale_value(5));
        
        QRegion region(path.toFillPolygon().toPolygon());
        region = region + QRect(0, 0, rc.width(), Utils::scale_value(5));
        
        setMask(region);
    }
    else
    {
        clearMask();
    }
#endif

    volumeGroup->updateSliderPosition();

    updateToolTipsPosition();

    updateConferenceModeButton();

    updateBandwidthButtonState();
}

void Ui::VideoPanel::resetHangupText()
{
    if (stopCallButton_)
    {
        stopCallButton_->setText("");
        stopCallButton_->repaint();
    }
}


void Ui::VideoPanel::onHangUpButtonClicked()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
    
    // Hide and disable on hangup
    volumeGroup->hideSlider();
    volumeGroup->setEnabled(false);
}

void Ui::VideoPanel::onVoipMediaLocalAudio(bool _enabled)
{
    if (micButton_)
    {
        micButton_->setProperty("CallPanelEnBtn", _enabled);
        micButton_->setProperty("CallPanelDisBtn", !_enabled);
        micButton_->setProperty("CallEnableMic", _enabled);
        micButton_->setProperty("CallDisableMic", !_enabled);
        micButton_->setStyle(QApplication::style());
    }
}

void Ui::VideoPanel::onVoipMediaLocalVideo(bool _enabled)
{
    if (!videoButton_)
    {
        return;
    }
    videoButton_->setProperty("CallPanelEnBtn", _enabled);
    videoButton_->setProperty("CallPanelDisBtn", !_enabled);
    videoButton_->setProperty("CallEnableCam", _enabled);
    videoButton_->setProperty("CallDisableCam", !_enabled);
    videoButton_->setStyle(QApplication::style());
}


void Ui::VideoPanel::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);

    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || (rootWidget_ && rootWidget_->isActiveWindow()))
        {
            if (container_)
            {
                container_->raise();
                raise();
            }
        }
    }
}

void Ui::VideoPanel::onAudioOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
}

void Ui::VideoPanel::onVideoOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

void Ui::VideoPanel::updateToolTipsPosition()
{
    if (minimalBandwidthTooltip_)
    {
        minimalBandwidthTooltip_->updatePosition();
    }
}

void Ui::VideoPanel::fadeIn(unsigned int _kAnimationDefDuration)
{
    isFadedVisible = true;
    videoPanelEffect_->fadeIn(_kAnimationDefDuration);
    minimalBandwidthTooltipEffect_->fadeIn(_kAnimationDefDuration);
}

void Ui::VideoPanel::fadeOut(unsigned int _kAnimationDefDuration)
{
    isFadedVisible = false;
    videoPanelEffect_->fadeOut(_kAnimationDefDuration);
    minimalBandwidthTooltipEffect_->fadeOut(_kAnimationDefDuration);
    minimalBandwidthTooltip_->hide();
    hideBandwidthTooltipTimer->stop();
    
    volumeGroup->hideSlider();
}

bool Ui::VideoPanel::isFadedIn()
{
    return isFadedVisible;
}

void Ui::VideoPanel::hideBandwidthTooltip()
{
    minimalBandwidthTooltip_->hide();
    hideBandwidthTooltipTimer->stop();
}

void Ui::VideoPanel::talkCreated()
{
    conferenceModeButton_->hide();
    minimalBandwidthMode_->hide();
}

void Ui::VideoPanel::talkStarted()
{
    updateBandwidthButtonState();
}

void Ui::VideoPanel::talkFinished()
{
    minimalBandwidthMode_->hide();
}

void Ui::VideoPanel::startToolTipHideTimer()
{
    if (minimalBandwidthTooltip_->isVisible())
    {
        hideBandwidthTooltipTimer->start();
    }
}

bool Ui::VideoPanel::isActiveWindow()
{
    return QWidget::isActiveWindow() || volumeGroup->verticalVolumeWidget()->isActiveWindow();
}

bool Ui::VideoPanel::isNormalPanelMode()
{
    auto rc = rect();

    return (rc.width() >= Utils::scale_value(kNormalModeMinWidth));
}

bool Ui::VideoPanel::isFitSpacersPanelMode()
{
	auto rc = rect();

	return (rc.width() < Utils::scale_value(kFitSpacerWidth));
}

void Ui::VideoPanel::activateVideoWindow()
{
    parentWidget()->activateWindow();
    parentWidget()->raise();
}

void Ui::VideoPanel::updateConferenceModeButton()
{
    conferenceModeButton_->setVisible(activeContact_.size() > 1);
}

void Ui::VideoPanel::updateBandwidthButtonState()
{
    minimalBandwidthMode_->setVisible(Ui::GetDispatcher()->getVoipController().hasEstablishCall() && isNormalPanelMode());
}

void Ui::VideoPanel::onChangeConferenceMode()
{
    bool isAllTheSame = conferenceModeButton_->property("ConferenceAllTheSame").toBool();
    isAllTheSame = !isAllTheSame;

    Utils::ApplyPropertyParameter(conferenceModeButton_, "ConferenceAllTheSame", isAllTheSame);
    Utils::ApplyPropertyParameter(conferenceModeButton_, "ConferenceOneIsBig", !isAllTheSame);

    emit updateConferenceMode(isAllTheSame ? voip_manager::ConferenceAllTheSame : voip_manager::ConferenceOneIsBig);
}

void Ui::VideoPanel::changeConferenceMode(voip_manager::ConferenceLayout layout)
{
    Utils::ApplyPropertyParameter(conferenceModeButton_, "ConferenceAllTheSame", layout == voip_manager::ConferenceAllTheSame);
    Utils::ApplyPropertyParameter(conferenceModeButton_, "ConferenceOneIsBig",   layout == voip_manager::ConferenceOneIsBig);
}

void Ui::VideoPanel::onAddUserClicked()
{
    emit addUserToConference();
}
