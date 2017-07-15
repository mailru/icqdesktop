#include "stdafx.h"
#include "FoundContacts.h"

#include "ContactWidget.h"
#include "../../contact_list/contact_profile.h"
#include "../../../cache/avatars/AvatarStorage.h"
#include "../../../cache/countries.h"
#include "../../../controls/FlowLayout.h"
#include "../../../controls/TransparentScrollBar.h"
#include "../../../utils/utils.h"


namespace Ui
{
    FoundContacts::FoundContacts(QWidget* _parent)
        :	QWidget(_parent),
        rootLayout_(new QVBoxLayout()),
        contactsLayout_(new FlowLayout(0, Utils::scale_value(24), Utils::scale_value(24))),
        area_(CreateScrollAreaAndSetTrScrollBar(this)),
        prevHScrollValue_(-1), prevVScrollValue_(-1)
    {
        rootLayout_->setContentsMargins(0, Utils::scale_value(24), 0, 0);
        rootLayout_->setSpacing(0);

        area_->setObjectName("results_scroll_area");
        Utils::grabTouchWidget(area_->viewport(), true);

        QWidget* scrollAreaWidget = new QWidget(area_);
        scrollAreaWidget->setObjectName("results_scroll_area_widget");
        area_->setWidget(scrollAreaWidget);
        area_->setWidgetResizable(true);

        scrollAreaWidget->setLayout(contactsLayout_);
        Utils::grabTouchWidget(scrollAreaWidget);


        rootLayout_->addWidget(area_);

        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(onAvatarLoaded(QString)), Qt::QueuedConnection);

        hookScroll();

        for (const auto& country : Ui::countries::get())
            countries_[country.iso_code_] = country.name_;

        setLayout(rootLayout_);
    }


    FoundContacts::~FoundContacts(void)
    {
    }

    void FoundContacts::hookScroll()
    {
        connect((ScrollAreaWithTrScrollBar *)area_, &ScrollAreaWithTrScrollBar::resized, this, [this]()
        {
            if (!items_.empty() && !area_->verticalScrollBar()->maximum())
                emit needMore((int)items_.size());
        });
        connect(area_->horizontalScrollBar(), &QAbstractSlider::valueChanged, this, [this](int _value)
        {
            if (prevHScrollValue_ != -1 && prevHScrollValue_ < _value)
            {
                int32_t maxValue = area_->horizontalScrollBar()->maximum();
                if ((maxValue - _value) < Utils::scale_value(360))
                    emit needMore((int)items_.size());
            }
            prevHScrollValue_ = _value;
        });
        connect(area_->verticalScrollBar(), &QAbstractSlider::valueChanged, this,  [this](int _value)
        {
            if (prevVScrollValue_ != -1 && prevVScrollValue_ < _value)
            {
                int32_t maxValue = area_->verticalScrollBar()->maximum();
                if ((maxValue - _value) < Utils::scale_value(360))
                    emit needMore((int)items_.size());
            }
            prevVScrollValue_ = _value;
        });
    }
    
    void FoundContacts::onAvatarLoaded(QString _aimid)
    {
        auto iter_cw = items_.find(_aimid);
        if (iter_cw != items_.end())
            iter_cw->second->update();
    }

    int FoundContacts::insertItems(const profiles_list& _profiles)
    {
        const auto prev = items_.size();
        for (auto profile : _profiles)
        {
            if (items_.find(profile->get_aimid()) == items_.end())
            {
                ContactWidget* cw = new ContactWidget(0, profile, countries_);
                
                connect(cw, SIGNAL(addContact(QString)), this, SLOT(onAddContact(QString)), Qt::QueuedConnection);
                connect(cw, SIGNAL(msgContact(QString)), this, SLOT(onMsgContact(QString)), Qt::QueuedConnection);
                connect(cw, SIGNAL(callContact(QString)), this, SLOT(onCallContact(QString)), Qt::QueuedConnection);
                connect(cw, SIGNAL(contactInfo(QString)), this, SLOT(onContactInfo(QString)), Qt::QueuedConnection);
                
                contactsLayout_->addWidget(cw);
                
                items_[profile->get_aimid()] = cw;
            }
        }
        if (!_profiles.empty() && !area_->verticalScrollBar()->maximum())
        {
            emit needMore((int)items_.size());
        }
        return (int)(items_.size() - prev);
    }

    void FoundContacts::clear()
    {
        while (contactsLayout_->count())
        {
            QLayoutItem* item = contactsLayout_->itemAt(0);
            contactsLayout_->removeItem(item);
            item->widget()->deleteLater();
        }

        items_.clear();
    }

    void FoundContacts::onAddContact(QString _contact)
    {
        emit addContact(_contact);
    }

    void FoundContacts::onMsgContact(QString _contact)
    {
        emit msgContact(_contact);
    }

    void FoundContacts::onCallContact(QString _contact)
    {
        emit callContact(_contact);
    }

    void FoundContacts::onContactInfo(QString _contact)
    {
        emit contactInfo(_contact);
    }

    bool FoundContacts::empty()
    {
        return !contactsLayout_->count();
    }

    void FoundContacts::contactAddResult(const QString& _contact, bool _res)
    {
        auto iter = items_.find(_contact);
        if (iter != items_.end())
            iter->second->onAddResult(_res);
    }
}


