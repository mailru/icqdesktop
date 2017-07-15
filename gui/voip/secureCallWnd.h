#ifndef __SECURE_CALL_WND_H__
#define __SECURE_CALL_WND_H__
#include "../controls/TextEmojiWidget.h"

namespace Ui
{

    class ImageContainer : public QWidget
    {
        Q_OBJECT

    public:
        ImageContainer(QWidget* _parent);
        virtual ~ImageContainer();

    public:
        void swapImagePack(std::vector<std::shared_ptr<QImage> >& _images, const QSize& imageDrawSize);
        void setKerning(int _kerning);

    private:
        void calculateRectDraw();

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent *) override;

    private:
        std::vector<std::shared_ptr<QImage> > images_;
        int kerning_;
        QRect rcDraw_;
        QSize imageDrawSize_;
    };

    class SecureCallWnd : public QMenu 
    {
        Q_OBJECT

        QVBoxLayout* rootLayout_;
        QWidget*     rootWidget_;
        ImageContainer* textSecureCode_;

    public:
        SecureCallWnd(QWidget* _parent = NULL);
        virtual ~SecureCallWnd();

        void setSecureCode(const std::string& _text);

    private:
        void showEvent  (QShowEvent* _e) override;
        void hideEvent  (QHideEvent* _e) override;
        void changeEvent(QEvent* _e)     override;
        void resizeEvent(QResizeEvent * event) override;

        QLabel* createUniformLabel_(const QString& _text, const unsigned _fontSize, QSizePolicy _policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        void updateMask();

    Q_SIGNALS:
        void onSecureCallWndOpened();
        void onSecureCallWndClosed();

    private Q_SLOTS:
        void onBtnOkClicked();
        void onDetailsButtonClicked();
    };

}

#endif//__SECURE_CALL_WND_H__
