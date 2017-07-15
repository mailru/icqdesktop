#pragma once

#include "../../types/contact.h"
#include "../../types/chat.h"

namespace Logic
{
	typedef std::shared_ptr<const QPixmap> QPixmapSCptr;

	class AvatarStorage : public QObject
	{
		friend AvatarStorage* GetAvatarStorage();

		Q_OBJECT

	Q_SIGNALS:
		void avatarChanged(QString aimId);

	private Q_SLOTS:
		void avatarLoaded(const QString& _aimId, QPixmap* _avatar, int _size);

        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo> _info);

		void cleanup();

    public Q_SLOTS:
        void UpdateAvatar(const QString& _aimId, bool force = true);
        
	public:
		~AvatarStorage();

		const QPixmapSCptr& Get(const QString& _aimId, const QString& _displayName, const int _sizePx, const bool _isFilled, bool& _isDefault, bool _regenerate);

		const QPixmapSCptr& GetRounded(const QString& _aimId, const QString& _displayName, const int _sizePx, const QString& _state, const bool _isFilled, bool& _isDefault, bool _regenerate, bool mini_icons);

		QString GetLocal(const QString& _aimId, const QString& _displayName, const int _sizePx, const bool _isFilled);

        void UpdateDefaultAvatarIfNeed(const QString& _aimId);
        void ForceRequest(const QString& _aimId, const int _sizePx);

        void SetAvatar(const QString& _aimId, const QPixmap& _pixmap);

    private:
		typedef std::map<QString, QPixmapSCptr> CacheMap;

		AvatarStorage();

		void CleanupSecondaryCaches(const QString& _aimId);

		const QPixmapSCptr& GetRounded(const QPixmap& _avatar, const QString& _aimId, const QString& _state, bool mini_icons, bool _isDefault);

		CacheMap AvatarsByAimIdAndSize_;

		CacheMap AvatarsByAimId_;

		CacheMap RoundedAvatarsByAimIdAndSize_;

		std::set<QString> RequestedAvatars_;

		QStringList LoadedAvatars_;

        QStringList LoadedAvatarsFails_;

        QMap<QString, int> ChatInfoRequested_;

		std::map<QString, QDateTime> TimesCache_;

		QTimer* Timer_;
	};

	AvatarStorage* GetAvatarStorage();
}
