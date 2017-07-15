#include "stdafx.h"

#include "GroupChatOperations.h"
#include "../core_dispatcher.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/ChatMembersModel.h"
#include "../utils/utils.h"
#include "../my_info.h"
#include "../utils/gui_coll_helper.h"
#include "MainPage.h"
#include "../gui_settings.h"
#include "contact_list/ContactList.h"
#include "contact_list/SelectionContactsForGroupChat.h"

#include "history_control/MessageStyle.h"
#include "../utils/InterConnector.h"
#include "../controls/CommonStyle.h"
#include "../controls/GeneralDialog.h"
#include "../controls/TextEditEx.h"
#include "../controls/ContactAvatarWidget.h"
#include "MainWindow.h"

namespace Ui
{
    GroupChatSettings::GroupChatSettings(QWidget *parent, const QString &buttonText, const QString &headerText, GroupChatOperations::ChatData &chatData): editorIsShown_(false), chatData_(chatData)
    {
        content_ = new QWidget(parent);
        content_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        content_->setMinimumWidth(Utils::scale_value(360));
        Utils::ApplyStyle(content_, "height: 10dip;");

        auto layout = Utils::emptyVLayout(content_);
        layout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(16), Utils::scale_value(24), 0);
        
        // name and photo
        auto subContent = new QWidget(content_);
        subContent->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        auto subLayout = new QHBoxLayout(subContent);
        subLayout->setSpacing(Utils::scale_value(16));
        subLayout->setContentsMargins(0, 0, 0, Utils::scale_value(16));
        subLayout->setAlignment(Qt::AlignTop);
        {
            photo_ = new ContactAvatarWidget(subContent, QString(), QString(), Utils::scale_value(56), true);
            photo_->SetMode(ContactAvatarWidget::Mode::CreateChat);
            photo_->SetVisibleShadow(false);
            photo_->SetVisibleSpinner(false);
            subLayout->addWidget(photo_);
            
            chatName_ = new TextEditEx(subContent, Fonts::appFontScaled(18), MessageStyle::getTextColor(), true, true);
            Utils::ApplyStyle(chatName_, CommonStyle::getLineEditStyle());
            chatName_->setWordWrapMode(QTextOption::NoWrap);//WrapAnywhere);
            chatName_->setObjectName("chat_name");
            chatName_->setPlaceholderText(QT_TRANSLATE_NOOP("groupchats", "Chat name"));
            chatName_->setPlainText("");//chatData.name);
            chatName_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            chatName_->setAutoFillBackground(false);
            chatName_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            chatName_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            chatName_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            chatName_->setCatchEnter(true);
            chatName_->setMinimumWidth(Utils::scale_value(200));
            Utils::ApplyStyle(chatName_, "padding: 0; margin: 0;");
            {
                auto chatNameLayout = Utils::emptyVLayout(chatName_);
                chatNameLayout->setAlignment(Qt::AlignBottom);
                
                auto horizontalLineWidget = new QWidget(chatName_);
                horizontalLineWidget->setFixedHeight(Utils::scale_value(1));
                Utils::ApplyStyle(horizontalLineWidget, "background-color: #579e1c;");
                chatNameLayout->addWidget(horizontalLineWidget);
            }
            subLayout->addWidget(chatName_);
        }
        layout->addWidget(subContent);
        
        // settings
        static auto createItem = [](QWidget *parent, const QString &iconPath, const QString &topic, const QString &text, QCheckBox *&switcher)
        {
            auto whole = new QWidget(parent);
            auto wholelayout = new QHBoxLayout(whole);
            whole->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            wholelayout->setContentsMargins(0, 0, 0, Utils::scale_value(8));
            {
                auto iconpart = new QWidget(whole);
                auto iconpartlayout = Utils::emptyVLayout(iconpart);
                iconpart->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
                iconpartlayout->setContentsMargins(0, Utils::scale_value(4), 0, 0);
                iconpartlayout->setAlignment(Qt::AlignTop);
                Utils::ApplyStyle(iconpart, "background-color: transparent; border: none;");
                {
                    auto i = new QPushButton(iconpart);
                    i->setFlat(true);
                    i->setObjectName("icon");
                    i->setFixedSize(Utils::scale_value(24), Utils::scale_value(24));
                    Utils::ApplyStyle(i, QString("QPushButton { image: url(%1); border: none; }").arg(iconPath));
                    iconpartlayout->addWidget(i);
                }
                wholelayout->addWidget(iconpart);
                
                auto textpart = new QWidget(whole);
                auto textpartlayout = Utils::emptyVLayout(textpart);
                textpart->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
                textpartlayout->setAlignment(Qt::AlignTop);
                Utils::ApplyStyle(textpart, "background-color: transparent; border: none;");
                {
                    {
                        auto t = new TextEditEx(textpart, Fonts::appFontScaled(16), CommonStyle::getTextCommonColor(), false, true);
                        t->setPlainText(topic, false, QTextCharFormat::AlignNormal);
                        t->setObjectName("topic");
                        t->setAlignment(Qt::AlignLeft);
                        t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                        t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                        t->setFrameStyle(QFrame::NoFrame);
                        t->setContentsMargins(0, 0, 0, 0);
                        t->setContextMenuPolicy(Qt::NoContextMenu);
                        textpartlayout->addWidget(t);
                    }
                    {
                        auto t = new TextEditEx(textpart, Fonts::appFontScaled(13), QColor("#767676"), false, true);
                        t->setPlainText(text, false, QTextCharFormat::AlignNormal);
                        t->setObjectName("text");
                        t->setAlignment(Qt::AlignLeft);
                        t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                        t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                        t->setFrameStyle(QFrame::NoFrame);
                        t->setContentsMargins(0, 0, 0, 0);
                        t->setContextMenuPolicy(Qt::NoContextMenu);
                        textpartlayout->addWidget(t);
                    }
                }
                wholelayout->addWidget(textpart);
                
                auto switchpart = new QWidget(whole);
                auto switchpartlayout = Utils::emptyVLayout(switchpart);
                switchpart->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
                switchpartlayout->setAlignment(Qt::AlignTop);
                Utils::ApplyStyle(switchpart, "background-color: transparent;");
                {
                    switcher = new QCheckBox(switchpart);
                    switcher->setObjectName("greenSwitcher");
                    switcher->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                    switcher->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
                    switcher->setChecked(false);
                    switchpartlayout->addWidget(switcher);
                }
                wholelayout->addWidget(switchpart);
            }
            return whole;
        };
        QCheckBox *publicChat;
        layout->addWidget(createItem(content_,
                                     ":/resources/livechat_public_100.png",
                                     QT_TRANSLATE_NOOP("groupchats", "Public chat"),
                                     QT_TRANSLATE_NOOP("groupchats", "Chat will be visible to everyone"),
                                     publicChat));
        publicChat->setChecked(chatData.publicChat);
        connect(publicChat, &QCheckBox::toggled, publicChat, [&chatData](bool checked){ chatData.publicChat = checked; });
        QCheckBox *approvedJoin;
        layout->addWidget(createItem(content_,
                                     ":/resources/livechat_approve_100.png",
                                     QT_TRANSLATE_NOOP("groupchats", "Join with Approval"),
                                     QT_TRANSLATE_NOOP("groupchats", "New members are waiting for admin approval"),
                                     approvedJoin));
        approvedJoin->setChecked(chatData.approvedJoin);
        connect(approvedJoin, &QCheckBox::toggled, approvedJoin, [&chatData](bool checked){ chatData.approvedJoin = checked; });

        QCheckBox *readOnly;
        layout->addWidget(createItem(content_,
                                     ":/resources/livechat_readonly_100.png",
                                     QT_TRANSLATE_NOOP("groupchats", "Read only"),
                                     QT_TRANSLATE_NOOP("groupchats", "New members can read, approved by admin members can send"),
                                     readOnly));
        readOnly->setChecked(chatData.readOnly);
        connect(readOnly, &QCheckBox::toggled, readOnly, [&chatData](bool checked){ chatData.readOnly = checked; });

        // general dialog
        dialog_.reset(new Ui::GeneralDialog(content_, Utils::InterConnector::instance().getMainWindow()));
        dialog_->setObjectName("chat.creation.settings");
        dialog_->addHead();
        dialog_->addLabel(headerText);
        dialog_->addAcceptButton(buttonText, Utils::scale_value(24), true);
        
        QObject::connect(chatName_, SIGNAL(enter()), dialog_.get(), SLOT(accept()));
        
        QTextCursor cursor = chatName_->textCursor();
        cursor.select(QTextCursor::Document);
        chatName_->setTextCursor(cursor);
        chatName_->setFrameStyle(QFrame::NoFrame);
        
        chatName_->setFocus();
        
        // editor
        auto hollow = new QWidget(dialog_->parentWidget());
        hollow->setObjectName("hollow");
        hollow->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        Utils::ApplyStyle(hollow, "background-color: transparent; margin: 0; padding: 0; border: none;");
        {
            auto hollowlayout = Utils::emptyVLayout(hollow);
            hollowlayout->setAlignment(Qt::AlignLeft);
            
            auto editor = new QWidget(hollow);
            editor->setObjectName("editor");
            editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            Utils::ApplyStyle(editor, "background-color: #fffff; margin: 0; padding: 0dip; border: none;");
            photo_->SetImageCropHolder(editor);
            {
                auto effect = new QGraphicsOpacityEffect(editor);
                effect->setOpacity(.0);
                editor->setGraphicsEffect(effect);
            }
            hollowlayout->addWidget(editor);
            
            // events

            QWidget::connect(photo_, &ContactAvatarWidget::avatarDidEdit, this, [=]()
            {
                lastCroppedImage_ = photo_->croppedImage();
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogIsShown, this, [=](QWidget *p)
            {
                if (!editorIsShown_)
                    showImageCropDialog();
                editorIsShown_ = true;
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogIsHidden, this, [=](QWidget *p)
            {
                if (editorIsShown_)
                    showImageCropDialog();
                editorIsShown_ = false;
            }, Qt::QueuedConnection);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::imageCropDialogResized, this, [=](QWidget *p)
            {
                editor->setFixedSize(p->size());
                p->move(0, 0);
            }, Qt::QueuedConnection);

            connect(dialog_.get(), &GeneralDialog::moved, this, [=](QWidget *p)
            {
                hollow->move(dialog_->x() + dialog_->width(), dialog_->y());
            }, Qt::QueuedConnection);
            connect(dialog_.get(), &GeneralDialog::resized, this, [=](QWidget *p)
            {
                hollow->setFixedSize(hollow->parentWidget()->width() - dialog_->x() + dialog_->width(), dialog_->height());
                const auto preferredWidth = (dialog_->parentWidget()->geometry().size().width() - dialog_->geometry().size().width() - Utils::scale_value(24)*4);
                photo_->SetImageCropSize(QSize(preferredWidth, content_->height()));
            }, Qt::QueuedConnection);
            connect(dialog_.get(), &GeneralDialog::hidden, this, [=](QWidget *p)
            {
                emit Utils::InterConnector::instance().closeAnyPopupWindow();
                emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
            }, Qt::QueuedConnection);
        }
        hollow->show();
    }
    
    GroupChatSettings::~GroupChatSettings()
    {
        //
    }

    bool GroupChatSettings::show()
    {
        if (!dialog_)
        {
            return false;
        }
        auto result = dialog_->showInPosition(-1, -1);
        if (!chatName_->getPlainText().isEmpty())
            chatData_.name = chatName_->getPlainText();
        return result;
    }

    void GroupChatSettings::showImageCropDialog()
    {
        auto editor = dialog_->parentWidget()->findChild<QWidget *>("editor");
        if (!editor)
            return;
        auto hollow = dialog_->parentWidget()->findChild<QWidget *>("hollow");
        if (!hollow)
            return;
        
        static const auto time = 200;
        const auto needHideEditor = editorIsShown_;
        editorIsShown_ = !editorIsShown_;
        // move settings
        {
            auto geometry = new QPropertyAnimation(dialog_.get(), "geometry", Utils::InterConnector::instance().getMainWindow());
            geometry->setDuration(time);
            auto rect = dialog_->geometry();
            geometry->setStartValue(rect);
            if (needHideEditor)
            {
                const auto screenRect = Utils::InterConnector::instance().getMainWindow()->geometry();
                const auto dialogRect = dialog_->geometry();
                rect.setX((screenRect.size().width() - dialogRect.size().width()) / 2);
            }
            else
            {
                const auto screenRect = Utils::InterConnector::instance().getMainWindow()->geometry();
                const auto dialogRect = dialog_->geometry();
                const auto editorRect = editor->geometry();
                const auto hollowRect = hollow->geometry();
                const auto between = (hollowRect.left() - dialogRect.right());
//                rect.setX((screenRect.size().width() - dialogRect.size().width() - editorRect.size().width() - Utils::scale_value(24)) / 2);
                rect.setX((screenRect.size().width() - dialogRect.size().width() - editorRect.size().width()) / 2 - between);
            }
            geometry->setEndValue(rect);
            geometry->start(QAbstractAnimation::DeleteWhenStopped);
        }
        // fade editor
        {
            auto effect = new QGraphicsOpacityEffect(Utils::InterConnector::instance().getMainWindow());
            editor->setGraphicsEffect(effect);
            auto fading = new QPropertyAnimation(effect, "opacity", Utils::InterConnector::instance().getMainWindow());
            fading->setEasingCurve(QEasingCurve::InOutQuad);
            fading->setDuration(time);
            if (needHideEditor)
            {
                fading->setStartValue(1.0);
                fading->setEndValue(0.0);
                fading->connect(fading, &QPropertyAnimation::finished, fading, [editor, hollow]() { editor->setFixedSize(hollow->size()); });
            }
            else
            {
                fading->setStartValue(0.0);
                fading->setEndValue(1.0);
                fading->connect(fading, &QPropertyAnimation::finished, fading, [editor]() { Utils::addShadowToWidget(editor); });
            }
            fading->start(QPropertyAnimation::DeleteWhenStopped);
        }
    }

    void createGroupChat(QStringList _members_aimIds)
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_open);

        SelectContactsWidget select_members_dialog(nullptr, Logic::MembersWidgetRegim::SELECT_MEMBERS, QT_TRANSLATE_NOOP("groupchats", "Select members"), QT_TRANSLATE_NOOP("groupchats", "Next"), QString(), Ui::MainPage::instance());
        emit Utils::InterConnector::instance().searchEnd();

        for (auto& member_aimId : _members_aimIds)
        {
            Logic::getContactListModel()->setChecked(member_aimId, true /* is_checked */);
        }
        
        if (select_members_dialog.show() == QDialog::Accepted)
        {
            //select_members_dialog.close();
//            if (Logic::getContactListModel()->GetCheckedContacts().size() == 1)
//            {
//                const auto& aimid = Logic::getContactListModel()->GetCheckedContacts()[0].get_aimid();
//                Ui::MainPage::instance()->selectRecentChat(aimid);
//            }
//            else
            {
                std::shared_ptr<GroupChatSettings> groupChatSettings;
                GroupChatOperations::ChatData chatData;
                if (callChatNameEditor(Ui::MainPage::instance(), chatData, groupChatSettings))
                {
                    const auto cropped = groupChatSettings->lastCroppedImage();
                    if (!cropped.isNull())
                    {
                        std::shared_ptr<QMetaObject::Connection> connection(new QMetaObject::Connection());
                        *connection = QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setAvatarId, [connection, chatData](QString avatarId)
                        {
                            if (!avatarId.isEmpty())
                                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_avatar);
                            Ui::MainPage::instance()->openCreatedGroupChat();
                            postCreateChatInfoToCore(MyInfo()->aimId(), chatData, avatarId);
                            Logic::getContactListModel()->clearChecked();
                            Ui::MainPage::instance()->clearSearchMembers();
                            QWidget::disconnect(*connection);
                        });
                        groupChatSettings->photo()->UpdateParams("", "");
                        groupChatSettings->photo()->applyAvatar(cropped);
                    }
                    else
                    {
                        Ui::MainPage::instance()->openCreatedGroupChat();
                        postCreateChatInfoToCore(MyInfo()->aimId(), chatData);
                        Logic::getContactListModel()->clearChecked();
                        Ui::MainPage::instance()->clearSearchMembers();
                    }
                }
                else
                {
                    Logic::getContactListModel()->clearChecked();
                    Ui::MainPage::instance()->clearSearchMembers();
                }
            }
        }
    }

    bool callChatNameEditor(QWidget* _parent, GroupChatOperations::ChatData &chatData, Out std::shared_ptr<GroupChatSettings> &groupChatSettings)
    {
        auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
        chatData.name = MyInfo()->friendlyName();
        if (chatData.name.isEmpty())
            chatData.name = MyInfo()->aimId();
        for (int i = 0, contactsSize = std::min((int)selectedContacts.size(), 2); i < contactsSize; ++i)
        {
            chatData.name += ", " + selectedContacts[i].Get()->GetDisplayName();
        }
        auto chat_name = chatData.name;
        {
            groupChatSettings.reset(new GroupChatSettings(_parent, QT_TRANSLATE_NOOP("groupchats","Done"), QT_TRANSLATE_NOOP("popup_window", "Chat settings"), chatData));
            auto result = groupChatSettings->show();
            if (chat_name != chatData.name)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_rename);
            chatData.name = chatData.name.isEmpty() ? chat_name : chatData.name;
            return result;
        }
    }

    void postCreateChatInfoToCore(const QString &_aimId, const GroupChatOperations::ChatData &chatData, QString avatarId)
    {
        if (chatData.publicChat)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_public);
        if (chatData.approvedJoin)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_approval);
        if (chatData.readOnly)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_create_readonly);
        
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("aimid", _aimId.toUtf8().data(), _aimId.toUtf8().size());
        collection.set_value_as_string("name", chatData.name.toUtf8().data(), chatData.name.toUtf8().size());
        collection.set_value_as_string("avatar", avatarId.toUtf8().data(), avatarId.toUtf8().size());
        {
            QStringList chat_members;
            auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
            for (const auto& contact : selectedContacts)
            {
                chat_members.push_back(contact.get_aimid());
            }
            core::ifptr<core::iarray> members_array(collection->create_array());
            members_array->reserve((int)chat_members.size());
            for (int i = 0; i < chat_members.size(); ++i)
            {
                auto member = chat_members[i];
                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_string(member.toStdString().c_str(), (int)member.length());
                members_array->push_back(val.get());
            }
            collection.set_value_as_array("members", members_array.get());
        }
        collection.set_value_as_bool("public", chatData.publicChat);
        collection.set_value_as_bool("approved", chatData.approvedJoin);
        collection.set_value_as_bool("link", chatData.joiningByLink);
        collection.set_value_as_bool("ro", chatData.readOnly);
        collection.set_value_as_bool("age", chatData.ageGate);
        Ui::GetDispatcher()->post_message_to_core("chats/create", collection.get());
    }

    void postAddChatMembersFromCLModelToCore(QString _aimId)
    {
        auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
        Logic::getContactListModel()->clearChecked();

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        QStringList chat_members;
        for (const auto& contact : selectedContacts)
        {
            chat_members.push_back(contact.get_aimid());
        }
        collection.set_value_as_string("aimid", _aimId.toUtf8().data(), _aimId.toUtf8().size());
        collection.set_value_as_string("m_chat_members_to_add", chat_members.join(";").toStdString());
        Ui::GetDispatcher()->post_message_to_core("add_members", collection.get());
    }

    void deleteMemberDialog(Logic::ChatMembersModel* _model, const QString& current, int _regim, QWidget* _parent)
    {
        QString text;
        
        const bool isVideoConference = _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;

        if (_regim == Logic::MembersWidgetRegim::MEMBERS_LIST || _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from this chat?");
        else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from ignore list?");
        else if (isVideoConference)
            text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from this call?");

        auto left_button_text = QT_TRANSLATE_NOOP("popup_window", "Cancel");
        auto right_button_text = QT_TRANSLATE_NOOP("popup_window", "Delete");

        auto member = _model->getMemberItem(current);
        if (!member)
        {
            assert(!"member not found in model, probably it need to refresh");
            return;
        }
        auto user_name = member->NickName_;
        auto label = user_name.isEmpty() ? member->AimId_ : user_name;

        if (Utils::GetConfirmationWithTwoButtons(left_button_text, right_button_text, text, label, _parent,
            isVideoConference ? _parent->parentWidget() : Utils::InterConnector::instance().getMainWindow()))
        {
            if (_regim == Logic::MembersWidgetRegim::MEMBERS_LIST || _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS)
            {
                auto aimId_ = _model->getChatAimId();
                ::Ui::gui_coll_helper collection(::Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_string("aimid", aimId_.toUtf8().data(), aimId_.toUtf8().size());
                collection.set_value_as_string("m_chat_members_to_remove", current.toStdString());
                Ui::GetDispatcher()->post_message_to_core("remove_members", collection.get());
            }
            else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            {
                Logic::getContactListModel()->ignoreContact(current, false);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_remove);
                // NOTE : when delete from ignore list, we don't need add contact to CL
            }
            else if (isVideoConference)
            {
                Ui::GetDispatcher()->getVoipController().setDecline(current.toStdString().c_str(), false);
            }
        }
    }
    


    void forwardMessage(QList<Data::Quote> quotes, bool fromMenu)
    {
        SelectContactsWidget shareDialog(QT_TRANSLATE_NOOP("popup_window", "Forward"), Ui::MainPage::instance());
        shareDialog.setSort(false /* isClSorting */);
        
        emit Utils::InterConnector::instance().searchEnd();
        const auto action = shareDialog.show();
        if (action == QDialog::Accepted)
        {
            const auto contact = shareDialog.getSelectedContact();
            
            if (contact != "")
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set<QString>("contact", contact);
                collection.set_value_as_string("message", "");
                if (!quotes.isEmpty())
                {
                    core::ifptr<core::iarray> quotesArray(collection->create_array());
                    quotesArray->reserve(quotes.size());
                    for (auto quote : quotes)
                    {
                        core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                        quote.isForward_ = true;
                        quote.serialize(quoteCollection.get());
                        core::coll_helper coll(collection->create_collection(), true);
                        core::ifptr<core::ivalue> val(collection->create_value());
                        val->set_as_collection(quoteCollection.get());
                        quotesArray->push_back(val.get());
                    }
                    collection.set_value_as_array("quotes", quotesArray.get());
                }
                
                Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());

                core::stats::event_props_type props;
                props.push_back(std::make_pair("Forward_MessagesCount", std::to_string(quotes.size())));
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::forward_messagescount, props);

                Ui::GetDispatcher()->post_stats_to_core(fromMenu ? core::stats::stats_event_names::forward_send_frommenu : core::stats::stats_event_names::forward_send_frombutton);
                
                Logic::getContactListModel()->setCurrent(contact, -1, true);

                Utils::InterConnector::instance().onSendMessage(contact);
            }
        }
    }

    void forwardMessage(const QString& _message, bool fromMenu)
    {
        SelectContactsWidget shareDialog(QT_TRANSLATE_NOOP("popup_window", "Forward"), Ui::MainPage::instance());
        shareDialog.setSort(false /* isClSorting */);

        emit Utils::InterConnector::instance().searchEnd();
        const auto action = shareDialog.show();
        if (action == QDialog::Accepted)
        {
            const auto contact = shareDialog.getSelectedContact();

            if (!contact.isEmpty())
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

                collection.set<QString>("contact", contact);
                collection.set_value_as_string("message", _message.toUtf8().data(), _message.toUtf8().size());

                Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());

                core::stats::event_props_type props;
                props.push_back(std::make_pair("Forward_MessagesCount", "1"));
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::forward_messagescount, props);
                Ui::GetDispatcher()->post_stats_to_core(fromMenu ? core::stats::stats_event_names::forward_send_frommenu : core::stats::stats_event_names::forward_send_frombutton);

                Logic::getContactListModel()->setCurrent(contact, -1, true);

                Utils::InterConnector::instance().onSendMessage(contact);
            }
        }
    }

}
