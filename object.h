// START A3HEADER
//
// This source file is part of the Atlantis PBM game program.
// Copyright (C) 1995-1999 Geoff Dunbar
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program, in the file license.txt. If not, write
// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// See the Atlantis Project web page for details:
// http://www.prankster.com/project
//
// END A3HEADER

#ifndef OBJECT_CLASS
#define OBJECT_CLASS

class Object;

#include <memory>

#include "gamedataarray.h"
#include "objecttype.h"
#include "alist.h"
#include "fileio.h"
#include "gamedefs.h"
#include "faction.h"
#include "items.h"
#include "unit.h"

class ObjectTypeItems : public Items
{
    private:
        bool is_wood_or_stone_ = false;

    public:
        static constexpr int I_WOOD_OR_STONE = -2;

        ObjectTypeItems() :
            Items()
        {
        }

        ObjectTypeItems(size_t type) :
            Items(type)
        {
        }
        
        ObjectTypeItems(const Types& type) :
            Items(type)
        {
        }

        ObjectTypeItems(const ValidEnum<_ItemTypes, _ItemTypes::NITEMS>& rhs) :
            Items(rhs)
        {
        }

        ObjectTypeItems(int type) :
            Items(type == I_WOOD_OR_STONE ? static_cast<int>(Types::NITEMS) : type),
            is_wood_or_stone_(type == I_WOOD_OR_STONE)
        {
        }

        bool isWoodOrStone() const { return isValid() && is_wood_or_stone_; }

        operator Types() const
        {
            if(isWoodOrStone())
            {
                throw std::logic_error("Tried to use IS_WOOD_OR_STONE object type as an index!");
            }
            return Items::operator Types();
        }

        operator size_t() const
        {
            if(isWoodOrStone())
            {
                throw std::logic_error("Tried to use IS_WOOD_OR_STONE object type as an index!");
            }
            return Items::operator size_t();
        }
};

class ObjectType {
    public:
        char const *name;
        enum {
            DISABLED    = 0x001,
            NOMONSTERGROWTH    = 0x002,
            NEVERDECAY    = 0x004,
            CANENTER    = 0x008,
            CANMODIFY    = 0x020,
            TRANSPORT    = 0x040,
            GROUP        = 0x080
        };
        int flags;

        int protect;
        int capacity;
        int sailors;
        int maxMages;

        ObjectTypeItems item;
        int cost;
        char const *skill;
        int level;

        int maxMaintenance;
        int maxMonthlyDecay;
        int maintFactor;

        Items monster;

        Items productionAided;
        int defenceArray[NUM_ATTACK_TYPES];
};

extern const GameDataArray<ObjectType> ObjectDefs;

AString *ObjectDescription(int obj);

Objects LookupObject(AString *token);

Objects ParseObject(AString *, int ships);

bool ObjectIsShip(const Objects&);

class Object
{
    public:
        using Handle = std::shared_ptr<Object>;
        using WeakHandle = std::weak_ptr<Object>;

        Object(const std::weak_ptr<ARegion>& region);
        ~Object();

        void Readin(Ainfile *f, const std::list<std::shared_ptr<Faction>>&, ATL_VER v);
        void Writeout(Aoutfile *f);
        void Report(Areport *, const Faction&, int, int, bool, int, int, bool, bool);

        void SetName(AString *);
        void SetDescribe(AString *);

        std::weak_ptr<Unit> GetUnit(int);
        std::weak_ptr<Unit> GetUnitAlias(int, int); /* alias, faction number */
        std::weak_ptr<Unit> GetUnitId(const UnitId&, size_t);

        // AS
        int IsRoad();

        int IsFleet();
        int IsBuilding();
        int CanModify();
        int CanEnter(ARegion *, Unit *);
        Unit *ForbiddenBy(ARegion *, Unit *);
        Unit *GetOwner();

        void SetPrevDir(int);
        void MoveObject(ARegion *toreg);
        
        // Fleets
        void ReadinFleet(Ainfile *f);
        void WriteoutFleet(Aoutfile *f);
        int CheckShip(int);
        int GetNumShips(int);
        void SetNumShips(int, int);
        void AddShip(int);
        AString FleetDefinition();
        int FleetCapacity();
        int FleetLoad();
        int FleetSailingSkill(int);
        int GetFleetSize();
        int GetFleetSpeed(int);
        
        AString *name;
        AString *describe;
        std::weak_ptr<ARegion> region;
        ssize_t inner;
        int num;
        Objects type;
        int incomplete;
        int capacity;
        int flying;
        int load;
        int runes;
        int prevdir;
        int mages;
        size_t shipno;
        int movepoints;
        std::list<std::shared_ptr<Unit>> units;
        std::list<std::shared_ptr<Item>> ships;
};

#endif
