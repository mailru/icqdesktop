#pragma once

#include "../types/snap.h"

namespace Ui
{
    class ActionButton;
    class CustomButton;
    class ContactAvatarWidget;
    class TextEditEx;
    class CustomButton;
    class LineWidget;
    class LabelEx;
    class FFMpegPlayer;

    class PreviewImageWidget : public QWidget
    {
        Q_OBJECT
    public:
        PreviewImageWidget(QWidget* parent);
        ~PreviewImageWidget();

        void setPreview(const QPixmap& preview) { Preview_ = preview; update(); }
        bool isEmpty() { return Preview_.isNull(); }

    protected:
        virtual void paintEvent(QPaintEvent* e);

    private:
        QPixmap Preview_;
    };

    struct SnapItem
    {
        SnapItem()
            : mediaId_(0)
        {
        }

        QString path_;
        qint32 mediaId_;
    };

    class BackWidget : public QWidget
    {
        Q_OBJECT

Q_SIGNALS:
        void clicked();
        void resized();

    public:
        BackWidget(QWidget* _parent);
        void initSnaps();
        void play();
        void pause();

    protected:
        virtual void mouseReleaseEvent(QMouseEvent *e);
        virtual void resizeEvent(QResizeEvent *e);
        virtual void paintEvent(QPaintEvent *e);

    private Q_SLOTS:
        void playSnap(QString, QString, QString, QString, qint64, bool);
        void fileLoaded();
        void mediaChanged(qint32);
        void hidePreview();
        void showPreview();
        void previewChanged(QString);
        void userSnapsRemoved(QString);
        void snapRemoved(QString, qint64, QString);

    private:
        void prepareOverlay();

    private:
        FFMpegPlayer* Player_;
        QMap<qint64, SnapItem> Snaps_;
        QMap<qint32, QString> Ids_;
        unsigned SnapsCount_;
        PreviewImageWidget* Preview_;
        PreviewImageWidget* Overlay_;
    };

    class MainMenu: public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void createGroupChat();
        void addContact();
        void contacts();
        void settings();
        void discover();
        void stories();
        void myProfile();
        void about();
        void contactUs();
    
    public:
        MainMenu(QWidget* _parent);
        ~MainMenu();

        void notifyApplicationWindowActive(const bool isActive);

    protected:
        virtual void resizeEvent(QResizeEvent *);
        virtual void showEvent(QShowEvent *);
        virtual void hideEvent(QHideEvent *);
        virtual void mouseReleaseEvent(QMouseEvent *);

    private Q_SLOTS:
        void Hide();
        void myInfoUpdated();
        void soundsChecked(int);
        void guiSettingsChanged(QString);
        void signOut();
        void resize();
        void snapsChanged();

    private:
        void updateState();

    private:
        ActionButton* CreateGroupchat_;
        ActionButton* AddContact_;
        ActionButton* Contacts_;
        ActionButton* Settings_;
        ActionButton* Discover_;
        ActionButton* Stories_;
        ActionButton* SignOut_;

        BackWidget* Background_;
        QWidget* Parent_;
        CustomButton* Close_;
        ContactAvatarWidget* Avatar_;
        TextEditEx* Name_;
        TextEditEx* Status_;
        CustomButton* SoundsButton_;
        QCheckBox* SoundsCheckbox_;
        QScrollArea* ScrollArea_;
        QWidget* MainWidget_;
        LineWidget* Line_;
        LabelEx* About_;
        LabelEx* ContactUs_;
    };
}
