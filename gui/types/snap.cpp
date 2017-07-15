#include "stdafx.h"
#include "snap.h"
#include "core_dispatcher.h"
#include "../main_window/history_control/complex_message/FileSharingUtils.h"
#include "../main_window/contact_list/SnapItemDelegate.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/utils.h"

namespace Logic
{
    SnapInfo::SnapInfo()
        : SnapId_(-1)
        , OriginalSnapId_(-1)
        , Duration_(0)
        , Views_(0)
        , Timestamp_(-1)
        , PreviewSeq_(-1)
    {
        auto previewSize = Logic::SnapItemDelegate::getSnapPreviewItemSize();

        auto preview = QImage(QSize(previewSize.width(), previewSize.height()), QImage::Format_ARGB32);
        QPainter p(&preview);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPixmap pix(previewSize.width(), previewSize.height());
        pix.fill(QColor("#ebebeb"));
        QColor color1(Qt::black);
        color1.setAlpha(0.3 * 255);
        QColor color2(Qt::black);
        color2.setAlpha(0.1 * 255);
        QColor color3(Qt::black);
        color3.setAlpha(0);

        auto gHeight = Logic::SnapItemDelegate::getGradientHeight();
        auto gRect = QRect(0, previewSize.height() - gHeight, previewSize.width(), gHeight);
        QLinearGradient g(gRect.topLeft(), gRect.bottomLeft());
        g.setColorAt(0, color3);
        g.setColorAt(0.7, color2);
        g.setColorAt(1, color1);
        QPainter gPainter(&pix);
        gPainter.setRenderHint(QPainter::Antialiasing);
        gPainter.setRenderHint(QPainter::SmoothPixmapTransform);
        gPainter.fillRect(gRect, g);

        p.fillRect(QRect(0, 0, previewSize.width(), previewSize.height()), Qt::white);
        auto b = QBrush(pix);
        p.setBrush(b);
        p.setPen(QPen(Qt::transparent, 0));
        p.drawRoundedRect(QRect(0, 0, previewSize.width(), previewSize.height()), Utils::scale_value(8), Utils::scale_value(8));

        MiniPreview_ = QPixmap::fromImage(preview);
    }

    void UnserializeSnapState(core::coll_helper* _helper, Logic::SnapState& _state)
    {
        if (_helper->is_value_exist("aimId"))
            _state.AimId_ = _helper->get_value_as_string("aimId");

        _state.Views_ = _helper->get_value_as_int("views");
        _state.ViewsCurrent_ = _helper->get_value_as_int("viewsCurrent");
        _state.LastViewedSnapId_ = _helper->get_value_as_int64("lastViewedSnapId");
        _state.LatestSnapId_ = _helper->get_value_as_int64("latestSnapId");
        _state.ViewNextSnapId_ = _helper->get_value_as_int64("lastViewedSnapId");
    }

    void UnserializeSnapInfo(core::coll_helper* _helper, Logic::SnapInfo& _info)
    {
        _info.SnapId_ = _helper->get_value_as_int64("snapId");
        _info.OriginalSnapId_ = _helper->get_value_as_int64("originalSnapId");
        _info.OriginalAimId_ = _helper->get_value_as_string("originalAimId");
        _info.OriginalFriendly_ = _helper->get_value_as_string("originalFriendly");
        _info.Url_ = _helper->get_value_as_string("url");
        _info.ContentType_ = _helper->get_value_as_string("contentType");
        _info.Duration_ = _helper->get_value_as_int("duration");
        _info.Views_ = _helper->get_value_as_int("views");
        _info.Timestamp_ = _helper->get_value_as_int("timestamp");
        _info.Ttl_ = _helper->get_value_as_int("ttl");
        _info.TtlId_ = Ui::ComplexMessage::extractIdFromFileSharingUri(_info.Url_);
    }

    void UnserializeUserSnapsInfo(core::coll_helper* _helper, Logic::UserSnapsInfo& _snaps)
    {
        _snaps.AimId_ = _helper->get_value_as_string("aimId");
        _snaps.IsOfficial_ = _helper->get_value_as_bool("official");
        _snaps.Friendly_ = _helper->get_value_as_string("friendly");
        _snaps.IsFriend_ = _helper->get_value_as_bool("buddy") || (Logic::getContactListModel()->getContactItem(_snaps.AimId_) != 0);
        auto state = _helper->get_value_as_collection("state");
        core::coll_helper h(state, false);
        UnserializeSnapState(&h, _snaps.State_);
        auto snaps = _helper->get_value_as_array("snaps");
        for (int i = 0; i < snaps->size(); ++i)
        {
            auto snap = snaps->get_at(i)->get_as_collection();
            core::coll_helper hs(snap, false);
            Logic::SnapInfo info;
            Logic::UnserializeSnapInfo(&hs, info);
            _snaps.Snaps_.push_back(info);
        }
    }
}
