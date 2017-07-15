#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/ContactListModel.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../my_info.h"
#include "../../../common.shared/version_info_constants.h"

using namespace Ui;

namespace
{
    const int TEXTEDIT_MIN_HEIGHT = 88;
    const int TEXTEDIT_MAX_HEIGHT = 252;
    const int CONTROLS_WIDTH = 440;
    const QColor ERROR_LABEL_COLOR("#d0021b");
    const QColor SENDING_LABEL_COLOR("#579e1c");
}

void GeneralSettingsWidget::Creator::initContactUs(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/)
{
    static std::map<QString, QString> filesToSend;

    auto scrollArea = CreateScrollAreaAndSetTrScrollBar(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(Utils::LoadStyle(":/main_window/settings/contact_us.qss"));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), 0, 0);

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    layout->addWidget(scrollArea);

    {
        static quint64 filesSizeLimiter = 0;
        TextEditEx *suggestioner = nullptr;
        TextEmojiWidget *suggestionMinSizeError = nullptr;
        TextEmojiWidget *suggestionMaxSizeError = nullptr;
        QWidget *suggestionerCover = nullptr;
        QWidget *attachWidget = nullptr;
        QWidget *filerCover = nullptr;
        TextEmojiWidget *attachTotalSizeError = nullptr;
        TextEmojiWidget *attachSizeError = nullptr;
        LineEditEx *email = nullptr;
        QWidget *emailerCover = nullptr;
        TextEmojiWidget *emailError = nullptr;

        auto successPage = new QWidget(scrollArea);
        auto sendingPage = new QWidget(scrollArea);

        Utils::grabTouchWidget(successPage);
        Utils::grabTouchWidget(sendingPage);

        auto sendingPageLayout = new QVBoxLayout(sendingPage);
        sendingPageLayout->setSpacing(Utils::scale_value(8));
        sendingPageLayout->setAlignment(Qt::AlignTop);
        {
            suggestioner = new TextEditEx(sendingPage, Fonts::appFontScaled(18), CommonStyle::getTextCommonColor(), true, false);
            Testing::setAccessibleName(suggestioner, "feedback_suggestioner");

            Utils::grabTouchWidget(suggestioner);
            suggestioner->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);
            suggestioner->setFixedWidth(Utils::scale_value(CONTROLS_WIDTH));
            suggestioner->setMinimumHeight(Utils::scale_value(TEXTEDIT_MIN_HEIGHT));
            suggestioner->setMaximumHeight(Utils::scale_value(TEXTEDIT_MAX_HEIGHT));
            suggestioner->setPlaceholderText(QT_TRANSLATE_NOOP("contactus_page","Your comments or suggestions..."));
            suggestioner->setAutoFillBackground(false);
            suggestioner->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            suggestioner->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            suggestioner->setProperty("normal", true);
            QObject::connect(&Utils::InterConnector::instance(), SIGNAL(generalSettingsContactUsShown()), suggestioner, SLOT(setFocus()), Qt::QueuedConnection);
            {
                suggestionerCover = new QWidget(suggestioner);
                Utils::grabTouchWidget(suggestionerCover);
                suggestionerCover->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                suggestionerCover->setFixedSize(suggestioner->minimumWidth(), suggestioner->minimumHeight());
                suggestionerCover->setVisible(false);
            }
            {
                scrollArea->connect(suggestioner->document(), &QTextDocument::contentsChanged, scrollArea, [=]()
                {
                    auto textControlHeight = suggestioner->document()->size().height() + Utils::scale_value(12 + 10 + 2); // paddings + border_width*2
                    if (textControlHeight > suggestioner->maximumHeight())
                        suggestioner->setMinimumHeight(suggestioner->maximumHeight());
                    else if (textControlHeight > Utils::scale_value(TEXTEDIT_MIN_HEIGHT))
                        suggestioner->setMinimumHeight(textControlHeight);
                    else
                        suggestioner->setMinimumHeight(Utils::scale_value(TEXTEDIT_MIN_HEIGHT));
                });
            }
            sendingPageLayout->addWidget(suggestioner);

            suggestionMinSizeError = new TextEmojiWidget(suggestioner, Fonts::appFontScaled(16), ERROR_LABEL_COLOR, Utils::scale_value(12));
            Utils::grabTouchWidget(suggestionMinSizeError);
            suggestionMinSizeError->setText(QT_TRANSLATE_NOOP("contactus_page", "Message is too short"));
            suggestionMinSizeError->setVisible(false);
            sendingPageLayout->addWidget(suggestionMinSizeError);

            suggestionMaxSizeError = new TextEmojiWidget(suggestioner, Fonts::appFontScaled(16), ERROR_LABEL_COLOR, Utils::scale_value(12));
            Utils::grabTouchWidget(suggestionMaxSizeError);
            suggestionMaxSizeError->setText(QT_TRANSLATE_NOOP("contactus_page", "Message is too long"));
            suggestionMaxSizeError->setVisible(false);
            sendingPageLayout->addWidget(suggestionMaxSizeError);
        }
        {
            attachWidget = new QWidget(sendingPage);
            Testing::setAccessibleName(attachWidget, "feedback_filer");

            Utils::grabTouchWidget(attachWidget);
            auto attachLayout = new QVBoxLayout(attachWidget);
            attachWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            attachWidget->setFixedWidth(Utils::scale_value(CONTROLS_WIDTH));
            attachLayout->setContentsMargins(0, Utils::scale_value(4), 0, 0);
            attachLayout->setSpacing(Utils::scale_value(8));
            {
                filerCover = new QWidget(attachWidget);
                Utils::grabTouchWidget(filerCover);
                filerCover->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                filerCover->setFixedSize(attachWidget->minimumWidth(), attachWidget->minimumHeight());
                filerCover->setVisible(false);
            }
            {
                attachSizeError = new TextEmojiWidget(attachWidget, Fonts::appFontScaled(16), ERROR_LABEL_COLOR, Utils::scale_value(12));
                Utils::grabTouchWidget(attachSizeError);
                attachSizeError->setText(QT_TRANSLATE_NOOP("contactus_page", "File size exceeds 1 MB"));
                attachSizeError->setVisible(false);
                attachLayout->addWidget(attachSizeError);

                attachTotalSizeError = new TextEmojiWidget(attachWidget, Fonts::appFontScaled(16), ERROR_LABEL_COLOR, Utils::scale_value(12));
                Utils::grabTouchWidget(attachTotalSizeError);
                attachTotalSizeError->setText(QT_TRANSLATE_NOOP("contactus_page", "Attachments size exceeds 25 MB"));
                attachTotalSizeError->setVisible(false);
                attachLayout->addWidget(attachTotalSizeError);

                auto bsc = [=](QString _fileName, QString _fileSize, QString _realName, qint64 _realSize)
                {
                    auto fileWidget = new QWidget(attachWidget);
                    auto fileLayout = new QHBoxLayout(fileWidget);
                    Utils::grabTouchWidget(fileWidget);
                    fileWidget->setObjectName(_fileName);
                    fileWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                    fileWidget->setFixedWidth(Utils::scale_value(CONTROLS_WIDTH));
                    fileWidget->setFixedHeight(Utils::scale_value(32));
                    fileWidget->setProperty("fileWidget", true);
                    fileLayout->setContentsMargins(Utils::scale_value(12), 0, 0, 0);
                    fileLayout->setSpacing(Utils::scale_value(12));
                    fileLayout->setAlignment(Qt::AlignVCenter);
                    {
                        auto fileInfoWidget = new QWidget(fileWidget);
                        auto fileInfoLayout = Utils::emptyHLayout(fileInfoWidget);
                        Utils::grabTouchWidget(fileInfoWidget);
                        fileInfoWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
                        fileInfoLayout->setAlignment(Qt::AlignLeft);
                        {
                            auto fileName = new QLabel(fileInfoWidget);
                            Utils::grabTouchWidget(fileName);
                            fileName->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                            fileName->setText(_fileName);
                            fileName->setProperty("fileName", true);
                            fileInfoLayout->addWidget(fileName);

                            auto fileSize = new QLabel(fileInfoWidget);
                            Utils::grabTouchWidget(fileSize);
                            fileSize->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                            fileSize->setText(QString(" - %1").arg(_fileSize));
                            fileSize->setProperty("fileSize", true);
                            fileInfoLayout->addWidget(fileSize);
                        }
                        fileLayout->addWidget(fileInfoWidget);

                        auto deleteFile = new QPushButton(fileWidget);
                        deleteFile->setObjectName("deleteFile");
                        deleteFile->setFlat(true);
                        deleteFile->setCursor(Qt::PointingHandCursor);
                        attachWidget->connect(deleteFile, &QPushButton::clicked, attachWidget, [=]()
                        {
                            filesToSend.erase(fileWidget->objectName());
                            filesSizeLimiter -= _realSize;
                            attachSizeError->setVisible(false);
                            attachTotalSizeError->setVisible(false);
                            fileWidget->setVisible(false);
                            delete fileWidget;
                        });
                        fileLayout->addWidget(deleteFile);
                    }
                    attachLayout->insertWidget(0, fileWidget); // always insert at the top
                };

                auto attachLinkWidget = new QWidget(sendingPage);
                auto attachLinkLayout = new QHBoxLayout(attachLinkWidget);
                Utils::grabTouchWidget(attachLinkWidget);
                attachLinkWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
                attachLinkWidget->setFixedWidth(Utils::scale_value(CONTROLS_WIDTH));
                attachLinkWidget->setCursor(Qt::PointingHandCursor);
                attachLinkLayout->setContentsMargins(0, Utils::scale_value(0), 0, 0);
                attachLinkLayout->setSpacing(Utils::scale_value(12));
                {
                    auto attachFilesRoutine = [=]()
                    {
                        attachSizeError->setVisible(false);
                        attachTotalSizeError->setVisible(false);
#ifdef __linux__
                        QWidget *parentForDialog = nullptr;
#else
                        QWidget *parentForDialog = scrollArea;
#endif //__linux__
                        static auto dd = QDir::homePath();
                        QFileDialog d(parentForDialog);
                        d.setDirectory(dd);
                        d.setFileMode(QFileDialog::ExistingFiles);
                        d.setNameFilter(QT_TRANSLATE_NOOP("contactus_page", "Images (*.jpg *.jpeg *.png *.bmp)"));
                        QStringList fileNames;
                        if (d.exec())
                        {
                            dd = d.directory().absolutePath();
                            auto selectedFiles = d.selectedFiles();
                            for (auto f: selectedFiles)
                            {
                                const auto rf = f;
                                const auto fileSize = QFileInfo(f).size();

                                if ((fileSize / 1024. / 1024.) > 1)
                                {
                                    attachSizeError->setVisible(true);
                                    continue;
                                }

                                filesSizeLimiter += fileSize;
                                if ((filesSizeLimiter / 1024. / 1024.) > 25)
                                {
                                    filesSizeLimiter -= fileSize;
                                    attachTotalSizeError->setVisible(true);
                                    break;
                                }

                                double fs = fileSize / 1024.f;
                                QString sizeString;
                                if (fs < 100)
                                {
                                    sizeString = QString("%1 %2").arg(QString::number(fs, 'f', 2)). arg(QT_TRANSLATE_NOOP("contactus_page", "KB"));
                                }
                                else
                                {
                                    fs /= 1024.;
                                    sizeString = QString("%1 %2").arg(QString::number(fs, 'f', 2)). arg(QT_TRANSLATE_NOOP("contactus_page", "MB"));
                                }

                                auto ls = f.lastIndexOf('/');
                                if (ls < 0 || ls >= f.length())
                                    ls = f.lastIndexOf('\\');
                                auto fsn = f.remove(0, ls + 1);

                                if (filesToSend.find(fsn) == filesToSend.end())
                                {
                                    //attachSizeError->setVisible(false);
                                    //attachTotalSizeError->setVisible(false);
                                    filesToSend.insert(std::make_pair(fsn, rf));
                                    bsc(fsn, sizeString, rf, fileSize);
                                }
                                else
                                {
                                    filesSizeLimiter -= fileSize;
                                }
                            }
                        }
                    };

                    auto attachImage = new QPushButton(attachLinkWidget);
                    attachImage->setFlat(true);
                    const QString addImageStyle = "QPushButton { border-image: url(:/resources/attach_image_100.png); } QPushButton:hover { border-image: url(:/resources/attach_image_100_hover.png); }";
                    Utils::ApplyStyle(attachImage, addImageStyle);
                    attachImage->setFixedSize(Utils::scale_value(24), Utils::scale_value(24));
                    scrollArea->connect(attachImage, &QPushButton::clicked, scrollArea, attachFilesRoutine);
                    attachLinkLayout->addWidget(attachImage);

                    auto attachLink = new TextEmojiWidget(attachLinkWidget, Fonts::appFontScaled(16), CommonStyle::getLinkColor(), Utils::scale_value(16));
                    Utils::grabTouchWidget(attachLink);
                    attachLink->setText(QT_TRANSLATE_NOOP("contactus_page", "Attach screenshot"));
                    scrollArea->connect(attachLink, &TextEmojiWidget::clicked, scrollArea, attachFilesRoutine);
                    attachLinkLayout->addWidget(attachLink);
                }
                attachLayout->addWidget(attachLinkWidget);
            }
            sendingPageLayout->addWidget(attachWidget);
        }
        {
            email = new LineEditEx(sendingPage);
            {
                auto f = Fonts::appFontScaled(18);
                email->setFont(f);
            }

            email->setContentsMargins(0, Utils::scale_value(16), 0, 0);
            email->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            email->setFixedWidth(Utils::scale_value(CONTROLS_WIDTH));
            email->setPlaceholderText(QT_TRANSLATE_NOOP("contactus_page", "Your Email"));
            email->setAutoFillBackground(false);

            QString communication_email = get_gui_settings()->get_value(settings_feedback_email, QString());
            QString aimid = MyInfo()->aimId();

            if (communication_email.isEmpty() && build::is_agent() && aimid.contains('@'))
                communication_email = aimid;
            email->setText(communication_email);
            email->setProperty("normal", true);
            Testing::setAccessibleName(email, "feedback_email");

            {
                emailerCover = new QWidget(email);
                Utils::grabTouchWidget(emailerCover);
                emailerCover->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                emailerCover->setFixedSize(email->minimumWidth(), email->minimumHeight());
                emailerCover->setVisible(false);
            }
            sendingPageLayout->addWidget(email);

            emailError = new TextEmojiWidget(sendingPage, Fonts::appFontScaled(16), ERROR_LABEL_COLOR, Utils::scale_value(12));
            Utils::grabTouchWidget(emailError);
            emailError->setText(QT_TRANSLATE_NOOP("contactus_page","Please enter a valid email address"));
            emailError->setVisible(false);
            sendingPageLayout->addWidget(emailError);

            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsShow, scrollArea, [=](int type)
            {
                if ((Utils::CommonSettingsType)type == Utils::CommonSettingsType::CommonSettingsType_ContactUs && !Utils::isValidEmailAddress(email->text()))
                    email->setText("");
                if (!email->property("normal").toBool())
                    Utils::ApplyPropertyParameter(email, "normal", true);
                emailError->setVisible(false);
            });
        }
        {
            auto sendWidget = new QWidget(sendingPage);
            auto sendLayout = new QHBoxLayout(sendWidget);
            Utils::grabTouchWidget(sendWidget);
            sendWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            sendLayout->setContentsMargins(0, Utils::scale_value(16), 0, 0);
            sendLayout->setSpacing(Utils::scale_value(12));
            sendLayout->setAlignment(Qt::AlignLeft);
            {
                auto sendButton = new QPushButton(sendWidget);
                sendButton->setFlat(true);
                sendButton->setText(QT_TRANSLATE_NOOP("contactus_page", "Send"));
                sendButton->setCursor(Qt::PointingHandCursor);
                sendButton->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
                sendButton->setMinimumWidth(Utils::scale_value(100));
                sendButton->setFixedHeight(Utils::scale_value(40));
                Testing::setAccessibleName(sendButton, "send_feedback");
                {
                    Utils::ApplyStyle(sendButton, (suggestioner->getPlainText().length() && email->text().length()) ? CommonStyle::getGreenButtonStyle() : CommonStyle::getDisabledButtonStyle());
                }
                sendLayout->addWidget(sendButton);
                auto updateSendButton = [=](bool state)
                {
                    Utils::ApplyStyle(sendButton, (state) ? CommonStyle::getGreenButtonStyle() : CommonStyle::getDisabledButtonStyle());
                };
                scrollArea->connect(suggestioner->document(), &QTextDocument::contentsChanged, scrollArea, [=]()
                {
                    bool state = suggestioner->getPlainText().length();
                    if (state && suggestioner->property("normal").toBool() != state)
                    {
                        Utils::ApplyPropertyParameter(suggestioner, "normal", state);
                        suggestionMinSizeError->setVisible(false);
                        suggestionMaxSizeError->setVisible(false);
                    }
                    updateSendButton(state && email->text().length());
                });
                scrollArea->connect(email, &QLineEdit::textChanged, scrollArea, [=](const QString &)
                {
                    bool state = email->text().length();
                    if (state && email->property("normal").toBool() != state)
                    {
                        Utils::ApplyPropertyParameter(email, "normal", state);
                        emailError->setVisible(false);
                    }
                    updateSendButton(suggestioner->getPlainText().length() && state);
                });

                auto errorOccuredSign = new TextEmojiWidget(sendWidget, Fonts::appFontScaled(16), ERROR_LABEL_COLOR, Utils::scale_value(16));
                Utils::grabTouchWidget(errorOccuredSign);
                errorOccuredSign->setText(QT_TRANSLATE_NOOP("contactus_page", "Error occured, try again later"));
                errorOccuredSign->setVisible(false);
                sendLayout->addWidget(errorOccuredSign);

                auto sendSpinner = new QLabel(sendWidget);
                auto abm = new QMovie(":/resources/gifs/r_spiner200.gif");
                Utils::grabTouchWidget(sendSpinner);
                sendSpinner->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                sendSpinner->setContentsMargins(0, 0, 0, 0);
                sendSpinner->setFixedSize(Utils::scale_value(40), Utils::scale_value(40));
                abm->setScaledSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));
                abm->start();
                sendSpinner->setMovie(abm);
                sendSpinner->setVisible(false);
                sendLayout->addWidget(sendSpinner);

                auto sendingSign = new TextEmojiWidget(sendWidget, Fonts::appFontScaled(16), SENDING_LABEL_COLOR, Utils::scale_value(16));
                Utils::grabTouchWidget(sendingSign);
                sendingSign->setText(QT_TRANSLATE_NOOP("contactus_page", "Sending..."));
                sendingSign->setVisible(false);
                sendLayout->addWidget(sendingSign);

                scrollArea->connect(sendButton, &QPushButton::pressed, scrollArea, [=]()
                {
                    get_gui_settings()->set_value(settings_feedback_email, email->text());

                    const auto sb = suggestioner->property("normal").toBool();
                    const auto eb = email->property("normal").toBool();
                    if (!sb)
                        suggestioner->setProperty("normal", true);
                    if (!eb)
                        email->setProperty("normal", true);
                    suggestionMinSizeError->setVisible(false);
                    suggestionMaxSizeError->setVisible(false);
                    emailError->setVisible(false);
                    attachSizeError->setVisible(false);
                    attachTotalSizeError->setVisible(false);
                    auto sg = suggestioner->getPlainText();
                    sg.remove(' ');
                    if (!sg.length())
                    {
                        suggestioner->setProperty("normal", false);
                    }
                    else if (suggestioner->getPlainText().length() < 1)
                    {
                        suggestioner->setProperty("normal", false);
                        suggestionMinSizeError->setVisible(true);
                    }
                    else if (suggestioner->getPlainText().length() > 2048)
                    {
                        suggestioner->setProperty("normal", false);
                        suggestionMaxSizeError->setVisible(true);
                    }
                    else if (!Utils::isValidEmailAddress(email->text()))
                    {
                        email->setProperty("normal", false);
                        emailError->setVisible(true);
                    }
                    else
                    {
                        sendButton->setVisible(false);
                        errorOccuredSign->setVisible(false);
                        sendSpinner->setVisible(true);
                        sendingSign->setVisible(true);

                        suggestioner->setEnabled(false);
                        attachWidget->setEnabled(false);
                        email->setEnabled(false);

                        suggestionerCover->setFixedSize(suggestioner->minimumWidth(), suggestioner->minimumHeight());
                        filerCover->setFixedSize(attachWidget->contentsRect().width(), attachWidget->contentsRect().height());
                        emailerCover->setFixedSize(email->minimumWidth(), email->minimumHeight());

                        suggestionerCover->setVisible(true);
                        filerCover->setVisible(true);
                        emailerCover->setVisible(true);

                        filerCover->raise();

                        Logic::getContactListModel()->getContactProfile("", [=](Logic::profile_ptr _profile, int32_t /*error*/)
                        {
                            if (_profile)
                            {
                                core::coll_helper col(GetDispatcher()->create_collection(), true);

                                col.set_value_as_string("url", (build::is_icq() ? "https://help.mail.ru/icqdesktop-support/all" : "https://help.mail.ru/agentdesktop-support/all"));

                                // fb.screen_resolution
                                col.set_value_as_string("screen_resolution", (QString("%1x%2").arg(qApp->desktop()->screenGeometry().width()).arg(qApp->desktop()->screenGeometry().height())).toStdString());
                                // fb.referrer
                                col.set_value_as_string("referrer", build::is_icq() ? "icq" : "agent");
                                {
                                    auto icqVer = (build::is_icq() ? QString("ICQ %1") : QString("Agent %1")).arg(VERSION_INFO_STR);
                                    auto osv = QSysInfo::prettyProductName();
                                    if (!osv.length() || osv == "unknown")
                                        osv = QString("%1 %2 (%3 %4)").arg(QSysInfo::productType()).arg(QSysInfo::productVersion()).arg(QSysInfo::kernelType()).arg(QSysInfo::kernelVersion());

                                    auto concat = (build::is_icq() ? QString("%1 %2 icq:%3") : QString("%1 %2 agent:%3")).arg(osv).arg(icqVer).arg(_profile ? _profile->get_aimid() : "");
                                    // fb.question.3004
                                    col.set_value_as_string("version", concat.toStdString());
                                    // fb.question.159
                                    col.set_value_as_string("os_version", osv.toStdString());
                                    // fb.question.178
                                    col.set_value_as_string("build_type", build::is_debug() ? "beta" : "live");
                                    // fb.question.3005
                                    if (platform::is_apple())
                                        col.set_value_as_string("platform", "OSx");
                                    else if (platform::is_windows())
                                        col.set_value_as_string("platform", "Windows");
                                    else if (platform::is_linux())
                                        col.set_value_as_string("platform", "Linux");
                                    else
                                        col.set_value_as_string("platform", "Unknown");
                                }
                                if (_profile)
                                {
                                    auto fn = QString("%1%2%3").arg(_profile->get_first_name()).arg(_profile->get_first_name().length() ? " " : "").arg(_profile->get_last_name());
                                    if (!fn.length())
                                    {
                                        if (_profile->get_contact_name().length())
                                            fn = _profile->get_contact_name();
                                        else if (_profile->get_displayid().length())
                                            fn = _profile->get_displayid();
                                        else if (_profile->get_friendly().length())
                                            fn = _profile->get_friendly();
                                    }
                                    // fb.user_name
                                    col.set_value_as_string("user_name", fn.toStdString());
                                }
                                else
                                {
                                    col.set_value_as_string("user_name", "");
                                }
                                // fb.message
                                col.set_value_as_string("message", suggestioner->getPlainText().toStdString());

                                // communication_email
                                col.set_value_as_string("communication_email", email->text().toStdString());
                                // Lang
                                col.set_value_as_string("language", QLocale::system().name().toStdString());
                                // attachements_count
                                col.set_value_as_string("attachements_count", QString::number((filesToSend.size() + 1)).toStdString()); // +1 'coz we're sending log txt
                                if (!filesToSend.empty())
                                {
                                    core::ifptr<core::iarray> farray(col->create_array());
                                    farray->reserve((int)filesToSend.size());
                                    for (const auto &f: filesToSend)
                                    {
                                        core::ifptr<core::ivalue> val(col->create_value());
                                        val->set_as_string(f.second.toUtf8().data(), (int)f.second.toUtf8().length());
                                        farray->push_back(val.get());
                                    }
                                    // fb.attachement
                                    col.set_value_as_array("attachement", farray.get());
                                }
                                GetDispatcher()->post_message_to_core("feedback/send", col.get());
                            }
                            else
                            {
                                emit GetDispatcher()->feedbackSent(false);
                            }
                        });
                    }
                    if (sb != suggestioner->property("normal").toBool())
                        Utils::ApplyStyle(suggestioner, suggestioner->styleSheet());
                    if (eb != email->property("normal").toBool())
                        Utils::ApplyStyle(email, email->styleSheet());
                });

                scrollArea->connect(GetDispatcher(), &core_dispatcher::feedbackSent, scrollArea, [=](bool succeeded)
                {
                    sendButton->setVisible(true);
                    sendSpinner->setVisible(false);
                    sendingSign->setVisible(false);

                    suggestioner->setEnabled(true);
                    attachWidget->setEnabled(true);
                    email->setEnabled(true);

                    suggestionerCover->setVisible(false);
                    filerCover->setVisible(false);
                    emailerCover->setVisible(false);

                    if (!succeeded)
                    {
                        errorOccuredSign->setVisible(true);
                    }
                    else
                    {
                        for (auto p: filesToSend)
                        {
                            auto f = attachWidget->findChild<QWidget *>(p.first);
                            if (f)
                            {
                                f->setVisible(false);
                                delete f;
                            }
                        }
                        filesToSend.clear();
                        filesSizeLimiter = 0;

                        suggestioner->setText("");

                        sendingPage->setVisible(false);
                        successPage->setVisible(true);
                    }
                });

            }
            sendingPageLayout->addWidget(sendWidget);
        }
        mainLayout->addWidget(sendingPage);

        auto successPageLayout = Utils::emptyVLayout(successPage);
        successPage->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);
        successPageLayout->setAlignment(Qt::AlignCenter);
        {
            auto successWidget = new QWidget(successPage);
            Utils::grabTouchWidget(successWidget);
            auto successImageLayout = Utils::emptyVLayout(successWidget);
            successWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            successImageLayout->setAlignment(Qt::AlignCenter);
            {
                auto successImage = new QPushButton(successWidget);
                successImage->setObjectName("successImage");
                successImage->setFlat(true);
                successImage->setFixedSize(Utils::scale_value(120), Utils::scale_value(136));
                successImageLayout->addWidget(successImage);
            }
            successPageLayout->addWidget(successWidget);

            auto thanksWidget = new QWidget(successPage);
            auto thanksLayout = Utils::emptyVLayout(thanksWidget);
            Utils::grabTouchWidget(thanksWidget);
            thanksWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            thanksLayout->setAlignment(Qt::AlignCenter);
            {
                auto thanksLabel = new TextEmojiWidget(thanksWidget, Fonts::appFontScaled(16), CommonStyle::getTextCommonColor(), Utils::scale_value(32));
                thanksLabel->setText(QT_TRANSLATE_NOOP("contactus_page", "Thank you for contacting us!"));
                thanksLabel->setSizePolicy(QSizePolicy::Policy::Preferred, thanksLabel->sizePolicy().verticalPolicy());
                thanksLayout->addWidget(thanksLabel);
            }
            successPageLayout->addWidget(thanksWidget);

            auto resendWidget = new QWidget(successPage);
            auto resendLayout = Utils::emptyVLayout(resendWidget);
            Utils::grabTouchWidget(resendWidget);
            resendWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            resendLayout->setAlignment(Qt::AlignCenter);
            {
                auto resendLink = new TextEmojiWidget(resendWidget, Fonts::appFontScaled(16), CommonStyle::getLinkColor(), Utils::scale_value(32));
                resendLink->setText(QT_TRANSLATE_NOOP("contactus_page", "Send another review"));
                resendLink->setSizePolicy(QSizePolicy::Policy::Preferred, resendLink->sizePolicy().verticalPolicy());
                resendLink->setCursor(Qt::PointingHandCursor);
                scrollArea->connect(resendLink, &TextEmojiWidget::clicked, scrollArea, [=]()
                {
                    successPage->setVisible(false);
                    sendingPage->setVisible(true);
                });
                resendLayout->addWidget(resendLink);
            }
            successPageLayout->addWidget(resendWidget);
        }
        successPage->setVisible(false);
        mainLayout->addWidget(successPage);
    }
}
