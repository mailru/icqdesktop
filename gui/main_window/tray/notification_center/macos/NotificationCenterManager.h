#pragma once

namespace Ui
{
	class NotificationCenterManager : public QObject
	{
		Q_OBJECT

	Q_SIGNALS:
		void messageClicked(QString, QString);
        void osxThemeChanged();
        
    private Q_SLOTS:
        void avatarChanged(QString);
        void avatarTimer();
        void displayTimer();
    private:
        
        std::set<QString> changedAvatars_;
        QTimer* avatarTimer_;
        QTimer* displayTimer_;
        
	public:
		NotificationCenterManager();
		~NotificationCenterManager();
        
		void DisplayNotification(const QString& aimdId, const QString& senderNick, const QString& message, const QString& mailId, const QString& displayName);
		void HideNotifications(const QString& aimId);

		void Activated(const QString& aimId, const QString& mailId);
        void themeChanged();
        
//		void Remove(Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> notification, const QString& aimId);
        
        static void updateBadgeIcon(int unreads);
	};
}
