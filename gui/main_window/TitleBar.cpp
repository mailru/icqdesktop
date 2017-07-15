#include "stdafx.h"
#include "TitleBar.h"
#include "MainWindow.h"

#include "../controls/CommonStyle.h"
#include "../core_dispatcher.h"
#include "../fonts.h"
#include "../my_info.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/UnknownsModel.h"

namespace Ui
{
    UnreadWidget::UnreadWidget(QWidget* _parent, bool _drawBadgeBorder, int32_t _badgeFontSize)
        : QWidget(_parent)
        , pressed_(false)
        , hovered_(false)
        , hoverable_(true)
        , unreads_(0)
        , pathIcon_(QString())
        , pathIconHovered_(QString())
        , pathIconPressed_(QString())
        , drawBadgeBorder_(_drawBadgeBorder)
        , fontSize_(_badgeFontSize)
    {
        setFixedSize(Utils::scale_value(Ui::TitleBar::icon_width),
                     Utils::scale_value(Ui::TitleBar::icon_height));
    }

    void UnreadWidget::setHoverVisible(bool _hoverVisible)
    {
        hoverable_ = _hoverVisible;
    }
    
    QPixmap UnreadWidget::renderToPixmap(unsigned _unreadsCount, bool _hoveredState, bool _pressedState)
    {
        auto pxSize = size();
        if (Utils::is_mac_retina())
            pxSize *= 2;
            
        QPixmap px(pxSize);
        Utils::check_pixel_ratio(px);
        px.fill(Qt::transparent);
        
        auto iconName = pathIcon_;
        if (_pressedState)
            iconName = pathIconPressed_;
        else if (_hoveredState)
            iconName = pathIconHovered_;
        
        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        auto icon = QPixmap(Utils::parse_image_name(iconName));
        Utils::check_pixel_ratio(icon);
        p.drawPixmap(0, 0, icon);
        
        if (_unreadsCount > 0)
        {
            const auto borderColor = QWidget::palette().color(QWidget::backgroundRole());
            const auto bgColor = QColor("#579e1c");
            const auto textColor = QColor("#ffffff");

            auto font = Fonts::appFontScaled(fontSize_, Fonts::FontWeight::Medium);
            auto balloonSizeScaled = Utils::getUnreadsSize(&p, font, drawBadgeBorder_, _unreadsCount, Utils::scale_value(Ui::TitleBar::balloon_size));
            Utils::drawUnreads(
                               &p,
                               font,
                               &bgColor,
                               &textColor,
                               drawBadgeBorder_? &borderColor: nullptr,
                               _unreadsCount,
                               Utils::scale_value(Ui::TitleBar::balloon_size),
                               Utils::scale_value(Ui::TitleBar::icon_width) - balloonSizeScaled.x(),
                               Utils::scale_value(Ui::TitleBar::balloon_size / 5)
                               );
        }
        return px;
    }

    void UnreadWidget::paintEvent(QPaintEvent *e)
    {
        auto px = renderToPixmap(unreads_, hoverable_ && hovered_, pressed_);
        QPainter p(this);
        p.drawPixmap(0, 0, px);
    }

    void UnreadWidget::mousePressEvent(QMouseEvent *e)
    {
        pressed_ = true;
        update();
        QWidget::mousePressEvent(e);
    }
    
    void UnreadWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        pressed_ = false;
        update();
        emit clicked();
        QWidget::mouseReleaseEvent(e);
    }

    void UnreadWidget::enterEvent(QEvent * e)
    {
        hovered_ = true;
        update();
        QWidget::enterEvent(e);
    }

    void UnreadWidget::leaveEvent(QEvent * e)
    {
        hovered_ = false;
        update();
        QWidget::leaveEvent(e);
    }

    void UnreadWidget::setUnreads(unsigned _unreads)
    {
        unreads_ = _unreads;
        update();
    }
    
    UnreadMsgWidget::UnreadMsgWidget(QWidget* parent)
        : UnreadWidget(parent, false, 14)
    {
        pathIcon_ = ":/resources/main_window/capture/i_recents_100.png";
        pathIconHovered_ = ":/resources/main_window/capture/i_recents_100_active.png";
        pathIconPressed_ = ":/resources/main_window/capture/i_recents_100_active.png";

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::im_created, this, &UnreadMsgWidget::loggedIn, Qt::QueuedConnection);
        connect(this, &UnreadMsgWidget::clicked, [](){ emit Utils::InterConnector::instance().activateNextUnread(); });
    }

    void UnreadMsgWidget::loggedIn()
    {
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);        
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updated, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);        
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &UnreadMsgWidget::updateIcon, Qt::QueuedConnection);
    }

    void UnreadMsgWidget::updateIcon()
    {
        const auto count = Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads();
        setUnreads(count);
    }
    
    UnreadMailWidget::UnreadMailWidget(QWidget* parent)
        : UnreadWidget(parent, false, 14)
    {
        pathIcon_ = ":/resources/main_window/capture/i_magent_100.png";
        pathIconHovered_ = ":/resources/main_window/capture/i_magent_100_active.png";
        pathIconPressed_ = ":/resources/main_window/capture/i_magent_100_active.png";
        
        connect(Ui::GetDispatcher(), &core_dispatcher::mailStatus, this, &UnreadMailWidget::mailStatus, Qt::QueuedConnection);
        connect(this, &UnreadMailWidget::clicked, this, &UnreadMailWidget::openMailBox);
    }

    void UnreadMailWidget::mailStatus(QString _email, unsigned _unreads, bool)
    {
        Email_ = _email;
        setUnreads(_unreads);
    }

    void UnreadMailWidget::openMailBox()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("email", Email_.isEmpty() ? MyInfo()->aimId() : Email_);
        Ui::GetDispatcher()->post_message_to_core("mrim/get_key", collection.get(), this, [this](core::icollection* _collection)
        {
            Utils::openMailBox(Email_, Ui::gui_coll_helper(_collection, false).get_value_as_string("key"), QString());
        });
    }
}
