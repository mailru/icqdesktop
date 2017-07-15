#include "stdafx.h"

#define TESTING_ENABLED 1

namespace Testing
{
    void setAccessibleName(QWidget* target, const QString& name)
    {
#ifdef TESTING_ENABLED
        assert(!!target);
        if (target != NULL)
        {
            target->setAccessibleName(name);
            target->setAccessibleDescription(name);
        }
#endif
    }

    bool isAccessibleRole(int _role)
    {
#ifdef TESTING_ENABLED
        return _role == Qt::AccessibleDescriptionRole || _role == Qt::AccessibleTextRole;
#else
        return false;
#endif
    }
}
