#pragma once

#include "../../namespaces.h"

namespace Themes
{
    class IThemePixmap;

    typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;

    enum class PixmapResourceId;
}

UI_NS_BEGIN

class ActionButtonWidgetLayout;

class ActionButtonWidget final : public QWidget
{
    Q_OBJECT

Q_SIGNALS:
    void startClickedSignal(QPoint globalClickCoords);

    void stopClickedSignal(QPoint globalClickCoords);

    void dragSignal();

public:
    struct ResourceSet
    {
        static const ResourceSet Play_;

        static const ResourceSet Gif_;

        static const ResourceSet DownloadPlainFile_;

        static const ResourceSet DownloadMediaFile_;

        static const ResourceSet ShareContent_;

        static const ResourceSet PttPause_;

        static const ResourceSet PttPlayGray_;

        static const ResourceSet PttPlayGreen_;

        static const ResourceSet PttTextGray_;

        static const ResourceSet PttTextGreen_;

        typedef Themes::PixmapResourceId ResId;

        ResourceSet(
            const ResId startButtonImage,
            const ResId startButtonImageActive,
            const ResId startButtonImageHover,
            const ResId stopButtonImage,
            const ResId stopButtonImageActive,
            const ResId stopButtonImageHover);

        ResId StartButtonImage_;
        ResId StartButtonImageActive_;
        ResId StartButtonImageHover_;
        ResId StopButtonImage_;
        ResId StopButtonImageActive_;
        ResId StopButtonImageHover_;
    };

    static QSize getMaxIconSize();

    ActionButtonWidget(const ResourceSet &resourceIds, QWidget *parent = nullptr);

    virtual ~ActionButtonWidget() override;

    QPoint getCenterBias() const;

    QSize getIconSize() const;

    QPoint getLogicalCenter() const;

    const QString& getProgressText() const;

    const QFont& getProgressTextFont() const;

    const ResourceSet& getResourceSet() const;

    void setProgress(const double progress);

    void setProgressPen(const QColor color, const double a, const double width);

    void setProgressText(const QString &progressText);

    void setResourceSet(const ResourceSet &resourceIds);

    virtual QSize sizeHint() const override;

    void startAnimation(const int32_t _delay = 0);

    void stopAnimation();

protected:
    virtual void enterEvent(QEvent *event) override;

    virtual void hideEvent(QHideEvent*) override;

    virtual void leaveEvent(QEvent *event) override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    virtual void paintEvent(QPaintEvent *event) override;

private:
    Q_PROPERTY(int ProgressBarBaseAngle READ getProgressBarBaseAngle WRITE setProgressBarBaseAngle)

    void createAnimationStartTimer();

    void drawIcon(QPainter &p);

    void drawProgress(QPainter &p);

    void drawProgressText(QPainter &p);

    int getProgressBarBaseAngle() const;

    void setProgressBarBaseAngle(const int32_t _val);

    void initResources();

    void resetLayoutGeometry();

    bool selectCurrentIcon();

    void startProgressBarAnimation();

    void stopProgressBarAnimation();

    QTimer *AnimationStartTimer_;

    Themes::IThemePixmapSptr CurrentIcon_;

    QPropertyAnimation *ProgressBaseAngleAnimation_;

    QPen ProgressPen_;

    Themes::IThemePixmapSptr StartButtonImage_;

    Themes::IThemePixmapSptr StartButtonImageHover_;

    Themes::IThemePixmapSptr StartButtonImageActive_;

    Themes::IThemePixmapSptr StopButtonImage_;

    Themes::IThemePixmapSptr StopButtonImageHover_;

    Themes::IThemePixmapSptr StopButtonImageActive_;

    ResourceSet ResourceIds_;

    double Progress_;

    int32_t ProgressBarAngle_;

    QString ProgressText_;

    QFont ProgressTextFont_;

    bool ResourcesInitialized_;

    bool IsHovered_;

    bool IsPressed_;

    bool IsAnimating_;

    ActionButtonWidgetLayout *Layout_;

private Q_SLOTS:
    void onAnimationStartTimeout();

};

UI_NS_END