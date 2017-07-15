#include "stdafx.h"

#include "SidebarUtils.h"

#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../fonts.h"
#include "../../utils/utils.h"

namespace
{
    const int link_offset_left = 52;
    const int link_offset_right = 16;
    const int link_offset_top = 6;
}

namespace Ui
{
    LineWidget::LineWidget(QWidget* /*parent*/, int leftMargin, int topMargin, int rightMargin, int bottomMargin)
    {
        auto rootLayout = Utils::emptyVLayout(this);
        rootLayout->addSpacerItem(new QSpacerItem(0, topMargin, QSizePolicy::Preferred, QSizePolicy::Fixed));
        {
            auto horLayout = Utils::emptyHLayout();
            horLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            line_ = new QWidget(this);
            line_->setStyleSheet("background: #d7d7d7");
            line_->setFixedHeight(Utils::scale_value(1));
            horLayout->addWidget(line_);
            horLayout->addSpacerItem(new QSpacerItem(rightMargin, 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            rootLayout->addLayout(horLayout);
        }
        rootLayout->addSpacerItem(new QSpacerItem(0, bottomMargin, QSizePolicy::Preferred, QSizePolicy::Fixed));
    }

    void LineWidget::setLineWidth(int width)
    {
        line_->setFixedWidth(width);
    }

    ActionButton::ActionButton(QWidget* parent, const QString& image, const QString& text, int height, int leftMargin, int textOffset)
        : QWidget(parent)
        , Hovered_(false)
        , Height_(height)
    {
        auto vLayout = Utils::emptyVLayout(this);
        auto hLayout = Utils::emptyHLayout();
        hLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
        Button_ = new CustomButton(this, image);
        Button_->setOffsets(textOffset, 0);
        Button_->setCursor(QCursor(Qt::PointingHandCursor));
        Button_->setText(text);
        Button_->setTextColor("#454545");
        Button_->setFont(Fonts::appFontScaled(16));
        Button_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        Button_->setAlign(Qt::AlignLeft);
        Button_->setFocusPolicy(Qt::NoFocus);
        Button_->setFixedHeight(height);
        Button_->setAttribute(Qt::WA_TransparentForMouseEvents);
        hLayout->addWidget(Button_);
        vLayout->addLayout(hLayout);
        {
            vLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(link_offset_top), QSizePolicy::Preferred, QSizePolicy::Fixed));
            auto horLayout = Utils::emptyHLayout();
            Link_ = new QLabel(this);
            Link_->setFont(Fonts::appFontScaled(12));
            horLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(link_offset_left), 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            horLayout->addWidget(Link_);
            Link_->hide();
            vLayout->addLayout(horLayout);
            vLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(link_offset_top), QSizePolicy::Preferred, QSizePolicy::Fixed));
        }
        setFixedHeight(height);
    }

    void ActionButton::setImage(const QString& image)
    {
        Button_->setImage(image);
    }

    void ActionButton::setText(const QString& text)
    {
        Button_->setText(text);
    }

    void ActionButton::setColor(const QString& color)
    {
        Button_->setTextColor(color);
    }

    void ActionButton::setFont(const QFont& font)
    {
        Button_->setFont(font);
    }

    void ActionButton::setLink(const QString& text, const QColor& color)
    {
        LinkText_ = text;
        if (!text.isEmpty())
        {
            Link_->setText(text);
            QPalette p;
            p.setColor(QPalette::Foreground, color);
            Link_->setPalette(p);
            Link_->show();
            setFixedHeight(Height_ + Utils::scale_value(link_offset_top * 2));
        }
        else
        {
            Link_->hide();
            setFixedHeight(Height_);
        }
        elideLink();
    }

    void ActionButton::elideLink()
    {
        QFontMetrics m(Link_->font());
        QString newText = LinkText_;
        int w = width() - Utils::scale_value(link_offset_left) - Utils::scale_value(link_offset_right);
        if (m.width(LinkText_) > w)
        {
            newText = m.elidedText(LinkText_, Qt::ElideRight, w);
        }
        Link_->setText(newText);
    }

    void ActionButton::paintEvent(QPaintEvent* e)
    {
        QWidget::paintEvent(e);
        if (Hovered_)
        {
            QPainter p(this);
            p.fillRect(rect(), Ui::CommonStyle::getContactListHoveredColor());
        }
    }

    void ActionButton::enterEvent(QEvent* e)
    {
        Hovered_ = true;
        update();
        QWidget::enterEvent(e);
    }

    void ActionButton::leaveEvent(QEvent* e)
    {
        Hovered_ = false;
        update();
        QWidget::leaveEvent(e);
    }

    void ActionButton::mouseReleaseEvent(QMouseEvent* e)
    {
        emit clicked();
        QWidget::mouseReleaseEvent(e);
    }

    void ActionButton::resizeEvent(QResizeEvent* e)
    {
        elideLink();
        QWidget::resizeEvent(e);
    }

    ClickedWidget::ClickedWidget(QWidget* parent)
        : QWidget(parent)
        , Hovered_(false)
        , Enabled_(true)
    {
    }

    void ClickedWidget::setEnabled(bool value)
    {
        Enabled_ = value;
        update();
    }

    void ClickedWidget::paintEvent(QPaintEvent* e)
    {
        QWidget::paintEvent(e);
        if (Hovered_ && Enabled_)
        {
            QPainter p(this);
            p.fillRect(rect(), Ui::CommonStyle::getContactListHoveredColor());
        }
    }

    void ClickedWidget::enterEvent(QEvent* e)
    {
        Hovered_ = true;
        update();
        QWidget::enterEvent(e);
    }

    void ClickedWidget::leaveEvent(QEvent* e)
    {
        Hovered_ = false;
        update();
        QWidget::leaveEvent(e);
    }

    void ClickedWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        if (Enabled_)
            emit clicked();
        QWidget::mouseReleaseEvent(e);
    }
}