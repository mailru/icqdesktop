#include "stdafx.h"
#include "ContactAvatarWidget.h"

#include "AvatarPreview.h"
#include "GeneralDialog.h"
#include "ImageCropper.h"
#include "../core_dispatcher.h"
#include "../my_info.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"

namespace
{
    const auto MIN_AVATAR_SIZE = 200;
    const int ADD_PHOTO_FONTSIZE = 24;
    const QColor STROKE_COLOR("#579e1c");
}

namespace Ui
{
    ContactAvatarWidget::ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _displayName, int _size, bool _autoUpdate)
        :  QPushButton(_parent)
        , aimid_(_aimid)
        , displayName_(_displayName)
        , size_(_size)
        , imageCropHolder_(nullptr)
        , imageCropSize_(QSize())
        , mode_(Mode::Common)
        , isVisibleShadow_(false)
        , isVisibleSpinner_(false)
        , isVisibleOutline_(false)
        , spinnerMovie_(nullptr)
        , connected_(false)
        , seq_(-1)
    {
        setFixedHeight(_size);
        setFixedWidth(_size);

        spinnerMovie_ = new QMovie(":/resources/gifs/r_spiner200.gif");
        spinnerMovie_->setScaledSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));
        connect(spinnerMovie_, &QMovie::frameChanged, this, &ContactAvatarWidget::frameChanged);

        if (_autoUpdate)
            connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)), Qt::QueuedConnection);

        infoForSetAvatar_.currentDirectory = QDir::homePath();
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setAvatar, this, &ContactAvatarWidget::setAvatar, Qt::QueuedConnection);

        connect(this, &ContactAvatarWidget::summonSelectFileForAvatar, this, &ContactAvatarWidget::selectFileForAvatar, Qt::QueuedConnection);
    }

    ContactAvatarWidget::~ContactAvatarWidget()
    {
    }

    QString ContactAvatarWidget::GetState()
    {
        if (mode_ == Mode::MyProfile)
        {
            return isVisibleShadow_ ? "photo enter" : "photo leave";
        }
        return "";
    }

	void ContactAvatarWidget::paintEvent(QPaintEvent* _e)
	{
        if (aimid_.isEmpty() && displayName_.isEmpty() && (!infoForSetAvatar_.croppedImage.isNull() || !infoForSetAvatar_.roundedCroppedImage.isNull()))
        {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            if (infoForSetAvatar_.roundedCroppedImage.isNull())
            {
                auto scaled = infoForSetAvatar_.croppedImage.scaled(QSize(size_, size_), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                infoForSetAvatar_.roundedCroppedImage = Utils::roundImage(scaled, "", false, false);
            }
            p.drawPixmap(0, 0, size_, size_, infoForSetAvatar_.roundedCroppedImage);
            return QWidget::paintEvent(_e);
        }

		bool isDefault = false;
        const auto &avatar = Logic::GetAvatarStorage()->GetRounded(aimid_, displayName_, isVisibleOutline_ ? Utils::scale_bitmap(size_ - Utils::scale_value(4)) : Utils::scale_bitmap(size_), GetState(), !Logic::getContactListModel()->isChat(aimid_), isDefault, false, false);

		if (avatar->isNull())
            return;

		QPainter p(this);
        if (isVisibleOutline_)
        {
            p.setRenderHint(QPainter::Antialiasing);
            QPen pen;
            pen.setBrush(Qt::white);
            pen.setWidth(Utils::scale_value(4));
            p.setPen(pen);
            p.drawEllipse(QPointF(size_ / 2, size_ / 2), size_ / 2 - Utils::scale_value(2), size_ / 2 - Utils::scale_value(2));
        }
        if (mode_ == Mode::MyProfile && isDefault)
        {
            p.setPen(Qt::NoPen);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            p.setBrush(QBrush(QColor(Qt::transparent)));
            p.drawEllipse(0, 0, size_, size_);

            QPen pen(STROKE_COLOR, Utils::scale_value(2), Qt::DashLine, Qt::RoundCap);
            p.setPen(pen);
            p.drawRoundedRect(
                pen.width(),
                pen.width(),
                size_ - (pen.width() * 2),
                size_ - (pen.width() * 2),
                (size_ / 2),
                (size_ / 2)
            );

            p.setFont(Fonts::appFontScaled(ADD_PHOTO_FONTSIZE, Fonts::FontWeight::Light));
            p.setPen(QPen(QColor("#767676")));

            p.drawText(QRectF(0, 0, size_, size_), Qt::AlignCenter, QT_TRANSLATE_NOOP("avatar_upload", "Add\nphoto"));
        }
        else
        {
            auto size = isVisibleOutline_ ? size_ - Utils::scale_value(4) : size_;
            auto from = isVisibleOutline_ ? Utils::scale_value(2) : 0;
		    p.drawPixmap(QRect(from, from, size, size), *avatar, avatar->rect());
        }

        if (isVisibleSpinner_)
        {
            auto spinner_size = spinnerMovie_->currentPixmap().size();
            p.drawPixmap(
                size_/2 - spinner_size.width()/2,
                size_/2 - spinner_size.width()/2,
                spinnerMovie_->currentPixmap()
            );
        }

		return QWidget::paintEvent(_e);
	}

    void ContactAvatarWidget::UpdateParams(const QString& _aimid, const QString& _displayName)
    {
        aimid_ = _aimid;
        displayName_ = _displayName;
    }

    void ContactAvatarWidget::avatarChanged(QString _aimId)
    {
        if (_aimId == aimid_)
            update();
    }

    void ContactAvatarWidget::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->source() == Qt::MouseEventNotSynthesized)
        {
            emit clickedInternal();
            _event->accept();
        }
    }

    void ContactAvatarWidget::enterEvent(QEvent* /*_event*/)
    {
        emit mouseEntered();
        update();
    }

    void ContactAvatarWidget::leaveEvent(QEvent* /*_event*/)
    {
        emit mouseLeft();
        update();
    }

    void ContactAvatarWidget::SetImageCropHolder(QWidget *_holder)
    {
        imageCropHolder_ = _holder;
    }

    void ContactAvatarWidget::SetImageCropSize(const QSize &_size)
    {
        imageCropSize_ = _size;
    }

    void ContactAvatarWidget::SetMode(ContactAvatarWidget::Mode _mode)
    {
        mode_ = _mode;
        if (mode_ == Mode::MyProfile)
        {
            if (!connected_)
            {
                connected_ = true;
                connect(this, &ContactAvatarWidget::clickedInternal, this, &ContactAvatarWidget::selectFileForAvatar, Qt::QueuedConnection);
                connect(this, &ContactAvatarWidget::mouseEntered, this, &ContactAvatarWidget::avatarEnter, Qt::QueuedConnection);
                connect(this, &ContactAvatarWidget::mouseLeft, this, &ContactAvatarWidget::avatarLeave, Qt::QueuedConnection);
                this->setCursor(Qt::CursorShape::PointingHandCursor);
            }
        }
        else if (mode_ == Mode::CreateChat)
        {
            if (!connected_)
            {
                connected_ = true;
                connect(this, &ContactAvatarWidget::clickedInternal, this, &ContactAvatarWidget::selectFileForAvatar, Qt::QueuedConnection);
                this->setCursor(Qt::CursorShape::PointingHandCursor);
            }
        }
        else
        {
            connected_ = false;
            disconnect(this, &ContactAvatarWidget::clickedInternal, this, &ContactAvatarWidget::selectFileForAvatar);
            disconnect(this, &ContactAvatarWidget::mouseEntered, this, &ContactAvatarWidget::avatarEnter);
            disconnect(this, &ContactAvatarWidget::mouseLeft, this, &ContactAvatarWidget::avatarLeave);
            this->setCursor(Qt::CursorShape::ArrowCursor);
        }

    }

    void ContactAvatarWidget::SetVisibleShadow(bool _isVisibleShadow)
    {
        isVisibleShadow_ = _isVisibleShadow;
    }

    void ContactAvatarWidget::frameChanged(int /*frame*/)
    {
        repaint();
    }

    void ContactAvatarWidget::SetVisibleSpinner(bool _isVisibleSpinner)
    {
        if (isVisibleSpinner_ == _isVisibleSpinner)
            return;

        isVisibleSpinner_ = _isVisibleSpinner;

        if (isVisibleSpinner_)
        {
            spinnerMovie_->start();
        }
        else
        {
            spinnerMovie_->stop();
        }
    }

    void ContactAvatarWidget::SetOutline(bool _isVisibleOutline)
    {
        isVisibleOutline_ = _isVisibleOutline;
        update();
    }

    void ContactAvatarWidget::applyAvatar(const QPixmap &alter)
    {
        if (alter.isNull())
            postSetAvatarToCore(infoForSetAvatar_.croppedImage);
        else
            postSetAvatarToCore(alter);
    }

    const QPixmap &ContactAvatarWidget::croppedImage() const
    {
        return infoForSetAvatar_.croppedImage;
    }
    
    void ContactAvatarWidget::postSetAvatarToCore(const QPixmap& _avatar)
    {
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        _avatar.save(&buffer, "PNG");

        core::coll_helper helper(GetDispatcher()->create_collection(), true);

        core::ifptr<core::istream> data_stream(helper->create_stream());
        if (byteArray.size())
            data_stream->write((const uint8_t*)byteArray.data(), (uint32_t)byteArray.size());
        helper.set_value_as_stream("avatar", data_stream.get());
        if (aimid_.isEmpty())
            helper.set_value_as_bool("chat", true);
        else if (aimid_ != MyInfo()->aimId())
            helper.set_value_as_string("aimid", aimid_.toStdString());

        seq_ = GetDispatcher()->post_message_to_core("set_avatar", helper.get());
    }

    void ContactAvatarWidget::selectFileForAvatar()
    {
        ResetInfoForSetAvatar();
        QFileDialog fileDialog(
#ifdef __linux__
0
#else
this
#endif //__linux__
);
        fileDialog.setDirectory(infoForSetAvatar_.currentDirectory);
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setNameFilter(QT_TRANSLATE_NOOP("avatar_upload", "Images (*.jpg *.jpeg *.png *.bmp)"));

        bool isContinue = true;
        QImage newAvatar;

        while (isContinue)
        {
            isContinue = false;
            if (fileDialog.exec())
            {
                infoForSetAvatar_.currentDirectory = fileDialog.directory().path();

                {
                    QFile file(fileDialog.selectedFiles()[0]);
                    if (!file.open(QIODevice::ReadOnly))
                    {
                        return;
                    }

                    const auto data = file.readAll();

                    QPixmap preview;
                    Utils::loadPixmap(data, Out preview);

                    if (preview.isNull())
                    {
                        return;
                    }

                    newAvatar = preview.toImage();
                }

                if (newAvatar.height() < MIN_AVATAR_SIZE || newAvatar.width() < MIN_AVATAR_SIZE)
                {
                    if (Utils::GetErrorWithTwoButtons(
                        QT_TRANSLATE_NOOP("avatar_upload", "Cancel"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Choose file"),
                        NULL,
                        QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Image should be at least 200x200 px"),
                        NULL))
                    {
                        isContinue = true;
                    }
                    else
                    {
                        return;
                    }
                }
            }
            else
            {
                return;
            }
        }

        infoForSetAvatar_.image = newAvatar;
        cropAvatar();
    }

    void ContactAvatarWidget::ResetInfoForSetAvatar()
    {
        infoForSetAvatar_.image = QImage();
        infoForSetAvatar_.croppingRect = QRectF();
        infoForSetAvatar_.croppedImage = QPixmap();
    }

    void ContactAvatarWidget::cropAvatar()
    {
        QWidget *hostWidget = nullptr;
        QLayout *hostLayout = nullptr;

        QWidget *mainWidget = nullptr;
        QLayout *mainLayout = nullptr;

        if (imageCropHolder_)
        {
            hostWidget = new QWidget(this);
            hostLayout = new QHBoxLayout(hostWidget);
            hostWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
            hostWidget->setFixedHeight(imageCropSize_.height());
            hostLayout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(12), Utils::scale_value(24), 0);

            mainWidget = new QWidget(hostWidget);
            mainLayout = Utils::emptyHLayout(mainWidget);
            mainWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            hostLayout->addWidget(mainWidget);
        }
        else
        {
            mainWidget = new QWidget(this);
            mainLayout = new QHBoxLayout(mainWidget);
            mainLayout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(12), Utils::scale_value(24), 0);
        }
        
        auto avatarCropper = new Ui::ImageCropper(mainWidget, imageCropSize_);
        avatarCropper->setProportion(QSizeF(1.0, 1.0));
        avatarCropper->setProportionFixed(true);
        avatarCropper->setBackgroundColor(QColor("#ffffff"));
        if (!infoForSetAvatar_.croppingRect.isNull())
        {
            avatarCropper->setCroppingRect(infoForSetAvatar_.croppingRect);
        }
        avatarCropper->setImage(QPixmap::fromImage(infoForSetAvatar_.image));
        mainLayout->addWidget(avatarCropper);

        if (!imageCropHolder_)
        {
            hostWidget = mainWidget;
        }

        GeneralDialog imageCropDialog(hostWidget, imageCropHolder_ ? imageCropHolder_ : Utils::InterConnector::instance().getMainWindow());
        imageCropDialog.setObjectName("image.cropper");
        if (imageCropHolder_)
        {
            imageCropDialog.setShadow(false);
            imageCropDialog.connect(&imageCropDialog, &GeneralDialog::moved, this, [=](QWidget *dlg)
            {
                emit Utils::InterConnector::instance().imageCropDialogMoved(dlg);
            });
            imageCropDialog.connect(&imageCropDialog, &GeneralDialog::resized, this, [=](QWidget *dlg)
            {
                emit Utils::InterConnector::instance().imageCropDialogResized(dlg);
            });
            imageCropDialog.connect(&imageCropDialog, &GeneralDialog::shown, this, [=](QWidget *dlg)
            {
                emit Utils::InterConnector::instance().imageCropDialogIsShown(dlg);
            });
            imageCropDialog.connect(&imageCropDialog, &GeneralDialog::hidden, this, [=](QWidget *dlg)
            {
                emit Utils::InterConnector::instance().imageCropDialogIsHidden(dlg);
            });
        }
        imageCropDialog.addHead();
        imageCropDialog.addLabel(mode_ == Mode::CreateChat ? QT_TRANSLATE_NOOP("avatar_upload", "Edit photo") : QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"));
        imageCropDialog.addButtonsPair(QT_TRANSLATE_NOOP("avatar_upload", "Back"), QT_TRANSLATE_NOOP("avatar_upload", "Continue"), Utils::scale_value(24), Utils::scale_value(12), false, false);
        imageCropDialog.setRightButtonDisableOnClicked(true);
        
        QObject::connect(&imageCropDialog, &GeneralDialog::leftButtonClicked, this, [=, &imageCropDialog]()
        {
            imageCropDialog.reject();
            QTimer::singleShot(500, [=](){ emit summonSelectFileForAvatar(); });
        },
        Qt::QueuedConnection);
        QObject::connect(&imageCropDialog, &GeneralDialog::rightButtonClicked, this, [=, &imageCropDialog]()
        {
            auto croppedImage = avatarCropper->cropImage();
            auto croppingRect = avatarCropper->getCroppingRect();
            
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            croppedImage.save(&buffer, "PNG");

            if (byteArray.size() > (8 * 1024 * 1024))
            {
                if (Utils::GetErrorWithTwoButtons(
                      QT_TRANSLATE_NOOP("avatar_upload", "Cancel"),
                      QT_TRANSLATE_NOOP("avatar_upload", "Change"),
                      NULL,
                      QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                      QT_TRANSLATE_NOOP("avatar_upload", "Image size should be 8 Mb or less"),
                      NULL))
                {
                    imageCropDialog.reject();
                    emit summonSelectFileForAvatar();
                }
                else
                {
                    imageCropDialog.reject();
                }
            }
            else
            {
                infoForSetAvatar_.croppedImage = croppedImage;
                infoForSetAvatar_.croppingRect = croppingRect;
                imageCropDialog.accept();
            }
            
            if (auto p = imageCropDialog.takeAcceptButton())
                p->setEnabled(true);
        },
        Qt::QueuedConnection);
        QObject::connect(&imageCropDialog, &GeneralDialog::shown, this, [=](QWidget *dlg)
        {
            //
        },
        Qt::QueuedConnection);

        if (!imageCropDialog.showInPosition(-1, -1))
        {
            return;
        }
        
        if (mode_ == Mode::CreateChat)
        {
            infoForSetAvatar_.roundedCroppedImage = QPixmap();
            emit avatarDidEdit();
        }
        else
        {
            openAvatarPreview();
        }
    }

    void ContactAvatarWidget::openAvatarPreview()
    {
        auto layout = new QHBoxLayout();

        auto spacerLeft = new QSpacerItem(Utils::scale_value(12), 1, QSizePolicy::Expanding);
        layout->addSpacerItem(spacerLeft);

        auto croppedAvatar = infoForSetAvatar_.croppedImage;
        auto previewAvatarWidget = new AvatarPreview(croppedAvatar, nullptr);
        previewAvatarWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        layout->addWidget(previewAvatarWidget);

        auto spacerRight = new QSpacerItem(Utils::scale_value(12), 1, QSizePolicy::Expanding);
        layout->addSpacerItem(spacerRight);

        auto avatarPreviewHost = new QWidget();
        avatarPreviewHost->setLayout(layout);

        Ui::GeneralDialog previewDialog(avatarPreviewHost, Utils::InterConnector::instance().getMainWindow());
        previewDialog.addHead();
        previewDialog.addLabel(QT_TRANSLATE_NOOP("avatar_upload", "Preview"));

        previewDialog.addButtonsPair(QT_TRANSLATE_NOOP("avatar_upload", "Back"), QT_TRANSLATE_NOOP("avatar_upload", "Save"),
            Utils::scale_value(24), Utils::scale_value(12));

        QObject::connect(&previewDialog, &GeneralDialog::leftButtonClicked, this, &ContactAvatarWidget::cropAvatar, Qt::QueuedConnection);

        if (previewDialog.showInPosition(-1, -1))
        {
            SetVisibleSpinner(true);
            postSetAvatarToCore(croppedAvatar);
        }
    }

    void ContactAvatarWidget::setAvatar(qint64 _seq, int _error)
    {
        if (_seq != seq_)
            return;

        SetVisibleSpinner(false);

        if (_error != 0)
        {
             if (Utils::GetErrorWithTwoButtons(
                QT_TRANSLATE_NOOP("avatar_upload", "Cancel"),
                QT_TRANSLATE_NOOP("avatar_upload", "Choose file"),
                NULL,
                QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                QT_TRANSLATE_NOOP("avatar_upload", "Avatar was not uploaded due to server error"),
                NULL))
             {
                selectFileForAvatar();
             }
        }
        else
        {
            if (build::is_agent())
            {
                Logic::GetAvatarStorage()->SetAvatar(aimid_, infoForSetAvatar_.croppedImage);
            }

            emit afterAvatarChanged();
        }
    }

    void ContactAvatarWidget::avatarEnter()
    {
        this->SetVisibleShadow(true);
    }

    void ContactAvatarWidget::avatarLeave()
    {
        this->SetVisibleShadow(false);
    }
}
