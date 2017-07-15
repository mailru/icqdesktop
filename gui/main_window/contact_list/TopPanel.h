#pragma once

#include "../../cache/avatars/AvatarStorage.h"
#include "../../types/snap.h"

namespace Ui
{
    class CustomButton;
    class SearchWidget;
    class UnreadMailWidget;

    class BurgerWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked();
        void back();

    public:
        BurgerWidget(QWidget* parent);
        void setBack(bool _back);

    protected:
        virtual void paintEvent(QPaintEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private:
        bool Back_;
    };

    class TopPanelWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void back();
        void burgerClicked();
        void discoverClicked();
        
    public:
        enum Mode
        {
            NORMAL = 0,
            COMPACT = 1,
            SPREADED = 2,
        };

        TopPanelWidget(QWidget* parent, SearchWidget* searchWidget);

        void setMode(Mode _mode);
        void setBack(bool _back);
        void searchActivityChanged(bool _active);


    protected:
        virtual void paintEvent(QPaintEvent* _e) override;

    private:
        BurgerWidget* Burger_;
        QWidget* LeftSpacer_;
        QWidget* RightSpacer_;
        QHBoxLayout* mainLayout;
        CustomButton* Discover_;
        SearchWidget* Search_;
        Mode Mode_;
    };
}
