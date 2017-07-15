#include "stdafx.h"
#include "CallPanelMain.h"

#include "PushButton_t.h"
#include "VideoPanelHeader.h"
#include "VoipTools.h"
#include "../core_dispatcher.h"
#include "../controls/CommonStyle.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/utils.h"
#include "../main_window/contact_list/SelectionContactsForGroupChat.h"
#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../main_window/MainPage.h"
#include "../utils/InterConnector.h"

extern const QString vertSoundBg;
extern const QString horSoundBg;

const QString buttonGoChat =
"QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip;"
"border-image: url(:/resources/video_panel/videoctrl_chat_mini_100.png); }"
"QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_chat_mini_100_hover.png); }";

const QString buttonCameraEnable =
"QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip;"
"border-image: url(:/resources/video_panel/videoctrl_camera_mini_100.png); }"
"QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_camera_mini_100_hover.png); }";

const QString buttonCameraDisable =
"QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip;"
"border-image: url(:/resources/video_panel/videoctrl_camera_off_mini_100.png); }"
"QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_camera_off_mini_100_hover.png); }";

const QString buttonStopCall =
"QPushButton { min-height: 40dip; max-height: 40dip; min-width: 40dip; max-width: 40dip;"
"border-image: url(:/resources/contr_endcall_100.png); }"
"QPushButton:hover { border-image: url(:/resources/contr_endcall_100_hover.png); }";

const QString buttonMicEnable =
"QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip;"
"border-image: url(:/resources/video_panel/videoctrl_micro_mini_100.png); }"
"QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_micro_mini_100_hover.png); }";

const QString buttonMicDisable =
"QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip;"
"border-image: url(:/resources/video_panel/videoctrl_micro_off_mini_100.png); }"
"QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_micro_off_mini_100_hover.png); }";

const QString buttonSoundEnable =
"QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip;"
"border-image: url(:/resources/video_panel/videoctrl_volume_mini_100.png); }"
"QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_volume_mini_100_hover.png); }";

const QString buttonSoundDisable =
"QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip;"
"border-image: url(:/resources/video_panel/videoctrl_volume_off_mini_100.png); }"
"QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_volume_off_mini_100_hover.png); }";

const QString loginNameLable =
"QPushButton:!enabled { color: #000000; }";

const QString backToVideoStyleIcon =
"QLabel {image: url(:/resources/video_panel/content_arrow_right_100.png); background-color: transparent;}";

const QString backToVideoStyle =
"QPushButton { width: auto; height: 24dip;"
"background-color: transparent; padding-left: 24dip; padding-right: 14dip;"
"border: none; font-size: 14dip; color: #866f3c; margin-bottom: 5dip; margin-top: 5dip; text-align: left;}"
"QPushButton:hover { width: auto; height: 24dip; background-color: rgba(242, 196, 46, 90%); }"
"QPushButton:hover:pressed { width: auto; height: 24dip; background-color: rgba(242, 196, 46, 90%); }";

const QString buttonAddChat =
"QPushButton { min-width: 40dip; max-width: 40dip; min-height: 40dip; max-height: 40dip;"
"border-image: url(:/resources/video_panel/videoctrl_add_mini_100.png); }"
"QPushButton:hover { border-image: url(:/resources/video_panel/videoctrl_add_mini_100_hover.png); }";


namespace
{
    enum
    {
        kmenu_item_volume = 0,
        kmenu_item_mic = 1,
        kmenu_item_cam = 2
    };
}

#define TOP_PANEL_COLOR                  QColor("#fddc6f")
#define TOP_PANEL_NAME_COLOR             QColor("#866f3c")
#define BOTTOM_PANEL_BETWEEN_BUTTONS_GAP Utils::scale_value(32)
#define TOP_PANEL_SECURE_CALL_OFFSET     Utils::scale_value(8)

#define SECURE_BTN_BORDER_W    Utils::scale_value(24)
#define SECURE_BTN_ICON_W      Utils::scale_value(16)
#define SECURE_BTN_ICON_H      SECURE_BTN_ICON_W
#define SECURE_BTN_TEXT_W      Utils::scale_value(50)
#define SECURE_BTN_ICON2TEXT_W TOP_PANEL_SECURE_CALL_OFFSET

#define COLOR_SECURE_BTN_ACTIVE   QColor("#f5c836")
#define COLOR_SECURE_BTN_INACTIVE Qt::transparent

#define BACK_TO_VIDEO_RIGHT_PADDING   18
#define BACK_TO_VIDEO_ICON_TEXT_SPACE 6
#define BACK_TO_VIDEO_LEFT_PADDING    24
#define BACK_TO_VIDEO_TEXT_SIZE   14
#define BACK_TO_VIDEO_ARROW_SIZE      0.45f
#define BACK_TO_VIDEO_TOP_AND_BOTTOM_PADDING    10
#define BACK_TO_VIDEO_ICON_OFFSET  14

extern std::string getFotmatedTime(unsigned ts);

Ui::SliderEx::SliderEx(QWidget* _parent)
    : QWidget(_parent)
{
    horizontalLayout_ = Utils::emptyHLayout(this);

    sliderIcon_ = new voipTools::BoundBox<PushButton_t>(this);
    sliderIcon_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    
    horizontalLayout_->addWidget(sliderIcon_);
    
    slider_ = new voipTools::BoundBox<QSlider>(this);
    slider_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    slider_->setOrientation(Qt::Horizontal);
    
    horizontalLayout_->addWidget(slider_);
    sliderIcon_->setText(QString(), QString());
    
    QMetaObject::connectSlotsByName(this);

    slider_->setMaximum(100);
    slider_->setMinimum(0);

    connect(slider_, SIGNAL(valueChanged(int)), this, SLOT(onVolumeChanged(int)), Qt::QueuedConnection);
    connect(slider_, SIGNAL(sliderReleased()), this, SLOT(onVolumeReleased()), Qt::QueuedConnection);
    connect(sliderIcon_, SIGNAL(clicked()), this, SLOT(onIconClicked()), Qt::QueuedConnection);
}

Ui::SliderEx::~SliderEx()
{

}

void Ui::SliderEx::setIconSize(const int _w, const int _h)
{
    if (!!sliderIcon_)
    {
        sliderIcon_->setIconSize(_w, _h);
    }
}

void Ui::SliderEx::onVolumeChanged(int _v)
{
    emit onSliderValueChanged(_v);
}

void Ui::SliderEx::onVolumeReleased()
{
    emit onSliderReleased();
}

void Ui::SliderEx::onIconClicked()
{
    emit onIconClick();
}

void Ui::SliderEx::setEnabled(bool _en)
{
    slider_->setEnabled(_en);
}

void Ui::SliderEx::setValue(int _v)
{
    slider_->setValue(_v);
}

void Ui::SliderEx::setPropertyForIcon(const char* _name, bool _val)
{
    sliderIcon_->setProperty(_name, _val);
    sliderIcon_->setStyle(QApplication::style());
}

void Ui::SliderEx::setIconForState(const PushButton_t::eButtonState _state, const std::string& _image)
{
    if (!!sliderIcon_)
    {
        sliderIcon_->setImageForState(_state, _image);
    }
}

void Ui::SliderEx::setPropertyForSlider(const char* _name, bool _val)
{
    slider_->setProperty(_name, _val);
    slider_->setStyle(QApplication::style());
}


Ui::BackToVideoButton::BackToVideoButton (QWidget * parent) : QPushButton(parent) 
{
	icon = new QLabel(this);
	Utils::ApplyStyle(icon, backToVideoStyleIcon);
	icon->resize(Utils::scale_value(QSize(20, 20)));
	icon->setScaledContents(true);
	updateIconPosition();
}

void Ui::BackToVideoButton::adjustSize()
{
    QFont f = font();
    int size = BACK_TO_VIDEO_TEXT_SIZE;
    f.setPixelSize(size);
    QFontMetrics fm(f);
    int textWidth = fm.width(text());
    
    int buttonWidth  = Utils::scale_value(BACK_TO_VIDEO_ARROW_SIZE * iconSize().width() + BACK_TO_VIDEO_RIGHT_PADDING + BACK_TO_VIDEO_ICON_TEXT_SPACE + textWidth + BACK_TO_VIDEO_LEFT_PADDING);
    setFixedWidth(buttonWidth);
	updateIconPosition();
}

void Ui::BackToVideoButton::updateIconPosition()
{
	icon->move(width() - icon->width() - Utils::scale_value(BACK_TO_VIDEO_ICON_OFFSET),
		height() / 2 + icon->height() / 2 - BACK_TO_VIDEO_TOP_AND_BOTTOM_PADDING);
}

Ui::CallPanelMainEx::CallPanelMainEx(QWidget* _parent, const CallPanelMainFormat& _panelFormat)
    : QWidget(_parent)
    , format_(_panelFormat)
    , nameLabel_(NULL)
    , rootWidget_(new QWidget(this))
    , buttonMaximize_(NULL)
    , buttonLocalCamera_(NULL)
    , buttonLocalMic_(NULL)
    , vVolControl_(this, false, true, vertSoundBg, [] (QPushButton& _btn, bool _muted)
{
    if (_muted)
    {
        Utils::ApplyStyle(&_btn, buttonSoundDisable);
    }
    else
    {
        Utils::ApplyStyle(&_btn, buttonSoundEnable);
    }
})
, hVolControl_(this, true, true, horSoundBg, [] (QPushButton& _btn, bool _muted)
{
    if (_muted)
    {
        Utils::ApplyStyle(&_btn, buttonSoundDisable);
    }
    else
    {
        Utils::ApplyStyle(&_btn, buttonSoundEnable);
    }
})
, secureCallEnabled_(false)
, secureCallWnd_(NULL)
{
    setFixedHeight(format_.topPartHeight + format_.bottomPartHeight);
    {
        QVBoxLayout* l = Utils::emptyVLayout();
        setLayout(l);
    }

    { // we need root widget to make transcluent window
        layout()->addWidget(rootWidget_);
    }

    {
        QVBoxLayout* l = Utils::emptyVLayout();
        rootWidget_->setLayout(l);
    }
    
    QString style = QString("QWidget { background: %1; }").arg(Utils::rgbaStringFromColor(TOP_PANEL_COLOR));
    Utils::ApplyStyle(rootWidget_, style);

    if (!platform::is_apple()) 
    { // top part
        QWidget* topPartWidget = new QWidget(rootWidget_);
        topPartWidget->setFixedHeight(format_.topPartHeight);
        {
            QHBoxLayout* l = Utils::emptyHLayout();
            topPartWidget->setLayout(l);
        }

        if (format_.topPartFormat & kVPH_ShowLogo)
        {
            QWidget* logoWidg = new QWidget(topPartWidget);
            logoWidg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            {
                QHBoxLayout* l = Utils::emptyHLayout();
                l->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
                logoWidg->setLayout(l);
            }
            topPartWidget->layout()->addWidget(logoWidg);

            QPushButton* logoBtn = new voipTools::BoundBox<QPushButton>(logoWidg);
            logoBtn->setObjectName("windowIcon");
            logoBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            logoBtn->setAttribute(Qt::WA_TransparentForMouseEvents);
            logoWidg->layout()->addWidget(logoBtn);

            QLabel* title = new voipTools::BoundBox<QLabel>(logoWidg);
            title->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));
            title->setContentsMargins(Utils::scale_value(8), 0, 0, 0);
            title->setText(build::is_icq() ? QT_TRANSLATE_NOOP("title", "ICQ") : QT_TRANSLATE_NOOP("title", "Mail.Ru Agent"));
            title->setAttribute(Qt::WA_TransparentForMouseEvents);
            title->setMouseTracking(true);
            logoWidg->layout()->addWidget(title);
        }

        if (format_.topPartFormat & kVPH_ShowName)
        {
            QWidget* buttonWidg = new QWidget(topPartWidget);
            buttonWidg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            {
                QHBoxLayout* l = Utils::emptyHLayout();
                l->setAlignment(Qt::AlignCenter);
                buttonWidg->setLayout(l);
            }
            topPartWidget->layout()->addWidget(buttonWidg);

            nameLabel_ = new voipTools::BoundBox<PushButton_t>(buttonWidg);
            nameLabel_->setFont(Fonts::appFontScaled(14));
            nameLabel_->setAlignment(Qt::AlignCenter);
            nameLabel_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
            nameLabel_->setPostfix(std::string(" " + getFotmatedTime(0)).c_str());
            nameLabel_->setFixedWidth(nameLabel_->precalculateWidth() + 2 * SECURE_BTN_BORDER_W);
            nameLabel_->setPostfixColor(TOP_PANEL_NAME_COLOR);
            
            // Make disable text color also black.
            Utils::ApplyStyle(nameLabel_, loginNameLable);

            connect(nameLabel_, SIGNAL(clicked()), this, SLOT(onSecureCallClicked()), Qt::QueuedConnection);
            buttonWidg->layout()->addWidget(nameLabel_);
        }

        {
            QWidget* sysWidg = new QWidget(topPartWidget);
            sysWidg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            {
                QHBoxLayout* l = Utils::emptyHLayout();
                l->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
                sysWidg->setLayout(l);
            }
            topPartWidget->layout()->addWidget(sysWidg);

            if (format_.topPartFormat & kVPH_ShowMin)
            {
                addButton<QPushButton>(*sysWidg, Ui::CommonStyle::getMinimizeButtonStyle(), SLOT(_onMinimize()));
            }

            if (format_.topPartFormat & kVPH_ShowMax)
            {
                buttonMaximize_ = addButton<QPushButton>(*sysWidg, Ui::CommonStyle::getMaximizeButtonStyle(), SLOT(_onMaximize()));
            }

            if (format_.topPartFormat & kVPH_ShowClose)
            {
                addButton<QPushButton>(*sysWidg, Ui::CommonStyle::getCloseButtonStyle(), SLOT(_onClose()));
            }
        }

        rootWidget_->layout()->addWidget(topPartWidget);
    }

    { // bottom part
        QWidget* bottomPartWidget = new voipTools::BoundBox<QWidget>(rootWidget_);
        bottomPartWidget->setFixedHeight(format_.bottomPartHeight);
        {
            QHBoxLayout* l = Utils::emptyHLayout();
            bottomPartWidget->setLayout(l);
        }
        bottomPartWidget->layout()->setAlignment(Qt::AlignCenter);

        addButton<QPushButton>(*bottomPartWidget, buttonAddChat, SLOT(addUserToVideoCall()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
        addButton<QPushButton>(*bottomPartWidget, buttonGoChat,        SLOT(onClickGoChat()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
        buttonLocalCamera_ = addButton<QPushButton>(*bottomPartWidget, buttonCameraDisable, SLOT(onCameraTurn()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
        addButton<QPushButton>(*bottomPartWidget, buttonStopCall,      SLOT(onStopCall()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);
        buttonLocalMic_ = addButton<QPushButton>(*bottomPartWidget, buttonMicDisable,    SLOT(onMicTurn()));
        bottomPartWidget->layout()->setSpacing(BOTTOM_PANEL_BETWEEN_BUTTONS_GAP);

        
        // Volume controls group.
        volumeGroup = addVolumeGroup(*bottomPartWidget, false, Utils::scale_value(660));

        if (format_.topPartFormat & kVPH_BackToVideo)
        {
            qobject_cast<QBoxLayout*>(bottomPartWidget->layout())->insertStretch(0, 1);
            backToVideo_ = addButton<BackToVideoButton>(*bottomPartWidget, backToVideoStyle, SIGNAL(onBackToVideo()), false, true);
            backToVideo_->setText(QT_TRANSLATE_NOOP("voip_pages", "Return to call"));
            backToVideo_->adjustSize();
        }

        
        rootWidget_->layout()->addWidget(bottomPartWidget);
    }

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const voip_manager::ContactsList&)), this, SLOT(onVoipCallNameChanged(const voip_manager::ContactsList&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipUpdateCipherState(const voip_manager::CipherState&)), this, SLOT(onVoipUpdateCipherState(const voip_manager::CipherState&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallTimeChanged(unsigned,bool)), this, SLOT(onVoipCallTimeChanged(unsigned,bool)), Qt::DirectConnection);

    QObject::connect(&hVolControl_, SIGNAL(onMuteChanged(bool)), this, SLOT(onMuteChanged(bool)), Qt::QueuedConnection);

    hVolControl_.hide();
    vVolControl_.hide();
}

Ui::CallPanelMainEx::~CallPanelMainEx()
{

}

void Ui::CallPanelMainEx::processOnWindowMaximized()
{
    if (buttonMaximize_)
    {
        Utils::ApplyStyle(buttonMaximize_, Ui::CommonStyle::getRestoreButtonStyle());
    }
}

void Ui::CallPanelMainEx::processOnWindowNormalled()
{
    if (buttonMaximize_)
    {
        Utils::ApplyStyle(buttonMaximize_, Ui::CommonStyle::getMaximizeButtonStyle());
    }
}

void Ui::CallPanelMainEx::onSoundTurn()
{
    assert(false);
}

void Ui::CallPanelMainEx::onMicTurn()
{
    Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
}

void Ui::CallPanelMainEx::_onMinimize()
{
    emit onMinimize();
}

void Ui::CallPanelMainEx::_onMaximize()
{
    emit onMaximize();
}

void Ui::CallPanelMainEx::onStopCall()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::CallPanelMainEx::_onClose()
{
    emit onClose();
}

void Ui::CallPanelMainEx::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
{
    if(_contacts.contacts.empty())
    {
        return;
    }

    activeContact_ = _contacts.contacts[0].contact;
    QString friendlyName = Logic::getContactListModel()->getDisplayName(activeContact_.c_str());
    friendlyName += " -";

    if (nameLabel_)
    {
        nameLabel_->setPrefix(friendlyName);
        nameLabel_->setFixedWidth(nameLabel_->precalculateWidth() + 2*SECURE_BTN_BORDER_W);
    }
}

void Ui::CallPanelMainEx::onClickGoChat()
{
    emit onClickOpenChat(activeContact_);
}

void Ui::CallPanelMainEx::onVoipMediaLocalVideo(bool _enabled)
{
    if (buttonLocalCamera_)
    {
        if (_enabled)
        {
            Utils::ApplyStyle(buttonLocalCamera_, buttonCameraEnable);
        }
        else
        {
            Utils::ApplyStyle(buttonLocalCamera_, buttonCameraDisable);
        }
    }
}

void Ui::CallPanelMainEx::onVoipMediaLocalAudio(bool _enabled)
{
    if (buttonLocalMic_)
    {
        if (_enabled)
        {
            Utils::ApplyStyle(buttonLocalMic_, buttonMicEnable);
        }
        else
        {
            Utils::ApplyStyle(buttonLocalMic_, buttonMicDisable);
        }
    }
}

void Ui::CallPanelMainEx::onCameraTurn()
{
    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

template<typename ButtonType>
ButtonType* Ui::CallPanelMainEx::addButton(
    QWidget& _parentWidget,
    const QString& _propertyName,
    const char* _slot,
    bool _bDefaultCursor,
    bool rightAlignment)
{
    ButtonType* btn = new voipTools::BoundBox<ButtonType>(&_parentWidget);
    Utils::ApplyStyle(btn, _propertyName);
    btn->setSizePolicy(rightAlignment ? QSizePolicy::Expanding : QSizePolicy::Preferred, QSizePolicy::Expanding);

    // For volume control we have blinking with cursor under mac.
    // We will use dfault cursor to fix it.
    if (!_bDefaultCursor)
    {
        btn->setCursor(QCursor(Qt::PointingHandCursor));
    }
    btn->setFlat(true);

    if (!rightAlignment)
    {
        _parentWidget.layout()->addWidget(btn);
    }
    else
    {
        QBoxLayout* layout = qobject_cast<QBoxLayout*>(_parentWidget.layout());
        if (layout)
        {
            qobject_cast<QBoxLayout*>(_parentWidget.layout())->addWidget(btn, 1, Qt::AlignRight);
        }
    }
    connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);

    // Disable dragging using using buttons.
    btn->setAttribute(Qt::WA_NoMousePropagation);

    return btn;
}

Ui::VolumeGroup* Ui::CallPanelMainEx::addVolumeGroup(QWidget& _parentWidget,  bool rightAlignment, int verticalSize)
{
    VolumeGroup* volumeGroup = new VolumeGroup(&_parentWidget, true, [] (QPushButton& _btn, bool _muted)
    {
        if (_muted)
        {
            Utils::ApplyStyle(&_btn, buttonSoundDisable);
        }
        else
        {
            Utils::ApplyStyle(&_btn, buttonSoundEnable);
        }
    }, verticalSize);
    
    volumeGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    
    if (!rightAlignment)
    {
        QBoxLayout* layout = qobject_cast<QBoxLayout*>(_parentWidget.layout());
        if (layout)
        {
            layout->addWidget(volumeGroup, 0, Qt::AlignVCenter);
        }
    }
    else
    {
        QBoxLayout* layout = qobject_cast<QBoxLayout*>(_parentWidget.layout());
        if (layout)
        {
            qobject_cast<QBoxLayout*>(_parentWidget.layout())->addWidget(volumeGroup, 1, Qt::AlignRight);
        }
    }

    // Disable dragging using using buttons.
    volumeGroup->setAttribute(Qt::WA_NoMousePropagation);

    return volumeGroup;
}


void Ui::CallPanelMainEx::hideEvent(QHideEvent* _e)
{
    QWidget::hideEvent(_e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::moveEvent(QMoveEvent* _e)
{
    QWidget::moveEvent(_e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
//    hVolControl_.hide();
//    vVolControl_.hide();
}

void Ui::CallPanelMainEx::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
    hVolControl_.hide();
    vVolControl_.hide();
}

void Ui::CallPanelMainEx::onVoipUpdateCipherState(const voip_manager::CipherState& _state)
{
    const bool enable = voip_manager::CipherState::kCipherStateEnabled == _state.state;
    secureCallEnabled_ = enable;
    if (nameLabel_)
    {
        nameLabel_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setColorForState(PushButton_t::hovered, enable ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setColorForState(PushButton_t::pressed, enable ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);

        nameLabel_->setEnabled(enable);
        nameLabel_->setOffsets(enable ? TOP_PANEL_SECURE_CALL_OFFSET : 0);
        nameLabel_->setImageForState(PushButton_t::normal, enable ? ":/resources/video_panel/content_securecall_100.png" : "");
        nameLabel_->setFixedWidth(nameLabel_->precalculateWidth() + 2*SECURE_BTN_BORDER_W);
        nameLabel_->setCursor(enable ? QCursor(Qt::PointingHandCursor) : QCursor(Qt::ArrowCursor));

        // Return postfix time text to default state.
        if (!enable)
        {
            nameLabel_->setPostfix(std::string(" " + getFotmatedTime(0)).c_str());
        }
    }

    if (secureCallWnd_ && voip_manager::CipherState::kCipherStateEnabled == _state.state)
    {
        secureCallWnd_->setSecureCode(_state.secureCode);
    }
}

void Ui::CallPanelMainEx::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    if (_contactEx.call_count <= 1)
    {
        voip_manager::CipherState state;
        state.state = voip_manager::CipherState::kCipherStateFailed;
        onVoipUpdateCipherState(state);

        if (secureCallWnd_)
        {
            secureCallWnd_->hide();
        }
    }
}

void Ui::CallPanelMainEx::onVoipCallTimeChanged(unsigned _secElapsed, bool /*have_call*/)
{
    if (nameLabel_)
    {
        nameLabel_->setPostfix(secureCallEnabled_ ? getFotmatedTime(_secElapsed).c_str() : std::string(" " + getFotmatedTime(_secElapsed)).c_str());
        nameLabel_->setFixedWidth(nameLabel_->precalculateWidth() + 2*SECURE_BTN_BORDER_W);
    }
}

void Ui::CallPanelMainEx::onSecureCallWndOpened()
{
    assert(nameLabel_);
    if (nameLabel_)
    {
        nameLabel_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_ACTIVE);
        nameLabel_->setColorForState(PushButton_t::hovered, COLOR_SECURE_BTN_ACTIVE);
        nameLabel_->setColorForState(PushButton_t::pressed, COLOR_SECURE_BTN_ACTIVE);
    }
}

void Ui::CallPanelMainEx::onSecureCallWndClosed()
{
    assert(nameLabel_);
    if (nameLabel_)
    {
        nameLabel_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setColorForState(PushButton_t::hovered, secureCallEnabled_ ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setColorForState(PushButton_t::pressed, secureCallEnabled_ ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
        nameLabel_->setCursor(secureCallEnabled_ ? QCursor(Qt::PointingHandCursor) : QCursor(Qt::ArrowCursor));
    }
}

void Ui::CallPanelMainEx::onSecureCallClicked()
{
    assert(nameLabel_);
    if (!nameLabel_)
    {
        return;
    }

    QRect rc = geometry();
    rc.moveTopLeft(mapToGlobal(rc.topLeft()));

    if (!secureCallWnd_)
    {
        secureCallWnd_ = new SecureCallWnd();
        connect(secureCallWnd_, SIGNAL(onSecureCallWndOpened()), this, SLOT(onSecureCallWndOpened()), Qt::QueuedConnection);
        connect(secureCallWnd_, SIGNAL(onSecureCallWndClosed()), this, SLOT(onSecureCallWndClosed()), Qt::QueuedConnection);
    }

    assert(secureCallWnd_);
    if (!secureCallWnd_)
    {
        return;
    }

    const QPoint windowTCPt(rc.center());
    const QPoint secureCallWndTLPt(windowTCPt.x() - secureCallWnd_->width()*0.5f, windowTCPt.y() - Utils::scale_value(12));

    voip_manager::CipherState cipherState;
    Ui::GetDispatcher()->getVoipController().getSecureCode(cipherState);

    if (voip_manager::CipherState::kCipherStateEnabled == cipherState.state)
    {
        secureCallWnd_->setSecureCode(cipherState.secureCode);
        secureCallWnd_->move(secureCallWndTLPt);
        secureCallWnd_->show();
        secureCallWnd_->raise();
        secureCallWnd_->setFocus(Qt::NoFocusReason);
    }
}

void Ui::CallPanelMainEx::addUserToVideoCall()
{
    showAddUserToVideoConverenceDialogMainWindow(this, parentWidget());
}