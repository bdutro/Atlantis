#ifndef UNITID_CLASS
#define UNITID_CLASS

#include <memory>
#include "validvalue.h"
#include "astring.h"

class UnitId {
    public:
        using Handle = std::shared_ptr<UnitId>;

        UnitId();
        ~UnitId();
        AString Print();

        ValidValue<size_t> unitnum; /* if 0, it is a new unit */
        int alias;
        size_t faction;

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
