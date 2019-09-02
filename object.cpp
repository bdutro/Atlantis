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
// 13/06/04 changed Object class to handle fleets (ravanrooke)

#include "object.h"
#include "items.h"
#include "skills.h"
#include "gamedata.h"
#include "unit.h"

Objects LookupObject(const AString& token)
{
    for (auto i = Objects::begin(); i != Objects::end(); ++i) {
        if (token == ObjectDefs[*i].name)
        {
            return *i;
        }
    }
    return Objects();
}

/* ParseObject checks for matching Object types AND
 * for matching ship-type items (which are also
 * produced using the build order) if the ships
 * argument is given.
 */
ssize_t ParseShipObject(const AString& token)
{
    // Check for ship-type items:
    for (auto i = Items::begin(); i != Items::end(); ++i) {
        if (ItemDefs[*i].type & IT_SHIP) {
            if ((token == ItemDefs[*i].name) ||
                (token == ItemDefs[*i].abr)) {
                    if (ItemDefs[*i].flags & ItemType::DISABLED) continue;
                    return -(static_cast<ssize_t>(*i)+1);
            }
        }
    }

    Objects parse_obj_result = ParseObject(token);

    if(parse_obj_result.isValid())
    {
        return static_cast<ssize_t>(parse_obj_result);
    }

    return -1;
}

Objects ParseObject(const AString& token)
{
    // Check for ship-type items:
    for (auto i = std::next(Objects::begin()); i != Objects::end(); ++i)
    {
        if (token == ObjectDefs[*i].name)
        {
            if (ObjectDefs[*i].flags & ObjectType::DISABLED)
            {
                return Objects();
            }
            return *i;
        }
    }

    return Objects();
}

bool ObjectIsShip(const Objects& ot)
{
    if (ObjectDefs[ot].capacity) return true;
    return false;
}

Object::Object(const ARegion::WeakHandle& reg)
{
    num = 0;
    type = Objects::Types::O_DUMMY;
    name = AString("Dummy");
    incomplete = 0;
    describe.clear();
    capacity = 0;
    mages = 0;
    inner = -1;
    runes = 0;
    region = reg;
    prevdir = -1;
    flying = 0;
    movepoints = Globals->PHASED_MOVE_OFFSET % Globals->MAX_SPEED;
    ships.clear();
}

Object::~Object()
{
    region.reset();
}

void Object::Writeout(Aoutfile *f)
{
    f->PutInt(num);
    if (IsFleet()) f->PutStr(ObjectDefs[Objects::Types::O_FLEET].name);
    else if (type.isValid()) f->PutStr(ObjectDefs[type].name);
    else f->PutStr("NO_OBJECT");
    f->PutInt(incomplete);
    f->PutStr(name);
    if (describe.Len()) {
        f->PutStr(describe);
    } else {
        f->PutStr("none");
    }
    f->PutInt(inner);
    if (Globals->PREVENT_SAIL_THROUGH && !Globals->ALLOW_TRIVIAL_PORTAGE)
        f->PutInt(prevdir);
    else
        f->PutInt(-1);
    f->PutInt(runes);
    f->PutInt(units.size());
    for(const auto& u: units)
    {
        u->Writeout(f);
    }
    WriteoutFleet(f);
}

void Object::Readin(Ainfile *f, const PtrList<Faction>& facs, ATL_VER v)
{
    num = f->GetInt<int>();

    AString temp = f->GetStr();
    type = LookupObject(temp);

    incomplete = f->GetInt<int>();

    name = f->GetStr();
    describe = f->GetStr();
    if (describe == "none") {
        describe.clear();
    }
    inner = f->GetInt<ssize_t>();
    prevdir = f->GetInt<Directions>();
    runes = f->GetInt<int>();

    // Now, fix up a save file if ALLOW_TRIVIAL_PORTAGE is allowed, just
    // in case it wasn't when the save file was made.
    if (Globals->ALLOW_TRIVIAL_PORTAGE) prevdir.invalidate();
    int i = f->GetInt<int>();
    for (int j=0; j<i; j++) {
        Unit::Handle temp = std::make_shared<Unit>();
        temp->Readin(f, facs, v);
        if (temp->faction.expired())
        {
            continue;
        }
        temp->MoveUnit(weak_from_this());
        if (!(temp->faction.lock()->IsNPC())) region.lock()->visited = 1;
    }
    mages = ObjectDefs[type].maxMages;
    ReadinFleet(f);
}

void Object::SetName(const AString& s)
{
    if (s.Len() && CanModify()) {
        AString newname = s.getlegal();
        if (!newname.Len()) {
            return;
        }
        newname += AString(" [") + num + "]";
        name = newname;
    }
}

void Object::SetDescribe(const AString& s)
{
    if (CanModify()) {
        if (s.Len())
        {
            AString newname = s.getlegal();
            describe = newname;
        }
        else
        {
            describe.clear();
        }
    }
}

bool Object::IsFleet()
{
    if (type == Objects::Types::O_FLEET) return true;
    if (ObjectDefs[type].sailors > 0) return true;
    if (!ships.empty()) return true;
    return false;
}

int Object::IsBuilding()
{
    if (ObjectDefs[type].protect)
        return 1;
    return 0;
}

int Object::CanModify()
{
    return (ObjectDefs[type].flags & ObjectType::CANMODIFY);
}

Unit::WeakHandle Object::GetUnit(size_t num)
{
    for(const auto& u: units)
    {
        if(u->num == num)
        {
            return u;
        }
    }
    return Unit::WeakHandle();
}

Unit::WeakHandle Object::GetUnitAlias(int alias, size_t faction)
{
    // First search for units with the 'formfaction'
    for(const auto& u: units)
    {
        if (u->alias == alias && u->formfaction.lock()->num == faction)
        {
            return u;
        }
    }
    // Now search against their current faction
    for(const auto& u: units)
    {
        if (u->alias == alias && u->faction.lock()->num == faction)
        {
            return u;
        }
    }
    return Unit::WeakHandle();
}

Unit::WeakHandle Object::GetUnitId(const UnitId& id, size_t faction)
{
    //if (id == 0) return Unit::WeakHandle();
    if (id.unitnum) {
        return GetUnit(id.unitnum);
    } else {
        if (id.faction) {
            return GetUnitAlias(id.alias, id.faction);
        } else {
            return GetUnitAlias(id.alias, faction);
        }
    }
}

bool Object::CanEnter(const ARegion::Handle&, const Unit::Handle& u)
{
    if (!(ObjectDefs[type].flags & ObjectType::CANENTER) &&
            (u->type == U_MAGE || u->type == U_NORMAL ||
             u->type == U_APPRENTICE)) {
        return false;
    }
    return true;
}

Unit::WeakHandle Object::ForbiddenBy(const ARegion::Handle& reg, const Unit::Handle& u)
{
    const auto owner = GetOwner();
    if (owner.expired() || type == Objects::Types::O_GATEWAY) {
        return Unit::WeakHandle();
    }

    if (owner.lock()->GetAttitude(*reg, u).isNotFriendly()) {
        return owner;
    }
    return Unit::WeakHandle();
}

Unit::WeakHandle Object::GetOwner()
{
    if(units.empty())
    {
        return Unit::WeakHandle();
    }
    return units.front();
}

void Object::Report(Areport *f, const Faction& fac, unsigned int obs, size_t truesight,
        bool detfac, unsigned int passobs, size_t passtrue, bool passdetfac, bool present)
{
    const auto& ob = ObjectDefs[type];

    if ((type != Objects::Types::O_DUMMY) && !present) {
        if (IsFleet() &&
                !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_SHIPS)) {
            // This is a ship and we don't see ships in transit
            return;
        }
        if (IsBuilding() &&
                !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_BUILDINGS)) {
            // This is a building and we don't see buildings in transit
            return;
        }
        if (IsRoad() &&
                !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_ROADS)) {
            // This is a road and we don't see roads in transit
            return;
        }
    }

    /* Fleet Report */
    if (IsFleet()) {
        AString temp = AString("+ ") + name + " : " + FleetDefinition();
        /* report ships:
        for (int item=0; item<NITEMS; item++) {
            int num = GetNumShips(item);
            if (num > 0) {
                if (num > 1) {
                    temp += AString(", ") + num + " " + ItemDefs[item].names;
                } else {
                    temp += AString(", ") + num + " " +ItemDefs[item].name;
                }
            }
        }
        */
        const auto owner = GetOwner();
        if ((!owner.expired() && &fac == owner.lock()->faction.lock().get()) || (obs > 9)){
            temp += ";";
            if (incomplete > 0) {
                temp += AString(" ") + incomplete + "% damaged;";
            }
            temp += AString(" Load: ") + FleetLoad() + "/" + FleetCapacity() + ";";
            temp += AString(" Sailors: ") + FleetSailingSkill(1) + "/" + GetFleetSize() + ";";
            temp += AString(" MaxSpeed: ") + GetFleetSpeed(1);
        }
        if (describe.Len()) {
            temp += AString("; ") + describe;
        }
        temp += ".";
        f->PutStr(temp);
        f->AddTab();
    } else if (type != Objects::Types::O_DUMMY) {
        AString temp = AString("+ ") + name + " : " + ob.name;
        if (incomplete > 0) {
            temp += AString(", needs ") + incomplete;
        } else if (Globals->DECAY &&
                !(ob.flags & ObjectType::NEVERDECAY) && incomplete < 1) {
            if (incomplete > (0 - ob.maxMonthlyDecay)) {
                temp += ", about to decay";
            } else if (incomplete > (0 - ob.maxMaintenance/2)) {
                temp += ", needs maintenance";
            }
        }
        if (inner != -1) {
            temp += ", contains an inner location";
        }
        if (runes) {
            temp += ", engraved with Runes of Warding";
        }
        if (describe.Len()) {
            temp += AString("; ") + describe;
        }
        if (!(ob.flags & ObjectType::CANENTER)) {
            temp += ", closed to player units";
        }
        temp += ".";
        f->PutStr(temp);
        f->AddTab();
    }

    for(const auto& u: units) {
        const auto u_fac = u->faction.lock();
        Attitudes attitude = fac.GetAttitude(u_fac->num);
        if (u_fac.get() == &fac) {
            u->WriteReport(f, 1, true, attitude, fac.showunitattitudes);
        } else {
            if (present) {
                u->WriteReport(f, obs, truesight, detfac, type != Objects::Types::O_DUMMY, attitude, fac.showunitattitudes);
            } else {
                if (((type == Objects::Types::O_DUMMY) &&
                    (Globals->TRANSIT_REPORT &
                     GameDefs::REPORT_SHOW_OUTDOOR_UNITS)) ||
                    ((type != Objects::Types::O_DUMMY) &&
                        (Globals->TRANSIT_REPORT &
                         GameDefs::REPORT_SHOW_INDOOR_UNITS)) ||
                    ((u->guard == GUARD_GUARD) &&
                        (Globals->TRANSIT_REPORT &
                         GameDefs::REPORT_SHOW_GUARDS))) {
                    u->WriteReport(f, passobs, passtrue, passdetfac,
                            type != Objects::Types::O_DUMMY, attitude, fac.showunitattitudes);
                }
            }
        }
    }
    f->EndLine();
    if (type != Objects::Types::O_DUMMY) {
        f->DropTab();
    }
}

void Object::SetPrevDir(const Directions& newdir)
{
    prevdir = newdir;
}

void Object::MoveObject(const ARegion::Handle& toreg)
{
    auto& reg_objs = region.lock()->objects;
    auto it = reg_objs.begin();
    while(it != reg_objs.end())
    {
        if(it->get() == this)
        {
            it = reg_objs.erase(it);
            continue;
        }
        ++it;
    }
    region = toreg;
    toreg->objects.push_back(shared_from_this());
}

bool Object::IsRoad()
{
    return type.isRoad();
}

/* Performs a basic check on items for ship-types.
 * (note: always fails for non-Fleet Objects.)
 */
bool Object::CheckShip(const Items& item)
{
    if (!item.isValid()) return false;
    if (!IsFleet()) return false;
    if (ItemDefs[item].type & IT_SHIP) return true;
    return false;
}

void Object::WriteoutFleet(Aoutfile *f)
{
    if (!IsFleet()) return;
    f->PutInt(ships.size());
    for(const auto& s: ships)
    {
        s->Writeout(f);
    }
}

void Object::ReadinFleet(Ainfile *f)
{
    if (type != Objects::Types::O_FLEET) return;
    size_t nships = f->GetInt<size_t>();
    for (size_t i = 0; i < nships; i++) {
        Item ship;
        ship.Readin(f);
        if (ship.type.isValid())
            SetNumShips(ship.type, ship.num);
    }
}

/* Returns the number of component ships of a given
 * type.
 */
size_t Object::GetNumShips(const Items& type)
{
    if (CheckShip(type)) {
        for(const auto& ship: ships) {
            if (ship->type == type) {
                return ship->num;
            }
        }
    }
    return 0;
}

/* Erases possible previous entries for ship type
 * and resets the number of ships.
 */
void Object::SetNumShips(const Items& type, size_t num)
{
    if (CheckShip(type)) {
        if (num > 0) {
            for(const auto& ship: ships) {
                if (ship->type == type) {
                    ship->num = num;
                    FleetCapacity();
                    return;
                }
            }
            auto& ship = ships.emplace_back(std::make_shared<Item>());
            ship->type = type;
            ship->num = num;
            FleetCapacity();
        } else {
            for(auto it = ships.begin(); it != ships.end(); ++it) {
                const auto& ship = *it;
                if (ship->type == type) {
                    ships.erase(it);
                    FleetCapacity();
                    return;
                }
            }
        }
    }
}

/* Adds one ship of the given type.
 */
void Object::AddShip(const Items& type)
{    
    if (!CheckShip(type)) return;
    size_t num = GetNumShips(type);
    num++;
    SetNumShips(type, num);    
}

/* Returns the String 'Fleet' for multi-ship fleets
 * and the name of the ship for single ship fleets
 */
AString Object::FleetDefinition()
{
    AString fleet;
    Items shiptype;
    size_t num = 0;
    for (auto i = Items::begin(); i != Items::end(); ++i) {
        if (ItemDefs[*i].type & IT_SHIP) {
            size_t sn = GetNumShips(*i);
            if (sn > 0) {
                num += sn;
                shiptype = *i;
            }
        }
    }
    if (num == 1)
    {
        fleet = ItemDefs[shiptype].name;
    }
    else {
        fleet = ObjectDefs[type].name;
        // report ships:
        for (auto i = Items::begin(); i != Items::end(); ++i) {
            const auto item = *i;
            num = GetNumShips(item);
            if (num > 0) {
                if (num > 1) {
                    fleet += AString(", ") + num + " " + ItemDefs[item].names;
                } else {
                    fleet += AString(", ") + num + " " +ItemDefs[item].name;
                }
            }
        }
    }
    return fleet;
}

/* Sets a fleet's sailing capacity.
 */
size_t Object::FleetCapacity()
{
    capacity = 0;
    // Calculate the maximum number of mages while we're at it
    mages = 0;
    if (!IsFleet()) return 0;
    // Fleets are assumed to be flying, at least until we find any
    // non-flying vessels in them
    flying = 1;
    for (auto i= Items::begin(); i != Items::end(); ++i) {
        const auto item = *i;
        size_t num = GetNumShips(item);
        if (num < 1) continue;
        if (ItemDefs[item].fly > 0) {
            capacity += num * ItemDefs[item].fly;
        } else {
            capacity += num * ItemDefs[item].swim;
            flying = 0;
        }
        AString oname = AString(ItemDefs[item].name);
        const auto ot = LookupObject(oname);
        if (ot.isValid() && !ot.isDummy()) {
            mages += num * ObjectDefs[ot].maxMages;
        }
    }
    return capacity;
}

/* Returns a fleet's load or -1 for non-fleet objects.
 */
int Object::FleetLoad()
{
    int load = -1;
    if (IsFleet()) {
        size_t wgt = 0;
        for(const auto& unit: units) {
            wgt += unit->Weight();
        }
        load = static_cast<int>(wgt);
    }
    return load;
}

/* Returns the total skill level of all sailors.
 * If report is not 0, returns the total skill level of all
 * units regardless if they have sail orders (for report
 * purposes).
 */
int Object::FleetSailingSkill(int report)
{
    int skill = -1;
    size_t slvl = 0;
    if (IsFleet()) {
        for(const auto& unit: units) {
            if ((report != 0) ||
                (unit->monthorders && unit->monthorders->type == Orders::Types::O_SAIL)) {
                slvl += unit->GetSkill(Skills::Types::S_SAILING) * unit->GetMen();
            }
        }
        skill = static_cast<int>(slvl);
    }
    return skill;
}

/* Returns fleet size - which is the total of
 * sailors needed to move the fleet.
 */
size_t Object::GetFleetSize()
{
    if (!IsFleet()) return 0;
    size_t inertia = 0;
    for (auto i = Items::begin(); i != Items::end(); ++i) {
        const auto item = *i;
        size_t num = GetNumShips(item);
        if (num > 0) inertia += num * ItemDefs[item].weight;
    }
    return (inertia / 50);
}

/* Returns the fleet speed - theoretical if report
 * argument is greater than zero (which means all
 * potential sailors issued a SAIL command). The
 * latter is mainly for report purposes. Game
 * functions for moving the fleet provide a zero
 * argument.
 */
unsigned int Object::GetFleetSpeed(int report)
{
    int tskill = FleetSailingSkill(report);
    size_t speed = Globals->MAX_SPEED;
    size_t weight = 0;
    size_t capacity = 0;
    size_t bonus;
    size_t windbonus = 0;

    if (!IsFleet()) return 0;

    for (auto i = Items::begin(); i != Items::end(); ++i) {
        const auto item = *i;
        size_t num = GetNumShips(item);
        if (num > 0) {
            weight += num * ItemDefs[item].weight;
            if (ItemDefs[item].fly > 0) {
                capacity += num * ItemDefs[item].fly;
            } else {
                capacity += num * ItemDefs[item].swim;
                flying = 0;
            }
            // Fleets travel as fast as their slowest ship
            if (ItemDefs[item].speed < speed)
                speed = ItemDefs[item].speed;
        }
    }
    // no ships no speed
    if (weight < 1) return 0;

    // check for sufficient sailing skill!
    if (tskill < static_cast<int>(weight / 50)) return 0;
    
    // count wind mages
    for(const auto& unit: units) {
        unsigned int wb = unit->GetAttribute("wind");
        if (wb > 0) {
            windbonus += wb * 12 * Globals->FLEET_WIND_BOOST;
        }
    }
    // speed gain through wind:
    bonus = windbonus / (weight / 50);
    if (bonus > Globals->FLEET_WIND_BOOST)
        bonus = Globals->FLEET_WIND_BOOST;
    speed += bonus;

    // speed bonus due to more / better skilled sailors:
    bonus = 0;
    while (tskill >= static_cast<int>(weight / 25)) {
        bonus++;
        tskill /= 2;
    }
    if (bonus > Globals->FLEET_CREW_BOOST)
        bonus = Globals->FLEET_CREW_BOOST;
    speed += bonus;

    // check for being overloaded
    if (FleetLoad() > static_cast<int>(capacity)) return 0;
    
    // speed bonus due to low load:
    int loadfactor = (static_cast<int>(capacity) / FleetLoad());
    bonus = 0;
    while (loadfactor >= 2) {
        bonus++;
        loadfactor /= 2;
    }
    if (bonus > Globals->FLEET_LOAD_BOOST)
        bonus = Globals->FLEET_LOAD_BOOST;
    speed += bonus;

    // Cap everything at max speed
    if (speed > Globals->MAX_SPEED) speed = Globals->MAX_SPEED;

    return static_cast<unsigned int>(speed);
}

AString::Handle ObjectDescription(const Objects& obj)
{
    if (ObjectDefs[obj].flags & ObjectType::DISABLED)
    {
        return nullptr;
    }

    ObjectType *o = &ObjectDefs[obj];
    AString::Handle temp = std::make_shared<AString>();
    *temp += AString(o->name) + ": ";
    if (ObjectDefs[obj].flags & ObjectType::GROUP) {
        *temp += "This is a group of ships.";
    } else if (o->capacity) {
        *temp += "This is a ship.";
    } else {
        *temp += "This is a building.";
    }

    if (Globals->LAIR_MONSTERS_EXIST && o->monster.isValid()) {
        *temp += " Monsters can potentially lair in this structure.";
        if (o->flags & ObjectType::NOMONSTERGROWTH) {
            *temp += " Monsters in this structures will never regenerate.";
        }
    }

    if (o->flags & ObjectType::CANENTER) {
        *temp += " Units may enter this structure.";
    }

    if (o->protect) {
        *temp += AString(" This structure provides defense to the first ") +
            o->protect + " men inside it.";
        // Now do the defences. First, figure out how many to do.
        int totaldef = 0; 
        for (int i=0; i<NUM_ATTACK_TYPES; i++) {
            totaldef += (o->defenceArray[i] != 0);
        }
    // Now add the description to temp
        *temp += AString(" This structure gives a defensive bonus of ");
        for (int i=0; i<NUM_ATTACK_TYPES; i++) {
            if (o->defenceArray[i]) {
                totaldef--;
                *temp += AString(o->defenceArray[i]) + " against " +
                    AttType(i) + AString(" attacks");
                
                if (totaldef >= 2) {
                    *temp += AString(", ");
                } else {
                    if (totaldef == 1) {    // penultimate bonus
                        *temp += AString(" and ");
                    } else {    // last bonus
                        *temp += AString(".");
                    }
                } // end if 
            }
        } // end for
    }

    /*
     * Handle all the specials
     */
    for (const auto& spd: SpecialDefs) {
        AString effect = "are";
        int match = 0;
        if (!(spd.targflags & SpecialType::HIT_BUILDINGIF) &&
                !(spd.targflags & SpecialType::HIT_BUILDINGEXCEPT)) {
            continue;
        }
        for (size_t j = 0; j < SPECIAL_BUILDINGS; j++)
            if (spd.buildings[j] == obj) match = 1;
        if (!match) continue;
        if (spd.targflags & SpecialType::HIT_BUILDINGEXCEPT) {
            effect += " not";
        }
        *temp += " Units in this structure ";
        *temp += effect + " affected by " + spd.specialname + ".";
    }

    if (o->sailors) {
        *temp += AString(" This ship requires ") + o->sailors +
            " total levels of sailing skill to sail.";
    }
    if (o->maxMages && Globals->LIMITED_MAGES_PER_BUILDING) {
        *temp += " This structure will allow ";
        if (o->maxMages > 1) {
            *temp += "up to ";
            *temp += o->maxMages;
            *temp += " mages";
        } else {
            *temp += "one mage";
        }
        *temp += " to study above level 2.";
    }
    bool buildable = true;
    if (!o->item.isValid() || o->skill == NULL)
    {
        buildable = false;
    }
    if (o->skill != NULL)
    {
        try
        {
            const auto& pS = FindSkill(o->skill);
            if (pS.flags & SkillType::DISABLED)
            {
                buildable = false;
            }
        }
        catch(const NoSuchItemException&)
        {
            buildable = false;
        }
    }
    if (!o->item.isWoodOrStone() && (ItemDefs[o->item].flags & ItemType::DISABLED))
    {
        buildable = false;
    }
    if (o->item.isWoodOrStone() &&
            (ItemDefs[Items::Types::I_WOOD].flags & ItemType::DISABLED) &&
            (ItemDefs[Items::Types::I_STONE].flags & ItemType::DISABLED))
    {
        buildable = false;
    }
    if (!buildable && !(ObjectDefs[obj].flags & ObjectType::GROUP)) {
        *temp += " This structure cannot be built by players.";
    }

    if (o->productionAided.isValid() &&
            !(ItemDefs[o->productionAided].flags & ItemType::DISABLED)) {
        *temp += " This trade structure increases the amount of ";
        if (o->productionAided == Items::Types::I_SILVER) {
            *temp += "entertainment";
        } else {
            *temp += ItemDefs[o->productionAided].names;
        }
        *temp += " available in the region.";
    }

    if (Globals->DECAY) {
        if (o->flags & ObjectType::NEVERDECAY) {
            *temp += " This structure will never decay.";
        } else {
            *temp += AString(" This structure can take ") + o->maxMaintenance +
                " units of damage before it begins to decay.";
            *temp += AString(" Damage can occur at a maximum rate of ") +
                o->maxMonthlyDecay + " units per month.";
            if (buildable) {
                *temp += AString(" Repair of damage is accomplished at ") +
                    "a rate of " + o->maintFactor + " damage units per " +
                    "unit of ";
                if (o->item.isWoodOrStone()) {
                    *temp += "wood or stone.";
                } else {
                    *temp += ItemDefs[o->item].name;
                }
            }
        }
    }

    return temp;
}

void Object::RemoveUnit(const Unit::Handle& u)
{
    for(auto it = units.begin(); it != units.end(); ++it)
    {
        const auto& unit = *it;
        if(unit == u)
        {
            units.erase(it);
            return;
        }
    }
}
