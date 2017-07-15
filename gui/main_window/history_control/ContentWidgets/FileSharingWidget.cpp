#include "stdafx.h"

#include "FileSharingWidget.h"

#include "../ActionButtonWidget.h"
#include "../FileSharingInfo.h"
#include "../FileSizeFormatter.h"
#include "../HistoryControlPage.h"
#include "../KnownFileTypes.h"
#include "../MessageItem.h"
#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"

#include "../complex_message/Style.h"

#include "../../contact_list/ContactList.h"
#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/SelectionContactsForGroupChat.h"
#include "../../MainPage.h"
#include "../../MainWindow.h"
#include "../../sounds/SoundsManager.h"

#include "../../../controls/CommonStyle.h"
#include "../../../core_dispatcher.h"
#include "../../../previewer/GalleryWidget.h"
#include "../../../gui_settings.h"
#include "../../../previewer/Previewer.h"
#include "../../../themes/ThemePixmap.h"
#include "../../../themes/ResourceIds.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/LoadMovieFromFileTask.h"
#include "../../../utils/LoadPixmapFromFileTask.h"
#include "../../../utils/utils.h"
#include "../../../utils/log/log.h"

#include "../../common.shared/loader_errors.h"

#include "../../../../corelib/enumerations.h"


#ifdef __APPLE__
#include "../../../utils/macos/mac_support.h"
#endif

using namespace Ui::ComplexMessage;

namespace
{
	Themes::IThemePixmapSptr downloadButton();

	Themes::IThemePixmapSptr downloadCancelButton(const bool isImage);

    int32_t getCtrlButtonTopOffset();

    Themes::IThemePixmapSptr playStartButton(const bool isGif, const bool isHovered);

	const auto DOWNLOADING_BAR_BASE_ANGLE_MIN = 90;

	const auto DOWNLOADING_BAR_BASE_ANGLE_MAX = 450;

    const auto RETRY_MAX = 5;

    const auto RETRY_INTERVAL_MS = 500;
}

namespace HistoryControl
{

	enum class FileSharingWidget::State
	{
		Min,

		Initial,

		PlainFile_Initial,
		PlainFile_MetainfoLoaded,
		PlainFile_Downloading,
		PlainFile_Downloaded,
		PlainFile_Uploading,
		PlainFile_UploadError,
		PlainFile_Uploaded,

        ImageFile_Min,

		ImageFile_Initial,
		ImageFile_MetainfoLoaded,
		ImageFile_Downloading,
		ImageFile_Downloaded,
		ImageFile_Uploading,
		ImageFile_Uploaded,

        ImageFile_Max,

		Deleted,

		Max
	};

    enum class FileSharingWidget::PreviewState
    {
        Min,

        NoPreview,
        MiniPreviewLoaded,
        FullPreviewLoaded,

        Max
    };

    FileSharingWidget::Retry::Retry()
        : FileMetainfo_(false)
        , FileMetainfoRetryCount_(0)
        , FileDownload_(false)
        , FileDownloadRetryCount_(0)
        , PreviewDownload_(false)
        , PreviewDownloadRetryCount_(0)
    {
    }

    bool FileSharingWidget::Retry::HasRetryFlagSet() const
    {
        return (FileMetainfo_ || FileDownload_ || PreviewDownload_);
    }

    bool FileSharingWidget::Retry::ShouldRetry() const
    {
        return (ShouldRetryFileMetainfo() || ShouldRetryFileDownload() || ShouldRetryPreviewDownload());
    }

    bool FileSharingWidget::Retry::ShouldRetryFileDownload() const
    {
        assert(FileDownloadRetryCount_ >= 0);
        assert(FileDownloadRetryCount_ <= RETRY_MAX);

        return (FileDownload_ && (FileDownloadRetryCount_ < RETRY_MAX));
    }

    bool FileSharingWidget::Retry::ShouldRetryFileMetainfo() const
    {
        assert(FileMetainfoRetryCount_ >= 0);
        assert(FileMetainfoRetryCount_ <= RETRY_MAX);

        return (FileMetainfo_ && (FileMetainfoRetryCount_ < RETRY_MAX));
    }

    bool FileSharingWidget::Retry::ShouldRetryPreviewDownload() const
    {
        assert(PreviewDownloadRetryCount_ >= 0);
        assert(PreviewDownloadRetryCount_ <= RETRY_MAX);

        return (PreviewDownload_ && (PreviewDownloadRetryCount_ < RETRY_MAX));
    }

	void FileSharingWidget::setDownloadingBarBaseAngle(int _val)
	{
		assert(_val >= DOWNLOADING_BAR_BASE_ANGLE_MIN);
		assert(_val <= DOWNLOADING_BAR_BASE_ANGLE_MAX);

		DownloadingBarBaseAngle_ = _val;

		update();
	}

	int FileSharingWidget::getDownloadingBarBaseAngle() const
	{
		assert(DownloadingBarBaseAngle_ >= DOWNLOADING_BAR_BASE_ANGLE_MIN);
		assert(DownloadingBarBaseAngle_ <= DOWNLOADING_BAR_BASE_ANGLE_MAX);

		return DownloadingBarBaseAngle_;
	}

    FileSharingWidget::FileSharingWidget(const FileSharingInfoSptr& fsInfo, const QString& contactUin)
        : PreviewContentWidget(nullptr, false, QString(), false, contactUin)
        , ParentItem_(nullptr)
        , FsInfo_(fsInfo)
        , CopyFile_(false)
        , SaveAs_(false)
        , PreviewState_(PreviewState::NoPreview)
        , PreviewDownloadId_(-1)
        , FileMetainfoDownloadId_(-1)
        , PreviewMetainfoDownloadId_(-1)
        , FileDownloadId_(-1)
        , IsCtrlButtonHovered_(false)
        , ShareButton_(nullptr)
        , IsVisible_(false)
        , autoplayVideo_(Ui::get_gui_settings()->get_value<bool>(settings_autoplay_video, true))
        , ref_(new bool(false))
    {
        initialize();
    }

	FileSharingWidget::FileSharingWidget(
        Ui::MessageItem* parent,
        const bool isOutgoing,
        const QString &contactUin,
        const FileSharingInfoSptr& fsInfo,
        const bool previewsEnabled)
		: PreviewContentWidget(parent, isOutgoing, QString(), previewsEnabled, contactUin)
        , ParentItem_(parent)
		, FsInfo_(fsInfo)
		, BytesTransferred_(0)
		, DownloadingBarBaseAngle_(DOWNLOADING_BAR_BASE_ANGLE_MIN)
		, BaseAngleAnimation_(nullptr)
        , CopyFile_(false)
        , SaveAs_(false)
        , PreviewState_(PreviewState::NoPreview)
        , PreviewDownloadId_(-1)
        , FileMetainfoDownloadId_(-1)
        , PreviewMetainfoDownloadId_(-1)
        , FileDownloadId_(-1)
        , IsCtrlButtonHovered_(false)
        , ShareButton_(nullptr)
        , IsInPreloadDistance_(true)
        , autoplayVideo_(Ui::get_gui_settings()->get_value<bool>(settings_autoplay_video, true))
        , ref_(new bool(false))
	{
		assert(!contactUin.isEmpty());
		assert(FsInfo_);
		assert(isOutgoing == FsInfo_->IsOutgoing());

		Private_.State_ = State::Initial;

		Metainfo_.FileSize_ = 0;

        if (FsInfo_->HasSize())
        {
		    setPreviewGenuineSize(FsInfo_->GetSize());
        }

        connectSignals();

        setInitialWidgetSizeAndState();

        const auto isImageFileInitial = isState(State::ImageFile_Initial);
        const auto isPlainFileInitial = isState(State::PlainFile_Initial);
        assert(isImageFileInitial || isPlainFileInitial);

        if (FsInfo_->IsOutgoing())
        {
            const auto &uploadingProcessId = FsInfo_->GetUploadingProcessId();
            if (!uploadingProcessId.isEmpty())
            {
                assert(FsInfo_->IsOutgoing());
                assert(FsInfo_->HasLocalPath());

                resumeUploading();

                return;
            }
        }

        if (isImageFileInitial)
        {
            requestPreviewMetainfo();
        }

        requestFileMetainfo();
        
        initialize();
	}

	FileSharingWidget::~FileSharingWidget()
	{
    }

    void FileSharingWidget::enterEvent(QEvent *event)
    {
        PreviewContentWidget::enterEvent(event);

        const auto isUploading = (
            isState(State::ImageFile_Uploading) ||
            isState(State::PlainFile_Uploading));
        if (!ShareButton_ && !isUploading)
        {
            initializeShareButton();
        }
        if (ShareButton_ && !isUploading)
        {
            updateShareButtonGeometry();

            ShareButton_->setVisible(true);
        }
    }

	void FileSharingWidget::initialize()
	{
        PreviewContentWidget::initialize();

		__TRACE(
            "fs",
            "initializing file sharing widget\n" <<
            FsInfo_->ToLogString());

        assert(!CurrentCtrlIcon_);
        CurrentCtrlIcon_ = downloadButton();

		setMouseTracking(true);

        initializeShareButton();

        invalidateSizes();
        update();
	}

	QString FileSharingWidget::toLogString() const
	{
		QString result;
		result.reserve(512);

		QTextStream fmt(&result);

		fmt << "    widget=<file_sharing>\n";

		if (FsInfo_->IsOutgoing())
		{
			fmt << "    type=<outgoing>\n"
				   "    local_path=<" << FsInfo_->GetLocalPath() << ">";
		}
		else
		{
			fmt << "    type=<incoming>\n"
				   "    uri=<" << FsInfo_->GetUri() << ">";
		}

		return result;
	}

    QString FileSharingWidget::toRecentsString() const
    {
        return QT_TRANSLATE_NOOP("contact_list", "File");
    }

	QString FileSharingWidget::toString() const
	{
		return toLink();
	}

    QString FileSharingWidget::toLink() const
    {
        if (!FsInfo_->HasUri())
        {
            return FsInfo_->GetLocalPath();
        }

        return FsInfo_->GetUri();
    }

    bool FileSharingWidget::hasOpenInBrowserMenu()
    {
        return FsInfo_->getContentType() != core::file_sharing_content_type::undefined && FsInfo_->getContentType() != core::file_sharing_content_type::ptt;
    }

    void FileSharingWidget::onActivityChanged(const bool isActive)
    {
        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        if (isActive && (!mainWindow || !mainWindow->isActive() || !ParentItem_ || Logic::getContactListModel()->selectedContact() != ParentItem_->getAimid()))
            return;

        if (isGifOrVideo() && videoPlayer_)
            videoPlayer_->setPaused(!isActive, false);
    }

    void FileSharingWidget::onVisibilityChanged(const bool isVisible)
    {
        if (IsVisible_ == isVisible)
            return;

        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        if (isVisible && (!mainWindow || !mainWindow->isActive() || !ParentItem_ || Logic::getContactListModel()->selectedContact() != ParentItem_->getAimid()))
            return;

        IsVisible_ = isVisible;

        if (isGifOrVideo() && videoPlayer_ && !videoPlayer_->getAttachedPlayer())
        {
            bool pause = !isVisible;
            videoPlayer_->setPaused(pause, false);
        }
    }

    void FileSharingWidget::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
    {
        if (!isGifOrVideo())
            return;

        auto isInPreload = isInPreloadRange(_widgetAbsGeometry, _viewportVisibilityAbsRect);
        if (IsInPreloadDistance_ == isInPreload)
        {
            return;
        }
        IsInPreloadDistance_ = isInPreload;

        onChangeLoadState(IsInPreloadDistance_);
    }

    void FileSharingWidget::copyFile()
    {
        if (DownloadedFileLocalPath_.isEmpty())
        {
            CopyFile_ = true;
            if (isImagePreview())
            {
                startDownloadingFullImage();
                return;
            }

            startDownloadingPlainFile();

            return;
        }

        Utils::copyFileToClipboard(DownloadedFileLocalPath_);
    }

    void FileSharingWidget::saveAs()
    {
        std::weak_ptr<bool> wr_ref = ref_;
        Utils::saveAs(Metainfo_.Filename_, [this, wr_ref](const QString& file, const QString& dir)
        {
            auto ref = wr_ref.lock();
            if (!ref)
                return;

            QString fullname = dir;
            const auto addTrailingSlash = (!dir.endsWith('\\') && !dir.endsWith('/'));
            if (addTrailingSlash)
            {
                fullname += "/";
            }
            fullname += file;
            QFile::remove(fullname);

            if (isImagePreview())
            {
                setState(State::ImageFile_Downloading);
            }
            else
            {
                setState(State::PlainFile_Downloading);
            }

            startDataTransferAnimation();

            const auto procId = Ui::GetDispatcher()->downloadSharedFile(aimId_, FsInfo_->GetUri(), false, fullname);

            assert(FileDownloadId_ == -1);
            FileDownloadId_ = procId;

            SaveAs_ = true;
        });
    }

    bool FileSharingWidget::hasContextMenu(QPoint) const
    {
        return true;
    }

	bool FileSharingWidget::canStartImageDownloading(const QPoint &mousePos) const
	{
        const auto isInRightState = isState(State::ImageFile_MetainfoLoaded);

		return (isInRightState && isOverPreview(mousePos));
	}

    void FileSharingWidget::connectErrorSignal()
    {
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingError,
            this,
            &FileSharingWidget::fileSharingError,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
    }

	void FileSharingWidget::connectFileDownloadSignals()
	{
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingFileDownloaded,
            this,
            &FileSharingWidget::fileDownloaded,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingFileDownloading,
            this,
            &FileSharingWidget::fileDownloading,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
	}

	void FileSharingWidget::connectMetainfoSignal()
	{
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingFileMetainfoDownloaded,
            this,
            &FileSharingWidget::onFileMetainfo,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingPreviewMetainfoDownloaded,
            this,
            &FileSharingWidget::onPreviewMetainfo,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
	}

	void FileSharingWidget::connectFileUploadingSignals()
	{
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingUploadingResult,
            this,
            &FileSharingWidget::fileSharingUploadingResult,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingUploadingProgress,
            this,
            &FileSharingWidget::fileSharingUploadingProgress,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
	}

    void FileSharingWidget::connectSignals()
    {
        connectErrorSignal();

        connectFileDownloadSignals();

        if (isOutgoing())
        {
            connectFileUploadingSignals();
        }

        connectMetainfoSignal();

        connectPreviewSignals();
    }

	void FileSharingWidget::convertToPlainFileView()
	{
		setState(State::PlainFile_MetainfoLoaded);
	}

	void FileSharingWidget::convertToUploadErrorView()
	{
		setState(State::PlainFile_UploadError);

		Metainfo_.Filename_ = "not found";
		Metainfo_.FileSize_ = 0;
		Metainfo_.FileSizeStr_.resize(0);

        update();
	}

    void FileSharingWidget::connectPreviewSignals()
    {
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::imageDownloaded,
            this,
            &FileSharingWidget::previewDownloaded,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::imageDownloadError,
            this,
            &FileSharingWidget::previewDownloadError,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
    }

	bool FileSharingWidget::isBlockElement() const
	{
		return (isState(State::PlainFile_Initial) ||
				isState(State::PlainFile_MetainfoLoaded) ||
				isState(State::PlainFile_Downloading) ||
				isState(State::PlainFile_Downloaded) ||
				isState(State::PlainFile_Uploading) ||
				isState(State::PlainFile_Uploaded) ||
				isState(State::PlainFile_UploadError));
	}

    bool FileSharingWidget::canReplace() const
    {
        return false;
    }

    bool FileSharingWidget::canUnload() const
	{
		return (!isState(State::PlainFile_Initial) &&
				!isState(State::PlainFile_Downloading) &&
				!isState(State::PlainFile_Uploading) &&
				!isState(State::ImageFile_Initial) &&
				!isState(State::ImageFile_Downloading));
	}

    bool FileSharingWidget::isPreloaderVisible() const
    {
        return !isDataTransferProgressVisible();
    }

    void FileSharingWidget::leaveEvent(QEvent *event)
    {
        PreviewContentWidget::leaveEvent(event);

        if (ShareButton_)
        {
            ShareButton_->setVisible(false);
        }
    }

	void FileSharingWidget::mouseMoveEvent(QMouseEvent *event)
	{
        const auto mousePos = event->pos();

        if (isControlButtonVisible())
		{
			if (isOverControlButton(mousePos) &&
				!isState(State::PlainFile_Downloaded) &&
				!isState(State::PlainFile_Uploaded) &&
				!isState(State::ImageFile_Uploaded))
			{
				setCursor(Qt::PointingHandCursor);

                if (!IsCtrlButtonHovered_)
                {
                    IsCtrlButtonHovered_ = true;
                    update();
                }

				return;
			}
		}

        if (IsCtrlButtonHovered_)
        {
            IsCtrlButtonHovered_ = false;
            update();
        }

		if (isOpenDownloadsDirButtonVisible())
		{
			if (isOverOpenDownloadsDirButton(mousePos))
			{
				setCursor(Qt::PointingHandCursor);
				return;
			}
		}

        const auto isClickable = (
            canStartImageDownloading(mousePos) ||
            isState(State::ImageFile_Downloaded) ||
            isState(State::ImageFile_Uploaded)
        );
		if (isClickable && isOverPreview(mousePos))
		{
			setCursor(Qt::PointingHandCursor);
			return;
		}

		setCursor(Qt::ArrowCursor);
		event->ignore();

        return PreviewContentWidget::mouseMoveEvent(event);
	}

	void FileSharingWidget::mouseReleaseEvent(QMouseEvent *event)
	{
        event->ignore();

        Ui::HistoryControlPage* page = Utils::InterConnector::instance().getHistoryPage(ParentItem_->getAimid());
        if (page && page->touchScrollInProgress())
        {
            return;
        }

		const auto isLeftClick = (event->button() == Qt::LeftButton);
		if (!isLeftClick)
		{
			return;
		}

		const auto mousePos = event->pos();

        const auto isOpenDownloadsDirButtonClicked = (
            isOpenDownloadsDirButtonVisible() &&
            isOverOpenDownloadsDirButton(mousePos));
        if (isOpenDownloadsDirButtonClicked)
        {
            openDownloadsDir();
            return;
        }

		const auto clickedOnButton = (isControlButtonVisible() && isOverControlButton(mousePos));

        const auto isDownloading = (isState(State::PlainFile_Downloading) || isState(State::ImageFile_Downloading));
		const auto isStopDownloadButtonClicked = (clickedOnButton && isDownloading);
		if (isStopDownloadButtonClicked)
		{
			stopDownloading();
			return;
		}

        const auto isUploading = (isState(State::PlainFile_Uploading) || isState(State::ImageFile_Uploading));
		const auto isStopUploadButtonClicked = (clickedOnButton && isUploading);
		if (isStopUploadButtonClicked)
		{
			stopUploading();
			return;
		}

        if (isState(State::PlainFile_MetainfoLoaded))
        {
            startDownloadingPlainFile();
            return;
        }

        const auto fullImageReady = (
            isState(State::ImageFile_Downloaded) ||
            isState(State::ImageFile_Uploaded));

		if (isOverPreview(mousePos))
		{
            const auto openPreviewer = (fullImageReady && !isGifOrVideo());
            if (openPreviewer)
            {
                showPreviewer(event->globalPos());
                return;
            }

            if (isGifOrVideo() && videoPlayer_)
            {
                onGifOrVideoClicked();
                return;
            }
		}

        if (canStartImageDownloading(mousePos))
        {
            setState(State::ImageFile_Downloading);

            startDownloadingFullImage();
        }

        return PreviewContentWidget::mouseReleaseEvent(event);
    }

    QString FileSharingWidget::elideFilename(const QString &text, const QFont &font, const int32_t maxTextWidth)
    {
        assert(maxTextWidth > 0);
        assert(!text.isEmpty());

        QFontMetrics fontMetrics(font);
        return fontMetrics.elidedText(text, Qt::ElideRight, maxTextWidth);
    }

    void FileSharingWidget::formatFileSizeStr()
    {
        if (!Metainfo_.FileSizeStr_.isEmpty())
        {
            return;
        }

        assert(Metainfo_.FileSize_ >= 0);

        Metainfo_.FileSizeStr_ = formatFileSize(Metainfo_.FileSize_);

        assert(!Metainfo_.FileSizeStr_.isEmpty());
    }

	const QRect& FileSharingWidget::getControlButtonRect(const QSize &iconSize) const
	{
        assert(!iconSize.isEmpty());

		if (isState(State::PlainFile_MetainfoLoaded) ||
			isState(State::PlainFile_Downloading) ||
			isState(State::PlainFile_Downloaded) ||
			isState(State::PlainFile_Uploading) ||
			isState(State::PlainFile_Uploaded))
		{
			return getControlButtonPlainRect(iconSize);
		}

		return getControlButtonPreviewRect(iconSize);
	}

	const QRect& FileSharingWidget::getControlButtonPlainRect(const QSize &iconSize) const
	{
        assert(!iconSize.isEmpty());

        auto &btnRect = Private_.ControlButtonPlainRect_;

        const auto resetRect = (
            btnRect.isEmpty() ||
            (btnRect.size() != iconSize));
        if (resetRect)
        {
			const auto x = Ui::MessageStyle::getRotatingProgressBarPenWidth();
			const auto y = getCtrlButtonTopOffset();

			btnRect = QRect(QPoint(x, y), iconSize);
		}

		return btnRect;
	}

	const QRect& FileSharingWidget::getControlButtonPreviewRect(const QSize &iconSize) const
	{
        assert(!iconSize.isEmpty());
		assert(isState(State::ImageFile_Downloading) ||
               isState(State::ImageFile_Downloaded) ||
               isState(State::ImageFile_MetainfoLoaded) ||
			   isState(State::ImageFile_Uploading) ||
               isState(State::ImageFile_Uploaded));

        auto &btnRect = Private_.ControlButtonPreviewRect_;

        const auto resetRect = (
            btnRect.isEmpty() ||
            (btnRect.size() != iconSize));
		if (resetRect)
		{
			QRect buttonRect(QPoint(), iconSize);
			buttonRect.moveCenter(getPreviewScaledRect().center());

			btnRect = buttonRect;
		}

		return btnRect;
	}

	bool FileSharingWidget::getLocalFileMetainfo()
	{
		assert(FsInfo_);

		const auto &localPath = FsInfo_->GetLocalPath();
		QFileInfo info(localPath);

		if (!info.exists())
		{
			return false;
		}

		assert(Metainfo_.Filename_.isEmpty());
		assert(Metainfo_.FileSize_ == 0);
		assert(Metainfo_.FileSizeStr_.isEmpty());

		Metainfo_.Filename_ = info.fileName();
		Metainfo_.FileSize_ = info.size();

		formatFileSizeStr();

		return true;
	}

	FileSharingWidget::State FileSharingWidget::getState() const
	{
		assert(Private_.State_ > State::Min);
		assert(Private_.State_ < State::Max);

		return Private_.State_;
	}

    void FileSharingWidget::initializeShareButton()
    {
        assert(!ShareButton_);

        ShareButton_ = new Ui::ActionButtonWidget(Ui::ActionButtonWidget::ResourceSet::ShareContent_, this);
        ShareButton_->setVisible(false);

        const auto success = QObject::connect(
            ShareButton_,
            &Ui::ActionButtonWidget::startClickedSignal,
            this,
            &FileSharingWidget::onShareButtonClicked);
        assert(success);
    }

	bool FileSharingWidget::isControlButtonVisible() const
	{
        if (isState(State::ImageFile_MetainfoLoaded) ||
            isState(State::ImageFile_Downloaded) ||
            isState(State::ImageFile_Uploaded))
        {
            const auto isGif = (FsInfo_->getContentType() == core::file_sharing_content_type::gif);
            if (isGif)
            {
                return !isGifOrVideoPlaying();
            }

            if (!autoplayVideo_)
            {
                const auto isVideo = (FsInfo_->getContentType() == core::file_sharing_content_type::video);
                return isVideo;
            }
            else
            {
                return false;
            }
            
        }

		return isState(State::PlainFile_MetainfoLoaded) ||
			   isState(State::PlainFile_Downloading) ||
			   isState(State::PlainFile_Downloaded) ||
			   isState(State::PlainFile_Uploading) ||
			   isState(State::PlainFile_Uploaded) ||
			   isState(State::ImageFile_Downloading) ||
			   isState(State::ImageFile_Uploading);
	}

	bool FileSharingWidget::isDataTransferProgressVisible() const
	{
		return isState(State::PlainFile_Downloading) ||
			   isState(State::PlainFile_Uploading) ||
			   isState(State::ImageFile_Downloading) ||
			   isState(State::ImageFile_Uploading);
	}

    bool FileSharingWidget::isFullImageDownloading() const
    {
        assert(isImagePreview());
        assert(FileDownloadId_ >= -1);

        return (FileDownloadId_ > 0);
    }

    bool FileSharingWidget::isGifOrVideo() const
    {
        return (
            isImagePreview() &&
            FsInfo_ &&
            (FsInfo_->getContentType() == core::file_sharing_content_type::gif || FsInfo_->getContentType() == core::file_sharing_content_type::video));
    }

    bool FileSharingWidget::isGif() const
    {
        return (
            isImagePreview() &&
            FsInfo_ &&
            (FsInfo_->getContentType() == core::file_sharing_content_type::gif));
    }

    bool FileSharingWidget::isGifOrVideoPlaying() const
    {
        assert(isGifOrVideo());

        return (videoPlayer_ &&
                (videoPlayer_->state() == QMovie::Running));
    }

    bool FileSharingWidget::isImagePreview() const
    {
        return (
            (getState() > State::ImageFile_Min) &&
            (getState() < State::ImageFile_Max)
        );
    }

	bool FileSharingWidget::isFilenameAndSizeVisible() const
	{
		return isState(State::PlainFile_MetainfoLoaded) ||
			   isState(State::PlainFile_Downloading) ||
			   isState(State::PlainFile_Downloaded) ||
			   isState(State::PlainFile_Uploading) ||
			   isState(State::PlainFile_Uploaded);
	}

	bool FileSharingWidget::isOpenDownloadsDirButtonVisible() const
	{
		return isState(State::PlainFile_Downloaded);
	}

	bool FileSharingWidget::isOverControlButton(const QPoint &p) const
	{
        assert(CurrentCtrlIcon_);

		auto downloadButtonRect = getControlButtonRect(CurrentCtrlIcon_->GetSize());
		const auto margins = contentsMargins();
		downloadButtonRect.translate(margins.left(), margins.top());

		return downloadButtonRect.contains(p);
	}

	bool FileSharingWidget::isOverOpenDownloadsDirButton(const QPoint &p) const
	{
		assert(isOpenDownloadsDirButtonVisible());

		const auto margins = contentsMargins();
		auto controlRect = OpenDownloadsDirButtonRect_;
		controlRect.translate(margins.left(), margins.top());

		return controlRect.contains(p);
	}

    bool FileSharingWidget::isOverPreview(const QPoint &p) const
    {
        return getPreviewScaledRect().contains(p);
    }

	bool FileSharingWidget::isPreviewVisible() const
	{
        assert(PreviewState_ > PreviewState::Min);
        assert(PreviewState_ < PreviewState::Max);

		return (
            (PreviewState_ == PreviewState::MiniPreviewLoaded) ||
            (PreviewState_ == PreviewState::FullPreviewLoaded)
        );
	}

	bool FileSharingWidget::isState(const State state) const
	{
		return (Private_.State_ == state);
	}

    void FileSharingWidget::loadGifOrVideo(const QString &localPath)
    {
        assert(isGifOrVideo());
        assert(!localPath.isEmpty());

        load_task_ = std::unique_ptr<Utils::LoadMovieToFFMpegPlayerFromFileTask>(
            new Utils::LoadMovieToFFMpegPlayerFromFileTask(localPath, isGif(), this));

        QObject::connect(
            load_task_.get(),
            &Utils::LoadMovieToFFMpegPlayerFromFileTask::loadedSignal,
            this,
            [this]
            (QSharedPointer<Ui::DialogPlayer> movie)
            {
                assert(movie);

                if (!IsInPreloadDistance_)
                    return;

                videoPlayer_ = movie;
                videoPlayer_->setPreview(getPreview());

                QObject::connect(
                    videoPlayer_.data(),
                    &Ui::DialogPlayer::paused,
                    this,
                    &FileSharingWidget::onPaused,
                    (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

                auto mainWindow = Utils::InterConnector::instance().getMainWindow();
                bool play = IsVisible_ && mainWindow->isActiveWindow();

                bool mute = true;
                int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);
                videoPlayer_->setParent(this);
                videoPlayer_->start(play);

                videoPlayer_->setVolume(volume);
                videoPlayer_->setMute(mute);
                
                const auto widgetRect = getPreviewScaledRect();
                videoPlayer_->updateSize(widgetRect);
                videoPlayer_->show();

                update();
            });

        load_task_->run();
    }

	bool FileSharingWidget::loadPreviewFromLocalFile()
	{
		assert(isState(State::ImageFile_Initial));

		const auto fileExists = QFile::exists(FsInfo_->GetLocalPath());
        if (!fileExists)
        {
            return false;
        }

        auto task = new Utils::LoadPixmapFromFileTask(FsInfo_->GetLocalPath());

        QObject::connect(
            task,
            &Utils::LoadPixmapFromFileTask::loadedSignal,
            this,
            &FileSharingWidget::localPreviewLoaded);

        QThreadPool::globalInstance()->start(task);

		return true;
	}

	void FileSharingWidget::renderPreview(QPainter &p, const bool /*isAnimating*/, QPainterPath& _path, const QColor& quote_color)
	{
        //qDebug() << "start renderPreview " << QTime::currentTime();

        if (isImagePreview())
        {
            PreviewContentWidget::renderPreview(p, isDataTransferProgressVisible(), _path, quote_color);
        }

        if (isControlButtonVisible())
        {
            renderControlButton(p);
        }
        
        if (isFilenameAndSizeVisible())
		{
			renderFilename(p);
			renderFileSizeAndProgress(p);
		}

		if (isDataTransferProgressVisible())
		{
			renderDataTransferProgress(p);

            if (isImagePreview())
            {
                renderDataTransferProgressText(p);
            }
		}

		if (isOpenDownloadsDirButtonVisible())
		{
			renderOpenDownloadsDirButton(p);
		}
	}

    void FileSharingWidget::resizeEvent(QResizeEvent *e)
    {
        Private_.ControlButtonPreviewRect_ = QRect();

        const auto widgetRect = getPreviewScaledRect();

        if (videoPlayer_)
            videoPlayer_->updateSize(widgetRect);
        
        PreviewContentWidget::resizeEvent(e);
    }

    bool FileSharingWidget::drag()
    {
        QPixmap p = Preview_.FullImg_;
        if (p.isNull() && FileTypeIcon_)
            p = FileTypeIcon_->GetPixmap();

        return Utils::dragUrl(this, p, FsInfo_ ? FsInfo_->GetUri() : Metainfo_.DownloadUri_);
    }

    void FileSharingWidget::onGifOrVideoClicked()
    {
        assert(isGifOrVideo());
        assert(videoPlayer_);

        if (!videoPlayer_)
        {
            return;
        }

        videoPlayer_->setPaused(isGifOrVideoPlaying(), true);

        update();
    }

    void FileSharingWidget::shareRoutine()
    {
        assert(FsInfo_->HasUri());
        
        Ui::SelectContactsWidget shareDialog(
             nullptr,
             Logic::MembersWidgetRegim::SHARE_LINK,
             QT_TRANSLATE_NOOP("popup_window", "Share link"),
             QT_TRANSLATE_NOOP("popup_window", "Copy link and close"),
             FsInfo_->GetUri(),
             Ui::MainPage::instance(),
             true);
        
        const auto action = shareDialog.show();
        if (action != QDialog::Accepted)
        {
            return;
        }
        
        const auto contact = shareDialog.getSelectedContact();
        if (contact != "")
        {
            Logic::getContactListModel()->setCurrent(contact, -1, true);
            Ui::GetDispatcher()->sendMessageToContact(contact, FsInfo_->GetUri());
        }
        else
        {
            QApplication::clipboard()->setText(FsInfo_->GetUri());
        }
    }
    
    void FileSharingWidget::onShareButtonClicked()
    {
        shareRoutine();
    }

	void FileSharingWidget::openDownloadsDir() const
	{
#ifdef _WIN32
		assert(QFile::exists(DownloadedFileLocalPath_));

		const auto param = ("/select," + QDir::toNativeSeparators(DownloadedFileLocalPath_));
		const auto command = "explorer " + param;
		QProcess::startDetached(command);
#else

#ifdef __APPLE__
        MacSupport::openFinder(DownloadedFileLocalPath_);
#else
        QDir dir(DownloadedFileLocalPath_);
        dir.cdUp();
        QDesktopServices::openUrl(dir.absolutePath());
#endif

#endif //_WIN32
	}

	void FileSharingWidget::renderControlButton(QPainter &p)
	{
		assert(isControlButtonVisible());

		if (isState(State::PlainFile_Downloading) ||
			isState(State::PlainFile_Uploading) ||
			isState(State::ImageFile_Downloading) ||
			isState(State::ImageFile_Uploading))
		{
            CurrentCtrlIcon_ = downloadCancelButton(isImagePreview());

            const auto btnRect = getControlButtonRect(CurrentCtrlIcon_->GetSize());
			CurrentCtrlIcon_->Draw(p, btnRect);

			return;
		}

        const auto isMetainfoReady = (
            isState(State::PlainFile_Downloaded) ||
            isState(State::PlainFile_Uploaded) ||
            isState(State::PlainFile_MetainfoLoaded));
        const auto isFilenameReady = (isMetainfoReady && !Metainfo_.Filename_.isEmpty());
		if (isFilenameReady)
		{
			if (!FileTypeIcon_)
			{
				FileTypeIcon_ = History::GetIconByFilename(Metainfo_.Filename_);
			}

            CurrentCtrlIcon_ = FileTypeIcon_;

            const auto btnRect = getControlButtonRect(CurrentCtrlIcon_->GetSize());
			FileTypeIcon_->Draw(p, btnRect);

			return;
		}

        if (isState(State::ImageFile_MetainfoLoaded) ||
            isState(State::ImageFile_Downloaded) ||
            isState(State::ImageFile_Uploaded))
        {
            const auto isGif = (FsInfo_->getContentType() == core::file_sharing_content_type::gif);
            const auto isVideo = (FsInfo_->getContentType() == core::file_sharing_content_type::video);

            assert(isGif || isVideo);

            CurrentCtrlIcon_ = playStartButton(isGif, IsCtrlButtonHovered_);

            const auto btnRect = getControlButtonRect(CurrentCtrlIcon_->GetSize());
            CurrentCtrlIcon_->Draw(p, btnRect);

            return;
        }

        CurrentCtrlIcon_ = downloadButton();

        const auto btnRect = getControlButtonRect(CurrentCtrlIcon_->GetSize());
        CurrentCtrlIcon_->Draw(p, btnRect);
        qDebug() << "draw gif button" << QTime::currentTime();
	}

	void FileSharingWidget::renderDataTransferProgress(QPainter &p)
	{
		assert(Metainfo_.FileSize_ >= 0);
		assert(BytesTransferred_ >= 0);
		assert(BytesTransferred_ <= Metainfo_.FileSize_);
		assert(DownloadingBarBaseAngle_ >= DOWNLOADING_BAR_BASE_ANGLE_MIN);
		assert(DownloadingBarBaseAngle_ <= DOWNLOADING_BAR_BASE_ANGLE_MAX);

		p.save();

		auto percents = 1.0;
		if (Metainfo_.FileSize_ > 0)
		{
			percents = std::max(0.05, (double)BytesTransferred_ / (double)Metainfo_.FileSize_);
		}
		assert(percents <= (1.0 + std::numeric_limits<double>::epsilon()));

		const auto angle = (int)std::ceil(percents * 360 * 16);
		assert(angle <= (361 * 16));

		const auto pen = Ui::MessageStyle::getRotatingProgressBarPen();
		p.setPen(pen);

        assert(CurrentCtrlIcon_);
        auto arcRect(
            getControlButtonRect(
                CurrentCtrlIcon_->GetSize()));

        const auto argRectMargin = (Ui::MessageStyle::getRotatingProgressBarPenWidth() / 2);
        const QMargins arcRectMargins(argRectMargin, argRectMargin, argRectMargin, argRectMargin);
        arcRect = arcRect.marginsRemoved(arcRectMargins);

		const auto baseAngle = (DownloadingBarBaseAngle_ * 16);
		p.drawArc(arcRect, -baseAngle, -angle);

		p.restore();
	}

    void FileSharingWidget::renderFilename(QPainter &p)
	{
        auto textX = CurrentCtrlIcon_ ? CurrentCtrlIcon_->GetWidth() : 0;
        textX += Style::Files::getFilenameLeftMargin();

        const auto textY = Style::Files::getFilenameBaseline();

        auto font = Style::Files::getFilenameFont();

        QString filename;

        if (!Metainfo_.Filename_.isEmpty())
        {
            const auto maxTextWidth = (
                width() -
                textX -
                Ui::MessageStyle::getBubbleHorPadding());
            filename = elideFilename(Metainfo_.Filename_, font, maxTextWidth);
        }
        else
        {
            filename = QT_TRANSLATE_NOOP("contact_list", "File");
        }

        assert(!filename.isEmpty());

		p.save();

		p.setFont(font);
		p.setPen(Ui::MessageStyle::getTextColor());

		p.drawText(textX, textY, filename);

		p.restore();
	}

	void FileSharingWidget::renderFileSizeAndProgress(QPainter &p)
	{
		assert(Metainfo_.FileSize_ >= 0);

        const auto isFileSizeUnknown = (Metainfo_.FileSize_ == 0);
        if (isFileSizeUnknown)
        {
            return;
        }

		formatFileSizeStr();

		p.save();

        QFont font = Fonts::appFontScaled(12);

		p.setFont(font);
        p.setPen(Style::Files::getFileSizeColor());

		auto x = CurrentCtrlIcon_ ? CurrentCtrlIcon_->GetWidth() : 0;
		x += Style::Files::getFilenameLeftMargin();

		auto y = Style::Files::getFileSizeBaseline();

		QString text;
		if (isState(State::PlainFile_Downloading) || isState(State::PlainFile_Uploading))
		{
			text = formatFileSize(BytesTransferred_);
			text += " of ";
		}

		text += Metainfo_.FileSizeStr_;

		p.drawText(x, y, text);

		FileSizeAndProgressStr_ = text;

		p.restore();
	}

    void FileSharingWidget::renderDataTransferProgressText(QPainter &p)
    {
        assert(isImagePreview());

        formatFileSizeStr();

        QString text;
        text.reserve(128);

        text += formatFileSize(BytesTransferred_);
        text += " of ";
        text += Metainfo_.FileSizeStr_;

        const auto font = Ui::MessageStyle::getRotatingProgressBarTextFont();
        const auto pen = Ui::MessageStyle::getRotatingProgressBarTextPen();

        const auto isTextChanged = (text != LastProgressText_);
        if (isTextChanged)
        {
            const auto &iconRect = getControlButtonRect(CurrentCtrlIcon_ ? CurrentCtrlIcon_->GetSize() : QSize(0, 0));

            QFontMetrics m(font);
            const auto textSize = m.boundingRect(text).size();

            ProgressTextRect_ = QRect(QPoint(), textSize);

            ProgressTextRect_.moveCenter(iconRect.center());

            ProgressTextRect_.moveTop(
                iconRect.bottom() + 1 +
                Ui::MessageStyle::getRotatingProgressBarTextTopMargin());

            LastProgressText_ = text;
        }

        p.save();

        p.setFont(font);
        p.setPen(pen);
        assert(!ProgressTextRect_.isEmpty());
        p.drawText(ProgressTextRect_, Qt::AlignHCenter, text);

        p.restore();
    }

	void FileSharingWidget::renderOpenDownloadsDirButton(QPainter &p)
	{
		assert(!FileSizeAndProgressStr_.isEmpty());

		p.save();

		QFont font = Style::Files::getShowInDirLinkFont();

		QFontMetrics m(font);
		const auto fileSizeAndProgressWidth = m.tightBoundingRect(FileSizeAndProgressStr_).width();

		auto x = CurrentCtrlIcon_ ? CurrentCtrlIcon_->GetWidth() : 0;
		x += Style::Files::getFilenameLeftMargin();
		x += fileSizeAndProgressWidth;
		x += Style::Files::getShowInDirLinkLeftMargin();

		auto y = Style::Files::getFileSizeBaseline();

        static const QString TEXT = QT_TRANSLATE_NOOP("chat_page","Show in folder");

		auto controlRect = m.boundingRect(TEXT);
		controlRect.translate(x, y);
        controlRect.setWidth(controlRect.width() + Utils::scale_value(2));
		OpenDownloadsDirButtonRect_ = controlRect;

		p.setFont(font);
		p.setPen(QPen(Ui::CommonStyle::getLinkColor()));
		p.drawText(controlRect, TEXT);

		p.restore();
	}

	void FileSharingWidget::requestFileMetainfo()
	{
		assert(FsInfo_);
		assert(isState(State::PlainFile_Initial) || isState(State::ImageFile_Initial));

		const auto procId = Ui::GetDispatcher()->downloadFileSharingMetainfo(FsInfo_->GetUri());

        assert(FileMetainfoDownloadId_ == -1);
        FileMetainfoDownloadId_ = procId;
	}

	void FileSharingWidget::requestPreview()
	{
        if (PreviewState_ == PreviewState::FullPreviewLoaded)
        {
            assert(!"invalid preview state");
            return;
        }

		const auto &previewUri = (
            (PreviewState_ == PreviewState::NoPreview) ?
                Metainfo_.MiniPreviewUri_ :
                Metainfo_.FullPreviewUri_
        );

		if (previewUri.isEmpty())
		{
            assert(!"unexpected preview uri");
			return;
		}

        assert(PreviewDownloadId_ == -1);
        PreviewDownloadId_ = Ui::GetDispatcher()->downloadImage(previewUri, "#", QString(), false, 0, 0);
	}

    void FileSharingWidget::requestPreviewMetainfo()
    {
        assert(FsInfo_);
        assert(isState(State::ImageFile_Initial));

        const auto procId = Ui::GetDispatcher()->getFileSharingPreviewSize(FsInfo_->GetUri());

        assert(PreviewMetainfoDownloadId_ == -1);
        PreviewMetainfoDownloadId_ = procId;
    }

	void FileSharingWidget::resumeUploading()
	{
		assert(FsInfo_);
		assert(FsInfo_->IsOutgoing());
		assert(isState(State::PlainFile_Initial) || isState(State::ImageFile_Initial));

		if (!getLocalFileMetainfo())
		{
			convertToUploadErrorView();
			return;
		}

		if (isState(State::PlainFile_Initial))
		{
			setState(State::PlainFile_Uploading);

            startDataTransferAnimation();
		}
		else
		{
			if (!loadPreviewFromLocalFile())
            {
                convertToUploadErrorView();
                return;
            }
		}

		__INFO(
			"fs",
			"resuming file sharing upload\n"
			"	local_path=<" << FsInfo_->GetLocalPath() << ">\n"
			"	uploading_id=<" << FsInfo_->GetUploadingProcessId() << ">");
	}

    void FileSharingWidget::retryRequest()
    {
        if (Retry_.ShouldRetryFileMetainfo())
        {
            ++Retry_.FileMetainfoRetryCount_;

            FileMetainfoDownloadId_ = -1;

            requestFileMetainfo();
        }

        if (Retry_.ShouldRetryFileDownload())
        {
            ++Retry_.FileDownloadRetryCount_;

            FileDownloadId_ = -1;

            startDownloadingPlainFile();
        }

        if (Retry_.ShouldRetryPreviewDownload())
        {
            ++Retry_.PreviewDownloadRetryCount_;

            PreviewDownloadId_ = -1;

            requestPreview();
        }
    }

    bool FileSharingWidget::retryRequestLater()
    {
        assert(Retry_.HasRetryFlagSet());

        if (!Retry_.ShouldRetry())
        {
            return false;
        }

        QTimer::singleShot(
            RETRY_INTERVAL_MS,
            Qt::VeryCoarseTimer,
            this,
            &FileSharingWidget::retryRequest
        );

        return true;
    }

    void FileSharingWidget::setBlockSizePolicy()
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFixedHeight(Style::Files::getFileBubbleHeight());
        emit forcedLayoutUpdatedSignal();
        updateGeometry();
        update();
    }

	void FileSharingWidget::setInitialWidgetSizeAndState()
	{
        if (!PreviewsEnabled_)
		{
			setState(State::PlainFile_Initial);
			return;
		}

		if (FsInfo_->HasSize())
		{
			setState(State::ImageFile_Initial);
			return;
		}

		if (!FsInfo_->HasLocalPath())
		{
			setState(State::PlainFile_Initial);
			return;
		}

		QFileInfo fileInfo(FsInfo_->GetLocalPath());
		const auto ext = fileInfo.suffix();
		if(ext.isEmpty() || !Utils::is_image_extension(ext))
		{
			setState(State::PlainFile_Initial);
			return;
		}

		setState(State::ImageFile_Initial);
	}

    void FileSharingWidget::setState(const State state)
	{
		assert(state > State::Min);
		assert(state < State::Max);
		assert(Private_.State_ > State::Min);
		assert(Private_.State_ < State::Max);

		const auto oldState = Private_.State_;
		Private_.State_ = state;
        emit stateChanged();

        if ((state == State::PlainFile_Initial) ||
            (state == State::PlainFile_MetainfoLoaded) ||
            (state == State::PlainFile_Uploading))
        {
            setBlockSizePolicy();
        }

		switch (oldState)
		{
			case State::Initial:
				assert(isState(State::PlainFile_Initial) ||
					   isState(State::ImageFile_Initial));
				break;

			case State::PlainFile_Initial:
				break;

			case State::PlainFile_MetainfoLoaded:
				assert(state == State::PlainFile_Downloading);
				break;

			case State::PlainFile_Downloading:
				assert((state == State::PlainFile_MetainfoLoaded) ||
					   (state == State::PlainFile_Downloaded) ||
                       (state == State::PlainFile_Downloading));
				break;

			case State::PlainFile_Uploading:
				assert(isState(State::PlainFile_Uploaded) ||
					   isState(State::PlainFile_UploadError) ||
					   isState(State::PlainFile_MetainfoLoaded) ||
					   isState(State::Deleted));
				break;

			case State::ImageFile_Initial:
				{
                    assert(isState(State::ImageFile_MetainfoLoaded) ||
						   isState(State::ImageFile_Uploading) ||
						   isState(State::ImageFile_Uploaded) ||
                           isState(State::ImageFile_Downloading) ||
                           isState(State::PlainFile_Uploading));
				}
				break;

			case State::ImageFile_MetainfoLoaded:
				assert(isState(State::PlainFile_MetainfoLoaded) ||
                       isState(State::ImageFile_Downloading));
				break;

			case State::ImageFile_Downloading:
                {
                    const auto isImageDownloaded = isState(State::ImageFile_Downloaded);
                    const auto isImageMetainfoLoaded = isState(State::ImageFile_MetainfoLoaded);

                    if (!isImageDownloaded && !isImageMetainfoLoaded)
                    {
                        assert(false);
                    }

                    if (isGifOrVideo() || SaveAs_ || CopyFile_ || !isImageDownloaded)
                    {
                        break;
                    }

                    showPreviewer(QPoint());
                }

                break;

			case State::ImageFile_Uploading:
				assert(isState(State::ImageFile_Uploaded) ||
					   isState(State::Deleted));
				break;

            case State::PlainFile_Downloaded:
                break;

			default:
				assert(!"unexpected internal state");
				break;
		}
	}

	void FileSharingWidget::startDownloadingPlainFile()
	{
		setState(State::PlainFile_Downloading);

		startDataTransferAnimation();

        const auto procId = Ui::GetDispatcher()->downloadSharedFile(aimId_, FsInfo_->GetUri(), false, QString(), true);

        assert(FileDownloadId_ == -1);
        FileDownloadId_ = procId;

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_file);
	}

	void FileSharingWidget::startDownloadingFullImage()
	{
		assert(isState(State::ImageFile_Downloading));

        // start animation

		startDataTransferAnimation();

        // start image downloading

        const auto procId = Ui::GetDispatcher()->downloadSharedFile(aimId_, FsInfo_->GetUri(), false);

        assert(FileDownloadId_ == -1);
        FileDownloadId_ = procId;

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_file);
	}

    void FileSharingWidget::showPreviewer(const QPoint& globalPos)
    {
        Q_UNUSED(globalPos);

        assert(!isGifOrVideo());

        Ui::MainPage::instance()->cancelSelection();

        Utils::InterConnector::instance().getMainWindow()->openGallery(
            getContact(), Data::Image(ParentItem_->getId(), FsInfo_->GetUri(), true), DownloadedFileLocalPath_);
    }

    void FileSharingWidget::startDataTransferAnimation()
	{
		if (!BaseAngleAnimation_)
		{
			BaseAngleAnimation_ = new QPropertyAnimation(this, "DownloadingBarBaseAngle");
			BaseAngleAnimation_->setDuration(700);
			BaseAngleAnimation_->setLoopCount(-1);
			BaseAngleAnimation_->setStartValue(DOWNLOADING_BAR_BASE_ANGLE_MIN);
			BaseAngleAnimation_->setEndValue(DOWNLOADING_BAR_BASE_ANGLE_MAX);
		}

		stopDataTransferAnimation();

		BaseAngleAnimation_->start();
	}

	void FileSharingWidget::stopDataTransferAnimation()
	{
        if (!BaseAngleAnimation_)
        {
            return;
        }

        BaseAngleAnimation_->stop();
	}

	void FileSharingWidget::stopDownloading()
	{
		assert(isState(State::PlainFile_Downloading) ||
			   isState(State::ImageFile_Downloading));

		if (isState(State::PlainFile_Downloading))
		{
			setState(State::PlainFile_MetainfoLoaded);
		}

        if (isState(State::ImageFile_Downloading))
        {
            setState(State::ImageFile_MetainfoLoaded);
        }

        Ui::GetDispatcher()->abortSharedFileDownloading(FsInfo_->GetUri());

		resetCurrentProcessId();

		BytesTransferred_ = 0;
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_cancel);
	}

	void FileSharingWidget::stopUploading()
	{
		assert(isState(State::PlainFile_Uploading) ||
			   isState(State::ImageFile_Uploading));
		assert(!FsInfo_->GetUploadingProcessId().isEmpty());
		assert(FsInfo_->HasLocalPath());

		setState(State::Deleted);

		Ui::GetDispatcher()->abortSharedFileUploading(
			ParentItem_->getAimid(),
			FsInfo_->GetLocalPath(),
			FsInfo_->GetUploadingProcessId()
		);

		emit removeMe();

		BytesTransferred_ = 0;
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_cancel);
	}

    void FileSharingWidget::updateShareButtonGeometry()
    {
        assert(ShareButton_);

        const auto &widgetContentRect = (isImagePreview() ? getLastPreviewGeometry() : rect());

        if (widgetContentRect.isEmpty())
        {
            return;
        }

        auto buttonX = (widgetContentRect.right() + 1);

        if (isImagePreview())
        {
            buttonX += Ui::MessageStyle::getTimeMarginX();
        }
        else
        {
            buttonX -= ShareButton_->sizeHint().width();
        }

        const auto buttonY = widgetContentRect.top();

        const QPoint buttonPos(buttonX, buttonY);

        const auto isButtonPosChanged = (ShareButton_->geometry().topLeft() != buttonPos);
        if (isButtonPosChanged)
        {
            ShareButton_->move(buttonPos);
        }
    }

	void FileSharingWidget::onFileMetainfo(qint64 seq, QString filename, QString downloadUri, qint64 size)
	{
		assert(!filename.isEmpty());
		assert(!downloadUri.isEmpty());
		assert(size > 0);

		if (seq != FileMetainfoDownloadId_)
		{
			return;
		}

        const auto isImageFileInitial = isState(State::ImageFile_Initial);
        const auto isPlainFileInitial = isState(State::PlainFile_Initial);

		Metainfo_.Filename_ = filename;
		Metainfo_.FileSize_ = size;
		Metainfo_.DownloadUri_ = downloadUri;

        if (isPlainFileInitial)
        {
            setState(State::PlainFile_MetainfoLoaded);
        }
        else if (isImageFileInitial)
        {
            setState(State::ImageFile_MetainfoLoaded);
        }

        FileMetainfoDownloadId_ = -1;
        Retry_.FileMetainfo_ = false;
        Retry_.FileMetainfoRetryCount_ = 0;

        invalidateSizes();
		update();
	}

    void FileSharingWidget::onPreviewMetainfo(qint64 seq, QString miniPreviewUri, QString fullPreviewUri)
    {
        assert(seq > 0);
        assert(!miniPreviewUri.isEmpty());
        assert(!fullPreviewUri.isEmpty());

        if (PreviewMetainfoDownloadId_ != seq)
        {
            return;
        }

        assert(isState(State::ImageFile_Initial) || isState(State::ImageFile_MetainfoLoaded));

        Metainfo_.MiniPreviewUri_ = miniPreviewUri;
        Metainfo_.FullPreviewUri_ = fullPreviewUri;

        requestPreview();
    }

	void FileSharingWidget::imageDownloaded(qint64 seq, QString uri, QPixmap preview, QString)
	{
		assert(!uri.isEmpty());

		if (!isCurrentProcessId(seq))
		{
			return;
		}

		const auto isDownloading = isState(State::ImageFile_Downloading);
		if (!isDownloading)
		{
            assert(!"unexpected widget state");
			return;
		}

		assert(!Metainfo_.DownloadUri_.isEmpty());
		assert(uri == Metainfo_.DownloadUri_);

        resetCurrentProcessId();

		if (preview.isNull())
		{
			return;
		}

		Preview_.FullImg_ = preview;
		setState(State::ImageFile_Downloaded);

        invalidateSizes();
		update();
	}

	void FileSharingWidget::fileDownloaded(qint64 seq, QString rawUri, QString localPath)
	{
		assert(!rawUri.isEmpty());
		assert(QFile::exists(localPath));

		if (seq != FileDownloadId_)
		{
			return;
		}

		DownloadedFileLocalPath_ = localPath;

        FileDownloadId_ = -1;
        Retry_.FileDownload_ = false;
        Retry_.FileMetainfoRetryCount_ = 0;

        stopDataTransferAnimation();

        const auto isPlainFileDownloading = isState(State::PlainFile_Downloading);
        if (isPlainFileDownloading)
        {
            setState(State::PlainFile_Downloaded);
        }

        const auto isImageDownloading = isState(State::ImageFile_Downloading);
        if (isImageDownloading)
        {
            setState(State::ImageFile_Downloaded);

            if (isGifOrVideo())
            {
                loadGifOrVideo(DownloadedFileLocalPath_);
            }
        }

        if (CopyFile_)
        {
            copyFile();
        }

        SaveAs_ = false;
        CopyFile_ = false;

        invalidateSizes();
        update();
	}

	void FileSharingWidget::fileDownloading(qint64 seq, QString uri, qint64 bytesDownloaded)
	{
		assert(bytesDownloaded >= 0);

		if (seq != FileDownloadId_)
		{
			return;
		}

		__DISABLE(
			__TRACE(
				"fs",
				"file downloading progress\n"
				<< "\tseq= " << seq << "\n"
				<< "\turi= " << uri << "\n"
				<< "\tbytes_downloaded= " << bytesDownloaded);
		);

		BytesTransferred_ = bytesDownloaded;

		update();
	}

	void FileSharingWidget::fileSharingError(qint64 seq, QString rawUri, qint32 errorCode)
	{
        assert(seq > 0);

        const auto isFileMetainfoRequestFailed = (seq == FileMetainfoDownloadId_);
        const auto isPreviewMetainfoRequestFailed = (seq == PreviewMetainfoDownloadId_);
        const auto isFileRequestFailed = (seq == FileDownloadId_);

        if (!isFileMetainfoRequestFailed &&
            !isPreviewMetainfoRequestFailed &&
            !isFileRequestFailed)
        {
            return;
        }

        if (isFileMetainfoRequestFailed)
        {
            Retry_.FileMetainfo_ = true;
        }

        if (isFileRequestFailed)
        {
            Retry_.FileDownload_ = true;
        }

        if (errorCode != static_cast<qint32>(loader_errors::cancelled) && Retry_.HasRetryFlagSet() && retryRequestLater())
        {
            return;
        }

        stopDataTransferAnimation();

        convertToPlainFileView();

        invalidateSizes();
        update();
	}

	void FileSharingWidget::fileSharingUploadingProgress(QString uploadingProcessId, qint64 bytesUploaded)
	{
		assert(bytesUploaded >= 0);
		assert(Metainfo_.FileSize_ >= 0);
		assert(!uploadingProcessId.isEmpty());

		const auto isMyProcessId = (uploadingProcessId == FsInfo_->GetUploadingProcessId());
		if (!isMyProcessId)
		{
			return;
		}

		BytesTransferred_ = bytesUploaded;

		__DISABLE(
			__TRACE(
				"fs",
				"uploading progress\n" <<
				"	transferred=<" << BytesTransferred_ << "/" << Metainfo_.FileSize_ << ">"
			);
		);

        update();
	}

    void FileSharingWidget::fileSharingUploadingResult(
        QString seq, bool success, QString localPath, QString uri, int contentType, bool isFileTooBig)
    {
        (void)success;

        const auto isMyProcessId = (seq == FsInfo_->GetUploadingProcessId());
        if (!isMyProcessId)
        {
            return;
        }

        __TRACE(
            "fs",
            "uploading finished\n" <<
            "    succeed=<" << logutils::yn(success) << ">\n"
            "    link=<" << uri << ">"
        );

        DownloadedFileLocalPath_ = localPath;

        FsInfo_->SetUri(uri);
        FsInfo_->setContentType(static_cast<core::file_sharing_content_type>(contentType));

        if (isState(State::ImageFile_Uploading))
        {
            setState(State::ImageFile_Uploaded);
        }
        else
        {
            setState(State::PlainFile_Uploaded);
        }

        stopDataTransferAnimation();

        if (isFileTooBig)
        {
            emit removeMe();
            return;
        }

        if (ShareButton_)
        {
            updateShareButtonGeometry();

            const auto cursorPos = mapFromGlobal(QCursor::pos());

            if (isOverPreview(cursorPos))
            {
                ShareButton_->setVisible(true);
            }
        }

        if (isGifOrVideo())
        {
            loadGifOrVideo(FsInfo_->GetLocalPath());
        }

        update();
    }

    void FileSharingWidget::localPreviewLoaded(QPixmap pixmap)
    {
        startDataTransferAnimation();

        if (pixmap.isNull())
        {
            setState(State::PlainFile_Uploading);

            return;
        }

        Preview_.FullImg_ = pixmap;

        setPreview(pixmap);

        setState(State::ImageFile_Uploading);
    }

    void FileSharingWidget::previewDownloaded(qint64 seq, QString uri, QPixmap preview, QString)
    {
        if (PreviewDownloadId_ != seq)
        {
            return;
        }

        assert(PreviewState_ != PreviewState::FullPreviewLoaded);

        if (preview.isNull())
        {
            return;
        }

        const auto isPreviewEmpty = (PreviewState_ == PreviewState::NoPreview);
        if (isPreviewEmpty)
        {
            assert(Preview_.FullImg_.isNull());

            setPreview(preview);

            PreviewState_ = PreviewState::MiniPreviewLoaded;

            PreviewDownloadId_ = -1;
            Retry_.PreviewDownload_ = false;
            Retry_.PreviewDownloadRetryCount_ = 0;

            requestPreview();

            return;
        }

        assert(PreviewState_ == PreviewState::MiniPreviewLoaded);

        setPreview(preview);

        PreviewState_ = PreviewState::FullPreviewLoaded;

        PreviewDownloadId_ = -1;
        Retry_.PreviewDownload_ = false;
        Retry_.PreviewDownloadRetryCount_ = 0;

        if (isGifOrVideo() && !isFullImageDownloading())
        {
            if (isGif() || Ui::get_gui_settings()->get_value<bool>(settings_autoplay_video, true))
            {
                setState(State::ImageFile_Downloading);
                startDownloadingFullImage();
            }
        }
    }

    void FileSharingWidget::previewDownloadError(int64_t seq, QString rawUri)
    {
        assert(seq > 0);
        assert(!rawUri.isEmpty());

        const auto isMySeq = (seq == PreviewDownloadId_);
        if (!isMySeq)
        {
            return;
        }

        Retry_.PreviewDownload_ = true;

        if (retryRequestLater())
        {
            return;
        }

        stopDataTransferAnimation();

        convertToPlainFileView();

        invalidateSizes();
        update();
    }

    void FileSharingWidget::onChangeLoadState(const bool _isLoad)
    {
        assert(isGifOrVideo());

        if (!videoPlayer_)
            return;

        videoPlayer_->setLoadingState(_isLoad);
    }

    bool FileSharingWidget::isInPreloadRange(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
    {
        auto intersected = _viewportVisibilityAbsRect.intersected(_widgetAbsGeometry);
        
        if (intersected.height() != 0)
            return true;

        return std::min(abs(_viewportVisibilityAbsRect.y() - _widgetAbsGeometry.y())
            , abs(_viewportVisibilityAbsRect.bottom() - _widgetAbsGeometry.bottom())) < 1000;
    }

    void FileSharingWidget::onPaused()
    {
        repaint();
    }
}

namespace
{
    using namespace Themes;

	IThemePixmapSptr downloadButton()
	{
		return GetPixmap(PixmapResourceId::FileSharingDownload);
	}

	IThemePixmapSptr downloadCancelButton(const bool isImage)
	{
        if (isImage)
        {
            return GetPixmap(PixmapResourceId::FileSharingMediaCancel);
        }

		return GetPixmap(PixmapResourceId::FileSharingPlainCancel);
	}

    int32_t getCtrlButtonTopOffset()
    {
        return Utils::scale_value(8);
    }

    IThemePixmapSptr playStartButton(const bool isGif, const bool isHovered)
    {
        if (isGif)
        {
            return GetPixmap(PixmapResourceId::FileSharingGifPlay);
        }

        if (isHovered)
        {
            return GetPixmap(PixmapResourceId::FileSharingMediaPlayHover);
        }

        return GetPixmap(PixmapResourceId::FileSharingMediaPlay);
    }

}

