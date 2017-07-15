#ifndef __AVATAR_CONTAINER_WIDGET_H__
#define __AVATAR_CONTAINER_WIDGET_H__

#include <string>
#include "../cache/avatars/AvatarStorage.h"

namespace Ui {
    
    class AvatarContainerWidget : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:

    public:
        AvatarContainerWidget(QWidget* _parent, int avatarSize, int _xOffset = 0, int _yOffset = 0);
        virtual ~AvatarContainerWidget();

        void setOverlap(float _per01);
        void addAvatar(const std::string& _userId);
        void removeAvatar(const std::string& _userId);
        void dropExcess(const std::vector<std::string>& _users);

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;

    private Q_SLOTS:
        void _avatarChanged(QString);

    private:
        std::map<std::string, Logic::QPixmapSCptr> avatars_;
        std::vector<QRect> avatarRects_;
        float overlapPer01_;

        int avatarSize_;
        int xOffset_; 
        int yOffset_;

        void addAvatarTo(const std::string& _userId, std::map<std::string, Logic::QPixmapSCptr>& _avatars);
        std::vector<QRect> calculateAvatarPositions(const QRect& _rcParent, QSize& _avatarsSize);
        QSize calculateAvatarSize();
    };

}

#endif//__AVATAR_CONTAINER_WIDGET_H__
