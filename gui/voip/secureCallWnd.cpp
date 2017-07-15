#include "stdafx.h"
#include "secureCallWnd.h"
#include "VoipTools.h"
#include "../core_dispatcher.h"
#include "../controls/CommonStyle.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/InterConnector.h"
#include "../utils/SChar.h"

#define SECURE_CALL_WINDOW_W Utils::scale_value(360)
#define SECURE_CALL_WINDOW_H Utils::scale_value(320)

#define SECURE_CALL_WINDOW_BORDER	 Utils::scale_value(24)
#define SECURE_CALL_WINDOW_BORDER_UP Utils::scale_value(22)
#define SECURE_CALL_WINDOW_UP_ARROW  Utils::scale_value(8)
#define SECURE_CALL_WINDOW_UP_OFFSET (SECURE_CALL_WINDOW_UP_ARROW + SECURE_CALL_WINDOW_BORDER_UP)

#define DETAILS_URL "https://icq.com/security-calls"

const QString HINT_STYLE = "QLabel { color : #767676; }";

const QString DETAILS_BUTTON_STYLE = QString(
    "QPushButton { text-align: bottom; vertical-align: text-bottom; color: %1;"
    "border-style: none; background-color: transparent; margin-bottom: 21dip; }"
).arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColor()));

const QString BACKGROUND_COLOR = "QWidget { background: #ffffff; }";


class WidgetWithBorder : public QWidget
{
public:
	WidgetWithBorder(QWidget* parent);

protected:

	virtual void paintEvent(QPaintEvent* _e) override;
};

Ui::ImageContainer::ImageContainer(QWidget* _parent)
    : QWidget(_parent)
    , kerning_(0)
{

}

Ui::ImageContainer::~ImageContainer()
{

}

void Ui::ImageContainer::setKerning(int _kerning)
{
    kerning_ = _kerning;
    calculateRectDraw();
}

void Ui::ImageContainer::calculateRectDraw()
{
    int height = 0;
    int width  = images_.empty() ? 0 : (images_.size() - 1) * kerning_;

    std::shared_ptr<QImage> image;
    for (unsigned ix = 0; ix < images_.size(); ++ix)
    {
        image = images_[ix];
        assert(!!image);
        if (!!image)
        {
            width += imageDrawSize_.width();
            height = std::max(imageDrawSize_.height(), height);
        }
    }

    const QRect  rc     = rect();
    const QPoint center = rc.center();

    const int w = std::min(rc.width(),  width);
    const int h = std::min(rc.height(), height);

    rcDraw_ = QRect(center.x() - w*0.5f, center.y() - h*0.5f, w, h);
}

void Ui::ImageContainer::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
    calculateRectDraw();
}

void Ui::ImageContainer::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.save();

    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    std::shared_ptr<QImage> image;
    QRect rcDraw = rcDraw_;
    for (unsigned ix = 0; ix < images_.size(); ++ix)
    {
        image = images_[ix];
        if (!!image)
        {
            QRect r(rcDraw.left(), rcDraw.top(), imageDrawSize_.width(), imageDrawSize_.height());
            auto resizedImage = image->scaled(Utils::scale_bitmap(r.size()), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter.drawImage(r, resizedImage, resizedImage.rect());
            rcDraw.setLeft(rcDraw.left() + kerning_ + imageDrawSize_.width());
        }
    }

    painter.restore();
}

void Ui::ImageContainer::swapImagePack(std::vector<std::shared_ptr<QImage> >& _images, const QSize& imageDrawSize)
{
    imageDrawSize_ = imageDrawSize;
    images_.swap(_images);
    calculateRectDraw();
}


QPolygon getPolygon(const QRect& rc)
{
    QRect rc1 = rc;
    
    
// Fixed border under mac.
#ifdef __APPLE__
    rc1.setWidth(rc.width() + 1);
    rc1.setHeight(rc.height() + 1);
#endif
    
	const int cx = (rc1.left() + rc1.right()) * 0.5f;
	const int cy = rc1.y();

	int polygon[7][2];
	polygon[0][0] = cx - SECURE_CALL_WINDOW_UP_ARROW;
	polygon[0][1] = cy + SECURE_CALL_WINDOW_UP_ARROW;

	polygon[1][0] = cx;
	polygon[1][1] = cy;

	polygon[2][0] = cx + SECURE_CALL_WINDOW_UP_ARROW;
	polygon[2][1] = cy + SECURE_CALL_WINDOW_UP_ARROW;

	polygon[3][0] = rc1.right();
	polygon[3][1] = rc1.y() + SECURE_CALL_WINDOW_UP_ARROW;

	polygon[4][0] = rc1.bottomRight().x();
	polygon[4][1] = rc1.bottomRight().y();

	polygon[5][0] = rc1.bottomLeft().x() + 1;
	polygon[5][1] = rc1.bottomLeft().y();

	polygon[6][0] = rc1.left() + 1;
	polygon[6][1] = rc1.y() + SECURE_CALL_WINDOW_UP_ARROW;

	QPolygon arrow;
	arrow.setPoints(7, &polygon[0][0]);

	return arrow;
}


QLabel* Ui::SecureCallWnd::createUniformLabel_(const QString& _text, const unsigned _fontSize, QSizePolicy _policy)
{
    assert(rootWidget_);

    QFont f = QApplication::font();
    f.setPixelSize(_fontSize);
    f.setStyleStrategy(QFont::PreferAntialias);

    QLabel* label = new voipTools::BoundBox<QLabel>(rootWidget_);
    label->setFont(f);
    label->setSizePolicy(_policy);
    label->setText(_text);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);

    return label;
}

Ui::SecureCallWnd::SecureCallWnd(QWidget* _parent)
    : QMenu(_parent)
    , rootLayout_(new QVBoxLayout())
    , rootWidget_(new voipTools::BoundBox<WidgetWithBorder>(this))
    , textSecureCode_ (new voipTools::BoundBox<ImageContainer>(rootWidget_))
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    Utils::ApplyStyle(this, BACKGROUND_COLOR);

    {
        QVBoxLayout* k = Utils::emptyVLayout();
        setLayout(k);
        k->addWidget(rootWidget_);
    }

    rootLayout_->setContentsMargins(SECURE_CALL_WINDOW_BORDER, SECURE_CALL_WINDOW_UP_OFFSET, SECURE_CALL_WINDOW_BORDER, SECURE_CALL_WINDOW_BORDER);
    rootLayout_->setSpacing(0);
    rootLayout_->setAlignment(Qt::AlignCenter);
    rootWidget_->setLayout(rootLayout_);

    { // window header text
        QLabel* label = createUniformLabel_(QT_TRANSLATE_NOOP("voip_pages", "Call is secured"), Utils::scale_value(24), QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
        assert(label);
        if (label)
        {
			label->setContentsMargins(0, 0, 0, Utils::scale_value(18));
            rootLayout_->addWidget(label);
        }
    }

    { // secure code emogi widget
        textSecureCode_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        //textSecureCode_->setFixedHeight(Utils::scale_value(80));
        textSecureCode_->setKerning(Utils::scale_value(12));
        rootLayout_->addWidget(textSecureCode_);
    }

    { // invitation description
        QLabel* label = createUniformLabel_(QT_TRANSLATE_NOOP("voip_pages", "For security check, you can verify your images with your partner"), Utils::scale_value(15));
        if (label)
        {
            Utils::ApplyStyle(label, HINT_STYLE);
            rootLayout_->addWidget(label);
        }
    }

    { // details button
        QFont f = QApplication::font();
        f.setPixelSize(Utils::scale_value(15));
        f.setStyleStrategy(QFont::PreferAntialias);
        f.setUnderline(true);

        QWidget* underBtnWidget = new QWidget(rootWidget_);
        underBtnWidget->setContentsMargins(0, 0, 0, 0);
        rootLayout_->addWidget(underBtnWidget);

        QHBoxLayout* underBtnLayout = Utils::emptyHLayout();
        underBtnWidget->setLayout(underBtnLayout);

        underBtnLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

        QPushButton* referenceBtn = new voipTools::BoundBox<QPushButton>(rootWidget_);
        referenceBtn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        referenceBtn->setFlat(true);
        referenceBtn->setFont(f);
        referenceBtn->setText(QT_TRANSLATE_NOOP("voip_pages", "How it works"));
        connect(referenceBtn, SIGNAL(clicked()), this, SLOT(onDetailsButtonClicked()), Qt::QueuedConnection);

        Utils::ApplyStyle(referenceBtn, DETAILS_BUTTON_STYLE);
        referenceBtn->setFocusPolicy(Qt::NoFocus);
        referenceBtn->setCursor(Qt::CursorShape::PointingHandCursor);
        underBtnLayout->addWidget(referenceBtn);

        underBtnLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    }

    { // btn OK -> start encryption
        QFont f = QApplication::font();
        f.setPixelSize(Utils::scale_value(17));
        f.setStyleStrategy(QFont::PreferAntialias);

        QWidget* underBtnWidget = new QWidget(rootWidget_);
        underBtnWidget->setContentsMargins(0, 0, 0, 0);
        rootLayout_->addWidget(underBtnWidget);

        QHBoxLayout* underBtnLayout = Utils::emptyHLayout();
        underBtnWidget->setLayout(underBtnLayout);

        underBtnLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

        QPushButton* btnOk = new voipTools::BoundBox<QPushButton>(underBtnWidget);
        btnOk->setCursor(QCursor(Qt::PointingHandCursor));
        Utils::ApplyStyle(btnOk, CommonStyle::getGrayButtonStyle());
        btnOk->setText(QT_TRANSLATE_NOOP("voip_pages", "Ok"));
        connect(btnOk, SIGNAL(clicked()), this, SLOT(onBtnOkClicked()), Qt::QueuedConnection);
        underBtnLayout->addWidget(btnOk);

        underBtnLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    }

    setMinimumSize(SECURE_CALL_WINDOW_W, SECURE_CALL_WINDOW_H);
    setMaximumSize(SECURE_CALL_WINDOW_W, SECURE_CALL_WINDOW_H);
    resize(SECURE_CALL_WINDOW_W, SECURE_CALL_WINDOW_H);
    updateMask();
}

void Ui::SecureCallWnd::setSecureCode(const std::string& _text)
{
    assert(textSecureCode_);
    assert(!_text.empty());

    voip_proxy::VoipEmojiManager& voipEmojiManager = Ui::GetDispatcher()->getVoipController().getEmojiManager();

    std::vector<std::shared_ptr<QImage> > images;
    images.reserve(5);

    auto decoded = QString::fromUtf8(_text.c_str());
    QTextStream textStream(&decoded, QIODevice::ReadOnly);

    unsigned codepoint;
    while (true)
    {
        const Utils::SChar superChar = Utils::ReadNextSuperChar(textStream);
        if (superChar.IsNull())
        {
            break;
        }

        QByteArray byteArray = superChar.ToQString().toUtf8();
        char* dataPtr = byteArray.data();
        size_t dataSz = byteArray.size();

        assert(dataPtr);
        assert(dataSz);

        if (!dataPtr) { continue; }
        if (!dataSz)  { continue; }

        codepoint = 0;
        codepoint |= ((*dataPtr & 0xff) << 24) * (dataSz >= 4); dataPtr += 1 * (dataSz >= 4);
        codepoint |= ((*dataPtr & 0xff) << 16) * (dataSz >= 3); dataPtr += 1 * (dataSz >= 3);
        codepoint |= ((*dataPtr & 0xff) <<  8) * (dataSz >= 2); dataPtr += 1 * (dataSz >= 2);
        codepoint |= ((*dataPtr & 0xff) <<  0) * (dataSz >= 1); dataPtr += 1 * (dataSz >= 1);

        std::shared_ptr<QImage> image(new QImage());
        if (voipEmojiManager.getEmoji(codepoint, Utils::scale_bitmap_with_value(64), *image))
        {
            images.push_back(image);
        }
    }
    
    textSecureCode_->swapImagePack(images, Utils::scale_value(QSize(64, 64)));
}

void Ui::SecureCallWnd::updateMask()
{
	auto arrow = getPolygon(rect());

    QPainterPath path(QPointF(0, 0));
    path.addPolygon(arrow);

    QRegion region(path.toFillPolygon().toPolygon());
    setMask(region);
}

Ui::SecureCallWnd::~SecureCallWnd()
{

}

void Ui::SecureCallWnd::showEvent(QShowEvent* _e)
{
    QMenu::showEvent(_e);
    emit onSecureCallWndOpened();
}

void Ui::SecureCallWnd::hideEvent(QHideEvent* _e)
{
    QMenu::hideEvent(_e);
    emit onSecureCallWndClosed();
}

void Ui::SecureCallWnd::changeEvent(QEvent* _e)
{
    QMenu::changeEvent(_e);
    if (QEvent::ActivationChange == _e->type())
    {
        if (!isActiveWindow())
        {
            hide();
        }
    }
}

void Ui::SecureCallWnd::onBtnOkClicked()
{
    hide();
    onSecureCallWndClosed();
}

void Ui::SecureCallWnd::onDetailsButtonClicked()
{
    QString url(DETAILS_URL);
    Utils::InterConnector::instance().unsetUrlHandler();
    QDesktopServices::openUrl(url);
    Utils::InterConnector::instance().setUrlHandler();
    
    // On Mac it does not hide automatically, we make it manually.
    hide();
    
    onSecureCallWndClosed();
}

void Ui::SecureCallWnd::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);
#ifdef __APPLE__
    updateMask();
#endif
}

WidgetWithBorder::WidgetWithBorder(QWidget* parent) : QWidget (parent) {}

void WidgetWithBorder::paintEvent(QPaintEvent* _e) 
{
	QWidget::paintEvent(_e);

	if (parentWidget())
	{
		// Draw the border.
		QPolygon polygon = getPolygon(parentWidget()->rect());
		polygon.push_back(polygon.point(0));

		QPainterPath path(QPointF(0, 0));
		path.addPolygon(polygon);

		QPainter painter(this);
        QColor borderColor("#000000");
        borderColor.setAlphaF(0.5);
		painter.strokePath(path, QPen(borderColor, Utils::scale_value(2)));
	}
}

