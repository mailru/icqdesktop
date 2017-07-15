#pragma once

namespace core
{
    class coll_helper;
}

namespace Logic
{
    struct SnapState
    {
        SnapState()
            : ViewNextSnapId_(-1)
            , LatestSnapId_(-1)
            , LastViewedSnapId_(-1)
            , Views_(0)
            , ViewsCurrent_(0)
        {
        }

        QString AimId_;
        qint64 ViewNextSnapId_;
        qint64 LatestSnapId_;
        qint64 LastViewedSnapId_;
        int32_t Views_;
        int32_t ViewsCurrent_;
    };

    struct SnapInfo
    {
        SnapInfo();

        QString AimId_;
        QString Url_;
        QString ContentType_;
        QString OriginalAimId_;
        QString OriginalFriendly_;
        qint64 SnapId_;
        qint64 OriginalSnapId_;
        int32_t Duration_;
        int32_t Views_;
        int32_t Timestamp_;
        int32_t Ttl_;

        qint64 getId() const { return OriginalSnapId_ == -1 ? SnapId_ : OriginalSnapId_; }

        bool operator==(const SnapInfo& other)
        {
            return AimId_ == other.AimId_ && SnapId_ == other.SnapId_ && OriginalSnapId_ == other.OriginalSnapId_;
        }

        //gui only values
        QString PreviewUri_;
        int64_t PreviewSeq_;

        QPixmap MiniPreview_;
        QPixmap FullPreview_;
        QPixmap SourcePreview_;

        QString LocalPath_;
        QString TtlId_;
    };

    struct UserSnapsInfo
    {
        UserSnapsInfo()
            : IsOfficial_(false)
            , IsFriend_(false)
            , LastSnapTimestamp_(0)
            , LastNewSnapTimestamp_(0)
        {
        }

        SnapState State_;
        QString AimId_;
        QString Friendly_;
        bool IsOfficial_;
        bool IsFriend_;
        QList<SnapInfo> Snaps_;

        //gui only values
        int32_t LastSnapTimestamp_;
        int32_t LastNewSnapTimestamp_;
    };

    void UnserializeSnapState(core::coll_helper* _helper, SnapState& _state);
    void UnserializeSnapInfo(core::coll_helper* _helper, SnapInfo& _info);
    void UnserializeUserSnapsInfo(core::coll_helper* _helper, UserSnapsInfo& _snaps);
}
