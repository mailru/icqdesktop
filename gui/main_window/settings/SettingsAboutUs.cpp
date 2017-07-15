#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/PictureWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../../common.shared/version_info_constants.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAbout(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBar(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto scrollAreaWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(scrollAreaWidget);

    auto scrollAreaLayout = Utils::emptyVLayout(scrollAreaWidget);
    scrollAreaLayout->setAlignment(Qt::AlignTop);
    scrollAreaLayout->setContentsMargins(Utils::scale_value(36), 0, Utils::scale_value(36), Utils::scale_value(36));

    scrollArea->setWidget(scrollAreaWidget);

    auto layout = Utils::emptyHLayout(_parent);
    layout->addWidget(scrollArea);

    {
        auto mainWidget = new QWidget(scrollArea);
        Utils::grabTouchWidget(mainWidget);
        auto mainLayout = new QHBoxLayout(mainWidget);
        mainLayout->setContentsMargins(0, Utils::scale_value(28), Utils::scale_value(48), 0);
        mainLayout->setSpacing(Utils::scale_value(32));
        mainLayout->setAlignment(Qt::AlignTop);
        {
            auto logoWidget = new QWidget(mainWidget);
            auto logoLayout = Utils::emptyVLayout(logoWidget);
            Utils::grabTouchWidget(logoWidget);
            logoLayout->setAlignment(Qt::AlignTop);
            {
                PictureWidget* logo = new PictureWidget(logoWidget,
                    build::is_icq() ?
                    ":/resources/main_window/content_logo_100.png" :
                    ":/resources/main_window/content_logo_agent_100.png");
                logo->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                logo->setFixedSize(QSize(Utils::scale_value(80), Utils::scale_value(80)));
                logoLayout->addWidget(logo);
            }
            mainLayout->addWidget(logoWidget);
        }
        {
            auto aboutWidget = new QWidget(mainWidget);
            auto aboutLayout = Utils::emptyVLayout(aboutWidget);
            Utils::grabTouchWidget(aboutWidget);
            aboutLayout->setAlignment(Qt::AlignTop);
            {
                auto versionLabel = QString("%1 (%2)")
                    .arg(build::is_icq() ? QT_TRANSLATE_NOOP("title", "ICQ") : QT_TRANSLATE_NOOP("title", "Mail.Ru Agent"))
                    .arg(VERSION_INFO_STR);
                auto versionText = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), -1);
                Utils::grabTouchWidget(versionText);
                versionText->setText(versionLabel);
                aboutLayout->addWidget(versionText);
            }
            {
                auto opensslLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
                Utils::grabTouchWidget(opensslLabel);
                opensslLabel->setMultiline(true);
                opensslLabel->setText(QT_TRANSLATE_NOOP("about_us", "This product includes software developed by the OpenSSL project for use in the OpenSSL Toolkit"));
                aboutLayout->addWidget(opensslLabel);
            }
            {
                auto opensslButton = new QPushButton(aboutWidget);
                auto opensslLayout = Utils::emptyVLayout(opensslButton);
                opensslButton->setFlat(true);
                opensslButton->setStyleSheet("background-color: transparent;");
                opensslButton->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                opensslButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
                opensslLayout->setAlignment(Qt::AlignTop);
                {
                    auto opensslLink = new TextEmojiWidget(opensslButton, Fonts::appFontScaled(16), Ui::CommonStyle::getLinkColor(), Utils::scale_value(28));
                    Utils::grabTouchWidget(opensslLink);
                    opensslLink->setText(QT_TRANSLATE_NOOP("about_us", "http://openssl.org"));
                    connect(opensslButton, &QPushButton::pressed, [opensslLink]()
                    {
                        QDesktopServices::openUrl(opensslLink->text());
                    });
                    opensslButton->setFixedHeight(opensslLink->height());
                    opensslLayout->addWidget(opensslLink);
                }
                aboutLayout->addWidget(opensslButton);
            }
            {
                auto voipCopyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
                Utils::grabTouchWidget(voipCopyrightLabel);
                voipCopyrightLabel->setMultiline(true);
                voipCopyrightLabel->setText(QT_TRANSLATE_NOOP("about_us", "Copyright © 2012, the WebRTC project authors. All rights reserved."));
                aboutLayout->addWidget(voipCopyrightLabel);
            }
            {
                auto emojiLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(28));
                Utils::grabTouchWidget(emojiLabel);
                emojiLabel->setMultiline(true);
                emojiLabel->setText(QT_TRANSLATE_NOOP("about_us", "Emoji provided free by Emoji One"));
                aboutLayout->addWidget(emojiLabel);
            }
            {
                auto copyrightLabel = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
                Utils::grabTouchWidget(copyrightLabel);
                copyrightLabel->setText(
                    build::is_icq() ? QT_TRANSLATE_NOOP("about_us", "© ICQ LLC") : QT_TRANSLATE_NOOP("about_us", "© Mail.Ru LLC")
                    + ", "
                    + QDate::currentDate().toString("yyyy"));
                aboutLayout->addWidget(copyrightLabel);

                auto presentedMailru = new TextEmojiWidget(aboutWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(32));
                Utils::grabTouchWidget(presentedMailru);
                presentedMailru->setText(QT_TRANSLATE_NOOP("about_us", "Presented by Mail.Ru"));
                presentedMailru->setVisible(build::is_icq());
                aboutLayout->addWidget(presentedMailru);
            }
            mainLayout->addWidget(aboutWidget);
        }
        scrollAreaLayout->addWidget(mainWidget);
    }
}