#include "stdafx.h"

#include "SearchMembersModel.h"
#include "SearchModelDLG.h"

#include "ContactList.h"

namespace Logic
{
    AbstractSearchModel::AbstractSearchModel(QObject* _parent/* = 0*/)
        : CustomAbstractListModel(_parent)
        , isClSorting_(true)
    {
    }
    
    AbstractSearchModel* getCurrentSearchModel(int _regim)
    {
        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
            return getSearchModelDLG();
        else if (!Logic::is_members_regim(_regim))
            return getCustomSearchModelDLG(false, _regim != Logic::MembersWidgetRegim::SHARE_LINK && _regim != Logic::MembersWidgetRegim::SHARE_TEXT && _regim != Logic::MembersWidgetRegim::CONTACT_LIST_POPUP);
        else
            return getSearchMemberModel();
    }

    void AbstractSearchModel::setSort(bool _isClSorting)
    {
        isClSorting_ = _isClSorting;
    }

    bool AbstractSearchModel::isClSorting() const
    {
        return isClSorting_;
    }
}
