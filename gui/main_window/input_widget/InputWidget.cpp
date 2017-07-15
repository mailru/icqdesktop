#include "stdafx.h"
#include "InputWidget.h"

#include "../ContactDialog.h"
#include "../history_control/MessageStyle.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/PictureWidget.h"
#include "../history_control/KnownFileTypes.h"
#include "../../themes/ThemePixmap.h"
#include "../../main_window/MainWindow.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../main_window/history_control/MessagesModel.h"
#include "../../main_window/sounds/SoundsManager.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"

#ifdef __APPLE__
#   include "../../utils/SChar.h"
#endif

const int widget_max_height = 230;
const int document_min_height = 32;
const int quote_line_width = 2;
const int max_quote_height = 66;
const int preview_max_height = 48;
const int preview_offset = 16;
const int preview_max_width = 132;
const int preview_offset_hor = 18;
const int preview_text_offset = 16;
const int icon_size = 32;

namespace Ui
{
    input_edit::input_edit(QWidget* _parent)
        : TextEditEx(_parent, MessageStyle::getTextFont(), CommonStyle::getTextCommonColor(), true, false)
    {
        limitCharacters(Utils::getInputMaximumChars());
        setUndoRedoEnabled(true);
    }

    bool input_edit::catchEnter(int _modifiers)
    {
        int key1_to_send = get_gui_settings()->get_value<int>(settings_key1_to_send_message, 0);

         // spike for fix invalid setting
        if (key1_to_send == 1)
        {
            key1_to_send = 0;
        }


        if (key1_to_send != 0)
        {
            bool b1 = ((_modifiers & Qt::ControlModifier) == Qt::ControlModifier) && (key1_to_send == Qt::Key_Control);
            bool b2 = ((_modifiers & Qt::ShiftModifier) == Qt::ShiftModifier) && (key1_to_send == Qt::Key_Shift);
            bool b3 = (_modifiers == Qt::Key_Enter) && ((_modifiers & Qt::Key_Enter) == Qt::Key_Enter) && (key1_to_send == Qt::Key_Enter);
            if (!b1 && !b2 && !b3)
                return false;
        }
        else if (_modifiers != Qt::NoModifier && _modifiers != Qt::KeypadModifier)
        {
            return false;
        }

        return true;
    }
    
    InputWidget::InputWidget(QWidget* parent, int height, const QString& styleSheet, bool messageOnly)
        : QWidget(parent)
        , text_edit_(new input_edit(this))
        , smiles_button_(new QPushButton(this))
        , send_button_(new QPushButton(this))
        , file_button_(new QPushButton(this))
        , anim_height_(0)
        , is_initializing_(false)
        , message_only_(messageOnly)
    {
        default_height_ = height == -1 ? Ui::CommonStyle::getBottomPanelHeight() : height;
        active_height_ = default_height_;
        need_height_ = active_height_;

        setVisible(false);
        setStyleSheet(Utils::LoadStyle(styleSheet.isEmpty() ? ":/main_window/input_widget/input_widget.qss" : styleSheet));

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto vLayout = Utils::emptyVLayout();
        
        quote_block_ = new QWidget(this);
        quote_block_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto quoteLayout = Utils::emptyHLayout(quote_block_);
        quoteLayout->setAlignment(Qt::AlignVCenter);
        quoteLayout->setContentsMargins(Utils::scale_value(52), Utils::scale_value(12), Utils::scale_value(16), 0);

        quote_line_ = new QWidget(quote_block_);
        quote_line_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        quote_line_->setStyleSheet(Utils::ScaleStyle("background: #579e1c;", Utils::getScaleCoefficient()));
        quote_line_->setFixedWidth(Utils::scale_value(quote_line_width));
        quoteLayout->addWidget(quote_line_);
        quote_text_widget_ = new QWidget(quote_block_);
        auto quote_text_layout = Utils::emptyVLayout(quote_text_widget_);
        quote_text_layout->setContentsMargins(Utils::scale_value(10), Utils::scale_value(4), 0, 0);

        quote_text_ = new TextEditEx(quote_block_, Fonts::appFontScaled(12), QColor(), false, false);
        quote_text_->setSize(QSizePolicy::Preferred, QSizePolicy::Preferred);
        quote_text_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        quote_text_->setStyleSheet(Utils::ScaleStyle("background: %1;", Utils::getScaleCoefficient())
            .arg(Utils::rgbaStringFromColor(CommonStyle::getBottomPanelColor())));
        quote_text_->setFrameStyle(QFrame::NoFrame);
        quote_text_->document()->setDocumentMargin(0);
        quote_text_->setOpenLinks(true);
        quote_text_->setOpenExternalLinks(true);
        quote_text_->setWordWrapMode(QTextOption::NoWrap);
        quote_text_->setContextMenuPolicy(Qt::NoContextMenu);

        QPalette p = quote_text_->palette();
        p.setColor(QPalette::Text, QColor("#767676"));
        p.setColor(QPalette::Link, QColor("#579e1c"));
        p.setColor(QPalette::LinkVisited, QColor("#579e1c"));
        quote_text_->setPalette(p);
        quote_text_layout->addWidget(quote_text_);
        quoteLayout->addWidget(quote_text_widget_);
        cancel_quote_ = new QPushButton(quote_block_);
        cancel_quote_->setObjectName("cancel_quote");
        cancel_quote_->setCursor(QCursor(Qt::PointingHandCursor));
        quoteLayout->addWidget(cancel_quote_);
        vLayout->addWidget(quote_block_);
        quote_block_->hide();

        auto hLayout = Utils::emptyHLayout();

        auto smiles_layout = Utils::emptyVLayout();
        smiles_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));

        smiles_button_->setObjectName("smiles_button");
        smiles_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        smiles_button_->setCursor(QCursor(Qt::PointingHandCursor));
        smiles_button_->setFocusPolicy(Qt::NoFocus);

        smiles_layout->addWidget(smiles_button_);
        smiles_layout->addSpacerItem(new QSpacerItem(0, default_height_ / 2 - Utils::scale_value(icon_size) / 2, QSizePolicy::Preferred, QSizePolicy::Fixed));

        cancel_files_ = new QPushButton(this);
        cancel_files_->setObjectName("cancel_files");
        cancel_files_->setCursor(QCursor(Qt::PointingHandCursor));
        hLayout->addLayout(smiles_layout);
        hLayout->addWidget(cancel_files_);

        connect(cancel_files_, SIGNAL(clicked()), this, SLOT(onFilesCancel()), Qt::QueuedConnection);

        cancel_files_->hide();

        setObjectName("input_edit_control");
        if (message_only_)
            text_edit_->setObjectName(platform::is_apple() ? "input_edit_control_mac" : "input_edit_control");
        else
            text_edit_->setObjectName("input_edit_control");

        text_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("input_widget","Message"));
        text_edit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        text_edit_->setAutoFillBackground(false);
        text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        text_edit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        hLayout->addWidget(text_edit_);

        files_block_ = new QWidget(this);
        files_block_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto filesLayout = Utils::emptyHLayout(files_block_);
        filesLayout->setContentsMargins(Utils::scale_value(preview_offset), 0, 0, 0);
        file_preview_ = new PictureWidget(files_block_);
        file_preview_->setObjectName("files_preview");
        file_preview_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        filesLayout->addWidget(file_preview_);
        filesLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(preview_offset), 0, QSizePolicy::Fixed));
        files_label_ = new QLabel(this);
        files_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QPalette pal;
        files_label_->setFont(Fonts::appFontScaled(16));
        pal.setColor(QPalette::Foreground, QColor("#999999"));
        files_label_->setPalette(pal);
        filesLayout->addWidget(files_label_);
        hLayout->addWidget(files_block_);
        files_block_->hide();

        input_disabled_block_ = new QWidget(this);
        input_disabled_block_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto disabled_layout = Utils::emptyHLayout(input_disabled_block_);
        disabled_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        disable_label_ = new QLabel(this);
        disable_label_->setFont(Fonts::appFontScaled(16));
        pal.setColor(QPalette::Foreground, QColor("#999999"));
        disable_label_->setPalette(pal);
        disable_label_->setAlignment(Qt::AlignCenter);
        disable_label_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        disable_label_->setWordWrap(true);
        disabled_layout->addWidget(disable_label_);
        connect(disable_label_, SIGNAL(linkActivated(const QString&)), this, SLOT(disableActionClicked(const QString&)), Qt::QueuedConnection);
        disabled_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        hLayout->addWidget(input_disabled_block_);
        input_disabled_block_->hide();

        connect(text_edit_, &TextEditEx::focusOut, [this]()
        {
            emit editFocusOut();
        });

        auto send_layout = Utils::emptyVLayout();
        send_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));

        send_button_->setObjectName("send_button");
        send_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        send_button_->setCursor(QCursor(Qt::PointingHandCursor));
        Testing::setAccessibleName(send_button_, "SendMessageButton");

        send_layout->addWidget(send_button_);
        send_layout->addSpacerItem(new QSpacerItem(0, default_height_ / 2 - Utils::scale_value(icon_size) / 2, QSizePolicy::Preferred, QSizePolicy::Fixed));
        hLayout->addLayout(send_layout);

        vLayout->addLayout(hLayout);
        setLayout(vLayout);

        file_button_->setObjectName("file_button");
        file_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        file_button_->setFocusPolicy(Qt::NoFocus);
        file_button_->setCursor(QCursor(Qt::PointingHandCursor));
        file_button_->raise();
        Testing::setAccessibleName(file_button_, "SendFileButton");
        file_button_->setVisible(!messageOnly);

        set_current_height(active_height_);

        connect(text_edit_->document(), SIGNAL(contentsChanged()), this, SLOT(edit_content_changed()), Qt::QueuedConnection);
        connect(text_edit_, SIGNAL(enter()), this, SLOT(send()), Qt::QueuedConnection);
        connect(text_edit_, SIGNAL(enter()), this, SLOT(stats_message_enter()), Qt::QueuedConnection);
        connect(send_button_, SIGNAL(clicked()), this, SLOT(send()), Qt::QueuedConnection);
        connect(send_button_, SIGNAL(clicked()), this, SLOT(stats_message_send()), Qt::QueuedConnection);
        connect(file_button_, SIGNAL(clicked()), this, SLOT(attach_file()), Qt::QueuedConnection);
        connect(file_button_, SIGNAL(clicked()), this, SLOT(stats_attach_file()), Qt::QueuedConnection);
        connect(cancel_quote_, SIGNAL(clicked()), this, SLOT(clear()), Qt::QueuedConnection);
        connect(this, SIGNAL(needUpdateSizes()), this, SLOT(updateSizes()), Qt::QueuedConnection);

        connect(Logic::getContactListModel(), SIGNAL(youRoleChanged(QString)), this, SLOT(chatRoleChanged(QString)), Qt::QueuedConnection);

        if (!message_only_)
        {
            connect(smiles_button_, &QPushButton::clicked, [this]()
            {
                text_edit_->setFocus();
                emit smilesMenuSignal();
            });
        }

        connect(text_edit_, SIGNAL(textChanged()), this, SLOT(typed()));

        active_document_height_ = text_edit_->document()->size().height();

        Testing::setAccessibleName(text_edit_, "InputTextEdit");
    }

    InputWidget::~InputWidget()
    {

    }

    void InputWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

    void InputWidget::resizeEvent(QResizeEvent * _e)
    {
        updateSizes();
        return QWidget::resizeEvent(_e);
    }

    void InputWidget::keyPressEvent(QKeyEvent * _e)
    {
        QString currentContact = Logic::getContactListModel()->selectedContact();
        if (!currentContact.isEmpty())
        {
            auto page = Utils::InterConnector::instance().getHistoryPage(currentContact);
            if (page && text_edit_->getPlainText().isEmpty())
            {
                if (_e->key() == Qt::Key_Up)
                {
                    QApplication::sendEvent((QObject*)page, _e);
                }

                if (_e->key() == Qt::Key_Down)
                {
                    QApplication::sendEvent((QObject*)page, _e);
                }
                
                if (_e->modifiers() == Qt::CTRL && _e->key() == Qt::Key_End)
                {
                    QApplication::sendEvent((QObject*)page, _e);
                }
                
                if (platform::is_apple() && ((_e->modifiers().testFlag(Qt::KeyboardModifier::MetaModifier) && _e->key() == Qt::Key_Right) || _e->key() == Qt::Key_End))
                {
                    QApplication::sendEvent((QObject*)page, _e);
                }
                
                if (_e->key() == Qt::Key_PageUp)
                {
                    QApplication::sendEvent((QObject*)page, _e);
                }
                
                if (_e->key() == Qt::Key_PageDown)
                {
                    QApplication::sendEvent((QObject*)page, _e);
                }
            }
        }
        if (_e->matches(QKeySequence::Paste) && !message_only_)
        {
            auto mimedata = QApplication::clipboard()->mimeData();
            if (mimedata && !Utils::haveText(mimedata))
            {
                bool hasUrls = false;
                if (mimedata->hasUrls())
                {
                    files_to_send_[contact_].clear();
                    QList<QUrl> urlList = mimedata->urls();
                    for (auto url : urlList)
                    {
                        if (url.isLocalFile())
                        {
                            auto path = url.toLocalFile();
                            auto fi = QFileInfo(path);
                            if (fi.exists() && !fi.isBundle() && !fi.isDir())
                                files_to_send_[contact_] << path;
                            hasUrls = true;
                        }
                    }
                }
                
                if (!hasUrls && mimedata->hasImage())
                {
#ifdef _WIN32
                    if (OpenClipboard(nullptr))
                    {
                        HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
                        image_buffer_[contact_] = qt_pixmapFromWinHBITMAP(hBitmap);
                        CloseClipboard();
                    }
#else
                    image_buffer_[contact_] = qvariant_cast<QPixmap>(mimedata->imageData());
#endif //_WIN32
                }
                edit_content_changed();
                initFiles(contact_);
            }
        }
        if (_e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            Qt::KeyboardModifiers modifiers = _e->modifiers();

            if (text_edit_->catchEnter(modifiers))
            {
                send();
            }
        }

        if (_e->matches(QKeySequence::Find))
        {
            emit ctrlFPressedInInputWidget();
        }

        return QWidget::keyPressEvent(_e);
    }

    void InputWidget::keyReleaseEvent(QKeyEvent *_e)
    {
        if (platform::is_apple())
        {
            if (_e->modifiers() & Qt::AltModifier)
            {
                QTextCursor cursor = text_edit_->textCursor();
                QTextCursor::MoveMode selection = (_e->modifiers() & Qt::ShiftModifier)?
                    QTextCursor::MoveMode::KeepAnchor:
                    QTextCursor::MoveMode::MoveAnchor;

                if (_e->key() == Qt::Key_Left)
                {
                    cursor.movePosition(QTextCursor::MoveOperation::PreviousWord, selection);
                    text_edit_->setTextCursor(cursor);
                }
                else if (_e->key() == Qt::Key_Right)
                {
                    cursor.movePosition(QTextCursor::MoveOperation::NextWord, selection);
                    text_edit_->setTextCursor(cursor);
                }
            }
        }

        QWidget::keyReleaseEvent(_e);
    }

    void InputWidget::resize_to(int _height)
    {
        if (get_current_height() == _height)
            return;

        QEasingCurve easing_curve = QEasingCurve::Linear;
        int duration = 100;

        int start_value = get_current_height();
        int end_value = _height;

        if (!anim_height_)
            anim_height_ = new QPropertyAnimation(this, "current_height");

        anim_height_->stop();
        anim_height_->setDuration(duration);
        anim_height_->setStartValue(start_value);
        anim_height_->setEndValue(end_value);
        anim_height_->setEasingCurve(easing_curve);
        anim_height_->start();
    }

    void InputWidget::edit_content_changed()
    {
        QString input_text = text_edit_->getPlainText();
        const int widget_min_height = default_height_;
        Logic::getContactListModel()->setInputText(contact_, input_text);

        send_button_->setEnabled(!input_text.trimmed().isEmpty() || !quotes_[contact_].isEmpty() || !files_to_send_[contact_].isEmpty() || !image_buffer_[contact_].isNull());
        file_button_->setVisible(input_text.isEmpty() && files_to_send_[contact_].isEmpty() && image_buffer_[contact_].isNull() && !disabled_.contains(contact_) && !message_only_);

        int doc_height = text_edit_->document()->size().height();

        int diff = doc_height - active_document_height_;

        active_document_height_ = doc_height;

        need_height_ = need_height_ + diff;

        int new_height = need_height_;

        if (need_height_ <= widget_min_height || doc_height <= Utils::scale_value(document_min_height))
        {
            new_height = widget_min_height;
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        else if (need_height_ > Utils::scale_value(widget_max_height))
        {
            new_height = Utils::scale_value(widget_max_height);
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else
        {
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }

        resize_to(new_height);
    }

    void InputWidget::quote(QList<Data::Quote> _quotes)
    {
        if (_quotes.isEmpty() || disabled_.contains(contact_))
            return;

        text_edit_->setFocus();

        quotes_[contact_] += _quotes;
        quote_block_->setVisible(true);
        initQuotes(contact_);
        initFiles(contact_);

        edit_content_changed();
    }

    void InputWidget::insert_emoji(int32_t _main, int32_t _ext)
    {
#ifdef __APPLE__
        text_edit_->insertPlainText(Utils::SChar(_main, _ext).ToQString());
#else
        text_edit_->insertEmoji(_main, _ext);
#endif
        text_edit_->setFocus();
    }

    void InputWidget::send_sticker(int32_t _set_id, int32_t _sticker_id)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", contact_);
        //ext:1:sticker:5
        collection.set_value_as_bool("sticker", true);
        collection.set_value_as_int("sticker/set_id", _set_id);
        collection.set_value_as_int("sticker/id", _sticker_id);

        if (!quotes_[contact_].isEmpty())
        {
            core::ifptr<core::iarray> quotesArray(collection->create_array());
            quotesArray->reserve(quotes_[contact_].size());
            for (auto quote : quotes_[contact_])
            {
                core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                quote.serialize(quoteCollection.get());
                core::coll_helper coll(collection->create_collection(), true);
                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(quoteCollection.get());
                quotesArray->push_back(val.get());
            }
            collection.set_value_as_array("quotes", quotesArray.get());
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_send_answer);

            core::stats::event_props_type props;
            props.push_back(std::make_pair("Quotes_MessagesCount", std::to_string(quotes_[contact_].size())));
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, props);
        }

        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());

        clear_quote(contact_);

        emit sendMessage(contact_);
    }

    void InputWidget::send()
    {
        QString text = text_edit_->getPlainText().trimmed();

        text = text.trimmed();

        if (text.isEmpty() && quotes_[contact_].isEmpty() && files_to_send_[contact_].isEmpty() && image_buffer_[contact_].isNull())
        {
            return;
        }

        if (!predefined_.isEmpty())
            text = predefined_ + " " + text;

        if (!image_buffer_[contact_].isNull())
        {
            QByteArray array;
            QBuffer b(&array);
            b.open(QIODevice::WriteOnly);
            image_buffer_[contact_].save(&b, "PNG");
            Ui::GetDispatcher()->uploadSharedFile(contact_, array, ".png");
            onFilesCancel();
            return;
        }

        if (!files_to_send_[contact_].isEmpty())
        {
            for (const auto &filename : files_to_send_[contact_])
            {
                QFileInfo fileInfo(filename);
                if (fileInfo.size() == 0)
                    continue;

                Ui::GetDispatcher()->uploadSharedFile(contact_, filename);

                core::stats::event_props_type props_file;
                props_file.push_back(std::make_pair("Filesharing_Filesize", std::to_string(1.0 * fileInfo.size() / 1024 / 1024)));
                props_file.push_back(std::make_pair("Filesharing_Is_Image", std::to_string(Utils::is_image_extension(fileInfo.suffix()))));
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_sent, props_file);
            }

            onFilesCancel();
            return;
        }

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set<QString>("contact", contact_);
        collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
        if (!quotes_[contact_].isEmpty())
        {
            core::ifptr<core::iarray> quotesArray(collection->create_array());
            quotesArray->reserve(quotes_[contact_].size());
            for (auto quote : quotes_[contact_])
            {
                core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                quote.isForward_ = false;
                quote.serialize(quoteCollection.get());
                core::coll_helper coll(collection->create_collection(), true);
                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(quoteCollection.get());
                quotesArray->push_back(val.get());
            }
            collection.set_value_as_array("quotes", quotesArray.get());
            Ui::GetDispatcher()->post_stats_to_core(text.isEmpty() ? core::stats::stats_event_names::quotes_send_alone : core::stats::stats_event_names::quotes_send_answer);

            core::stats::event_props_type props;
            props.push_back(std::make_pair("Quotes_MessagesCount", std::to_string(quotes_[contact_].size())));
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, props);

        }

        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());

        text_edit_->clear();
        clear_quote(contact_);
        onFilesCancel();

        GetSoundsManager()->playOutgoingMessage();

        emit sendMessage(contact_);
    }

    void InputWidget::attach_file()
    {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setViewMode(QFileDialog::Detail);
        dialog.setDirectory(get_gui_settings()->get_value<QString>(settings_upload_directory, ""));
        if (platform::is_apple())
            dialog.setFilter(QDir::Files); // probably it'll also be useful for other platforms
        if (!dialog.exec())
            return;

        get_gui_settings()->set_value<QString>(settings_upload_directory, dialog.directory().absolutePath());

        QStringList selectedFiles = dialog.selectedFiles();
        if (selectedFiles.isEmpty())
            return;

        core::stats::event_props_type props;
        props.push_back(std::make_pair("Filesharing_Count", std::to_string(selectedFiles.size())));
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_sent_count, props);

        for (const auto &filename : selectedFiles)
        {
            QFileInfo fileInfo(filename);
            if (fileInfo.size() == 0)
                continue;

            Ui::GetDispatcher()->uploadSharedFile(contact_, filename);

            core::stats::event_props_type props_file;
            props_file.push_back(std::make_pair("Filesharing_Filesize", std::to_string(1.0 * fileInfo.size() / 1024 / 1024)));
            props_file.push_back(std::make_pair("Filesharing_Is_Image", std::to_string(Utils::is_image_extension(fileInfo.suffix()))));
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_sent, props_file);
        }
    }

    void InputWidget::contactSelected(QString contact)
    {
        if (contact_ == contact)
            return;

        if (!quotes_[contact].isEmpty())
        {
            initQuotes(contact);
            quote_block_->setVisible(true);

            edit_content_changed();
        }
        else
        {
            clear_quote(contact);
        }

        if (!files_to_send_[contact].isEmpty() || !image_buffer_[contact].isNull())
        {
            initFiles(contact);

            edit_content_changed();
        }
        else
        {
            clear_files(contact);
        }
        
        contact_ = contact;

        setVisible(true);
        activateWindow();
        is_initializing_ = true;
        text_edit_->setPlainText(Logic::getContactListModel()->getInputText(contact_), false);
        is_initializing_ = false;

        auto contactDialog = Utils::InterConnector::instance().getContactDialog();
        if (contactDialog)
            contactDialog->hideSmilesMenu();

        auto role = Logic::getContactListModel()->getYourRole(contact_);
        if (role == "notamember")
            disable(NotAMember);
        else if (role == "readonly")
            disable(ReadOnlyChat);
        else
            enable();

        text_edit_->setFocus(Qt::FocusReason::MouseFocusReason);
        
    }

    void InputWidget::hide()
    {
        contact_.clear();
        setVisible(false);
        text_edit_->setPlainText("", false);
    }

    void InputWidget::hideNoClear()
    {
        setVisible(false);
    }

    void InputWidget::disable(DisableReason reason)
    {
        disabled_.insert(contact_, reason);
        text_edit_->clear();
        text_edit_->setEnabled(false);
        auto contactDialog = Utils::InterConnector::instance().getContactDialog();
        if (contactDialog)
            contactDialog->hideSmilesMenu();

        if (reason == ReadOnlyChat)
        {
            disable_label_->setFixedWidth(width());
            disable_label_->setVisible(true);
            disable_label_->setText(QT_TRANSLATE_NOOP("input_widget", "This chat is read-only"));
        }
        else if (reason == NotAMember)
        {
            disable_label_->setFixedWidth(width());
            disable_label_->setVisible(true);
            disable_label_->setText(QString(QT_TRANSLATE_NOOP("input_widget", "You are not a member of this chat. ")) 
                                    + QString("<a style=\"text-decoration:none\" href=\"leave\"><span style=\"color:#579e1c;\">") 
                                    + QString(QT_TRANSLATE_NOOP("input_widget", "Leave and delete")) + QString("</span></a>"));
        }

        clear_files(contact_);
        clear_quote(contact_);
        smiles_button_->hide();
        text_edit_->hide();
        file_button_->hide();
        send_button_->hide();
        input_disabled_block_->setVisible(true);
        input_disabled_block_->setFixedHeight(default_height_);
    }

    void InputWidget::enable()
    {
        disabled_.remove(contact_);
        text_edit_->setEnabled(true);
        initFiles(contact_);
        initQuotes(contact_);
        smiles_button_->show();
        text_edit_->show();
        file_button_->show();
        send_button_->show();
        edit_content_changed();
        input_disabled_block_->hide();
        
        emit needUpdateSizes();
    }

    void InputWidget::predefined(const QString& _contact, const QString& _text)
    {
        contact_ = _contact;
        predefined_ = _text;
    }

    void InputWidget::set_current_height(int _val)
    {
        text_edit_->setFixedHeight(_val);

        active_height_ = _val;

        emit sizeChanged();
    }

    int InputWidget::get_current_height() const
    {
        return active_height_;
    }

    void InputWidget::setFocusOnInput()
    {
        text_edit_->setFocus();
    }

    void InputWidget::setFontColor(const QColor& _color)
    {
        QPalette p = text_edit_->palette();
        p.setColor(QPalette::Text, _color);
        text_edit_->setPalette(p);
    }

    QPixmap InputWidget::getFilePreview(const QString& contact)
    {
        QPixmap result ;
        if (files_to_send_[contact].isEmpty())
        {
            if (image_buffer_[contact].isNull())
                return QPixmap();

            result = image_buffer_[contact];
        }

        if (!files_to_send_[contact].isEmpty())
        {
            auto file = files_to_send_[contact].at(0);

            if (Utils::is_image_extension(QFileInfo(file).suffix()))
            {
                auto reader = std::unique_ptr<QImageReader>(new QImageReader(file));
                
                QString type = reader->format();
                
                if (type.isEmpty())
                {
                    reader.reset(new QImageReader(file));
                    reader->setDecideFormatFromContent(true);
                    type = reader->format();
                }
                
                result = QPixmap::fromImageReader(reader.get());
            }
            else
            {
                auto fileTypeIcon = History::GetIconByFilename(file);
                result = fileTypeIcon->GetPixmap();
            }
        }

        int h = Utils::scale_bitmap(Utils::scale_value(preview_max_height));
        int w = Utils::scale_bitmap(Utils::scale_value(preview_max_width));
        result = result.scaledToHeight(h, Qt::SmoothTransformation);
        if (result.width() > w)
        {
            result = result.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }

        return result;
    }

    QString InputWidget::getFileSendText(const QString& contact)
    {
        if (files_to_send_[contact].isEmpty())
        {
            if (!image_buffer_[contact].isNull())
                return QT_TRANSLATE_NOOP("input_widget", "Send image");

            return QString();
        }

        auto file = files_to_send_[contact].at(0);
        bool image = true;
        for (auto file_iter : files_to_send_[contact])
        {
            if (!Utils::is_image_extension(QFileInfo(file_iter).suffix()))
            {
                image = false;
                break;
            }
        }

        if (files_to_send_[contact].size() == 1)
        {
            if (image)
                return QT_TRANSLATE_NOOP("input_widget", "Send image");
            else
                return QT_TRANSLATE_NOOP("input_widget", "Send file");
        }
        
        QString numberString;
        if (image)
            numberString = Utils::GetTranslator()->getNumberString(files_to_send_[contact].size(), QT_TRANSLATE_NOOP3("input_widget", "image", "1"), QT_TRANSLATE_NOOP3("input_widget", "images", "2"), QT_TRANSLATE_NOOP3("input_widget", "images", "5"), QT_TRANSLATE_NOOP3("input_widget", "images", "21"));
        else
            numberString = Utils::GetTranslator()->getNumberString(files_to_send_[contact].size(), QT_TRANSLATE_NOOP3("input_widget", "file", "1"), QT_TRANSLATE_NOOP3("input_widget", "files", "2"), QT_TRANSLATE_NOOP3("input_widget", "files", "5"), QT_TRANSLATE_NOOP3("input_widget", "files", "21"));

        return QString(QT_TRANSLATE_NOOP("input_widget", "Send %1 %2 %3 %4")).arg("<font color=black>").arg(files_to_send_[contact].size()).arg(QString("</font>")).arg(numberString);
    }

    void InputWidget::typed()
    {
        if (is_initializing_)
            return;
        if (!isVisible())
            return;

        static qint64 typedTime = 0;
        typedTime = QDateTime::currentMSecsSinceEpoch();

        static qint64 prevCheckingTime = 0;
        const qint64 currCheckingTime = QDateTime::currentMSecsSinceEpoch();

        static qint64 prevTypingTime = 0;
        const qint64 currTypingTime = QDateTime::currentMSecsSinceEpoch();
        
        if ((currCheckingTime - prevCheckingTime) >= 1000)
        {
            if ((currTypingTime - prevTypingTime) >= 4000)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", contact_);
                collection.set_value_as_int("status", (int32_t)core::typing_status::typing);
                Ui::GetDispatcher()->post_message_to_core("message/typing", collection.get());

                prevTypingTime = currTypingTime;
            }
            prevCheckingTime = currCheckingTime;
        }
        
        auto contact = contact_;
        QTimer::singleShot(1000, [contact]()
        {
            if ((QDateTime::currentMSecsSinceEpoch() - typedTime) >= 1000)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", contact);
                collection.set_value_as_int("status", (int32_t)core::typing_status::typed);
                Ui::GetDispatcher()->post_message_to_core("message/typing", collection.get());

                typedTime = 0;
                prevCheckingTime = 0;
                prevTypingTime = 0;
            }
        });

    }

    void InputWidget::stats_message_enter()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_enter_button);
    }


    void InputWidget::stats_message_send()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_send_button);
    }

    void InputWidget::stats_attach_file()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dialog);
    }

    void InputWidget::clear_quote(const QString& contact)
    {
        quotes_[contact].clear();
        quote_text_->clear();
        quote_block_->setVisible(false);
        edit_content_changed();
    }

    void InputWidget::clear_files(const QString& contact)
    {
        files_to_send_[contact].clear();
        initFiles(contact);
        edit_content_changed();
    }
    
    void InputWidget::clear()
    {
        clear_quote(contact_);
        clear_files(contact_);
    }

    void InputWidget::updateSizes()
    {
        initQuotes(contact_);
        edit_content_changed();

        QRect edit_rect = text_edit_->geometry();
        file_button_->setGeometry(edit_rect.right() - Utils::scale_value(24) - Utils::scale_value(24)/*margin*/,
            edit_rect.top() + ((edit_rect.height() - Utils::scale_value(24))/2),
            Utils::scale_value(24),
            Utils::scale_value(24));

        disable_label_->setFixedWidth(width());
    }

    void InputWidget::initQuotes(const QString& contact)
    {
        if (quotes_[contact].isEmpty())
            return;

        quote_text_->clear();

        QString text;
        int i = 0;
        int needHeight = 0;
        for (auto quote : quotes_[contact])
        {
            if (!text.isEmpty())
                text += "\n";

            QString quoteText = quote.senderFriendly_;
            quoteText += ": ";
            quoteText += quote.text_;
            quoteText.replace(QChar::LineSeparator, QChar::Space);
            quoteText.replace(QChar::LineFeed, QChar::Space);

            QFontMetrics m(quote_text_->font());
            auto elided = m.elidedText(quoteText, Qt::ElideRight, width() - cancel_quote_->width() - Utils::scale_value(100));
            text += elided;
            if (++i <= 3)
                needHeight += m.size(Qt::TextSingleLine, elided).height();
        }
        
        quote_text_->setPlainText(text, false);
        
        if (quotes_[contact].size() <= 3)
        {
            auto docSize = quote_text_->document()->documentLayout()->documentSize().toSize();
            auto height = docSize.height();
            needHeight = height;
        }
        
        needHeight += Utils::scale_value(20);

        quote_text_widget_->setFixedHeight(needHeight);
        quote_block_->setFixedHeight(needHeight);
        quote_line_->setFixedHeight(needHeight - Utils::scale_value(2));
    }

    void InputWidget::initFiles(const QString& contact)
    {
        const auto show = !files_to_send_[contact].isEmpty() || !image_buffer_[contact].isNull();

        QString first;
        if (!files_to_send_[contact].isEmpty())
            first = files_to_send_[contact].at(0);

        auto p = getFilePreview(contact);
        files_label_->setText(getFileSendText(contact));
        files_label_->setTextFormat(Qt::RichText);
        file_preview_->setFixedSize(p.width() / Utils::scale_bitmap(1), p.height() / Utils::scale_bitmap(1) + Utils::scale_value(preview_offset));
        file_preview_->setImage(p, Utils::is_image_extension(QFileInfo(first).suffix()) || !image_buffer_[contact].isNull() ? Utils::scale_value(8) : -1);

        smiles_button_->setVisible(!show);
        cancel_files_->setVisible(show);

        files_block_->setVisible(show);
        text_edit_->setVisible(!show);

        file_button_->setVisible(!show && text_edit_->getPlainText().isEmpty() && files_to_send_[contact_].isEmpty() && image_buffer_[contact_].isNull());

        quote_block_->setVisible(!show && !quotes_[contact].isEmpty());

        if (show)
        {
            auto contactDialog = Utils::InterConnector::instance().getContactDialog();
            if (contactDialog)
                contactDialog->hideSmilesMenu();
            files_block_->setFixedHeight(default_height_);
        }
    }

    void InputWidget::onFilesCancel()
    {
        files_to_send_[contact_].clear();
        image_buffer_[contact_] = QPixmap();
        initFiles(contact_);
        edit_content_changed();
    }

    void InputWidget::disableActionClicked(const QString& action)
    {
        if (action == "leave" && disabled_.contains(contact_) && disabled_[contact_] == NotAMember)
            Logic::getContactListModel()->removeContactFromCL(contact_);
    }

    void InputWidget::chatRoleChanged(QString contact)
    {
        auto role = Logic::getContactListModel()->getYourRole(contact);
        if (contact_ == contact)
        {
            if (role == "notamember")
            {
                disable(NotAMember);
                return;
            }
            else if (role == "readonly")
            {
                disable(ReadOnlyChat);
                return;
            }
        }

        if (disabled_.contains(contact))
        {
            if (contact_ == contact)
                enable();
            else if (role == "notamember")
                disabled_[contact] = NotAMember;
            else if (role == "readonly")
                disabled_[contact] = ReadOnlyChat;
            else
                disabled_.remove(contact);
        }
    }
}
