#include "stdafx.h"
#include "LoginPage.h"

#include "../core_dispatcher.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../controls/BackButton.h"
#include "../controls/ConnectionSettingsWidget.h"
#include "../controls/CountrySearchCombobox.h"
#include "../controls/CommonStyle.h"
#include "../controls/LineEditEx.h"
#include "../controls/TextEditEx.h"
#include "../controls/PictureWidget.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"

namespace
{
    enum LoginPagesIndex
    {
        SUBPAGE_PHONE_LOGIN_INDEX = 0,
        SUBPAGE_PHONE_CONF_INDEX = 1,
        SUBPAGE_UIN_LOGIN_INDEX = 2,
    };

    qint64 phoneInfoLastRequestSpecialId_ = 0; // using for settings only (last entered uin/phone)
    qint64 phoneInfoLastRequestId_ = 0;
    int phoneInfoRequestsCount_ = 0;

    QString getEditPhoneStyle()
    {
        return QString(
            "QPushButton { background-color: #ffffff; color: %1; margin-left: 16dip; "
            "min-height: 40dip; max-height: 40dip; min-width: 100dip;border-style: none; } "
            "QPushButton:hover { color: %2; } QPushButton:pressed { color: %3; } ")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColor()))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorHovered()))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorPressed()));
    }

    QString getResendButtonStyle()
    {
        return QString(
            "QPushButton:disabled { background-color: #ffffff; min-height: 30dip; max-height: 30dip; color: #767676; border-style: none; } "
            "QPushButton:enabled { background-color: #ffffff; min-height: 30dip; max-height: 30dip; color: %1; border-style: none; } "
            "QPushButton:hover { color: %2; } QPushButton:pressed { color: %3; } ")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColor()))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorHovered()))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorPressed()));
    }

    QString getSwitchLoginStyle()
    {
        return QString(
            "QPushButton { min-width: 340dip; max-width: 340dip; min-height: 30dip; max-height: 30dip; "
            "background-color: #ffffff;  margin-bottom: 12dip; color: %1; border-style: none; } "
            "QPushButton:hover { color: %2; }  QPushButton:pressed { color: %3; } ")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColor()))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorHovered()))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorPressed()));
    }

    QString getCountryComboboxStyle()
    {
        return QString(
            "QLineEdit { background-position: right; margin-right: 12dip; background-repeat: no-repeat; font-size: 18dip; } "
            "QLineEdit:focus { padding-left: 32dip; margin-right: 0dip; background-repeat: no-repeat; } "
        );
    }

    QString getPasswordForgottenLabelStyle()
    {
        return QString(
            "QPushButton { background-color: transparent; color: %1; margin: 0; padding: 0; border-style: none; } "
            "QPushButton:hover { color: %2; } QPushButton:pressed { color: %3; } ")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColor()))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorHovered()))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorPressed()));
    }
}

namespace Ui
{
    LoginPage::LoginPage(QWidget* _parent, bool _isLogin)
        : QWidget(_parent)
        , countryCode_(new LineEditEx(this))
        , phone_(new LineEditEx(this))
        , combobox_(new CountrySearchCombobox(this))
        , remainingSeconds_(0)
        , timer_(new QTimer(this))
        , isLogin_(_isLogin)
        , codeLength_(4)
        , sendSeq_(0)
        , phoneChangedAuto_(false)
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/login_page.qss"));
        QVBoxLayout* verticalLayout = Utils::emptyVLayout(this);

        auto backButtonWidget = new QWidget(this);
        auto backButtonLayout = Utils::emptyHLayout(backButtonWidget);
        Utils::ApplyStyle(backButtonWidget, "background-color: transparent;");
        backButtonWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        backButtonLayout->setContentsMargins(Utils::scale_value(14), Utils::scale_value(14), Utils::scale_value(14), Utils::scale_value(14));

        {
            backButton = new BackButton(backButtonWidget);
            backButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            backButton->setFlat(true);
            backButton->setFocusPolicy(Qt::NoFocus);
            backButton->setCursor(Qt::PointingHandCursor);
            backButtonLayout->addWidget(backButton);
        }

        {
            auto buttonsSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            backButtonLayout->addItem(buttonsSpacer);
        }

        {
            proxySettingsButton = new QPushButton(backButtonWidget);
            proxySettingsButton->setObjectName("settingsButton");
            proxySettingsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            proxySettingsButton->setFlat(true);
            proxySettingsButton->setFocusPolicy(Qt::NoFocus);
            proxySettingsButton->setCursor(Qt::PointingHandCursor);
            backButtonLayout->addWidget(proxySettingsButton);
        }

        verticalLayout->addWidget(backButtonWidget);

        if (isLogin_)
        {
            QSpacerItem* verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            verticalLayout->addItem(verticalSpacer);
        }

        QWidget* mainWidget = new QWidget(this);
        mainWidget->setObjectName("mainWidget");
        mainWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        QHBoxLayout* mainLayout = Utils::emptyHLayout(mainWidget);

        if (isLogin_)
        {
            QSpacerItem* horizontalSpacer_6 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            mainLayout->addItem(horizontalSpacer_6);
        }

        QWidget* controlsWidget = new QWidget(mainWidget);
        controlsWidget->setObjectName("controlsWidget");
        controlsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        QVBoxLayout* controlsLayout = Utils::emptyVLayout(controlsWidget);

        PictureWidget* logoWidget = new PictureWidget(controlsWidget, 
            build::is_icq() ?
                ":/resources/main_window/content_logo_100.png" :
                ":/resources/main_window/content_logo_agent_100.png");

        logoWidget->setFixedSize(Utils::scale_value(80), Utils::scale_value(80));
        controlsLayout->addWidget(logoWidget);
        controlsLayout->setAlignment(logoWidget, Qt::AlignHCenter);
        logoWidget->setVisible(isLogin_);

        hintLabel = new QLabel(controlsWidget);
        hintLabel->setObjectName("hint");

        passwordForgottenLabel = new QPushButton(controlsWidget);
        passwordForgottenLabel->setObjectName("passwordForgotten");
        passwordForgottenLabel->setFlat(true);
        passwordForgottenLabel->setCursor(Qt::PointingHandCursor);
        Utils::ApplyStyle(passwordForgottenLabel, getPasswordForgottenLabelStyle());
        connect(passwordForgottenLabel, &QPushButton::clicked, this, []()
        {
            QDesktopServices::openUrl(QUrl(build::is_icq()
                ? "https://icq.com/password/" 
                : "https://e.mail.ru/cgi-bin/passremind"));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::login_forgot_password);
        });

        if (isLogin_)
        {
            hintLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        }

        controlsLayout->addWidget(hintLabel);

        QWidget* centerWidget = new QWidget(controlsWidget);
        centerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QHBoxLayout* horizontalLayout = Utils::emptyHLayout(centerWidget);

        if (isLogin_)
        {
            QSpacerItem* horizontalSpacer_9 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            horizontalLayout->addItem(horizontalSpacer_9);
        }

        loginStakedWidget = new QStackedWidget(centerWidget);
        loginStakedWidget->setObjectName("stackedWidget");
        loginStakedWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QWidget* phoneLoginWidget = new QWidget();
        phoneLoginWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout* phoneLoginLayout = Utils::emptyVLayout(phoneLoginWidget);

        countrySearchWidget = new QWidget(phoneLoginWidget);
        countrySearchWidget->setObjectName("countryWidget");
        QVBoxLayout* countrySearchLayout = Utils::emptyVLayout(countrySearchWidget);

        phoneLoginLayout->addWidget(countrySearchWidget);

        phoneWidget = new QFrame(phoneLoginWidget);
        phoneWidget->setObjectName("phoneWidget");
        phoneWidget->setFocusPolicy(Qt::ClickFocus);
        phoneWidget->setFrameShape(QFrame::NoFrame);
        phoneWidget->setFrameShadow(QFrame::Plain);
        phoneWidget->setLineWidth(0);
        phoneWidget->setProperty("Common", true);
        QHBoxLayout* phoneWidgetLayout = Utils::emptyHLayout(phoneWidget);

        phoneLoginLayout->addWidget(phoneWidget);

        QSpacerItem* verticalSpacer_3 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        phoneLoginLayout->addItem(verticalSpacer_3);

        loginStakedWidget->addWidget(phoneLoginWidget);
        QWidget* phoneConfirmWidget = new QWidget();
        phoneConfirmWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout* phoneConfirmLayout = Utils::emptyVLayout(phoneConfirmWidget);

        QWidget* enteredPhoneWidget = new QWidget(phoneConfirmWidget);
        enteredPhoneWidget->setObjectName("enteredPhone");
        QHBoxLayout* enteredPhoneLayout = Utils::emptyHLayout(enteredPhoneWidget);
        QSpacerItem* horizontalSpacer_4 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        enteredPhoneLayout->addItem(horizontalSpacer_4);

        enteredPhone = new QLabel(enteredPhoneWidget);
        enteredPhone->setFont(Fonts::appFontScaled(24, Fonts::FontWeight::Light));

        enteredPhoneLayout->addWidget(enteredPhone);

        editPhoneButton = new QPushButton(enteredPhoneWidget);
        editPhoneButton->setCursor(QCursor(Qt::PointingHandCursor));
        editPhoneButton->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(editPhoneButton, getEditPhoneStyle());
        editPhoneButton->setText(QT_TRANSLATE_NOOP("login_page", "Edit"));

        enteredPhoneLayout->addWidget(editPhoneButton);

        QSpacerItem* horizontalSpacer_5 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        enteredPhoneLayout->addItem(horizontalSpacer_5);

        phoneConfirmLayout->addWidget(enteredPhoneWidget);

        resendButton = new QPushButton(phoneConfirmWidget);
        resendButton->setCursor(QCursor(Qt::PointingHandCursor));
        resendButton->setFocusPolicy(Qt::StrongFocus);
        resendButton->setFont(Fonts::appFontScaled(15));
        Utils::ApplyStyle(resendButton, getResendButtonStyle());

        phoneConfirmLayout->addWidget(resendButton);

        codeEdit = new LineEditEx(phoneConfirmWidget);
        codeEdit->setObjectName("code");
        codeEdit->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(codeEdit, Ui::CommonStyle::getLineEditStyle());
        codeEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
        codeEdit->setAlignment(Qt::AlignCenter);
        codeEdit->setPlaceholderText(QT_TRANSLATE_NOOP("login_page", "Code from SMS"));
        Testing::setAccessibleName(codeEdit, "StartWindowSMScodeField");

        phoneConfirmLayout->addWidget(codeEdit);

        QSpacerItem* verticalSpacer_4 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        phoneConfirmLayout->addItem(verticalSpacer_4);

        loginStakedWidget->addWidget(phoneConfirmWidget);
        QWidget* uinLoginWidget = new QWidget();
        uinLoginWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QVBoxLayout * uinLoginLayout = Utils::emptyVLayout(uinLoginWidget);

        uinEdit = new LineEditEx(uinLoginWidget);
        uinEdit->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(uinEdit, Ui::CommonStyle::getLineEditStyle());
        uinEdit->setPlaceholderText(build::is_icq() ?
            QT_TRANSLATE_NOOP("login_page", "UIN or Email")
            : QT_TRANSLATE_NOOP("login_page", "Email")
        );
        uinEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
        Testing::setAccessibleName(uinEdit, "StartWindowUinField");

        uinLoginLayout->addWidget(uinEdit);

        passwordEdit = new LineEditEx(uinLoginWidget);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(passwordEdit, Ui::CommonStyle::getLineEditStyle());
        passwordEdit->setPlaceholderText(QT_TRANSLATE_NOOP("login_page", "Password"));
        passwordEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
        Testing::setAccessibleName(passwordEdit, "StartWindowPasswordField");

        uinLoginLayout->addWidget(passwordEdit);

        keepLogged = new QCheckBox(uinLoginWidget);
        keepLogged->setObjectName("greenCheckBox");
        keepLogged->setVisible(isLogin_);
        keepLogged->setText(QT_TRANSLATE_NOOP("login_page", "Keep me signed in"));
        keepLogged->setChecked(get_gui_settings()->get_value(settings_keep_logged_in, true));

        uinLoginLayout->addWidget(keepLogged);

        uinLoginLayout->addStretch();

        loginStakedWidget->addWidget(uinLoginWidget);

        horizontalLayout->addWidget(loginStakedWidget);

        QSpacerItem* horizontalSpacer_8 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_8);

        controlsLayout->addWidget(centerWidget);
        controlsLayout->addWidget(passwordForgottenLabel);

        QWidget* nextButtonWidget = new QWidget(controlsWidget);
        nextButtonWidget->setObjectName("nextButton");

        QVBoxLayout* verticalLayout_8 = Utils::emptyVLayout(nextButtonWidget);
        nextButton = new QPushButton(nextButtonWidget);
        nextButton->setCursor(QCursor(Qt::PointingHandCursor));
        nextButton->setAutoDefault(true);
        nextButton->setDefault(false);
        Utils::ApplyStyle(nextButton, CommonStyle::getGreenButtonStyle());
        nextButton->setText(QT_TRANSLATE_NOOP("login_page", "Continue"));

        Testing::setAccessibleName(nextButton, "StartWindowLoginButton");

        verticalLayout_8->addWidget(nextButton);

        controlsLayout->addWidget(nextButtonWidget);

        if (isLogin_)
        {
            controlsLayout->setAlignment(nextButtonWidget, Qt::AlignHCenter);
        }
        else
        {
            controlsLayout->setAlignment(nextButtonWidget, Qt::AlignLeft);
        }

        QWidget* widget = new QWidget(controlsWidget);
        widget->setObjectName("errorWidget");
        QVBoxLayout* verticalLayout_7 = Utils::emptyVLayout(widget);
        errorLabel = new QLabel(widget);
        errorLabel->setObjectName("errorLabel");
        errorLabel->setWordWrap(true);
        errorLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        errorLabel->setOpenExternalLinks(true);

        if (isLogin_)
        {
            errorLabel->setAlignment(Qt::AlignCenter);
        }
        else
        {
            errorLabel->setAlignment(Qt::AlignLeft);
        }

        verticalLayout_7->addWidget(errorLabel);

        controlsLayout->addWidget(widget);

        mainLayout->addWidget(controlsWidget);

        if (isLogin_)
        {
            QSpacerItem* horizontalSpacer_7 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            mainLayout->addItem(horizontalSpacer_7);
        }
        else
        {
            mainLayout->setAlignment(Qt::AlignLeft);
        }

        verticalLayout->addWidget(mainWidget);

        QSpacerItem* verticalSpacer_2 = new QSpacerItem(0, 3, QSizePolicy::Minimum, QSizePolicy::Expanding);
        verticalLayout->addItem(verticalSpacer_2);

        {
            QWidget* switchLoginWidget = new QWidget(this);
            switchLoginWidget->setObjectName("switchLogin");
            QHBoxLayout* switchLoginLayout = Utils::emptyHLayout(switchLoginWidget);
            QSpacerItem* horizontalSpacer = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

            switchLoginLayout->addItem(horizontalSpacer);

            switchButton = new QPushButton(switchLoginWidget);
            switchButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            switchButton->setCursor(QCursor(Qt::PointingHandCursor));
            switchButton->setFont(Fonts::appFontScaled(15));
            Utils::ApplyStyle(switchButton, getSwitchLoginStyle());
            switchButton->setVisible(isLogin_);
            Testing::setAccessibleName(switchButton, "StartWindowChangeLoginType");
            switchLoginLayout->addWidget(switchButton);

            QSpacerItem* horizontalSpacer_2 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            switchLoginLayout->addItem(horizontalSpacer_2);
            verticalLayout->addWidget(switchLoginWidget);
        }

        loginStakedWidget->setCurrentIndex(2);

        QMetaObject::connectSlotsByName(this);

        connect(keepLogged, &QCheckBox::toggled, [](bool v)
        {
            if (get_gui_settings()->get_value(settings_keep_logged_in, true) != v)
                get_gui_settings()->set_value(settings_keep_logged_in, v);
        });

        Q_UNUSED(this);

        loginStakedWidget->setCurrentIndex(2);
        nextButton->setDefault(false);

        QMetaObject::connectSlotsByName(this);
        init();
    }

    LoginPage::~LoginPage()
    {
    }

    void LoginPage::keyPressEvent(QKeyEvent* _event)
    {
        if (_event->key() == Qt::Key_Return &&
            (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX ||
                loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX))
        {
            nextButton->click();
        }
        return QWidget::keyPressEvent(_event);
    }

    void LoginPage::paintEvent(QPaintEvent* _event)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_event);
    }

    void LoginPage::showEvent(QShowEvent *_event)
    {
        if (isLogin_)
        {
            uinEdit->setText(get_gui_settings()->get_value(login_page_last_entered_uin, QString()));
            auto lastPhone = get_gui_settings()->get_value(login_page_last_entered_phone, QString());
            if (!lastPhone.isEmpty())
            {
                QTimer::singleShot(500, this, [=]() // workaround: phoneinfo/set_can_change_hosts_scheme ( https://jira.mail.ru/browse/IMDESKTOP-3773 )
                {
                    phoneInfoRequestsCount_++;
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_string("phone", lastPhone.toStdString());
                    collection.set_value_as_string("gui_locale", get_gui_settings()->get_value(settings_language, QString("")).toStdString());
                    phoneInfoLastRequestSpecialId_ = GetDispatcher()->post_message_to_core("phoneinfo", collection.get());
                });
            }
        }
    }

    void LoginPage::init()
    {
        QMap<QString, QString> countryCodes = Utils::getCountryCodes();
        combobox_->setFont(Fonts::appFontScaled(18));
        Utils::ApplyStyle(combobox_, Ui::CommonStyle::getLineEditStyle() + getCountryComboboxStyle());
        combobox_->setComboboxViewClass("CountrySearchView");
        combobox_->setClass("CountySearchWidgetInternal");
        combobox_->setContextMenuPolicy(Qt::NoContextMenu);
        combobox_->setPlaceholder(QT_TRANSLATE_NOOP("login_page","Type country or code"));
        countrySearchWidget->layout()->addWidget(combobox_);
        combobox_->setSources(countryCodes);

        connect(combobox_, SIGNAL(selected(QString)), this, SLOT(countrySelected(QString)), Qt::QueuedConnection);
        connect(this, SIGNAL(country(QString)), this, SLOT(redrawCountryCode()), Qt::QueuedConnection);
        connect(nextButton, SIGNAL(clicked()), this, SLOT(nextPage()), Qt::QueuedConnection);
        connect(backButton, SIGNAL(clicked()), this, SLOT(prevPage()), Qt::QueuedConnection);
        connect(editPhoneButton, SIGNAL(clicked()), this, SLOT(prevPage()), Qt::QueuedConnection);
        connect(editPhoneButton, SIGNAL(clicked()), this, SLOT(stats_edit_phone()), Qt::QueuedConnection);
        connect(switchButton, SIGNAL(clicked()), this, SLOT(switchLoginType()), Qt::QueuedConnection);
        connect(resendButton, SIGNAL(clicked()), this, SLOT(sendCode()), Qt::QueuedConnection);
        connect(resendButton, SIGNAL(clicked()), this, SLOT(stats_resend_sms()), Qt::QueuedConnection);
        connect(timer_, SIGNAL(timeout()), this, SLOT(updateTimer()), Qt::QueuedConnection);

        connect(proxySettingsButton, SIGNAL(clicked()), this, SLOT(openProxySettings()), Qt::QueuedConnection);

        countryCode_->setObjectName("countryCodeEdit");
        phone_->setObjectName("phoneEdit");
        phone_->setProperty("Common", true);
        phone_->setAttribute(Qt::WA_MacShowFocusRect, false);
        phone_->setPlaceholderText(QT_TRANSLATE_NOOP("login_page","your phone number"));
        phoneWidget->layout()->addWidget(countryCode_);
        phoneWidget->layout()->addWidget(phone_);
        Testing::setAccessibleName(phone_, "StartWindowPhoneNumberField");

        connect(countryCode_, SIGNAL(focusIn()), this, SLOT(setPhoneFocusIn()), Qt::QueuedConnection);
        connect(countryCode_, SIGNAL(focusOut()), this, SLOT(setPhoneFocusOut()), Qt::QueuedConnection);
        connect(phone_, SIGNAL(focusIn()), this, SLOT(setPhoneFocusIn()), Qt::QueuedConnection);
        connect(phone_, SIGNAL(focusOut()), this, SLOT(setPhoneFocusOut()), Qt::QueuedConnection);

        connect(uinEdit, SIGNAL(textChanged(QString)), this, SLOT(clearErrors()), Qt::DirectConnection);
        connect(passwordEdit, SIGNAL(textEdited(QString)), this, SLOT(clearErrors()), Qt::DirectConnection);
        connect(codeEdit, SIGNAL(textChanged(QString)), this, SLOT(clearErrors()), Qt::DirectConnection);
        connect(codeEdit, SIGNAL(textChanged(QString)), this, SLOT(codeEditChanged(QString)), Qt::DirectConnection);
        connect(countryCode_, SIGNAL(textChanged(QString)), this, SLOT(clearErrors()), Qt::DirectConnection);
        connect(countryCode_, SIGNAL(textEdited(QString)), this, SLOT(countryCodeChanged(QString)), Qt::DirectConnection);
        connect(phone_, SIGNAL(textChanged(QString)), this, SLOT(phoneTextChanged()), Qt::DirectConnection);
        connect(phone_, SIGNAL(emptyTextBackspace()), this, SLOT(emptyPhoneRemove()), Qt::QueuedConnection);

        QObject::connect(Ui::GetDispatcher(), SIGNAL(getSmsResult(int64_t, int, int)), this, SLOT(getSmsResult(int64_t, int, int)), Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), SIGNAL(loginResult(int64_t, int)), this, SLOT(loginResult(int64_t, int)), Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), SIGNAL(loginResultAttachUin(int64_t, int)), this, SLOT(loginResultAttachUin(int64_t, int)), Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), SIGNAL(loginResultAttachPhone(int64_t, int)), this, SLOT(loginResultAttachPhone(int64_t, int)), Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), SIGNAL(phoneInfoResult(qint64, Data::PhoneInfo)), this, SLOT(phoneInfoResult(qint64, Data::PhoneInfo)), Qt::DirectConnection);
        QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::authError, this, &LoginPage::authError, Qt::QueuedConnection);
        
        countryCode_->setValidator(new QRegExpValidator(QRegExp("[\\+\\d]\\d*")));
        phone_->setValidator(new QRegExpValidator(QRegExp("\\d*")));
        codeEdit->setValidator(new QRegExpValidator(QRegExp("\\d*")));

        combobox_->selectItem(Utils::GetTranslator()->getCurrentPhoneCode());
        errorLabel->hide();
        phone_->setFocus();
        countryCode_->setFocusPolicy(Qt::ClickFocus);
        countryCode_->setAttribute(Qt::WA_MacShowFocusRect, false);

        initLoginSubPage(
            isLogin_ ? (build::is_agent() ? SUBPAGE_UIN_LOGIN_INDEX : SUBPAGE_PHONE_LOGIN_INDEX)
            : SUBPAGE_PHONE_LOGIN_INDEX
            );
    }

    void LoginPage::updateFocus()
    {
        uinEdit->setFocus();
    }

    void LoginPage::initLoginSubPage(int _index)
    {
        setFocus();
        backButton->setVisible(isLogin_ && _index == SUBPAGE_PHONE_CONF_INDEX);
        proxySettingsButton->setVisible(isLogin_ && _index != SUBPAGE_PHONE_CONF_INDEX);
        nextButton->setVisible(_index != SUBPAGE_PHONE_CONF_INDEX);

        QString nextPageText;
        if (isLogin_)
        {
            nextPageText = _index == SUBPAGE_PHONE_LOGIN_INDEX ? QT_TRANSLATE_NOOP("login_page","Continue") : QT_TRANSLATE_NOOP("login_page","Start messaging");
        }
        else
        {
            nextPageText = _index == SUBPAGE_PHONE_LOGIN_INDEX ? QT_TRANSLATE_NOOP("login_page","Continue") : QT_TRANSLATE_NOOP("login_page","Continue");
        }

        nextButton->setText(nextPageText);
        switchButton->setText(_index == SUBPAGE_UIN_LOGIN_INDEX ?
            QT_TRANSLATE_NOOP("login_page","Login via phone")
            : build::is_icq() ? 
                QT_TRANSLATE_NOOP("login_page","Login with UIN/Email")
                : QT_TRANSLATE_NOOP("login_page", "Login with Email")
        );
        switch (_index)
        {
        case SUBPAGE_PHONE_LOGIN_INDEX:
            hintLabel->setText(QT_TRANSLATE_NOOP("login_page", "Enter phone number"));
            passwordForgottenLabel->hide();
            phone_->setFocus();
            break;

        case SUBPAGE_PHONE_CONF_INDEX:
            hintLabel->setText(QT_TRANSLATE_NOOP("login_page", "Enter code from SMS"));
            passwordForgottenLabel->hide();
            codeEdit->setFocus();
            break;

        case SUBPAGE_UIN_LOGIN_INDEX:
            if (isLogin_)
            {
                hintLabel->setText(
                    build::is_icq() ? QT_TRANSLATE_NOOP("login_page", "Enter UIN or Email")
                    : QT_TRANSLATE_NOOP("login_page", "Enter your Email")
                );
                passwordForgottenLabel->setText(QT_TRANSLATE_NOOP("login_page","Forgot password?"));
                passwordForgottenLabel->setFixedHeight(QFontMetrics(passwordForgottenLabel->font()).height() * 2);
                passwordForgottenLabel->show();
            }
            else
            {
                hintLabel->setVisible(false);
                passwordForgottenLabel->hide();
            }
            uinEdit->setFocus();
            break;
        }
        loginStakedWidget->setCurrentIndex(_index);
    }

    void LoginPage::setPhoneFocusIn()
    {
        phoneWidget->setProperty("Focused", true);
        phoneWidget->setProperty("Error", false);
        phoneWidget->setProperty("Common", false);
        phoneWidget->setStyle(QApplication::style());
        emit country(countryCode_->text());
    }

    void LoginPage::setPhoneFocusOut()
    {
        phoneWidget->setProperty("Focused", false);
        phoneWidget->setProperty("Error", false);
        phoneWidget->setProperty("Common", true);
        phoneWidget->setStyle(QApplication::style());
        emit country(countryCode_->text());
    }

    void LoginPage::redrawCountryCode()
    {
        QFontMetrics fm = countryCode_->fontMetrics();
        int w = fm.boundingRect(countryCode_->text()).width() + 5;

        QRect content = phoneWidget->contentsRect();
        countryCode_->resize(w, countryCode_->height());
        phone_->resize(content.width() - w, phone_->height());
        countryCode_->move(content.x(), content.y());
        phone_->move(content.x() + w, content.y());
    }

    void LoginPage::countrySelected(QString _code)
    {
        if (prevCountryCode_ == _code)
            return;

        if (!prevCountryCode_.isEmpty())
        {
            core::stats::event_props_type props;
            props.push_back(std::make_pair("prev_code", prevCountryCode_.toStdString()));
            props.push_back(std::make_pair("next_code", _code.toStdString()));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_edit_country, props);
        }

        prevCountryCode_ = _code;
        countryCode_->setText(_code);
        redrawCountryCode();

        phone_->setFocus();
    }

    void LoginPage::nextPage()
    {
        static const auto waitServerResponseForPhoneNumberCheckInMsec = 7500;
        static const auto checkServerResponseEachNthMsec = 50;
        static const auto maxTriesOfCheckingForServerResponse = (waitServerResponseForPhoneNumberCheckInMsec / checkServerResponseEachNthMsec);
        static int triesPhoneAuth = 0;
        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX)
        {
            if (isEnabled())
            {
                setEnabled(false);
            }
            if (phoneInfoRequestsCount_ > 0)
            {
                ++triesPhoneAuth;
                if (triesPhoneAuth > maxTriesOfCheckingForServerResponse)
                {
                    triesPhoneAuth = 0;
                    setEnabled(true);
                    errorLabel->setVisible(true);
                    setErrorText(core::le_unknown_error);
                    phoneInfoRequestsCount_ = 0;
                    phoneInfoLastRequestId_ = 0;
                    return;
                }
                QTimer::singleShot(checkServerResponseEachNthMsec, this, SLOT(nextPage()));
                return;
            }
        }
        triesPhoneAuth = 0;
        setEnabled(true);
        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX && receivedPhoneInfo_.isValid())
        {
            bool isMobile = false;
            for (auto status: receivedPhoneInfo_.prefix_state_)
            {
                if (QString(status.c_str()).toLower() == "mobile")
                {
                    isMobile = true;
                    break;
                }
            }
            const auto isOk = (QString(receivedPhoneInfo_.status_.c_str()).toLower() == "ok");
            const auto message = QString(!receivedPhoneInfo_.printable_.empty() ? receivedPhoneInfo_.printable_[0].c_str() : "");
            if ((!isMobile || !isOk) && !message.isEmpty())
            {
                errorLabel->setVisible(true);
                setErrorText(message);
                phone_->setFocus();
                return;
            }
            else
            {
                errorLabel->setVisible(false);
            }
        }
        setFocus();
        clearErrors(true);
        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX)
        {
            enteredPhone->setText(countryCode_->text() + phone_->text());
            enteredPhone->adjustSize();
            countryCode_->setEnabled(false);
            phone_->setEnabled(false);
            sendCode();
        }
        else if (loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX)
        {
            uinEdit->setEnabled(false);
            passwordEdit->setEnabled(false);
            gui_coll_helper collection(GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("login", uinEdit->text());
            collection.set_value_as_qstring("password", passwordEdit->text());
            collection.set_value_as_bool("save_auth_data", keepLogged->isChecked());
            collection.set_value_as_bool("not_log", true);
            if (isLogin_)
            {
                sendSeq_ = GetDispatcher()->post_message_to_core("login_by_password", collection.get());
            }
            else
            {
                sendSeq_ = GetDispatcher()->post_message_to_core("login_by_password_for_attach_uin", collection.get());
            }
        }
    }

    void LoginPage::prevPage()
    {
        clearErrors();
        initLoginSubPage(SUBPAGE_PHONE_LOGIN_INDEX);
    }

    void LoginPage::switchLoginType()
    {
        setFocus();
        clearErrors();
        initLoginSubPage(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX ? SUBPAGE_PHONE_LOGIN_INDEX : SUBPAGE_UIN_LOGIN_INDEX);
        GetDispatcher()->post_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
            ? core::stats::stats_event_names::reg_page_uin : core::stats::stats_event_names::reg_page_phone);
    }

    void LoginPage::stats_edit_phone()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_edit_phone);
    }

    void LoginPage::phoneTextChanged()
    {
        if (!phoneChangedAuto_)
        {
            clearErrors();
        }
    }

    void LoginPage::stats_resend_sms()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_sms_resend);
    }

    void LoginPage::updateTimer()
    {
        resendButton->setEnabled(false);
        QString text = remainingSeconds_ ? QT_TRANSLATE_NOOP("login_page","Resend code in ") : QT_TRANSLATE_NOOP("login_page","Resend code");
        if (remainingSeconds_ == 60)
        {
            text += "1:00";
        }
        else if (remainingSeconds_ > 0)
        {
            text += QString(remainingSeconds_ >= 10 ? "0:%1" : "0:0%1").arg(remainingSeconds_);
        }
        else
        {
            resendButton->setEnabled(true);
        }
        resendButton->setText(text);

        if (remainingSeconds_)
        {
            --remainingSeconds_;
            timer_->start(1000);
        }
        else
        {
            timer_->stop();
        }
    }

    void LoginPage::sendCode()
    {
        timer_->stop();
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("country", countryCode_->text());
        collection.set_value_as_qstring("phone", phone_->text());
        collection.set_value_as_qstring("locale", Utils::GetTranslator()->getCurrentLang());
        collection.set_value_as_bool("is_login", isLogin_);

        sendSeq_ = GetDispatcher()->post_message_to_core("login_get_sms_code", collection.get());
        remainingSeconds_ = 60;
        updateTimer();
    }

    void LoginPage::getSmsResult(int64_t _seq, int _result, int _codeLength)
    {
        if (_seq != sendSeq_)
            return;

        countryCode_->setEnabled(true);
        phone_->setEnabled(true);
        setErrorText(_result);
        errorLabel->setVisible(_result);
        if (_result == core::le_success)
        {
            if (_codeLength != 0)
                codeLength_ = _codeLength;
            clearErrors();
            return initLoginSubPage(SUBPAGE_PHONE_CONF_INDEX);
        }

        phoneWidget->setProperty("Error", true);
        phoneWidget->setProperty("Focused", false);
        phoneWidget->setProperty("Common", false);
        phoneWidget->setStyle(QApplication::style());
        phone_->setProperty("Error", true);
        phone_->setStyle(QApplication::style());
        emit country(countryCode_->text());
    }

    void LoginPage::updateErrors(int _result)
    {
        codeEdit->setEnabled(true);
        uinEdit->setEnabled(true);
        passwordEdit->setEnabled(true);
        setErrorText(_result);
        errorLabel->setVisible(_result);

        if (loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX)
        {
            Utils::ApplyStyle(uinEdit, Ui::CommonStyle::getLineEditErrorStyle());
            passwordEdit->clear();
            uinEdit->setFocus();
        }
        else
        {
            Utils::ApplyStyle(codeEdit, Ui::CommonStyle::getLineEditErrorStyle());
            codeEdit->setFocus();
        }
    }

    void LoginPage::loginResult(int64_t _seq, int _result)
    {
        if (sendSeq_ > 0 && _seq != sendSeq_)
            return;

        updateErrors(_result);

        if (_result == 0)
        {
            if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_CONF_INDEX)
            {
                codeEdit->setText("");
                initLoginSubPage(SUBPAGE_PHONE_LOGIN_INDEX);
            }

            GetDispatcher()->post_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::stats_event_names::reg_login_uin
                : core::stats::stats_event_names::reg_login_phone);

            clearErrors();
            emit loggedIn();
            
            phoneInfoRequestsCount_ = 0;
            phoneInfoLastRequestId_ = 0;
            receivedPhoneInfo_ = Data::PhoneInfo();
        }
    }

    void LoginPage::authError(const int _result)
    {
        updateErrors(_result);
    }

    void LoginPage::loginResultAttachUin(int64_t _seq, int _result)
    {
        if (_seq != sendSeq_)
            return;
        updateErrors(_result);
        if (_result == 0)
        {
            emit attached();
        }
    }

    void LoginPage::loginResultAttachPhone(int64_t _seq, int _result)
    {
        if (_seq != sendSeq_)
            return;
        updateErrors(_result);
        if (_result == 0)
        {
            emit attached();
        }
    }

    void LoginPage::clearErrors(bool ignorePhoneInfo/* = false*/)
    {
        errorLabel->hide();

        Utils::ApplyStyle(uinEdit, Ui::CommonStyle::getLineEditStyle());
        Utils::ApplyStyle(codeEdit, Ui::CommonStyle::getLineEditStyle());

        phone_->setProperty("Error", false);
        phone_->setProperty("Common", true);
        phone_->setStyle(QApplication::style());

        emit country(countryCode_->text());

        if (loginStakedWidget->currentIndex() == SUBPAGE_PHONE_LOGIN_INDEX && !ignorePhoneInfo)
        {
            if (phone_->text().length() >= 3)
            {
                phoneInfoRequestsCount_++;

                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_string("phone", (countryCode_->text() + phone_->text()).toStdString());
                collection.set_value_as_string("gui_locale", get_gui_settings()->get_value(settings_language, QString("")).toStdString());
                phoneInfoLastRequestId_ = GetDispatcher()->post_message_to_core("phoneinfo", collection.get());
            }
            else
            {
                phoneInfoLastRequestId_ = 0;
                receivedPhoneInfo_ = Data::PhoneInfo();
            }
        }
        else
        {
            phoneInfoRequestsCount_ = 0;
            phoneInfoLastRequestId_ = 0;
        }
    }

    void LoginPage::phoneInfoResult(qint64 _seq, Data::PhoneInfo _data)
    {
        phoneInfoRequestsCount_--;

        if (phoneInfoLastRequestId_ == _seq || phoneInfoLastRequestSpecialId_ == _seq)
        {
            receivedPhoneInfo_ = _data;

            if (receivedPhoneInfo_.isValid() && !receivedPhoneInfo_.modified_phone_number_.empty())
            {
                auto code = countryCode_->text();
                if (!receivedPhoneInfo_.info_iso_country_.empty())
                {
                    combobox_->selectItem(Utils::getCountryNameByCode(receivedPhoneInfo_.info_iso_country_.c_str()));
                }
                else if (!receivedPhoneInfo_.modified_prefix_.empty() && code != receivedPhoneInfo_.modified_prefix_.c_str())
                {
                    code = QString(receivedPhoneInfo_.modified_prefix_.c_str());
                    combobox_->selectItem(QString(code).remove('+'));
                }
                phoneChangedAuto_ = true;
                phone_->setText(QString(receivedPhoneInfo_.modified_phone_number_.c_str()).remove(0, code.length()));
                phoneChangedAuto_ = false;
            }
        }

        if (phoneInfoLastRequestSpecialId_ == _seq)
        {
            phoneInfoLastRequestSpecialId_ = 0;
        }
    }

    void LoginPage::setErrorText(int _result)
    {
        setFocus();
        switch (_result)
        {
        case core::le_wrong_login:
            errorLabel->setText(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX ?
                QT_TRANSLATE_NOOP("login_page","Wrong UIN/Email or password. Please try again.")
                : QT_TRANSLATE_NOOP("login_page","You have entered an invalid code. Please try again."));
            GetDispatcher()->post_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::stats_event_names::reg_error_uin
                : core::stats::stats_event_names::reg_error_code);
            break;
        case core::le_wrong_login_2x_factor:
            errorLabel->setText(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX ?
                QT_TRANSLATE_NOOP("login_page", "Two-factor authentication is on, please create an app password <a href=\"https://e.mail.ru/settings/2-step-auth\">here</a> to login")
                : QT_TRANSLATE_NOOP("login_page","You have entered an invalid code. Please try again."));
            GetDispatcher()->post_stats_to_core(loginStakedWidget->currentIndex() == SUBPAGE_UIN_LOGIN_INDEX
                ? core::stats::stats_event_names::reg_error_uin
                : core::stats::stats_event_names::reg_error_code);
            break;
        case core::le_error_validate_phone:
            errorLabel->setText(QT_TRANSLATE_NOOP("login_page","Invalid phone number. Please try again."));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_phone);
            break;
        case core::le_success:
            errorLabel->setText("");
            break;
        case core::le_attach_error_busy_phone:
            errorLabel->setText(QT_TRANSLATE_NOOP("sidebar","This phone number is already attached to another account.\nPlease edit phone number and try again."));
            break;
        default:
            errorLabel->setText(QT_TRANSLATE_NOOP("login_page","Error occured, try again later"));
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_error_other);
            break;
        }
    }

    void LoginPage::setErrorText(const QString& _customError)
    {
        errorLabel->setText(_customError);
    }

    void LoginPage::codeEditChanged(QString _code)
    {
        if (_code.length() == codeLength_)
        {
            setFocus();
            codeEdit->setEnabled(false);
            get_gui_settings()->set_value(settings_keep_logged_in, true);
            gui_coll_helper collection(GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("phone", phone_->text());
            collection.set_value_as_qstring("sms_code", _code);
            collection.set_value_as_bool("save_auth_data", true);
            collection.set_value_as_bool("is_login", isLogin_);
            sendSeq_ = GetDispatcher()->post_message_to_core("login_by_phone", collection.get());
        }
    }

    void LoginPage::countryCodeChanged(QString _text)
    {
        if (!_text.isEmpty() && _text != "+")
        {
            combobox_->selectItem(_text);
            if (!combobox_->containsCode(_text))
            {
                countryCode_->setText(prevCountryCode_);
                if (phone_->text().isEmpty())
                    phone_->setText(_text.mid(prevCountryCode_.length(), _text.length() - prevCountryCode_.length()));
                phone_->setFocus();
            }
        }

        prevCountryCode_ = _text;
    }

    void LoginPage::emptyPhoneRemove()
    {
        countryCode_->setFocus();
        QString code = countryCode_->text();
        countryCode_->setText(code.isEmpty() ? code : code.left(code.length() - 1));
    }

    void LoginPage::openProxySettings()
    {
        auto connection_settings_widget_ = new ConnectionSettingsWidget(this);
        connection_settings_widget_->show();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_open);
    }
}
