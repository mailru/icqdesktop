#include "stdafx.h"
#include "AvatarContainerWidget.h"

#include "NameAndStatusWidget.h"
#include "VoipTools.h"
#include "../types/contact.h"
#include "../utils/utils.h"

Ui::AvatarContainerWidget::AvatarContainerWidget(QWidget* _parent, int _avatarSize, int _xOffset, int _yOffset)
    : QWidget(_parent)
    , avatarSize_(_avatarSize)
    , xOffset_(_xOffset)
    , yOffset_(_yOffset)
    , overlapPer01_(0.0f)
{
        assert(avatarSize_ > 0);

        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(_avatarChanged(QString)), Qt::QueuedConnection);
}

Ui::AvatarContainerWidget::~AvatarContainerWidget()
{

}

void Ui::AvatarContainerWidget::setOverlap(float _per01)
{
    overlapPer01_ = std::max(0.0f, std::min(1.0f, _per01));
}

void Ui::AvatarContainerWidget::addAvatarTo(const std::string& _userId, std::map<std::string, Logic::QPixmapSCptr>& _avatars)
{
    if (avatarSize_ <= 0)
    {
        assert(!"wrong avatar size");
        return;
    }

    bool isDefault = false;
    _avatars[_userId] = Logic::GetAvatarStorage()->GetRounded(_userId.c_str(), QString(), Utils::scale_bitmap(avatarSize_), QString(), true, isDefault, false, false);
}

void Ui::AvatarContainerWidget::_avatarChanged(QString _userId)
{
    std::string userIdUtf8 = _userId.toStdString();
    std::map<std::string, Logic::QPixmapSCptr>::iterator it = avatars_.find(userIdUtf8);
    if (it != avatars_.end())
    {
        addAvatarTo(userIdUtf8, avatars_);
    }

	repaint();
}

void Ui::AvatarContainerWidget::addAvatar(const std::string& _userId)
{
    addAvatarTo(_userId, avatars_);

    QSize sz = calculateAvatarSize();
    setMinimumSize(sz);

    avatarRects_ = calculateAvatarPositions(rect(), sz);

	repaint();
}

QSize Ui::AvatarContainerWidget::calculateAvatarSize()
{
    QSize sz;
    sz.setHeight(avatarSize_ + 2* yOffset_);
    if (!avatars_.empty())
    {
        sz.setWidth(avatarSize_ + (int(avatarSize_ * (1.0f - overlapPer01_) + 0.5f) * ((int)avatars_.size() - 1)) + 2* xOffset_);
    }
    return sz;
}

void Ui::AvatarContainerWidget::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
    QSize result;
    avatarRects_ = calculateAvatarPositions(rect(), result);
}

std::vector<QRect> Ui::AvatarContainerWidget::calculateAvatarPositions(const QRect& _rcParent, QSize& _avatarsSize)
{
    std::vector<QRect> positions;
    avatarRects_.clear();

    if (!avatars_.empty())
    {
        if (_avatarsSize.width() <= 0 || _avatarsSize.height() <= 0)
        {
            _avatarsSize = calculateAvatarSize();
        }

        QRect remains = _rcParent;
        remains.setLeft  (remains.left()   + xOffset_);
        remains.setRight (remains.right()  - xOffset_);
        remains.setTop   (remains.top()    + yOffset_);
        remains.setBottom(remains.bottom() - yOffset_);

        remains.setLeft(std::max((remains.right() + remains.left() - _avatarsSize.width()) / 2, remains.left()));
        remains.setRight(std::min(remains.right(), remains.left() + _avatarsSize.width()));
        remains.setTop(std::max((remains.bottom() + remains.top() - _avatarsSize.height()) / 2, remains.top()));
        remains.setBottom(std::min(remains.bottom(), remains.top() + _avatarsSize.height()));

        for (std::map<std::string, Logic::QPixmapSCptr>::iterator it = avatars_.begin(); it != avatars_.end(); it++)
        {
            if (remains.width() < avatarSize_ || remains.height() < avatarSize_)
            {
                break;
            }

            QRect rc_draw = remains;
            rc_draw.setRight(rc_draw.left() + avatarSize_);
            positions.push_back(rc_draw);

            remains.setLeft(remains.left() + avatarSize_ * (1.0f - overlapPer01_));
        }
    }

    return positions;
}

void Ui::AvatarContainerWidget::dropExcess(const std::vector<std::string>& _users)
{
    std::map<std::string, Logic::QPixmapSCptr> tmpAvatars;
    for (unsigned ix = 0; ix < _users.size(); ix++)
    {
        const std::string& userId = _users[ix];

        auto it = avatars_.find(userId);
        if (it != avatars_.end())
        {
            tmpAvatars[userId] = it->second;
        }
        else
        {
            addAvatarTo(userId, tmpAvatars);
        }
    }

    tmpAvatars.swap(avatars_);

    QSize sz = calculateAvatarSize();
    setMinimumSize(sz);
    avatarRects_ = calculateAvatarPositions(rect(), sz);

	repaint();
}

void Ui::AvatarContainerWidget::removeAvatar(const std::string& _userId)
{
    avatars_.erase(_userId);

	repaint();
}

void Ui::AvatarContainerWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    assert(avatarRects_.size() <= avatars_.size());
    for (unsigned ix = 0; ix < avatarRects_.size(); ix++)
    {
        const QRect& rc = avatarRects_[ix];
        
        auto avatar = avatars_.begin();
        std::advance(avatar, ix);

        Logic::QPixmapSCptr pixmap = avatar->second;
        painter.setRenderHint(QPainter::HighQualityAntialiasing);
        
        auto resizedImage = pixmap->scaled(Utils::scale_bitmap(rc.size()), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        
        painter.drawPixmap(rc, resizedImage, resizedImage.rect());
    }
}
