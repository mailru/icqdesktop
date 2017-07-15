#pragma once

#include "CommonUI.h"
#include "Masks.h"


namespace Ui
{
	// Draw round icon for mask.
	class MaskWidget : public QPushButton
	{
		Q_OBJECT

        // Used for animation.
        Q_PROPERTY(int xPos    READ x WRITE setXPos)
        Q_PROPERTY(int yCenter READ yCenter WRITE setYCenter)
        Q_PROPERTY(int yPos    READ y WRITE setYPos)
        Q_PROPERTY(int xCenter READ xCenter WRITE setXCenter)

	public:

		MaskWidget(const voip_masks::Mask* mask);

		QString maskPath();

		void setPixmap(const QPixmap& image);
		QPixmap pixmap();
		void setChecked(bool check);
        bool isEmptyMask();
        bool isLoaded();
        const QString& name();

        void setXPos(int x);
        void setYPos(int x);

        void setYCenter(int y);
        void setXCenter(int y);

        int  yCenter();
        int  xCenter();

        void updateSize();

    public slots:
        void setMaskEngineReady(bool ready);

    protected slots:
        void updatePreview();
        void updateJsonPath();
        void updateLoadingProgress(unsigned);

	protected:

        void changeEvent(QEvent * event) override;
        void moveEvent(QMoveEvent *event) override;

		QPixmap image_;
		QString maskPath_;
        QString name_;
        const voip_masks::Mask* mask_;
        float loadingProgress_;
        bool maskEngineReady_;
        bool applyWhenEnebled_;
        QPixmap closeIcon_;

        QPropertyAnimation* resizeAnimationMin_;
        QPropertyAnimation* resizeAnimationMax_;

		void paintEvent(QPaintEvent * event) override;
	};

    // Use this widget to show masks with animation. 
    class ScrollWidget : public QWidget
    {
        Q_OBJECT

    public:
        ScrollWidget();

        void animationShow();
        void animationHide();

    Q_SIGNALS:

        void animationFinished(bool out);
        void animationRunned(bool out);

    protected Q_SLOTS:

        // Set out true for close and false for open.
        void runAnimationVertical(bool out);
        void runAnimationHorizontal(bool out);

    protected:

        void prerunAnimation(bool out, bool vertical);

        // Added animation for icons.
        template <typename T> void addAnimation(QObject* mask, const QByteArray& param, const T& start, const T& finish,
            float duration, const QEasingCurve& curve, bool out);
    };


	struct UIEffects;	

    class MaskPanel : public BaseVideoPanel
    {
    Q_OBJECT

    Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();
		void onkeyEscPressed();
		void makePreviewPrimary();
        void makeInterlocutorPrimary();
        void onShowMaskList();
        void onHideMaskList();
        void getCallStatus(bool& isAccepted);
        void animationRunningFinished(bool out);

    public:
		MaskPanel(QWidget* _parent, QWidget* _container, int topOffset, int bottomOffset);
        ~MaskPanel();

		void updatePosition(const QWidget& parent) override;

		void setMaskList(const voip_masks::MaskList& maskList);

		virtual void fadeIn(unsigned int duration) override {}
		virtual void fadeOut(unsigned int duration) override {}

		void setPanelMode(QBoxLayout::Direction direction);
        void setTopOffset(int topOffset);

        void chooseFirstMask();

        bool isOpened();

	public Q_SLOTS:
        void hideMaskListWithOutAnimation();
		void hideMaskList();
		void selectMask(MaskWidget* mask);
        void callDestroyed();
        void animationFinished(bool out);
        void scrollToWidget(QString maskName);
        void animationRunned(bool out);

	protected Q_SLOTS:

		void showMaskList();
		MaskWidget* appendItem(const voip_masks::Mask* mask);
		void changedMask();
		void scrollListUp();
		void scrollListDown();
		void updateUpDownButtons();
        void setVideoStatus(bool);
        void updateMaskList(const voip_manager::ContactEx&  contactEx);
        void autoHidePreviewPrimary();

        void onVoipMinimalBandwidthChanged(bool _bEnable);

    protected:
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void keyReleaseEvent(QKeyEvent* _e) override;
        void resizeEvent(QResizeEvent * event) override;
        void wheelEvent(QWheelEvent * event) override;

		QWidget* createMaskListWidget();
        MaskWidget* getSelectedWidget();
        MaskWidget* getFirstMask();
        void clearMaskList();

        // Send statistic to core.
        void sendOpenStatistic();
        // Send mask select statistic to core.
        void sendSelectMaskStatistic(MaskWidget* mask);

        // Enumirate all masks. First mask is empty mask.
        void enumerateMask(std::function<void(MaskWidget* mask)> func);
		
		void updateMaskList();

    private:

        QWidget* container_;
        QWidget* rootWidget_;

        UIEffects* panelEffect_;

		QScrollArea* maskList_;
        ScrollWidget *scrollWidget_;
		QVBoxLayout* layoutMaskListWidget_;

		MaskWidget*  currentMaskButton_;

		QVBoxLayout* maskLayout_;

		// Offsets for panel layout.
		int topOffset_;
		int bottomOffset_;

		// Scroll mask buttons.
		QPushButton* upButton_;
		QPushButton* downButton_;

		QWidget* maskListWidget_;

		// Current layout direction.
		QBoxLayout::Direction direction_;

        // Is local video on of off.
        bool hasLocalVideo_;

        // This timer hide mask panel in 7 seconds.
        QTimer* hidePreviewPrimaryTimer_;

        // Background widget to able process mouse events on transparent parts of panel.
        PanelBackground* backgroundWidget_;

        // Is hide/show animation running in this moment.
        bool animationRunning_;
        // Force hide mask list on animation finish.
        bool hideMaskListOnAnimationFinish_;
    };
}
