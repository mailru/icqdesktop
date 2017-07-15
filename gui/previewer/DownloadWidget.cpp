#include "stdafx.h"

#include "../main_window/history_control/ActionButtonWidget.h"
#include "../themes/ResourceIds.h"
#include "../utils/utils.h"

#include "DownloadWidget.h"

namespace
{
    const int DownloadWidgetWidth = 320;
    const int DownloadWidgetHeight = 200;

    static const Ui::ActionButtonWidget::ResourceSet DownloadButtonResources(
        Themes::PixmapResourceId::PreviewerReload,
        Themes::PixmapResourceId::PreviewerReloadHover,
        Themes::PixmapResourceId::PreviewerReloadHover,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel,
        Themes::PixmapResourceId::FileSharingMediaCancel);
}

Previewer::DownloadWidget::DownloadWidget(QWidget* _parent)
    : QFrame(_parent)
    , isLayoutSetted_(false)
{
    const auto style = Utils::LoadStyle(":/resources/previewer/qss/download.qss");

    holder_ = new QFrame();
    holder_->setProperty("DownloadWidget", true);
    holder_->setStyleSheet(style);
    holder_->setFixedSize(Utils::scale_value(DownloadWidgetWidth), Utils::scale_value(DownloadWidgetHeight));

    topSpacer_ = new QSpacerItem(0, 0);

    button_ = new Ui::ActionButtonWidget(DownloadButtonResources);
    connect(button_, &Ui::ActionButtonWidget::startClickedSignal,
        [this]()
        {
            startLoading();
            emit tryDownloadAgain();
        });

    connect(button_, &Ui::ActionButtonWidget::stopClickedSignal,
        [this]()
        {
            stopLoading();
            emit cancelDownloading();
        });

    auto buttonLayout = Utils::emptyHLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->addWidget(button_);

    auto buttonHolder = new QFrame();
    buttonHolder->setLayout(buttonLayout);

    textSpacer_ = new QSpacerItem(0, 0);

    errorMessage_ = new QLabel(QT_TRANSLATE_NOOP("previewer", "Unable to download the image"));
    errorMessage_->setStyleSheet(style);
    errorMessage_->setProperty("ErrorMessage", true);
    errorMessage_->setAlignment(Qt::AlignHCenter);
    errorMessage_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QVBoxLayout* layout = Utils::emptyVLayout();
    layout->addSpacerItem(topSpacer_);
    layout->addWidget(buttonHolder);
    layout->addSpacerItem(textSpacer_);
    layout->addWidget(errorMessage_);
    layout->addStretch();

    holder_->setLayout(layout);

    QVBoxLayout* mainLayout = Utils::emptyVLayout();
    mainLayout->addWidget(holder_);
    mainLayout->setAlignment(Qt::AlignCenter);

    setLayout(mainLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void Previewer::DownloadWidget::setupLayout()
{
    if (isLayoutSetted_)
        return;

    isLayoutSetted_ = true;

    const auto buttonHeight = button_->height();

    const auto topSpacing = (Utils::scale_value(DownloadWidgetHeight) - buttonHeight) / 2;

    const auto metrics = QFontMetrics(errorMessage_->font());
    const auto overline = metrics.overlinePos();
    const auto textHeight = errorMessage_->height();

    const auto textSpacing = (topSpacing - textHeight) / 2 - overline;

    topSpacer_->changeSize(100, topSpacing);
    textSpacer_->changeSize(100, textSpacing);
}

void Previewer::DownloadWidget::startLoading()
{
    setupLayout();
    button_->startAnimation();
    errorMessage_->hide();
}

void Previewer::DownloadWidget::stopLoading()
{
    setupLayout();
    button_->stopAnimation();
    errorMessage_->show();
}

void Previewer::DownloadWidget::onDownloadingError()
{
    stopLoading();
}

void Previewer::DownloadWidget::mousePressEvent(QMouseEvent* _event)
{
    const auto pos = holder_->mapFromParent(_event->pos());
    const auto rect = holder_->frameRect();
    if (rect.contains(pos))
    {
        emit clicked();
        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}
