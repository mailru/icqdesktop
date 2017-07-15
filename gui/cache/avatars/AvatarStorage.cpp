#include "stdafx.h"
#include "AvatarStorage.h"

#include "../../core_dispatcher.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../main_window/history_control/HistoryControlPage.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"

namespace
{
	QString CreateKey(const QString& _aimId, const int _sizePx);

	static int CLEANUP_TIMEOUT = 5 * 60 * 1000; //5min

    static int CREATE_TIMEOUT = 2 * 60 * 1000; //2min

    static int REQUEST_TIMEOUT = 15 * 1000; //15 sec
}

namespace Logic
{
	AvatarStorage::AvatarStorage()
		: Timer_(new QTimer(this))
	{
		Timer_->setSingleShot(false);
		Timer_->setInterval(CLEANUP_TIMEOUT);
		Timer_->start();

        connect(Ui::GetDispatcher(), SIGNAL(avatarLoaded(const QString&, QPixmap*, int)), this, SLOT(avatarLoaded(const QString&, QPixmap*, int)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(avatarUpdated(const QString &)), this, SLOT(UpdateAvatar(const QString &)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), this, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::QueuedConnection);
		connect(Timer_, SIGNAL(timeout()), this, SLOT(cleanup()), Qt::QueuedConnection);
	}

	AvatarStorage::~AvatarStorage()
	{
	}

	const QPixmapSCptr& AvatarStorage::Get(const QString& _aimId, const QString& _displayName, const int _sizePx, const bool _isFilled, bool& _isDefault, bool _regenerate)
	{
		assert(_sizePx > 0);

		TimesCache_[_aimId] = QDateTime::currentDateTime();

		Out _isDefault = !LoadedAvatars_.contains(_aimId);
		const auto key = CreateKey(_aimId, _sizePx);

		auto iterByAimIdAndSize = AvatarsByAimIdAndSize_.find(key);
		if (iterByAimIdAndSize != AvatarsByAimIdAndSize_.end())
		{
			return iterByAimIdAndSize->second;
		}

		auto iterByAimId = AvatarsByAimId_.find(_aimId);
		if (iterByAimId == AvatarsByAimId_.end())
		{
			auto drawDisplayName = _displayName.trimmed();
			if (drawDisplayName.isEmpty())
            {
				drawDisplayName = getContactListModel()->getDisplayName(_aimId).trimmed();
            }

			auto defaultAvatar = Utils::getDefaultAvatar(_aimId, drawDisplayName, _sizePx, _aimId == "mail" ? false : _isFilled);
			assert(defaultAvatar);

			const auto result = AvatarsByAimId_.emplace(_aimId, std::make_shared<QPixmap>(std::move(defaultAvatar)));
			assert(result.second);

			iterByAimId = result.first;
		}

		const auto &avatarByAimId = *iterByAimId->second;
        assert(!avatarByAimId.isNull());

		const auto regenerateAvatar = ((avatarByAimId.width() < _sizePx) && _isDefault) && _aimId != _displayName;
		if (regenerateAvatar || _regenerate)
		{
			AvatarsByAimId_.erase(iterByAimId);
			CleanupSecondaryCaches(_aimId);
			return Get(_aimId, _displayName, _sizePx, _isFilled, _isDefault, _regenerate);
		}

        int avatarWidth = avatarByAimId.width();
        int avatarHeight = avatarByAimId.height();
        QPixmap scaledImage;
        if (avatarHeight >= avatarWidth)
            scaledImage = avatarByAimId.scaledToWidth(_sizePx, Qt::SmoothTransformation);
        else
            scaledImage = avatarByAimId.scaledToHeight(_sizePx, Qt::SmoothTransformation);

		const auto result = AvatarsByAimIdAndSize_.emplace(key, std::make_shared<QPixmap>(std::move(scaledImage)));
		assert(result.second);

        if (_aimId == "mail")
            return result.first->second;

		auto requestedAvatarsIter = RequestedAvatars_.find(_aimId);
		if (requestedAvatarsIter == RequestedAvatars_.end() || (avatarByAimId.width() < _sizePx && avatarByAimId.height() < _sizePx))
		{
			Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
			collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_int("size", _sizePx); //request only needed size

			Ui::GetDispatcher()->post_message_to_core("avatars/get", collection.get());

			RequestedAvatars_.insert(_aimId);
		}
        else
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_int("size", _sizePx); //request only needed size

            Ui::GetDispatcher()->post_message_to_core("avatars/show", collection.get());
        }

		return result.first->second;
	}

    void AvatarStorage::SetAvatar(const QString& _aimId, const QPixmap& _pixmap)
    {
        assert(!_aimId.isEmpty());

        auto iter = AvatarsByAimId_.find(_aimId);
        if (iter == AvatarsByAimId_.end())
            return;

        const auto size = iter->second->height();
        auto scaled = _pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        AvatarsByAimId_[_aimId] = std::make_shared<QPixmap>(scaled);

        CleanupSecondaryCaches(_aimId);

        LoadedAvatars_.removeAll(_aimId);
        LoadedAvatars_ << _aimId;

        emit avatarChanged(_aimId);
    }

    void AvatarStorage::UpdateAvatar(const QString& _aimId, bool force)
    {
        if (!force && LoadedAvatars_.contains(_aimId))
            return;

        if (TimesCache_.find(_aimId) != TimesCache_.end())
        {
            auto iter = AvatarsByAimId_.find(_aimId);
            if (iter != AvatarsByAimId_.end())
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", _aimId);
                collection.set_value_as_int("size", iter->second->height());
                collection.set_value_as_bool("force", true);
                Ui::GetDispatcher()->post_message_to_core("avatars/get", collection.get());
            }

            AvatarsByAimId_.erase(_aimId);
            CleanupSecondaryCaches(_aimId);
            RequestedAvatars_.erase(_aimId);
            LoadedAvatars_.removeAll(_aimId);
            TimesCache_.erase(_aimId);
        }
    }
    
    void AvatarStorage::ForceRequest(const QString& _aimId, const int _sizePx)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_int("size", _sizePx);
        collection.set_value_as_bool("force", true);
        Ui::GetDispatcher()->post_message_to_core("avatars/get", collection.get());
    }
    
	const QPixmapSCptr& AvatarStorage::GetRounded(const QString& _aimId, const QString& _displayName, const int _sizePx, const QString& _state, const bool _isFilled, bool& _isDefault, bool _regenerate, bool mini_icons)
	{
		assert(_sizePx > 0);

		const auto &avatar = Get(_aimId, _displayName, _sizePx, _isFilled, _isDefault, _regenerate);
        if (avatar->isNull())
        {
            assert(!"avatar is null");
            return avatar;
        }

		return GetRounded(*avatar, _aimId, _state, mini_icons, _isDefault);
	}

	QString AvatarStorage::GetLocal(const QString& _aimId, const QString& _displayName, const int _sizePx, const bool _isFilled)
	{
		bool isDefault = false;
		QPixmapSCptr avatar = Get(_aimId, _displayName, _sizePx, _isFilled, isDefault, false);

		QFile file(QString(QDir::tempPath() + "/%1_%2.png").arg(_aimId).arg(isDefault ? "_def" : "_"));
		if (!file.exists())
		{
			file.open(QIODevice::WriteOnly);
			avatar->save(&file, "PNG");
		}

		QFileInfo fileInfo(file);
		return fileInfo.absoluteFilePath();
	}

	void AvatarStorage::avatarLoaded(const QString& _aimId, QPixmap* _pixmap, int _size)
	{
		if (!_pixmap)
		{
            if (Logic::getContactListModel()->isChat(_aimId) && !ChatInfoRequested_.contains(_aimId))
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("aimid", _aimId);
                collection.set_value_as_int("limit", Logic::ChatMembersModel::get_limit(1));
                Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
                ChatInfoRequested_.insert(_aimId, _size);
            }

            return;
		}

        if (_pixmap->isNull())
        {
            if (!LoadedAvatarsFails_.contains(_aimId))
            {
                LoadedAvatarsFails_ << _aimId;
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", _aimId);
                collection.set_value_as_int("size", _size);
                collection.set_value_as_bool("force", true);

                Ui::GetDispatcher()->post_message_to_core("avatars/get", collection.get());
            }

            return;
        }

		assert(!_aimId.isEmpty());
        LoadedAvatarsFails_.removeAll(_aimId);

        QPixmapSCptr avatar(_pixmap);
        auto scaledImage = avatar->scaled(_size, _size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPixmapSCptr cache = std::make_shared<QPixmap>(std::move(scaledImage));
		auto iterPixmap = AvatarsByAimId_.find(_aimId);
		if (iterPixmap != AvatarsByAimId_.end())
		{
			iterPixmap->second = cache;
		}
		else
		{
			AvatarsByAimId_.emplace(_aimId, cache);
		}

		CleanupSecondaryCaches(_aimId);

		LoadedAvatars_ << _aimId;

		emit avatarChanged(_aimId);
	}

    void AvatarStorage::chatInfo(qint64, std::shared_ptr<Data::ChatInfo> _info)
    {
        if (!ChatInfoRequested_.contains(_info->AimId_))
            return;

        auto createTime = QDateTime::fromTime_t(_info->CreateTime_);
        if (createTime.msecsTo(QDateTime::currentDateTime()) < CREATE_TIMEOUT)
        {
            QString aimId = _info->AimId_;
            int size = ChatInfoRequested_[_info->AimId_];
            QTimer::singleShot(REQUEST_TIMEOUT, [aimId, size]() 
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", aimId);
                collection.set_value_as_int("size", size);
                collection.set_value_as_bool("force", true);
                Ui::GetDispatcher()->post_message_to_core("avatars/get", collection.get());
            });
        }

        ChatInfoRequested_.remove(_info->AimId_);
    }

    void AvatarStorage::UpdateDefaultAvatarIfNeed(const QString& _aimId)
    {
        assert(!_aimId.isEmpty());

        if (LoadedAvatars_.contains(_aimId))
            return;

        if (AvatarsByAimId_.find(_aimId) != AvatarsByAimId_.end())
            AvatarsByAimId_.erase(_aimId);
        CleanupSecondaryCaches(_aimId);
        emit avatarChanged(_aimId);
    }

	void AvatarStorage::cleanup()
	{
		QDateTime now = QDateTime::currentDateTime();
        auto historyPage = Utils::InterConnector::instance().getHistoryPage(Logic::getContactListModel()->selectedContact());
		for (std::map<QString, QDateTime>::iterator iter = TimesCache_.begin(); iter != TimesCache_.end();)
		{
			if (iter->second.msecsTo(now) >= CLEANUP_TIMEOUT)
			{
                auto aimId = iter->first;
                if (Logic::getContactListModel()->contains(aimId) || (historyPage && historyPage->contains(aimId)))
                {
                    iter->second = now;
                    ++iter;
                    continue;
                }

				AvatarsByAimId_.erase(aimId);
				CleanupSecondaryCaches(aimId);
				RequestedAvatars_.erase(aimId);
				LoadedAvatars_.removeAll(aimId);
				iter = TimesCache_.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

    // TODO : use two-step hash here
	void AvatarStorage::CleanupSecondaryCaches(const QString& _aimId)
	{
		const auto cleanupSecondaryCache = [&_aimId](CacheMap &cache)
		{
			for (auto i = cache.begin(); i != cache.end(); ++i)
			{
				const auto &key = i->first;
				if (!key.startsWith(_aimId))
				{
					continue;
				}

				for(;;)
				{
					i = cache.erase(i);

					if (i == cache.end())
					{
						break;
					}

					const auto &key = i->first;
					if (!key.startsWith(_aimId))
					{
						break;
					}
				}

				break;
			}
		};

		cleanupSecondaryCache(RoundedAvatarsByAimIdAndSize_);
		cleanupSecondaryCache(AvatarsByAimIdAndSize_);
	}

	const QPixmapSCptr& AvatarStorage::GetRounded(const QPixmap& _avatar, const QString& _aimId, const QString& _state, bool mini_icons, bool _isDefault)
	{
		assert(!_avatar.isNull());

		QString key;
		key.reserve(128);
		key += _aimId;
		key += "/";
		key += QString::number(_avatar.width());
		key += "/";
		key += _state;

		auto i = RoundedAvatarsByAimIdAndSize_.find(key);
		if (i == RoundedAvatarsByAimIdAndSize_.end())
		{
			const auto roundedAvatar = Utils::roundImage(_avatar, _state, _isDefault, mini_icons);
			i = RoundedAvatarsByAimIdAndSize_
				.emplace(
					key,
					std::make_shared<QPixmap>(std::move(roundedAvatar)))
				.first;
		}

		return i->second;
	}

	AvatarStorage* GetAvatarStorage()
	{
		static std::unique_ptr<AvatarStorage> storage(new AvatarStorage());
		return storage.get();
	}
}

namespace
{
	QString CreateKey(const QString &_aimId, const int _sizePx)
	{
		assert(_sizePx > 0);

		QString result;
		result.reserve(128);

		result += _aimId;
		result += "/";
		result += QString::number(_sizePx);

		return result;
	}
}
