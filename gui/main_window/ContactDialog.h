#pragma once

namespace Ui
{
	class HistoryControl;
    class HistoryControlPage;
	class InputWidget;
    class Sidebar;

	namespace Smiles
	{
		class SmilesMenu;
	}
    class ContactDialog;

    class DragOverlayWindow : public QWidget
    {
        Q_OBJECT

    public:
        DragOverlayWindow(ContactDialog* _parent);

    protected:
        virtual void paintEvent(QPaintEvent* _e);
        virtual void dragEnterEvent(QDragEnterEvent* _e);
        virtual void dragLeaveEvent(QDragLeaveEvent* _e);
        virtual void dragMoveEvent(QDragMoveEvent* _e);
        virtual void dropEvent(QDropEvent* _e);

    private:
        ContactDialog* Parent_;
    };

	class ContactDialog : public QWidget
	{
		Q_OBJECT

	public Q_SLOTS:

		void onContactSelected(QString _aimId, qint64 _messageId, qint64 _quoteId);
		void onSmilesMenu();
		void onInputEditFocusOut();
        void onSendMessage(QString _contact);

		void onContactSelectedToLastMessage(QString _aimId, qint64 _messageId);

    private Q_SLOTS:
        void updateDragOverlay();
        void historyControlClicked();
        void onCtrlFPressedInInputWidget();

	Q_SIGNALS:
		void contactSelected(QString _aimId, qint64 _messageId, qint64 _quoteId);
		void sendMessage(QString);
        void clicked();

		void contactSelectedToLastMessage(QString _aimId, qint64 _messageId);

	private:
		HistoryControl*				historyControlWidget_;
		InputWidget*				inputWidget_;
		Smiles::SmilesMenu*			smilesMenu_;
        DragOverlayWindow*          dragOverlayWindow_;
        Sidebar*                    sidebar_;
        QTimer*                     overlayUpdateTimer_;
        QTimer*                     sidebarUpdateTimer_;
        QStackedWidget*             topWidget_;
        QVBoxLayout*                rootLayout_;
        QMap<QString, QWidget*>     topWidgetsCache_;
        bool                        sidebarVisible_;
        QHBoxLayout*                layout_;

		void initSmilesMenu();
		void initInputWidget();

	public:
		ContactDialog(QWidget* _parent);
		~ContactDialog();
        void cancelSelection();
        void hideInput();

        void showDragOverlay();
        void hideDragOverlay();

        void showSidebar(const QString& _aimId, int _page);
        bool isSidebarVisible() const;
        void setSidebarVisible(bool _show, bool _force = false);
        bool needShowSidebar() const;
        void hideSmilesMenu();

        static bool needShowSidebar(int _contactDialogWidth);
        static bool sideBarShowSingle(int _contactDialogWidth);
        static std::string getSideBarPolicy(int _contactDialogWidth);

        void takeSidebar();
        Sidebar* getSidebar() const;

        void insertTopWidget(const QString& _aimId, QWidget* _widget);
        void removeTopWidget(const QString& _aimId);

        HistoryControlPage* getHistoryPage(const QString& _aimId) const;
        const Smiles::SmilesMenu* getSmilesMenu() const;

        void setFocusOnInputWidget();
        Ui::InputWidget* getInputWidget() const;

        void notifyApplicationWindowActive(const bool isActive);

    protected:
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dragLeaveEvent(QDragLeaveEvent *);
        virtual void dragMoveEvent(QDragMoveEvent *);
        virtual void resizeEvent(QResizeEvent*);
	};
}