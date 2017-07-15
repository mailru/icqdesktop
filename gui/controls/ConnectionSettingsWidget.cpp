#include "stdafx.h"
#include "ConnectionSettingsWidget.h"

#include "GeneralCreator.h"
#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "../core_dispatcher.h"
#include "../controls/CommonStyle.h"
#include "../controls/LineEditEx.h"
#include "../fonts.h"
#include "../main_window/MainWindow.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"

namespace Ui
{
    int text_edit_width = -1; // take from showPasswordCheckbox_
    const int SPACER_HEIGHT = 56;
    const int PORT_EDIT_WIDTH = 100;
    const int BOTTOM_OFFSET = 25;

    ConnectionSettingsWidget::ConnectionSettingsWidget(QWidget* _parent)
        : QWidget(_parent)
        , selectedProxyIndex_(0)
        , mainWidget_(new QWidget(this))
    {
        mainWidget_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        addressEdit_ = new LineEditEx(mainWidget_);
        portEdit_ = new LineEditEx(mainWidget_);
        usernameEdit_ = new LineEditEx(mainWidget_);
        passwordEdit_ = new LineEditEx(mainWidget_);
        showPasswordCheckbox_ = new QCheckBox(mainWidget_);
        showPasswordCheckbox_->setObjectName("greenCheckBox");

        auto layout = new QVBoxLayout(mainWidget_);
        layout->setContentsMargins(Utils::scale_value(24), 0, Utils::scale_value(24), 0);
        mainWidget_->setLayout(layout);

        {
            showPasswordCheckbox_->setText(QT_TRANSLATE_NOOP("connection_settings","Proxy server requires password"));
            showPasswordCheckbox_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            text_edit_width = Utils::unscale_value(showPasswordCheckbox_->sizeHint().width());
        }
        {
            fillProxyTypesAndNames();
            selectedProxyIndex_ = proxyTypeToIndex(core::proxy_types::auto_proxy);

            dropper_ = GeneralCreator::addDropper(
                mainWidget_,
                layout,
                QT_TRANSLATE_NOOP("connection_settings", "Type:"),
                typesNames_,
                selectedProxyIndex_,
                text_edit_width,
                [this](QString v, int ix, TextEmojiWidget*)
            {
                selectedProxyIndex_ = ix;
                updateVisibleParams(ix);
                addressEdit_->setFocus();
            },
                [](bool) -> QString { return ""; });
        }

        {
            auto proxyLayout = new QHBoxLayout();

            addressEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("connection_settings", "Hostname"));
            addressEdit_->setAlignment(Qt::AlignLeft);
            addressEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            addressEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            addressEdit_->setFont(Fonts::appFontScaled(18));
            Utils::ApplyStyle(addressEdit_, Ui::CommonStyle::getLineEditStyle());
            proxyLayout->addWidget(addressEdit_);

            horizontalSpacer_ = new QSpacerItem(0, Utils::scale_value(SPACER_HEIGHT), QSizePolicy::Preferred, QSizePolicy::Minimum);
            proxyLayout->addItem(horizontalSpacer_);

            portEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("connection_settings", "Port"));
            portEdit_->setAlignment(Qt::AlignLeft);
            portEdit_->setFixedWidth(Utils::scale_value(PORT_EDIT_WIDTH));
            portEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            portEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            portEdit_->setFont(Fonts::appFontScaled(18));
            Utils::ApplyStyle(portEdit_, Ui::CommonStyle::getLineEditStyle());
            portEdit_->setValidator(new QIntValidator(0, 65535));
            proxyLayout->addWidget(portEdit_);

            layout->addLayout(proxyLayout);
        }
        {
            layout->addWidget(showPasswordCheckbox_);
        }
        {
            usernameEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("connection_settings", "Username"));
            usernameEdit_->setAlignment(Qt::AlignLeft);
            usernameEdit_->setMinimumWidth(Utils::scale_value(text_edit_width));
            usernameEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            usernameEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            usernameEdit_->setFont(Fonts::appFontScaled(18));
            Utils::ApplyStyle(usernameEdit_, Ui::CommonStyle::getLineEditStyle());
            layout->addWidget(usernameEdit_);

            passwordEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("connection_settings", "Password"));
            passwordEdit_->setAlignment(Qt::AlignLeft);
            passwordEdit_->setMinimumWidth(Utils::scale_value(text_edit_width));
            passwordEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            passwordEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            passwordEdit_->setFont(Fonts::appFontScaled(18));
            Utils::ApplyStyle(passwordEdit_, Ui::CommonStyle::getLineEditStyle());
            passwordEdit_->setEchoMode(QLineEdit::Password);
            layout->addWidget(passwordEdit_);
        }

        generalDialog_ = new GeneralDialog(mainWidget_, Utils::InterConnector::instance().getMainWindow());
        generalDialog_->setKeepCenter(true);
        mainWidget_->setStyleSheet(Utils::LoadStyle(":/main_window/settings/general_settings.qss"));

        connect(showPasswordCheckbox_, &QCheckBox::toggled, [this](bool _isVisible)
        {
            setVisibleAuth(_isVisible);
            if (_isVisible)
            {
                usernameEdit_->setFocus();
            }
        });

        QObject::connect(addressEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(portEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(usernameEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(passwordEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);

        generalDialog_->addHead();
        generalDialog_->addLabel(QT_TRANSLATE_NOOP("connection_settings", "Connection settings"));
        generalDialog_->addAcceptButton(QT_TRANSLATE_NOOP("connection_settings", "Done"), Utils::scale_value(BOTTOM_OFFSET), true);
    }

    void ConnectionSettingsWidget::show()
    {
        fill();
        if (generalDialog_->showInPosition(-1, -1))
        {
            saveProxy();
        }
        delete generalDialog_;
    }

    void ConnectionSettingsWidget::enterClicked()
    {
        if (!addressEdit_->text().isEmpty() && !portEdit_->text().isEmpty())
        {
            generalDialog_->accept();
        }
    }

    void ConnectionSettingsWidget::saveProxy() const
    {
        auto userProxy = Utils::get_proxy_settings();

        userProxy->type_ = activeProxyTypes_[selectedProxyIndex_];
        userProxy->username_ = usernameEdit_->text();
        userProxy->password_ = passwordEdit_->text();
        userProxy->proxyServer_ = addressEdit_->text();
        userProxy->port_ = portEdit_->text().isEmpty() ? Utils::ProxySettings::invalidPort : portEdit_->text().toInt();
        userProxy->needAuth_ = showPasswordCheckbox_->isChecked();

        userProxy->postToCore();

        core::stats::event_props_type props;
        std::stringstream str;
        str << userProxy->type_;
        props.push_back(std::make_pair("Proxy_Type", str.str()));
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_set, props);
    }

    void ConnectionSettingsWidget::setVisibleAuth(bool _useAuth)
    {
        if (_useAuth)
        {
            usernameEdit_->setVisible(true);
            passwordEdit_->setVisible(true);
        }
        else
        {
            usernameEdit_->setVisible(false);
            passwordEdit_->setVisible(false);
        }
    }

    void ConnectionSettingsWidget::setVisibleParams(int _ix, bool _useAuth)
    {
        if (_ix == 0)
        {
            addressEdit_->setVisible(false);
            portEdit_->setVisible(false);
            showPasswordCheckbox_->setVisible(false);
            setVisibleAuth(false);
            horizontalSpacer_->changeSize(0, Utils::scale_value(1), QSizePolicy::Preferred, QSizePolicy::Minimum);
        }
        else
        {
            addressEdit_->setVisible(true);
            portEdit_->setVisible(true);
            showPasswordCheckbox_->setVisible(true);
            setVisibleAuth(_useAuth);
            horizontalSpacer_->changeSize(0, Utils::scale_value(SPACER_HEIGHT), QSizePolicy::Preferred, QSizePolicy::Minimum);
        }
    }

    void ConnectionSettingsWidget::updateVisibleParams(int _ix)
    {
        setVisibleParams(_ix, showPasswordCheckbox_->isChecked());
    }

    void ConnectionSettingsWidget::fill()
    {
        auto userProxy = Utils::get_proxy_settings();

        usernameEdit_->setText(userProxy->username_);
        addressEdit_->setText(userProxy->proxyServer_);
        portEdit_->setText(userProxy->port_ == Utils::ProxySettings::invalidPort ? "" : QString::number(userProxy->port_));
        passwordEdit_->setText(userProxy->password_);
        showPasswordCheckbox_->setChecked(userProxy->needAuth_);

        selectedProxyIndex_ = proxyTypeToIndex(userProxy->type_);
        setVisibleParams(selectedProxyIndex_, userProxy->needAuth_);

        dropper_.currentSelected->setText(typesNames_[selectedProxyIndex_]);
    }

    void ConnectionSettingsWidget::fillProxyTypesAndNames()
    {
        activeProxyTypes_.push_back(core::proxy_types::auto_proxy);
        activeProxyTypes_.push_back(core::proxy_types::http_proxy);
        activeProxyTypes_.push_back(core::proxy_types::socks4);
        activeProxyTypes_.push_back(core::proxy_types::socks5);

        for (auto proxyType : activeProxyTypes_)
        {
            std::stringstream str;
            str << proxyType;
            typesNames_.push_back(QString(str.str().c_str()));
        }
    }

    int ConnectionSettingsWidget::proxyTypeToIndex(core::proxy_types _type) const
    {
        auto typeIter = std::find(activeProxyTypes_.begin(), activeProxyTypes_.end(), _type);

        int index = 0;
        if (typeIter != activeProxyTypes_.end())
            index =  std::distance(activeProxyTypes_.begin(), typeIter);
        return index;
    }
}
