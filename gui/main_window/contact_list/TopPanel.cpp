#include "stdafx.h"
#include "TopPanel.h"
#include "../TitleBar.h"
#include "../../my_info.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/SChar.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../main_window/contact_list/SearchWidget.h"
#include "../../fonts.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../cache/snaps/SnapStorage.h"

namespace
{
    const int left_margin = 16;
    const int right_margin = 13;
    const int icon_size = 28;

    const int burger_width = 20;
    const int burger_width_compact = 30;
    const int burger_height = 20;

    const int button_width = 28;
    const int button_height = 24;
}

namespace Ui
{
    BurgerWidget::BurgerWidget(QWidget* parent)
        : QWidget(parent)
        , Back_(false)
    {
    }

    void BurgerWidget::paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        QImage pix(QSize(burger_width * 2, burger_height * 2), QImage::Format_ARGB32);
        QPainter painter(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor("#999999"));
        pen.setWidth(4);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        pix.fill(Qt::transparent);

        painter.drawLine(QPoint(0, 6), QPoint(40, 6));
        painter.drawLine(QPoint(0, 18), QPoint(40, 18));
        painter.drawLine(QPoint(0, 30), QPoint(40, 30));

        painter.end();
        Utils::check_pixel_ratio(pix);
        if (Utils::is_mac_retina())
            pix = pix.scaled(QSize(width() * 2, height() * 2), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        else
            pix = pix.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        
        p.fillRect(rect(), Ui::CommonStyle::getTopPanelColor());
        p.drawImage(0, 0, pix);
    }

    void BurgerWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        if (Back_)
            emit back();
        else
            emit clicked();

        return QWidget::mouseReleaseEvent(e);
    }

    void BurgerWidget::setBack(bool _back)
    {
        Back_ = _back;
        update();
    }

    TopPanelWidget::TopPanelWidget(QWidget* parent, SearchWidget* searchWidget)
        : QWidget(parent)
        , Mode_(NORMAL)
    {
        mainLayout = Utils::emptyHLayout();
        LeftSpacer_ = new QWidget(this);
        LeftSpacer_->setFixedWidth(Utils::scale_value(left_margin));
        LeftSpacer_->setStyleSheet(
            QString("border-style: none; border-bottom-style:solid; background: %1;")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor()))
        );
        mainLayout->addWidget(LeftSpacer_);
        Burger_ = new BurgerWidget(this);
        Burger_->setFixedSize(Utils::scale_value(burger_width), Utils::scale_value(burger_height));
        Burger_->setCursor(Qt::PointingHandCursor);
        mainLayout->addWidget(Burger_);
        searchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mainLayout->addWidget(searchWidget);
        Search_ = searchWidget;
        Discover_ = new CustomButton(this, ":/resources/explore_100.png");
        Discover_->setFillColor(Qt::white);
        Discover_->setHoverImage(":/resources/explore_100_active.png");
        Discover_->setFixedSize(Utils::scale_value(button_width), Utils::scale_value(button_height));
        Discover_->setCursor(Qt::PointingHandCursor);
        mainLayout->addWidget(Discover_);
        RightSpacer_ = new QWidget(this);
        RightSpacer_->setStyleSheet(
            QString("border-style: none; border-bottom-style:solid; border-right-style:solid; background: %1;")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor()))
            );
        mainLayout->addWidget(RightSpacer_);
        RightSpacer_->setFixedWidth(Utils::scale_value(right_margin));
        setLayout(mainLayout);

        Utils::ApplyStyle(this,
            QString("background-color: %1;"
                "border-style: solid;"
                "border-color: #d7d7d7;"
                "border-width: 1dip;"
                "border-top: none;"
                "border-left: none;"
                "border-bottom: none;"
            ).arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor())));

        connect(Burger_, SIGNAL(back()), this, SIGNAL(back()), Qt::QueuedConnection);
        connect(Burger_, SIGNAL(clicked()), this, SIGNAL(burgerClicked()), Qt::QueuedConnection);
        connect(Discover_, SIGNAL(clicked()), this, SIGNAL(discoverClicked()), Qt::QueuedConnection);
    }

    void TopPanelWidget::setMode(Mode _mode)
    {
        const auto isCompact = (_mode == Ui::TopPanelWidget::COMPACT);

        Burger_->setFixedWidth(Utils::scale_value(isCompact ? burger_width_compact : burger_width));
        Burger_->setVisible(_mode != SPREADED);
        Discover_->setVisible(!isCompact && _mode != SPREADED && !Search_->isActive());
        Search_->setVisible(!isCompact);
        LeftSpacer_->setVisible(_mode != SPREADED);
        
        if (isCompact)
        {
            RightSpacer_->setStyleSheet(
                QString("border-style: none; border-bottom-style:solid; border-right-style:none; background: %1;")
                .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor()))
                );
        }
        else
        {
            RightSpacer_->setStyleSheet(
                QString("border-style: none; border-bottom-style:solid; border-right-style:solid; background: %1;")
                .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor()))
                );
        }
        RightSpacer_->setStyle(QApplication::style());

        Mode_ = _mode;
    }

    void TopPanelWidget::setBack(bool _back)
    {
        if (Mode_ == Ui::TopPanelWidget::COMPACT)
            Burger_->setBack(_back);
    }

    void TopPanelWidget::searchActivityChanged(bool _active)
    {
        Discover_->setVisible(!_active && Mode_ == Ui::TopPanelWidget::NORMAL);
        Burger_->setVisible(Mode_ != Ui::TopPanelWidget::SPREADED);
        RightSpacer_->setVisible(!_active);
    }

    void TopPanelWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
