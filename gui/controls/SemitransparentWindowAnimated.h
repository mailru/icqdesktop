#pragma once

namespace Ui
{
    class qt_gui_settings;

    class SemitransparentWindowAnimated : public QWidget
    {
        Q_OBJECT

    public:
        SemitransparentWindowAnimated(QWidget* _parent, int _duration);
        ~SemitransparentWindowAnimated();

        Q_PROPERTY(int step READ getStep WRITE setStep)

        void setStep(int _val) { Step_ = _val; update(); }
        int getStep() const { return Step_; }

        void Show();
        void Hide();
        void forceHide();

        bool isMainWindow() const;
        void updateSize();

    private Q_SLOTS:
        void finished();

    protected:
        virtual void paintEvent(QPaintEvent*) override;
        virtual void mousePressEvent(QMouseEvent *e) override;

    private:
        QPropertyAnimation* Animation_;
        int Step_;
        bool main_;
        bool isMainWindow_;
    };
}
