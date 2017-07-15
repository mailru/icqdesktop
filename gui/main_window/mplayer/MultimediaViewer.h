#pragma once

#include "../../../gui.shared/implayer.h"

namespace Ui
{
    //class VideoPlayer;

    class MultimediaViewer : public QWidget
    {
        Q_OBJECT

    protected:

        //VideoPlayer* videoPlayer_;

        bool closed_;

    Q_SIGNALS:

        void closed();

        void mediaLoaded();

    public:

        MultimediaViewer(QWidget* _parent);
        virtual ~MultimediaViewer();

        void showWindow();

        void playMedia(const QString& _mediaSource);

        void closeViewer();

    protected:

        virtual void keyPressEvent(QKeyEvent*) override;

        virtual void mousePressEvent(QMouseEvent*) override;

        virtual void resizeEvent(QResizeEvent*) override;

        virtual void paintEvent(QPaintEvent* _e) override;
        
        virtual void changeEvent(QEvent* _e) override;

        void releaseKeyboardAndClose();
    };
}
