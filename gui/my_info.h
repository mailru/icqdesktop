#pragma once

#include "../corelib/collection_helper.h"

namespace Ui
{
    class my_info : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void received();

    private:
        struct MyInfoData
        {
            MyInfoData()
                : flags_(0)
                , auto_created_(false)
                , hasMail_(false)
            {}

            QString aimId_;
            QString displayId_;
            QString friendlyName_;
            QString state_;
            QString userType_;
            QString phoneNumber_;
            uint32_t flags_;
            QString largeIconId_;
            bool auto_created_;
            bool hasMail_;

        };
        MyInfoData data_;
        MyInfoData prevData_;

    public:
        my_info();
        void unserialize(core::coll_helper* _collection);
        bool haveConnectedEmail() { return data_.hasMail_; }

        QString aimId() const { return data_.aimId_; };
        QString displayId() const { return data_.displayId_; };
        QString friendlyName() const {return data_.friendlyName_; };
        QString state() const { return data_.state_; };
        QString userType() const { return data_.userType_; };
        QString phoneNumber() const { return data_.phoneNumber_; };
        uint32_t flags() const { return data_.flags_; };
        bool auto_created() const { return data_.auto_created_; };
        QString largeIconId() const { return data_.largeIconId_; };

        void CheckForUpdate() const;
    };

    my_info* MyInfo();
}