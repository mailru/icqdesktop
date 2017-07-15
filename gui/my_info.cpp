#include "stdafx.h"
#include "my_info.h"
#include "core_dispatcher.h"
#include "cache/avatars/AvatarStorage.h"
#include "gui_settings.h"

namespace Ui
{
    my_info::my_info()
    {
    }

    void my_info::unserialize(core::coll_helper* _collection)
    {
        prevData_ = data_;
        data_.aimId_ = _collection->get_value_as_string("aimId");
        data_.displayId_ = _collection->get_value_as_string("displayId");
        data_.friendlyName_ = _collection->get_value_as_string("friendly");
        data_.state_ = _collection->get_value_as_string("state");
        data_.userType_ = _collection->get_value_as_string("userType");
        data_.phoneNumber_ = _collection->get_value_as_string("attachedPhoneNumber");
        data_.flags_ = _collection->get_value_as_uint("globalFlags");
        data_.largeIconId_ = _collection->get_value_as_string("largeIconId");
        data_.hasMail_ = _collection->get_value_as_bool("hasMail");

        get_gui_settings()->set_value(login_page_last_entered_phone, data_.phoneNumber_);
        get_gui_settings()->set_value(login_page_last_entered_uin, data_.aimId_);
        
        emit received();
    }

    my_info* MyInfo()
    {
        static std::unique_ptr<my_info> info(new my_info());
        return info.get();
    }

    void my_info::CheckForUpdate() const
    {
        if (prevData_.largeIconId_ != data_.largeIconId_)
        {
            Logic::GetAvatarStorage()->UpdateAvatar(aimId());
            emit Logic::GetAvatarStorage()->avatarChanged(aimId());
        }
    }
}
