#ifndef UNITID_CLASS
#define UNITID_CLASS

#include "astring.h"

class UnitId {
    public:
        UnitId();
        ~UnitId();
        AString Print();

        size_t unitnum; /* if 0, it is a new unit */
        int alias;
        int faction;

        bool operator<(const UnitId& rhs) const
        {
            if(faction < rhs.faction)
            {
                return true;
            }
            else if(faction > rhs.faction)
            {
                return false;
            }
            else
            {
                return unitnum < rhs.unitnum;
            }
        }

        bool operator==(const UnitId& rhs) const
        {
            return (unitnum == rhs.unitnum) && (alias == rhs.alias) && (faction == rhs.faction);
        }
};

#endif
