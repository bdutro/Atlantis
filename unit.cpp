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

#include <algorithm>

#include "unit.h"
#include "gamedata.h"

AString UnitId::Print() const
{
    if (unitnum) {
        return AString(unitnum);
    } else {
        if (faction) {
            return AString("faction ") + AString(faction) + " new " +
                AString(alias);
        } else {
            return AString("new ") + AString(alias);
        }
    }
}

Unit::WeakHandle GetUnitList(const WeakPtrList<Unit>& list, const Unit::Handle& u)
{
    for(const auto& p: list) {
        if (p.lock() == u) return p;
    }
    return Unit::WeakHandle();
}

Unit::Unit()
{
    name = 0;
    describe = 0;
    num = 0;
    type = U_NORMAL;
    alias = 0;
    guard = GUARD_NONE;
    reveal = REVEAL_NONE;
    flags = FLAG_NOCROSS_WATER;
    movepoints = Globals->PHASED_MOVE_OFFSET % Globals->MAX_SPEED;
    inTurnBlock = 0;
    presentTaxing = 0;
    format = 0;
    free = 0;
    practiced = false;
    moved = 0;
    phase.invalidate();
    savedmovement = 0;
    ClearOrders();
    raised = 0;
}

Unit::Unit(size_t seq, const Faction::Handle& f, int a)
{
    num = seq;
    type = U_NORMAL;
    name = new AString;
    describe = 0;
    *name = AString("Unit (") + num + ")";
    faction = f;
    formfaction = f;
    alias = a;
    guard = 0;
    reveal = REVEAL_NONE;
    flags = FLAG_NOCROSS_WATER;
    movepoints = Globals->PHASED_MOVE_OFFSET % Globals->MAX_SPEED;
    inTurnBlock = 0;
    presentTaxing = 0;
    format = 0;
    free = 0;
    practiced = false;
    moved = 0;
    phase.invalidate();
    savedmovement = 0;
    ClearOrders();
    raised = 0;
}

Unit::~Unit()
{
    if (name)
    {
        delete name;
    }
    if (describe)
    {
        delete describe;
    }
}

void Unit::SetMonFlags()
{
    guard = GUARD_AVOID;
    SetFlag(FLAG_HOLDING, 1);
}

void Unit::MakeWMon(char const *monname, const Items& mon, size_t num)
{
    AString *temp = new AString(monname);
    SetName(temp);

    type = U_WMON;
    items.SetNum(mon, num);
    SetMonFlags();
}

void Unit::Writeout(Aoutfile *s)
{
    s->PutStr(*name);
    if (describe) {
        s->PutStr(*describe);
    } else {
        s->PutStr("none");
    }
    s->PutInt(num);
    s->PutInt(type);
    s->PutInt(faction.lock()->num);
    s->PutInt(guard);
    s->PutInt(reveal);
    s->PutInt(free);
    if (readyItem.isValid())
    {
        s->PutStr(ItemDefs[readyItem].abr);
    }
    else
    {
        s->PutStr("NO_ITEM");
    }
    for (size_t i = 0; i < MAX_READY; ++i) {
        if (readyWeapon[i].isValid())
        {
            s->PutStr(ItemDefs[readyWeapon[i]].abr);
        }
        else
        {
            s->PutStr("NO_ITEM");
        }
        if (readyArmor[i].isValid())
        {
            s->PutStr(ItemDefs[readyArmor[i]].abr);
        }
        else
        {
            s->PutStr("NO_ITEM");
        }
    }
    s->PutInt(flags);
    items.Writeout(s);
    skills.Writeout(s);
    if (combat.isValid())
    {
        s->PutStr(SkillDefs[combat].abbr);
    }
    else
    {
        s->PutStr("NO_SKILL");
    }
    s->PutInt(savedmovement);
    s->PutInt(savedmovedir);
    s->PutInt(visited.size());
    for (const auto& v: visited)
    {
        s->PutStr(v.c_str());
    }
}

void Unit::Readin(Ainfile *s, const PtrList<Faction>& facs, ATL_VER)
{
    name = s->GetStr();
    describe = s->GetStr();
    if (*describe == "none") {
        delete describe;
        describe = 0;
    }
    num = s->GetInt<size_t>();
    type = s->GetInt<int>();
    const size_t fac = s->GetInt<size_t>();
    faction = GetFaction(facs, fac);
    guard = s->GetInt<int>();
    if (guard == GUARD_ADVANCE)
    {
        guard = GUARD_NONE;
    }
    if (guard == GUARD_SET)
    {
        guard = GUARD_GUARD;
    }
    reveal = s->GetInt<int>();

    /* Handle the new 'ready item', ready weapons/armor, and free */
    free = 0;
    readyItem.invalidate();
    for (size_t i = 0; i < MAX_READY; i++) {
        readyWeapon[i].invalidate();
        readyArmor[i].invalidate();
    }

    free = s->GetInt<size_t>();
    AString *temp;
    temp = s->GetStr();
    readyItem = LookupItem(*temp);
    delete temp;
    for (size_t i = 0; i < MAX_READY; i++) {
        temp = s->GetStr();
        readyWeapon[i] = LookupItem(*temp);
        delete temp;
        temp = s->GetStr();
        readyArmor[i] = LookupItem(*temp);
        delete temp;
    }
    flags = s->GetInt<int>();

    items.Readin(s);
    skills.Readin(s);
    temp = s->GetStr();
    combat = LookupSkill(*temp);
    delete temp;
    savedmovement = s->GetInt<int>();
    savedmovedir = s->GetInt<Directions>();
    size_t num_visited = s->GetInt<size_t>();
    while (num_visited-- > 0) {
        temp = s->GetStr();
        visited.insert(temp->Str());
        delete temp;
    }
}

AString Unit::MageReport()
{
    AString temp;

    if (combat.isValid()) {
        temp = AString(". Combat spell: ") + SkillStrs(combat);
    }
    return temp;
}

AString Unit::ReadyItem()
{
    AString temp, weaponstr, armorstr, battlestr;

    unsigned int item = 0;
    for (const auto& ready: readyWeapon) {
        if (ready.isValid()) {
            if (item)
            {
                weaponstr += ", ";
            }
            weaponstr += ItemString(ready, 1);
            ++item;
        }
    }
    if (item > 0)
    {
        weaponstr = AString("Ready weapon") + (item == 1?"":"s") + ": " + weaponstr;
    }
    unsigned int weapon = item;

    item = 0;
    for (const auto& ready: readyArmor) {
        if (ready.isValid()) {
            if (item)
            {
                armorstr += ", ";
            }
            armorstr += ItemString(ready, 1);
            ++item;
        }
    }
    if (item > 0)
    {
        armorstr = AString("Ready armor: ") + armorstr;
    }
    unsigned int armor = item;

    if (readyItem.isValid()) {
        battlestr = AString("Ready item: ") + ItemString(readyItem, 1);
        item = 1;
    } else
        item = 0;

    if (weapon || armor || item) {
        temp += AString(". ");
        if (weapon)
        {
            temp += weaponstr;
        }
        if (armor)
        {
            if (weapon)
            {
                temp += ". ";
            }
            temp += armorstr;
        }
        if (item)
        {
            if (armor || weapon)
            {
                temp += ". ";
            }
            temp += battlestr;
        }
    }
    return temp;
}

AString Unit::StudyableSkills()
{
    AString temp;

    bool j = false;
    for (auto i = Skills::begin(); i != Skills::end(); ++i)
    {
        if (SkillDefs[*i].depends[0].skill != nullptr)
        {
            if (CanStudy(*i))
            {
                if (j)
                {
                    temp += ", ";
                }
                else
                {
                    temp += ". Can Study: ";
                    j=true;
                }
                temp += SkillStrs(*i);
            }
        }
    }
    return temp;
}

AString Unit::GetName(unsigned int obs)
{
    AString ret = *name;
    unsigned int stealth = GetAttribute("stealth");
    if (reveal == REVEAL_FACTION || obs > stealth) {
        ret += ", ";
        ret += *faction.lock()->name;
    }
    return ret;
}

bool Unit::CanGetSpoil(const Item::Handle& i)
{
    if (!i)
    {
        return false;
    }
    if (ItemDefs[i->type].type & IT_SHIP) {
        // Don't pick up an incomplete ship if we already have one
        if (items.GetNum(i->type) > 0)
        {
            return false;
        }
    }
    const size_t weight = ItemDefs[i->type].weight;
    if (!weight)
    {
        return true; // any unit can carry 0 weight spoils
    }

    if (flags & FLAG_NOSPOILS)
    {
        return false;
    }

    size_t load = items.Weight();
    
    if (flags & FLAG_FLYSPOILS) {
        const auto capacity = ItemDefs[i->type].fly;
        if (FlyingCapacity() + capacity < load + weight)
        {
            return false;
        }
    }

    if (flags & FLAG_RIDESPOILS) {
        const auto capacity = ItemDefs[i->type].ride;
        if (RidingCapacity() + capacity < load + weight)
        {
            return false;
        }
    }

    if (flags & FLAG_WALKSPOILS) {
        auto capacity = ItemDefs[i->type].walk;
        if (ItemDefs[i->type].hitchItem)
        {
            if (items.GetNum(ItemDefs[i->type].hitchItem) > items.GetNum(i->type))
            {
                capacity = ItemDefs[i->type].hitchwalk;
            }
        }
        if (WalkingCapacity() + capacity < load + weight)
        {
            return false;
        }
    }

    if (flags & FLAG_SWIMSPOILS) {
        auto capacity = ItemDefs[i->type].swim;
        if (ItemDefs[i->type].type & IT_SHIP)
        {
            capacity = 0;
        }
        if (SwimmingCapacity() + capacity < load + weight)
        {
            return false;
        }
    }

    if ((flags & FLAG_SAILSPOILS) && !object.expired())
    {
        const auto object_s = object.lock();
        if(object_s->IsFleet())
        {
            const int load = object_s->FleetLoad();
            if (object_s->FleetCapacity() < static_cast<size_t>(load) + weight)
            {
                return false;
            }
        }
    }

    return true; // all spoils
}

AString Unit::SpoilsReport() {
    AString temp;
    if (GetFlag(FLAG_NOSPOILS))
    {
        temp = ", weightless battle spoils";
    }
    else if (GetFlag(FLAG_FLYSPOILS))
    {
        temp = ", flying battle spoils";
    }
    else if (GetFlag(FLAG_WALKSPOILS))
    {
        temp = ", walking battle spoils";
    }
    else if (GetFlag(FLAG_RIDESPOILS))
    {
        temp = ", riding battle spoils";
    }
    else if (GetFlag(FLAG_SAILSPOILS))
    {
        temp = ", sailing battle spoils";
    }
    return temp;
}

void Unit::WriteReport(Areport *f,
                       size_t truesight,
                       bool detfac,
                       const Attitudes& attitude,
                       bool showattitudes)
{
    WriteReport_(f, 2, truesight, detfac, attitude, showattitudes);
}

void Unit::WriteReport(Areport *f,
                       unsigned int obs,
                       size_t truesight,
                       bool detfac,
                       bool autosee,
                       const Attitudes& attitude,
                       bool showattitudes)
{
    unsigned int stealth = GetAttribute("stealth");
    if (obs < stealth) {
        /* The unit cannot be seen */
        if (reveal == REVEAL_FACTION) {
            obs = 1;
        } else {
            if (guard == GUARD_GUARD || reveal == REVEAL_UNIT || autosee) {
                obs = 0;
            } else {
                return;
            }
        }
    } else {
        if (obs == stealth) {
            /* Can see unit, but not Faction */
            if (reveal == REVEAL_FACTION) {
                obs = 1;
            } else {
                obs = 0;
            }
        } else {
            /* Can see unit and Faction */
            obs = 1;
        }
    }

    WriteReport_(f, obs, truesight, detfac, attitude, showattitudes);
}

void Unit::WriteReport_(Areport *f,
                       unsigned int obs,
                       size_t truesight,
                       bool detfac,
                       const Attitudes& attitude,
                       bool showattitudes)
{
    /* Setup True Sight */
    if (obs == 2) {
        truesight = 1;
    } else {
        if (GetSkill(Skills::Types::S_ILLUSION) > truesight) {
            truesight = 0;
        } else {
            truesight = 1;
        }
    }

    if (detfac && obs != 2)
    {
        obs = 1;
    }

    /* Write the report */
    AString temp;
    if (obs == 2) {
        temp += AString("* ") + *name;
    } else {
        if (showattitudes) {
            switch (attitude) {
            case A_ALLY: 
                temp += AString("= ") +*name;
                break;
            case A_FRIENDLY: 
                temp += AString(": ") +*name;
                break;
            case A_NEUTRAL: 
                temp += AString("- ") +*name;
                break;
            case A_UNFRIENDLY: 
                temp += AString("% ") +*name;
                break;
            case A_HOSTILE: 
                temp += AString("! ") +*name;
                break;
            }
        } else {
            temp += AString("- ") + *name;
        }
    }

    if (guard == GUARD_GUARD) temp += ", on guard";
    if (obs > 0) {
        temp += AString(", ") + *faction.lock()->name;
        if (guard == GUARD_AVOID) temp += ", avoiding";
        if (GetFlag(FLAG_BEHIND)) temp += ", behind";
    }

    if (obs == 2) {
        if (reveal == REVEAL_UNIT) temp += ", revealing unit";
        if (reveal == REVEAL_FACTION) temp += ", revealing faction";
        if (GetFlag(FLAG_HOLDING)) temp += ", holding";
        if (GetFlag(FLAG_AUTOTAX)) temp += ", taxing";
        if (GetFlag(FLAG_NOAID)) temp += ", receiving no aid";
        if (GetFlag(FLAG_SHARING)) temp += ", sharing";
        if (GetFlag(FLAG_CONSUMING_UNIT)) temp += ", consuming unit's food";
        if (GetFlag(FLAG_CONSUMING_FACTION))
            temp += ", consuming faction's food";
        if (GetFlag(FLAG_NOCROSS_WATER)) temp += ", won't cross water";
        temp += SpoilsReport();
    }

    temp += items.Report(obs, truesight, 0);

    if (obs == 2) {
        temp += ". Weight: ";
        temp += AString(items.Weight());
        temp += ". Capacity: ";
        temp += AString(FlyingCapacity());
        temp += "/";
        temp += AString(RidingCapacity());
        temp += "/";
        temp += AString(WalkingCapacity());
        temp += "/";
        temp += AString(SwimmingCapacity());
        temp += ". Skills: ";
        temp += skills.Report(GetMen());
    }

    if (obs == 2 && (type == U_MAGE || type == U_GUARDMAGE)) {
        temp += MageReport();
    }

    if (obs == 2) {
        temp += ReadyItem();
        temp += StudyableSkills();
        if (visited.size() > 0) {
            unsigned int count = 0;
            temp += ". Has visited ";
            for (const auto& v: visited) {
                count++;
                if (count > 1) {
                    if (count == visited.size())
                    {
                        temp += " and ";
                    }
                    else
                    {
                        temp += ", ";
                    }
                }
                temp += v.c_str();
            }
        }
    }

    if (describe) {
        temp += AString("; ") + *describe;
    }
    temp += ".";
    f->PutStr(temp);
}

AString Unit::TemplateReport()
{
    /* Write the report */
    AString temp;
    temp = *name;

    if (guard == GUARD_GUARD) temp += ", on guard";
    if (guard == GUARD_AVOID) temp += ", avoiding";
    if (GetFlag(FLAG_BEHIND)) temp += ", behind";
    if (reveal == REVEAL_UNIT) temp += ", revealing unit";
    if (reveal == REVEAL_FACTION) temp += ", revealing faction";
    if (GetFlag(FLAG_HOLDING)) temp += ", holding";
    if (GetFlag(FLAG_AUTOTAX)) temp += ", taxing";
    if (GetFlag(FLAG_NOAID)) temp += ", receiving no aid";
    if (GetFlag(FLAG_SHARING)) temp += ", sharing";
    if (GetFlag(FLAG_CONSUMING_UNIT)) temp += ", consuming unit's food";
    if (GetFlag(FLAG_CONSUMING_FACTION)) temp += ", consuming faction's food";
    if (GetFlag(FLAG_NOCROSS_WATER)) temp += ", won't cross water";
    temp += SpoilsReport();

    temp += items.Report(2, 1, 0);
    temp += ". Weight: ";
    temp += AString(items.Weight());
    temp += ". Capacity: ";
    temp += AString(FlyingCapacity());
    temp += "/";
    temp += AString(RidingCapacity());
    temp += "/";
    temp += AString(WalkingCapacity());
    temp += "/";
    temp += AString(SwimmingCapacity());
    temp += ". Skills: ";
    temp += skills.Report(GetMen());

    if (type == U_MAGE || type == U_GUARDMAGE) {
        temp += MageReport();
    }
    temp += ReadyItem();
    temp += StudyableSkills();
    if (visited.size() > 0) {
        unsigned int count;

        count = 0;
        temp += ". Has visited ";
        for (const auto& v: visited) {
            count++;
            if (count > 1) {
                if (count == visited.size())
                {
                    temp += " and ";
                }
                else
                {
                    temp += ", ";
                }
            }
            temp += v.c_str();
        }
    }

    if (describe) {
        temp += AString("; ") + *describe;
    }
    temp += ".";
    return temp;
}

AString *Unit::BattleReport(unsigned int obs)
{
    AString *temp = new AString("");
    if (Globals->BATTLE_FACTION_INFO)
        *temp += GetName(obs);
    else
        *temp += *name;

    if (GetFlag(FLAG_BEHIND)) *temp += ", behind";

    *temp += items.BattleReport();

    for(const auto& s: skills) {
        if (SkillDefs[s.type].flags & SkillType::BATTLEREP) {
            const size_t lvl = GetAvailSkill(s.type);
            if (lvl) {
                *temp += ", ";
                *temp += SkillDefs[s.type].name;
                *temp += " ";
                *temp += lvl;
            }
        }
    }

    if (describe) {
        *temp += "; ";
        *temp += *describe;
    }

    *temp += ".";
    return temp;
}

void Unit::ClearOrders()
{
    canattack = 1;
    nomove = 0;
    routed = 0;
    enter = 0;
    build = 0;
    destroy = 0;
    attackorders.reset();
    evictorders.reset();
    stealorders.reset();
    promote = 0;
    taxing = TAX_NONE;
    advancefrom.reset();
    monthorders.reset();
    inTurnBlock = 0;
    presentTaxing = 0;
    presentMonthOrders.reset();
    ClearCastOrders();
}

void Unit::ClearCastOrders()
{
    castorders.reset();
    teleportorders.reset();
}

void Unit::DefaultOrders(const Object::Handle& obj)
{
    ClearOrders();
    if (type == U_WMON)
    {
        if (!ObjectDefs[obj->type].monster.isValid())
        {
            // count starts at 2 to give a 2 / (available dirs + 2)
            // chance of a wandering monster not moving
            unsigned int count = 2;
            const size_t weight = items.Weight();
            const auto r = obj->region.lock();
            for (const auto& n_w: r->neighbors)
            {
                if (n_w.expired())
                {
                    continue;
                }
                const auto n = n_w.lock();
                if (TerrainDefs[n->type].similar_type == Regions::Types::R_OCEAN &&
                        !CanReallySwim() &&
                        !(CanFly(weight) &&
                            Globals->FLIGHT_OVER_WATER == GameDefs::WFLIGHT_UNLIMITED))
                {
                    continue;
                }
                if (TerrainDefs[n->type].similar_type != Regions::Types::R_OCEAN &&
                        !CanWalk(weight) &&
                        !CanRide(weight) &&
                        !CanFly(weight))
                {
                    continue;
                }
                count++;

            }
            count = getrandom(count);
            for (auto i = Directions::begin(); i != Directions::end(); ++i)
            {
                const auto& n_w = r->neighbors[*i];
                if (n_w.expired())
                {
                    continue;
                }

                const auto n = n_w.lock();
                if (TerrainDefs[n->type].similar_type == Regions::Types::R_OCEAN &&
                        !CanReallySwim() &&
                        !(CanFly(weight) &&
                            Globals->FLIGHT_OVER_WATER == GameDefs::WFLIGHT_UNLIMITED))
                {
                    continue;
                }
                if (TerrainDefs[n->type].similar_type != Regions::Types::R_OCEAN &&
                        !CanWalk(weight) &&
                        !CanRide(weight) &&
                        !CanFly(weight))
                {
                    continue;
                }
                if (!count--)
                {
                    MoveOrder::Handle o = std::make_shared<MoveOrder>();
                    o->advancing = false;
                    int aper = Hostile();
                    aper *= Globals->MONSTER_ADVANCE_HOSTILE_PERCENT;
                    aper /= 100;
                    if (aper < Globals->MONSTER_ADVANCE_MIN_PERCENT)
                    {
                        aper = Globals->MONSTER_ADVANCE_MIN_PERCENT;
                    }
                    if (getrandom(100) < aper)
                    {
                        o->advancing = true;
                    }
                    auto& d = o->dirs.emplace_back(std::make_shared<MoveDir>());
                    d->dir = *i;
                    monthorders = o;
                }
            }
        }
    }
    else if (type == U_GUARD)
    {
        if (guard != GUARD_GUARD)
        {
            guard = GUARD_SET;
        }
    }
    else if (type == U_GUARDMAGE)
    {
        combat = Skills::Types::S_FIRE;
    }
    else
    {
        /* Set up default orders for factions which submit none */
        if (obj->region.lock()->type != Regions::Types::R_NEXUS)
        {
            if (GetFlag(FLAG_AUTOTAX) &&
                    Globals->TAX_PILLAGE_MONTH_LONG && Taxers(1))
            {
                taxing = TAX_AUTO;
            }
            else
            {
                ProduceOrder::Handle order = std::make_shared<ProduceOrder>();
                order->skill.invalidate();
                order->item = Items::Types::I_SILVER;
                order->target = 0;
                monthorders = order;
            }
        }
    }
}

void Unit::PostTurn(const ARegion&)
{
    if (type == U_WMON)
    {
        auto it = items.begin();
        while(it != items.end())
        {
            const auto& i = *it;
            if (!(ItemDefs[i->type].type & IT_MONSTER))
            {
                it = items.erase(it);
                continue;
            }
            ++it;
        }
        if (free > 0)
        {
            --free;
        }
    }
}

void Unit::SetName(AString *s)
{
    if (s) {
        AString *newname = s->getlegal();
        if (!newname) {
            delete s;
            return;
        }
        *newname += AString(" (") + num + ")";
        delete s;
        delete name;
        name = newname;
    }
}

void Unit::SetDescribe(AString *s)
{
    if (describe) delete describe;
    if (s) {
        AString *newname = s->getlegal();
        delete s;
        describe = newname;
    } else
        describe = 0;
}

bool Unit::IsAlive()
{
    if (type == U_MAGE || type == U_APPRENTICE)
    {
        return(GetMen());
    }
    else
    {
        for(const auto& i: items) {
            if (IsSoldier(i->type) && i->num > 0)
            {
                return true;
            }
        }
    }
    return false;
}

void Unit::SetMen(const Items& t, size_t n)
{
    if (ItemDefs[t].type & IT_MAN)
    {
        const size_t oldmen = GetMen();
        items.SetNum(t, n);
        const size_t newmen = GetMen();
        if (newmen < oldmen)
        {
            skills.Split(oldmen, oldmen - newmen);
        }
    }
    else
    {
        /* This is probably a monster in this case */
        items.SetNum(t, n);
    }
}

size_t Unit::GetMen(const Items& t)
{
    return items.GetNum(t);
}

size_t Unit::GetMons()
{
    return items.GetNumMatching(IT_MONSTER);
}

size_t Unit::GetMen()
{
    return items.GetNumMatching(IT_MAN);
}

size_t Unit::GetLeaders()
{
    return items.GetNumMatching(IT_LEADER);
}

size_t Unit::GetSoldiers()
{
    return items.GetNumSoldiers();
}

void Unit::SetMoney(size_t n)
{
    items.SetNum(Items::Types::I_SILVER, n);
}

size_t Unit::GetMoney()
{
    return items.GetNum(Items::Types::I_SILVER);
}

size_t Unit::GetSharedNum(const Items& item)
{
    size_t count = 0;

    if (ItemDefs[item].type & IT_MAN)
    {
        return items.GetNum(item);
    }

    const auto faction_s = faction.lock();
    for(const auto& obj: object.lock()->region.lock()->objects)
    {
        for(const auto& u: obj->units)
        {
            if ((u->num == num) || (u->faction.lock() == faction_s && u->GetFlag(FLAG_SHARING)))
            {
                count += u->items.GetNum(item);
            }
        }
    }

    return count;
}

void Unit::ConsumeShared(const Items& item, size_t n)
{
    if (items.GetNum(item) >= n)
    {
        // This unit doesn't need to use shared resources
        items.SetNum(item, items.GetNum(item) - n);
        return;
    }

    // Use up items carried by the using unit first
    n -= items.GetNum(item);
    items.SetNum(item, 0);

    const auto faction_s = faction.lock();
    for(const auto& obj: object.lock()->region.lock()->objects)
    {
        for(const auto& u: obj->units)
        {
            if (u->faction.lock() == faction_s && u->GetFlag(FLAG_SHARING))
            {
                if (u->items.GetNum(item) < 1)
                {
                    continue;
                }
                if (u->items.GetNum(item) >= n)
                {
                    u->items.SetNum(item, u->items.GetNum(item) - n);
                    u->Event(*(u->name) + " shares " + ItemString(item, n) +
                            " with " + *name + ".");
                    return;
                }
                u->Event(*(u->name) + " shares " +
                        ItemString(item, u->items.GetNum(item)) +
                        " with " + *name + ".");
                n -= u->items.GetNum(item);
                u->items.SetNum(item, 0);
            }
        }
    }
}

size_t Unit::GetSharedMoney()
{
    return GetSharedNum(Items::Types::I_SILVER);
}

void Unit::ConsumeSharedMoney(size_t n)
{
    return ConsumeShared(Items::Types::I_SILVER, n);
}

size_t Unit::GetAttackRiding()
{
    size_t riding = 0;
    if (type == U_WMON)
    {
        for(const auto& i: items)
        {
            if (ItemDefs[i->type].type & IT_MONSTER)
            {
                if (ItemDefs[i->type].fly)
                {
                    return 5;
                }
                if (ItemDefs[i->type].ride)
                {
                    riding = 3;
                }
            }
        }
        return riding;
    }
    else
    {
        riding = GetSkill(Skills::Types::S_RIDING);
        unsigned int lowriding = 0;
        size_t minweight = 10000;
        for(const auto& i: items)
        {
            if (ItemDefs[i->type].type & IT_MAN)
            {
                if (ItemDefs[i->type].weight < minweight)
                {
                    minweight = ItemDefs[i->type].weight;
                }
            }
        }
        for(const auto& i: items)
        {
            if (ItemDefs[i->type].fly - ItemDefs[i->type].weight >= minweight)
            {
                return riding;
            }
            if (ItemDefs[i->type].ride-ItemDefs[i->type].weight >= minweight)
            {
                if (riding <= 3)
                {
                    return riding;
                }
                lowriding = 3;
            }
        }
        return lowriding;
    }
}

size_t Unit::GetDefenseRiding()
{
    if (guard == GUARD_GUARD)
    {
        return 0;
    }

    size_t riding = 0;
    size_t weight = Weight();

    if (CanFly(weight))
    {
        riding = 5;
    }
    else if (CanRide(weight))
    {
        riding = 3;
    }

    if (GetMen())
    {
        size_t manriding = GetSkill(Skills::Types::S_RIDING);
        if (manriding < riding)
        {
            return manriding;
        }
    }

    return riding;
}

size_t Unit::GetSkill(const Skills& sk)
{
    if (sk == Skills::Types::S_TACTICS)
    {
        return GetAttribute("tactics");
    }
    if (sk == Skills::Types::S_STEALTH)
    {
        return GetAttribute("stealth");
    }
    if (sk == Skills::Types::S_OBSERVATION)
    {
        return GetAttribute("observation");
    }
    if (sk == Skills::Types::S_ENTERTAINMENT)
    {
        return GetAttribute("entertainment");
    }

    return GetAvailSkill(sk);
}

void Unit::SetSkill(const Skills& sk, size_t level)
{
    skills.SetDays(sk, GetDaysByLevel(level) * GetMen());
    skills.SetExp(sk, 0);
}

size_t Unit::GetAvailSkill(const Skills& sk)
{
    AString str;
    size_t retval = GetRealSkill(sk);

    for(const auto& i: items)
    {
        if (ItemDefs[i->type].flags & ItemType::DISABLED)
        {
            continue;
        }
        if (ItemDefs[i->type].type & IT_MAGEONLY
                && type != U_MAGE
                && type != U_APPRENTICE
                && type != U_GUARDMAGE)
        {
            continue;
        }
        if ((SkillDefs[sk].flags & SkillType::MAGIC)
                && type != U_MAGE
                && type != U_APPRENTICE
                && type != U_GUARDMAGE)
        {
            continue;
        }
        if (i->num < GetMen())
        {
            continue;
        }
        str = ItemDefs[i->type].grantSkill;
        if (ItemDefs[i->type].grantSkill && LookupSkill(str) == sk)
        {
            size_t grant = 0;
            for (const auto& skill_str: ItemDefs[i->type].fromSkills) {
                if (skill_str) {
                    str = skill_str;

                    const Skills fromSkill = LookupSkill(str);
                    if (fromSkill.isValid()) {
                        /*
                            Should this use GetRealSkill or GetAvailSkill?
                            GetAvailSkill could cause unbounded recursion,
                            but only if the GM sets up items stupidly...
                        */
                        if (grant < GetRealSkill(fromSkill))
                        {
                            grant = GetRealSkill(fromSkill);
                        }
                    }
                }
            }
            if (grant < ItemDefs[i->type].minGrant)
            {
                grant = ItemDefs[i->type].minGrant;
            }
            if (grant > ItemDefs[i->type].maxGrant)
            {
                grant = ItemDefs[i->type].maxGrant;
            }
            
            if (grant > retval)
            {
                retval = grant;
            }
        }
    }

    return retval;
}

size_t Unit::GetRealSkill(const Skills& sk)
{
    if (GetMen()) {
        return GetLevelByDays(skills.GetDays(sk)/GetMen());
    } else {
        return 0;
    }
}

void Unit::ForgetSkill(const Skills& sk)
{
    skills.SetDays(sk, 0);
    if (type == U_MAGE) {
        for(const auto& s: skills) {
            if (SkillDefs[s.type].flags & SkillType::MAGIC)
            {
                return;
            }
        }
        type = U_NORMAL;
    }
    if (type == U_APPRENTICE) {
        for(const auto& s: skills) {
            if (SkillDefs[s.type].flags & SkillType::APPRENTICE)
            {
                return;
            }
        }
        type = U_NORMAL;
    }
}

bool Unit::CheckDepend(size_t lev, const SkillDepend &dep)
{
    AString skname = dep.skill;
    Skills sk = LookupSkill(skname);
    if (!sk.isValid())
    {
        return false;
    }
    size_t temp = GetRealSkill(sk);
    if (temp < dep.level)
    {
        return false;
    }
    if (lev >= temp)
    {
        return false;
    }
    return true;
}

bool Unit::CanStudy(const Skills& sk)
{
    if (skills.GetStudyRate(sk, GetMen()) < 1)
    {
        return false;
    }

    if (Globals->SKILL_LIMIT_NONLEADERS &&
        IsNormal() &&
        skills.GetDays(sk) < 1 &&
        !skills.empty())
    {
        if (!Globals->MAGE_NONLEADERS || !(SkillDefs[sk].flags & SkillType::MAGIC))
        {
            return false;
        }
    }
    
    size_t curlev = GetRealSkill(sk);

    if (SkillDefs[sk].flags & SkillType::DISABLED)
    {
        return false;
    }

    for (const auto& dep: SkillDefs[sk].depends) {
        if (!dep.skill)
        {
            return true;
        }
        try
        {
            const SkillType& pS = FindSkill(dep.skill);
            if (pS.flags & SkillType::DISABLED)
            {
                continue;
            }
        }
        catch(const NoSuchItemException&)
        {
        }
        if (!CheckDepend(curlev, dep))
        {
            return false;
        }
    }
    return true;
}

bool Unit::Study(const Skills& sk, int days)
{
    if (Globals->SKILL_LIMIT_NONLEADERS && !IsLeader()) {
        if (SkillDefs[sk].flags & SkillType::MAGIC) {
            for(const auto& s: skills) {
                if (!(SkillDefs[s.type].flags & SkillType::MAGIC)) {
                    Error("STUDY: Non-leader mages cannot possess non-magical skills.");
                    return false;
                }
            }
        } else if (!skills.empty()) {
            const auto& s = skills.front();
            if ((s.type != sk) && (s.days > 0)) {
                Error("STUDY: Can know only 1 skill.");
                return false;
            }
        }
    }
    const unsigned int max = GetSkillMax(sk);
    if (GetRealSkill(sk) >= max) {
        Error("STUDY: Maximum level for skill reached.");
        return false;
    }

    if (!CanStudy(sk)) {
        if (GetRealSkill(sk) > 0)
            Error("STUDY: Doesn't have the pre-requisite skills to study that.");
        else
            Error("STUDY: Can't study that.");
        return false;
    }

    skills.SetDays(sk, skills.GetDays(sk) + static_cast<size_t>(days));
    AdjustSkills();

    /* Check to see if we need to show a skill report */
    const size_t lvl = GetRealSkill(sk);

    const auto faction_s = faction.lock();
    size_t shown = faction_s->skills.GetDays(sk);
    while (lvl > shown) {
        shown++;
        faction_s->skills.SetDays(sk, shown);
        faction_s->shows.emplace_back(std::make_shared<ShowSkill>(sk, shown));
    }
    return true;
}

unsigned int Unit::GetSkillMax(const Skills& sk)
{
    unsigned int max = 0;

    if (SkillDefs[sk].flags & SkillType::DISABLED)
    {
        return 0;
    }

    for(const auto& i: items) {
        if (ItemDefs[i->type].flags & ItemType::DISABLED)
        {
            continue;
        }
        if (!(ItemDefs[i->type].type & IT_MAN))
        {
            continue;
        }
        unsigned int m = SkillMax(SkillDefs[sk].abbr, i->type);
        if ((max == 0 && m > max) || (m < max))
        {
            max = m;
        }
    }
    return max;
}

unsigned int Unit::Practice(const Skills& sk)
{
    unsigned int bonus = Globals->SKILL_PRACTICE_AMOUNT;
    if (bonus == 0)
    {
        bonus = Globals->REQUIRED_EXPERIENCE / 8;
    }
    if (practiced || (bonus < 1))
    {
        return 1;
    }
    const size_t days = skills.GetDays(sk);
    const size_t men = GetMen();

    Skills reqsk;
    size_t reqlev = 0;

    if (GetAvailSkill(sk) > GetRealSkill(sk)) {
        // This is a skill granted by an item, so try to practice
        // the skills it depends on (if any)
        AString str;

        reqlev = 0;

        for(const auto& it: items) {
            if (ItemDefs[it->type].flags & ItemType::DISABLED)
            {
                continue;
            }
            if (ItemDefs[it->type].type & IT_MAGEONLY
                    && type != U_MAGE
                    && type != U_APPRENTICE
                    && type != U_GUARDMAGE)
            {
                continue;
            }
            if ((SkillDefs[sk].flags & SkillType::MAGIC)
                    && type != U_MAGE
                    && type != U_APPRENTICE
                    && type != U_GUARDMAGE)
            {
                continue;
            }
            if (it->num < GetMen())
            {
                continue;
            }
            str = ItemDefs[it->type].grantSkill;
            if (ItemDefs[it->type].grantSkill && LookupSkill(str) == sk)
            {
                for (const auto& fs: ItemDefs[it->type].fromSkills)
                {
                    if (fs)
                    {
                        str = fs;
                        Skills fromSkill = LookupSkill(str);
                        if (fromSkill.isValid() && GetRealSkill(fromSkill) > reqlev)
                        {
                            reqsk = fromSkill;
                            reqlev = GetRealSkill(fromSkill);
                        }
                    }
                }
            }
        }

        if (reqlev > 0) {
            // Since granting items use the highest contributing
            // skill, practice that skill.
            Practice(reqsk);
            return 1;
        }
    }

    if (men < 1 || ((days < 1) && (!Globals->REQUIRED_EXPERIENCE)))
    {
        return 0;
    }

    const size_t max = GetSkillMax(sk);
    const size_t curlev = GetRealSkill(sk);
    if (curlev >= max)
    {
        return 0;
    }

    for (const auto& d: SkillDefs[sk].depends)
    {
        AString skname = d.skill;
        reqsk = LookupSkill(skname);
        if (!reqsk.isValid())
        {
            break;
        }
        if (SkillDefs[reqsk].flags & SkillType::DISABLED)
        {
            continue;
        }
        if (SkillDefs[reqsk].flags & SkillType::NOEXP)
        {
            continue;
        }
        reqlev = GetRealSkill(reqsk);
        if (reqlev <= curlev)
        {
            if (Practice(reqsk))
            {
                return 1;
            }
            // We don't meet the reqs, and can't practice that
            // req, but we still need to check the other reqs.
            bonus = 0;
        }
    }

    if (bonus)
    {
        if (!Globals->REQUIRED_EXPERIENCE)
        {
            Study(sk, static_cast<int>(men * bonus));
        }
        else
        {
            // check if it's a nonleader and this is not it's
            // only skill
            if (Globals->SKILL_LIMIT_NONLEADERS && !IsLeader())
            {
                for(const auto& s: skills) {
                    if ((s.days > 0) && (s.type != sk))
                    {
                        return 0;
                    }
                }
            }
            // don't raise exp above the maximum days for
            // that unit
            const size_t max = men * GetDaysByLevel(GetSkillMax(sk));
            size_t exp = skills.GetExp(sk);
            exp += men * bonus;
            if (exp > max) exp = max;
            skills.SetExp(sk, exp);
        }
        practiced = true;
    }

    return bonus;
}

bool Unit::IsLeader()
{
    if (GetLeaders())
    {
        return true;
    }
    return false;
}

bool Unit::IsNormal()
{
    if (GetMen() && !IsLeader())
    {
        return true;
    }
    return false;
}

void Unit::AdjustSkills()
{
    if (!IsLeader() && Globals->SKILL_LIMIT_NONLEADERS)
    {
        //
        // Not a leader: can only know 1 skill
        //
        if (skills.size() > 1)
        {
            //
            // Find highest skill, eliminate others
            //
            size_t max = 0;
            SkillList::iterator maxskill = skills.end();
            for(auto it = skills.begin(); it != skills.end(); ++it)
            {
                if (it->days > max)
                {
                    max = it->days;
                    maxskill = it;
                }
            }

            auto it = skills.begin();
            while(it != skills.end())
            {
                if (it != maxskill)
                {
                    // Allow multiple skills if they're all
                    // magical ones
                    if ((SkillDefs[maxskill->type].flags & SkillType::MAGIC) &&
                            (SkillDefs[it->type].flags & SkillType::MAGIC) )
                    {
                        ++it;
                        continue;
                    }
                    if ((Globals->REQUIRED_EXPERIENCE) && (it->exp > 0))
                    {
                        ++it;
                        continue;
                    }
                    it = skills.erase(it);
                    continue;
                }
                ++it;
            }
        }
    }

    // Everyone: limit all skills to their maximum level
    for(auto& theskill: skills)
    {
        const unsigned int max = GetSkillMax(theskill.type);
        if (GetRealSkill(theskill.type) >= max)
        {
            theskill.days = GetDaysByLevel(max) * GetMen();
        }
    }
}

size_t Unit::MaintCost()
{
    size_t retval = 0;
    if (type == U_WMON || type == U_GUARD || type == U_GUARDMAGE)
    {
        return 0;
    }

    const size_t leaders = GetLeaders();
    const ssize_t nonleaders_s = static_cast<ssize_t>(GetMen() - leaders);
    const size_t nonleaders = nonleaders_s < 0 ? 0 : static_cast<size_t>(nonleaders_s);

    // Handle leaders
    // Leaders are counted at maintenance_multiplier * skills in all except
    // the case where it's not being used (mages, leaders, all)

    size_t i;
    if (Globals->MULTIPLIER_USE != GameDefs::MULT_NONE)
    {
        i = leaders * SkillLevels() * Globals->MAINTENANCE_MULTIPLIER;
        if (i < (leaders * Globals->LEADER_COST))
        {
            i = leaders * Globals->LEADER_COST;
        }
    }
    else
    {
        i = leaders * Globals->LEADER_COST;
    }
    retval += i;

    // Handle non-leaders
    // Non leaders are counted at maintenance_multiplier * skills only if
    // all characters pay that way.
    if (Globals->MULTIPLIER_USE == GameDefs::MULT_ALL)
    {
        i = nonleaders * SkillLevels() * Globals->MAINTENANCE_MULTIPLIER;
        if (i < (nonleaders * Globals->MAINTENANCE_COST))
        {
            i = nonleaders * Globals->MAINTENANCE_COST;
        }
    }
    else
    {
        i = nonleaders * Globals->MAINTENANCE_COST;
    }
    retval += i;

    return retval;
}

void Unit::Short(int needed, int hunger)
{
    if (faction.lock()->IsNPC())
    {
        return; // Don't starve monsters and the city guard!
    }

    if (needed < 1 && hunger < 1)
    {
        return;
    }

    switch(Globals->SKILL_STARVATION)
    {
        case GameDefs::STARVE_MAGES:
            if (type == U_MAGE)
            {
                SkillStarvation();
            }
            return;
        case GameDefs::STARVE_LEADERS:
            if (GetLeaders())
            {
                SkillStarvation();
            }
            return;
        case GameDefs::STARVE_ALL:
            SkillStarvation();
            return;
    }

    unsigned int n = 0;
    for (auto i = Items::begin(); i != Items::end(); ++i) {
        if (!(ItemDefs[*i].type & IT_MAN))
        {
            // Only men need sustenance.
            continue;
        }

        if (ItemDefs[*i].type & IT_LEADER)
        {
            // Don't starve leaders just yet.
            continue;
        }

        while (GetMen(*i))
        {
            if (getrandom(100) < Globals->STARVE_PERCENT)
            {
                SetMen(*i, GetMen(*i) - 1);
                n++;
            }
            if (Globals->MULTIPLIER_USE == GameDefs::MULT_ALL)
            {
                size_t levels = SkillLevels();
                size_t cost = levels * Globals->MAINTENANCE_MULTIPLIER;
                if (cost < Globals->MAINTENANCE_COST)
                {
                    cost = Globals->MAINTENANCE_COST;
                }
                needed -= static_cast<int>(cost);
            }
            else
            {
                needed -= static_cast<int>(Globals->MAINTENANCE_COST);
            }
            hunger -= static_cast<int>(Globals->UPKEEP_MINIMUM_FOOD);
            if (needed < 1 && hunger < 1)
            {
                if (n)
                {
                    Error(AString(n) + " starve to death.");
                }
                return;
            }
        }
    }

    // Now starve leaders
    for (auto i = Items::begin(); i != Items::end(); ++i)
    {
        if (!(ItemDefs[*i].type & IT_MAN))
        {
            // Only men need sustenance.
            continue;
        }

        if (!(ItemDefs[*i].type & IT_LEADER))
        {
            // now we're doing leaders
            continue;
        }

        while (GetMen(*i))
        {
            if (getrandom(100) < Globals->STARVE_PERCENT)
            {
                SetMen(*i, GetMen(*i) - 1);
                n++;
            }
            if (Globals->MULTIPLIER_USE != GameDefs::MULT_NONE)
            {
                size_t levels = SkillLevels();
                size_t cost = levels * Globals->MAINTENANCE_MULTIPLIER;
                if (cost < Globals->LEADER_COST)
                {
                    cost = Globals->LEADER_COST;
                }
                needed -= static_cast<int>(cost);
            }
            else
            {
                needed -= static_cast<int>(Globals->LEADER_COST);
            }
            hunger -= static_cast<int>(Globals->UPKEEP_MINIMUM_FOOD);
            if (needed < 1 && hunger < 1)
            {
                if (n)
                {
                    Error(AString(n) + " starve to death.");
                }
                return;
            }
        }
    }
}

size_t Unit::Weight()
{
    return items.Weight();
}

size_t Unit::FlyingCapacity()
{
    size_t cap = 0;
    for(const auto& i: items)
    {
        // except ship items
        if (ItemDefs[i->type].type & IT_SHIP)
        {
            continue;
        }
        cap += ItemDefs[i->type].fly * i->num;
    }

    return cap;
}

size_t Unit::RidingCapacity()
{
    size_t cap = 0;
    for(const auto& i: items)
    {
        cap += ItemDefs[i->type].ride * i->num;
    }

    return cap;
}

size_t Unit::SwimmingCapacity()
{
    size_t cap = 0;
    for(const auto& i: items)
    {
        // except ship items
        if (ItemDefs[i->type].type & IT_SHIP)
        {
            continue;
        }
        cap += ItemDefs[i->type].swim * i->num;
    }

    return cap;
}

size_t Unit::WalkingCapacity()
{
    size_t cap = 0;
    for(const auto& i: items)
    {
        cap += ItemDefs[i->type].walk * i->num;
        if (ItemDefs[i->type].hitchItem.isValid())
        {
            const auto& hitch = ItemDefs[i->type].hitchItem;
            if (!(ItemDefs[hitch].flags & ItemType::DISABLED))
            {
                const size_t hitches = items.GetNum(hitch);
                const size_t hitched = std::min(i->num, hitches);
                cap += hitched * ItemDefs[i->type].hitchwalk;
            }
        }
    }

    return cap;
}

bool Unit::CanFly(size_t weight)
{
    if (FlyingCapacity() >= weight)
    {
        return true;
    }
    return false;
}

bool Unit::CanReallySwim()
{
    if (IsAlive() && (SwimmingCapacity() >= items.Weight()))
    {
        return true;
    }
    return false;
}

bool Unit::CanSwim()
{
    if (CanReallySwim())
    {
        return true;
    }
    if ((Globals->FLIGHT_OVER_WATER != GameDefs::WFLIGHT_NONE) && CanFly())
    {
        return true;
    }
    return false;
}

bool Unit::CanFly()
{
    return CanFly(items.Weight());
}

bool Unit::CanRide(size_t weight)
{
    if (RidingCapacity() >= weight)
    {
        return true;
    }
    return false;
}

bool Unit::CanWalk(size_t weight)
{
    if (WalkingCapacity() >= weight)
    {
        return true;
    }
    return false;
}

int Unit::MoveType(ARegion::Handle r)
{
    if (!r)
    {
        r = object.lock()->region.lock();
    }

    size_t weight = items.Weight();
    if (!weight)
    {
        return M_NONE;
    }
    if (CanFly(weight))
    {
        return M_FLY;
    }
    if (TerrainDefs[r->type].similar_type != Regions::Types::R_OCEAN)
    {
        if (CanRide(weight))
        {
            return M_RIDE;
        }
        if (CanWalk(weight))
        {
            return M_WALK;
        }
    }
    else
    {
        /* Check if we should be able to 'swim' */
        /* This should become it's own M_TYPE sometime */
        if (CanSwim())
        {
            return M_SWIM;
        }
    }
    if (r->type == Regions::Types::R_NEXUS)
    {
        return M_WALK;
    }
    return M_NONE;
}

static unsigned int ContributesToMovement(int movetype, const Items& item)
{
    switch(movetype)
    {
        case M_WALK:
            if (ItemDefs[item].walk > 0)
            {
                return ItemDefs[item].walk;
            }
            break;
        case M_RIDE:
            if (ItemDefs[item].ride > 0)
            {
                return ItemDefs[item].ride;
            }
            break;
        case M_FLY:
            if (ItemDefs[item].fly > 0)
            {
                return ItemDefs[item].fly;
            }
            break;
        case M_SWIM:
            // incomplete ship items do have a "swimming"
            // capacity given, but don't help us to swim
            if (ItemDefs[item].type & IT_SHIP)
            {
                return 0;
            }
            if (ItemDefs[item].swim > 0)
            {
                return ItemDefs[item].swim;
            }
            break;
    }
    
    return 0;
}

unsigned int Unit::CalcMovePoints(const ARegion::Handle& r)
{
    int movetype = MoveType(r);
    if (movetype == M_NONE)
    {
        return 0;
    }

    unsigned int speed = 0;
    for(const auto& i: items) {
        if (ContributesToMovement(movetype, i->type))
        {
            if (ItemDefs[i->type].speed > speed)
            {
                speed = ItemDefs[i->type].speed;
            }
        }
    }

    ssize_t weight = static_cast<ssize_t>(items.Weight());
    while (weight > 0 && speed > 0)
    {
        for(const auto& i: items)
        {
            const unsigned int cap = ContributesToMovement(movetype, i->type);
            if (ItemDefs[i->type].speed == speed)
            {
                if (cap > 0)
                {
                    weight -= static_cast<ssize_t>(cap * i->num);
                }
                else if (ItemDefs[i->type].hitchItem.isValid())
                {
                    const size_t hitches = std::min(i->num, items.GetNum(ItemDefs[i->type].hitchItem));
                    weight -= static_cast<ssize_t>(hitches * ItemDefs[i->type].hitchwalk);
                }
            }
        }
        if (weight > 0) {
            // Hm, can't move at max speed.  There must be
            // items with different speeds, and we have to
            // use some of the slower ones...
            speed--;
        }
    }

    if (weight > 0)
    {
        return 0; // not that this should be possible!
    }

    if (movetype == M_FLY)
    {
        if (GetAttribute("wind") > 0)
        {
            speed += Globals->FLEET_WIND_BOOST;
        }
    }

    if (speed > Globals->MAX_SPEED)
    {
        speed = Globals->MAX_SPEED;
    }

    return speed;
}

bool Unit::CanMoveTo(const ARegion& r1, const ARegion& r2)
{
    if (&r1 == &r2)
    {
        return true;
    }

    bool exit = true;

    Directions dir;
    for (auto i = Directions::begin(); i != Directions::end(); ++i)
    {
        if (r1.neighbors[*i].lock().get() == &r2)
        {
            exit = false;
            dir = *i;
            break;
        }
    }
    if (exit)
    {
        return false;
    }

    exit = true;
    for (auto i = Directions::begin(); i != Directions::end(); ++i)
    {
        if (r2.neighbors[*i].lock().get() == &r1)
        {
            exit = false;
            break;
        }
    }
    if (exit)
    {
        return false;
    }

    int mt = MoveType();
    if (((TerrainDefs[r1.type].similar_type == Regions::Types::R_OCEAN) ||
                (TerrainDefs[r2.type].similar_type == Regions::Types::R_OCEAN)) &&
            (!CanSwim() || GetFlag(FLAG_NOCROSS_WATER)))
    {
        return false;
    }
    int mp = static_cast<int>(CalcMovePoints() - moved);
    if (mp < static_cast<int>(r2.MoveCost(mt, r1, dir, 0)))
    {
        return false;
    }
    return true;
}

bool Unit::CanCatch(const ARegion& r, const Unit::Handle& u)
{
    return faction.lock()->CanCatch(r, u);
}

unsigned int Unit::CanSee(const ARegion& r, const Unit::Handle& u, int practice)
{
    return faction.lock()->CanSee(r, u, practice);
}

int Unit::AmtsPreventCrime(const Unit::Handle& u)
{
    if (!u)
    {
        return 0;
    }

    const size_t amulets = items.GetNum(Items::Types::I_AMULETOFTS);
    if ((u->items.GetNum(Items::Types::I_RINGOFI) < 1) || (amulets < 1))
    {
        return 0;
    }
    const size_t men = GetMen();
    if (men <= amulets)
    {
        return 1;
    }
    if (!Globals->PROPORTIONAL_AMTS_USAGE)
    {
        return 0;
    }
    if (getrandom(men) < amulets)
    {
        return 1;
    }
    return 0;
}

int Unit::GetAttitude(const ARegion& r, const Unit::Handle& u)
{
    const auto faction_s = faction.lock();
    const auto ufac = u->faction.lock();
    if (faction_s == ufac)
    {
        return A_ALLY;
    }
    int att = faction_s->GetAttitude(ufac->num);
    if (att >= A_FRIENDLY && att >= faction_s->defaultattitude)
    {
        return att;
    }

    if (CanSee(r, u) == 2)
    {
        return att;
    }
    else
    {
        return faction_s->defaultattitude;
    }
}

int Unit::Hostile()
{
    if (type != U_WMON)
    {
        return 0;
    }
    int retval = 0;
    for(const auto& i: items)
    {
        if (ItemDefs[i->type].type & IT_MONSTER)
        {
            const auto& mp = FindMonster(ItemDefs[i->type].abr, (ItemDefs[i->type].type & IT_ILLUSION));
            int hos = mp.hostile;
            if (hos > retval)
            {
                retval = hos;
            }
        }
    }
    return retval;
}

bool Unit::Forbids(const ARegion& r, const Unit::Handle& u)
{
    if (guard != GUARD_GUARD)
    {
        return false;
    }
    if (!IsAlive())
    {
        return false;
    }
    if (!CanSee(r, u, Globals->SKILL_PRACTICE_AMOUNT > 0))
    {
        return false;
    }
    if (!CanCatch(r, u))
    {
        return false;
    }
    if (GetAttitude(r, u) < A_NEUTRAL)
    {
        return true;
    }
    return false;
}

/* This function was modified to either return the amount of
   taxes this unit is eligible for (numtaxers == 0) or the
   number of taxing men (numtaxers > 0).
*/
size_t Unit::Taxers(unsigned int numtaxers)
{
    const size_t totalMen = GetMen();
    size_t illusions = 0;
    size_t creatures = 0;
    size_t taxers = 0;
    size_t basetax = 0;
    size_t weapontax = 0;
    size_t armortax = 0;
    
    // check out items
    size_t numMelee= 0;
    size_t numUsableMelee = 0;
    size_t numBows = 0;
    size_t numUsableBows = 0;
    size_t numMounted= 0;
    size_t numUsableMounted = 0;
    size_t numMounts = 0;
    size_t numUsableMounts = 0;
    size_t numBattle = 0;
    size_t numUsableBattle = 0;
    size_t numArmor = 0;

    for(const auto& pItem: items)
    {
        if (ItemDefs[pItem->type].type & IT_MAN)
        {
            continue;
        }

        if ((ItemDefs[pItem->type].type & IT_BATTLE))
        {
            try
            {
                const auto& pBat = FindBattleItem(ItemDefs[pItem->type].abr);
                if(pBat.flags & BattleItemType::SPECIAL)
                {
                    // Only consider offensive items
                    if ((Globals->WHO_CAN_TAX & GameDefs::TAX_USABLE_BATTLE_ITEM) &&
                    (!(pBat.flags & BattleItemType::MAGEONLY) ||
                     type == U_MAGE || type == U_APPRENTICE))
                    {
                        numUsableBattle += pItem->num;
                        numBattle += pItem->num;
                        continue; // Don't count this as a weapon as well!
                    }
                    if (Globals->WHO_CAN_TAX & GameDefs::TAX_BATTLE_ITEM)
                    {
                        numBattle += pItem->num;
                        continue; // Don't count this as a weapon as well!
                    }
                }
            }
            catch(const NoSuchItemException&)
            {
            }

        }
        if (ItemDefs[pItem->type].type & IT_WEAPON)
        {
            const auto& pWep = FindWeapon(ItemDefs[pItem->type].abr);
            size_t num = pItem->num;
            size_t basesk = 0;
            AString skname = pWep.baseSkill;
            Skills sk = LookupSkill(skname);
            if (sk.isValid())
            {
                basesk = GetSkill(sk);
            }

            if (basesk == 0)
            {
                skname = pWep.orSkill;
                sk = LookupSkill(skname);
                if (sk.isValid())
                {
                    basesk = GetSkill(sk);
                }
            }

            if (!(pWep.flags & WeaponType::NEEDSKILL))
            {
                if (basesk)
                {
                    numUsableMelee += num;
                }
                numMelee += num;
            }
            else if (pWep.flags & WeaponType::NOFOOT)
            {
                if (basesk)
                {
                    numUsableMounted += num;
                }
                numMounted += num;
            }
            else
            {
                if (pWep.flags & WeaponType::RANGED)
                {
                    if (basesk)
                    {
                        numUsableBows += num;
                    }
                    numBows += num;
                }
                else
                {
                    if (basesk)
                    {
                        numUsableMelee += num;
                    }
                    numMelee += num;
                }
            }
        }

        if (ItemDefs[pItem->type].type & IT_MOUNT)
        {
            const auto& pm = FindMount(ItemDefs[pItem->type].abr);
            if (pm.skill)
            {
                const AString& skname = pm.skill;
                Skills sk = LookupSkill(skname);
                if (pm.minBonus <= static_cast<int>(GetSkill(sk)))
                {
                    numUsableMounts += pItem->num;
                }
            }
            else
            {
                numUsableMounts += pItem->num;
            }
            numMounts += pItem->num;
        }

        if (ItemDefs[pItem->type].type & IT_MONSTER)
        {
            if (ItemDefs[pItem->type].type & IT_ILLUSION)
            {
                illusions += pItem->num;
            }
            else
            {
                creatures += pItem->num;
            }
        }
        
        if (ItemDefs[pItem->type].type & IT_ARMOR) {
            numArmor += pItem->num;
        }
    }


    // Ok, now process the counts!
    if ((Globals->WHO_CAN_TAX & GameDefs::TAX_ANYONE) ||
        ((Globals->WHO_CAN_TAX & GameDefs::TAX_COMBAT_SKILL) &&
         GetSkill(Skills::Types::S_COMBAT)) ||
        ((Globals->WHO_CAN_TAX & GameDefs::TAX_BOW_SKILL) &&
         (GetSkill(Skills::Types::S_CROSSBOW) || GetSkill(Skills::Types::S_LONGBOW))) ||
        ((Globals->WHO_CAN_TAX & GameDefs::TAX_RIDING_SKILL) &&
         GetSkill(Skills::Types::S_RIDING)) ||
        ((Globals->WHO_CAN_TAX & GameDefs::TAX_STEALTH_SKILL) &&
         GetSkill(Skills::Types::S_STEALTH)))
    {
        basetax = totalMen;
        taxers = totalMen;
        
        // Weapon tax bonus
        if ((Globals->WHO_CAN_TAX & GameDefs::TAX_ANYONE) ||
        ((Globals->WHO_CAN_TAX & GameDefs::TAX_COMBAT_SKILL) &&
         GetSkill(Skills::Types::S_COMBAT)) ||
        ((Globals->WHO_CAN_TAX & GameDefs::TAX_STEALTH_SKILL) &&
         GetSkill(Skills::Types::S_STEALTH)))
        {
             if (numUsableMounted > numUsableMounts)
             {
                 weapontax = numUsableMounts;
             }
             else
             {
                 weapontax = numUsableMounted;
             }
             weapontax += numMelee;
         }
         
        if (((Globals->WHO_CAN_TAX & GameDefs::TAX_BOW_SKILL) &&
         (GetSkill(Skills::Types::S_CROSSBOW) || GetSkill(Skills::Types::S_LONGBOW))))
        {
             weapontax += numUsableBows;
        }
        if ((Globals->WHO_CAN_TAX & GameDefs::TAX_RIDING_SKILL) &&
         GetSkill(Skills::Types::S_RIDING))
        {
             if (weapontax < numUsableMounts)
             {
                 weapontax = numUsableMounts;
             }
        }
        
    }
    else
    {
        if (Globals->WHO_CAN_TAX & GameDefs::TAX_USABLE_WEAPON)
        {
            if (numUsableMounted > numUsableMounts)
            {
                weapontax = numUsableMounts;
                taxers = numUsableMounts;
                numMounts -= numUsableMounts;
                numUsableMounts = 0;
            }
            else
            {
                weapontax = numUsableMounted;
                taxers = numUsableMounted;
                numMounts -= numUsableMounted;
                numUsableMounts -= numUsableMounted;
            }
            weapontax += numMelee + numUsableBows;
            taxers += numMelee + numUsableBows;
        }
        else if (Globals->WHO_CAN_TAX & GameDefs::TAX_ANY_WEAPON)
        {
            weapontax = numMelee + numBows + numMounted;
            taxers = numMelee + numBows + numMounted;
        }
        else
        {
            if (Globals->WHO_CAN_TAX &
                    GameDefs::TAX_MELEE_WEAPON_AND_MATCHING_SKILL)
            {
                if (numUsableMounted > numUsableMounts)
                {
                    weapontax += numUsableMounts;
                    taxers += numUsableMounts;
                    numMounts -= numUsableMounts;
                    numUsableMounts = 0;
                }
                else
                {
                    weapontax += numUsableMounted;
                    taxers += numUsableMounted;
                    numMounts -= numUsableMounted;
                    numUsableMounts -= numUsableMounted;
                }
                weapontax += numUsableMelee;
                taxers += numUsableMelee;
            }
            if (Globals->WHO_CAN_TAX &
                    GameDefs::TAX_BOW_SKILL_AND_MATCHING_WEAPON)
            {
                weapontax += numUsableBows;
                taxers += numUsableBows;
            }
        }

        if (Globals->WHO_CAN_TAX & GameDefs::TAX_HORSE)
        {
            weapontax += numMounts;
            taxers += numMounts;
        }
        else if (Globals->WHO_CAN_TAX & GameDefs::TAX_HORSE_AND_RIDING_SKILL)
        {
            weapontax += numMounts;
            taxers += numUsableMounts;
        }

        if (Globals->WHO_CAN_TAX & GameDefs::TAX_BATTLE_ITEM)
        {
            weapontax += numBattle;
            taxers += numBattle;
        }
        else if (Globals->WHO_CAN_TAX & GameDefs::TAX_USABLE_BATTLE_ITEM)
        {
            weapontax += numUsableBattle;
            taxers += numUsableBattle;
        }
        
    }

    // Ok, all the items categories done - check for mages taxing
    if (type == U_MAGE)
    {
        if (Globals->WHO_CAN_TAX & GameDefs::TAX_ANY_MAGE)
        {
            basetax = totalMen;
            taxers = totalMen;
        }
        else
        {
            if (Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_COMBAT_SPELL)
            {
                if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_DAMAGE) &&
                        SkillDefs[combat].flags & SkillType::DAMAGE)
                {
                    basetax = totalMen;
                    taxers = totalMen;
                }

                if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_FEAR) &&
                        SkillDefs[combat].flags & SkillType::FEAR)
                {
                    basetax = totalMen;
                    taxers = totalMen;
                }

                if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_OTHER) &&
                        SkillDefs[combat].flags & SkillType::MAGEOTHER)
                {
                    basetax = totalMen;
                    taxers = totalMen;
                }
            }
            else
            {
                for(const auto& s: skills)
                {
                    if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_DAMAGE) &&
                            SkillDefs[s.type].flags & SkillType::DAMAGE)
                    {
                        basetax = totalMen;
                        taxers = totalMen;
                        break;
                    }
                    if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_FEAR) &&
                            SkillDefs[s.type].flags & SkillType::FEAR)
                    {
                        basetax = totalMen;
                        taxers = totalMen;
                        break;
                    }
                    if ((Globals->WHO_CAN_TAX & GameDefs::TAX_MAGE_OTHER) &&
                            SkillDefs[s.type].flags & SkillType::MAGEOTHER)
                    {
                        basetax = totalMen;
                        taxers = totalMen;
                        break;
                    }
                }
            }
        }
    }
    
    armortax = numArmor;
    
    // Check for overabundance
    if (weapontax > totalMen)
    {
        weapontax = totalMen;
    }
    if (armortax > weapontax)
    {
        armortax = weapontax;
    }
    
    // Adjust basetax in case of weapon taxation
    if (basetax < weapontax)
    {
        basetax = weapontax;
    }

    // Now check for an overabundance of tax enabling objects
    if (taxers > totalMen)
    {
        taxers = totalMen;
    }

    // And finally for creatures
    if (Globals->WHO_CAN_TAX & GameDefs::TAX_CREATURES)
    {
        basetax += creatures;
        taxers += creatures;
    }
    if (Globals->WHO_CAN_TAX & GameDefs::TAX_ILLUSIONS)
    {
        basetax += illusions;
        taxers += illusions;
    }
    if (numtaxers)
    {
        return taxers;
    }

    size_t taxes = Globals->TAX_BASE_INCOME * basetax
        + Globals->TAX_BONUS_WEAPON * weapontax
        + Globals->TAX_BONUS_ARMOR * armortax;

    return taxes;
}

int Unit::GetFlag(int x)
{
    return (flags & x);
}

void Unit::SetFlag(int x, int val)
{
    if (val)
        flags = flags | x;
    else
        if (flags & x) flags -= x;
}

void Unit::CopyFlags(const Unit::Handle& x)
{
    flags = x->flags;
    guard = GUARD_NONE;
    if (Taxers(1))
    {
        if (x->guard != GUARD_SET && x->guard != GUARD_ADVANCE)
        {
            guard = x->guard;
        }
    }
    else
    {
        if (x->guard == GUARD_AVOID)
        {
            guard = GUARD_AVOID;
        }
        SetFlag(FLAG_AUTOTAX, 0);
    }
    reveal = x->reveal;
}

Items Unit::GetBattleItem(const AString &itm)
{
    const Items item = LookupItem(itm);
    if (!item.isValid())
    {
        return Items();
    }

    size_t num = items.GetNum(item);
    if (num < 1)
    {
        return Items();
    }

    if (!(ItemDefs[item].type & IT_BATTLE))
    {
        return Items();
    }
    // Exclude weapons.  They will be handled later.
    if (ItemDefs[item].type & IT_WEAPON)
    {
        return Items();
    }
    items.SetNum(item, num - 1);
    return item;
}

Items Unit::GetArmor(const AString &itm, bool ass)
{
    Items item = LookupItem(itm);

    try
    {
        const ArmorType& pa = FindArmor(itm.Str());

        if (ass && !(pa.flags & ArmorType::USEINASSASSINATE))
        {
            return Items();
        }

        const size_t num = items.GetNum(item);
        if (num < 1)
        {
            return Items();
        }

        if (!(ItemDefs[item].type & IT_ARMOR))
        {
            return Items();
        }
        items.SetNum(item, num - 1);
        return item;
    }
    catch(const NoSuchItemException&)
    {
        return Items();
    }
}

Items Unit::GetMount(const AString& itm, bool canFly, bool canRide, unsigned int &bonus)
{
    bonus = 0;

    // This region doesn't allow riding or flying, so no mounts, bail
    if (!canFly && !canRide)
    {
        return Items();
    }

    Items item = LookupItem(itm);

    const MountType& pMnt = FindMount(itm.Str());

    const size_t num = items.GetNum(item);
    if (num < 1)
    {
        return Items();
    }

    if (canFly)
    {
        // If the mount cannot fly, and the region doesn't allow
        // riding mounts, bail
        if (!ItemDefs[item].fly && !canRide)
        {
            return Items();
        }
    }
    else
    {
        // This region allows riding mounts, so if the mount
        // can not carry at a riding level, bail
        if (!ItemDefs[item].ride)
        {
            return Items();
        }
    }

    if (pMnt.skill)
    {
        const AString& skname = pMnt.skill;
        Skills sk = LookupSkill(skname);
        bonus = static_cast<unsigned int>(GetSkill(sk));
        if (static_cast<int>(bonus) < pMnt.minBonus)
        {
            // Unit isn't skilled enough for this mount
            bonus = 0;
            return Items();
        }
        // Limit to max mount bonus;
        if (static_cast<int>(bonus) > pMnt.maxBonus)
        {
            bonus = static_cast<unsigned int>(pMnt.maxBonus);
        }
        // If the mount can fly and the terrain doesn't allow
        // flying mounts, limit the bonus to the maximum hampered
        // bonus allowed by the mount
        if (ItemDefs[item].fly && !canFly)
        {
            if (static_cast<int>(bonus) > pMnt.maxHamperedBonus)
            {
                bonus = static_cast<unsigned int>(pMnt.maxHamperedBonus);
            }
        }

        // Practice the mount's skill
        Practice(sk);
    }

    // Get the mount
    items.SetNum(item, num - 1);
    return item;
}

Items Unit::GetWeapon(const AString& itm, const Items& riding, unsigned int ridingBonus,
        unsigned int &attackBonus, unsigned int &defenseBonus, int &attacks)
{
    Items item = LookupItem(itm);

    try
    {
        const auto& pWep = FindWeapon(itm.Str());

        const size_t num = items.GetNum(item);
        if (num < 1)
        {
            return Items();
        }

        if (!(ItemDefs[item].type & IT_WEAPON))
        {
            return Items();
        }

        attackBonus = 0;
        defenseBonus = 0;
        attacks = 1;

        // Found a weapon, check flags and skills
        int baseSkillLevel = CanUseWeapon(pWep, riding);
        // returns -1 if weapon cannot be used, else the usable skill level
        if (baseSkillLevel == -1)
        {
            return Items();
        }

        // Attack and defense skill
        attackBonus = static_cast<unsigned int>(baseSkillLevel + pWep.attackBonus);
        if (pWep.flags & WeaponType::NOATTACKERSKILL)
        {
            defenseBonus = static_cast<unsigned int>(pWep.defenseBonus);
        }
        else
        {
            defenseBonus = static_cast<unsigned int>(baseSkillLevel + pWep.defenseBonus);
        }
        // Riding bonus
        if (pWep.flags & WeaponType::RIDINGBONUS)
        {
            attackBonus += ridingBonus;
        }
        if (pWep.flags & (WeaponType::RIDINGBONUSDEFENSE|WeaponType::RIDINGBONUS))
        {
            defenseBonus += ridingBonus;
        }
        // Number of attacks
        attacks = pWep.numAttacks;
        // Note: NUM_ATTACKS_SKILL must be > NUM_ATTACKS_HALF_SKILL
        if (attacks >= WeaponType::NUM_ATTACKS_SKILL)
        {
            attacks += baseSkillLevel - WeaponType::NUM_ATTACKS_SKILL;
        }
        else if (attacks >= WeaponType::NUM_ATTACKS_HALF_SKILL)
        {
            attacks += (baseSkillLevel +1)/2 - WeaponType::NUM_ATTACKS_HALF_SKILL;
        }
        // Sanity check
        if (attacks == 0)
        {
            attacks = 1;
        }

        // get the weapon
        items.SetNum(item, num-1);
        return item;
    }
    catch(const NoSuchItemException&)
    {
        return Items();
    }
}

void Unit::Detach()
{
    if (!object.expired())
    {
        auto& ulist = object.lock()->units;
        for(auto it = ulist.begin(); it != ulist.end(); ++it)
        {
            const auto& u = *it;
            if(u.get() == this)
            {
                ulist.erase(it);
                break;
            }
        }
    }
    object.reset();
}

void Unit::MoveUnit(const std::weak_ptr<Object>& toobj)
{
    if (!object.expired())
    {
        auto& ulist = object.lock()->units;
        for(auto it = ulist.begin(); it != ulist.end(); ++it)
        {
            const auto& u = *it;
            if(u.get() == this)
            {
                ulist.erase(it);
                break;
            }
        }
    }
    object = toobj;
    if (!object.expired()) {
        object.lock()->units.push_back(shared_from_this());
    }
}

void Unit::DiscardUnfinishedShips() {
    bool discard = false;
    // remove all unfinished ship-type items
    for (auto i = Items::begin(); i != Items::end(); ++i)
    {
        if (ItemDefs[*i].type & IT_SHIP)
        {
            if (items.GetNum(*i) > 0)
            {
                discard = true;
            }
            items.SetNum(*i, 0);
        }
    }
    if (discard)
    {
        Event("discards all unfinished ships.");    
    }
}

void Unit::Event(const AString & s)
{
    AString temp = *name + ": " + s;
    faction.lock()->Event(temp);
}

void Unit::Error(const AString & s)
{
    AString temp = *name + ": " + s;
    faction.lock()->Error(temp);
}

unsigned int Unit::GetAttribute(char const *attrib)
{
    try
    {
        const AttribModType& ap = FindAttrib(attrib);
        unsigned int base = 0;
        unsigned int bonus = 0;
        ValidValue<unsigned int> monbase;
        unsigned int monbonus = 0;

        if (ap.flags & AttribModType::CHECK_MONSTERS)
        {
            for(const auto& i: items)
            {
                if (ItemDefs[i->type].type & IT_MONSTER)
                {
                    const MonType& mp = FindMonster(ItemDefs[i->type].abr,
                            (ItemDefs[i->type].type & IT_ILLUSION));
                    int val = 0;
                    const std::string temp = attrib;
                    if (temp == "observation")
                    {
                        val = mp.obs;
                    }
                    else if (temp == "stealth")
                    {
                        val = mp.stealth;
                    }
                    else if (temp == "tactics")
                    {
                        val = mp.tactics;
                    }
                    else
                    {
                        continue;
                    }
                    if (!monbase.isValid())
                    {
                        if(val >= 0)
                        {
                            monbase = static_cast<unsigned int>(val);
                        }
                    }
                    else if (ap.flags & AttribModType::USE_WORST)
                    {
                        if(val < 0)
                        {
                            monbase.invalidate();
                        }
                        else
                        {
                            monbase = std::min(static_cast<unsigned int>(val),
                                               static_cast<unsigned int>(monbase));
                        }
                    }
                    else
                    {
                        if(val >= 0)
                        {
                            monbase = std::max(static_cast<unsigned int>(val),
                                               static_cast<unsigned int>(monbase));
                        }
                    }
                }
            }
        }

        for (const auto& mod: ap.mods)
        {
            unsigned int val = 0;
            if (mod.flags & AttribModItem::SKILL)
            {
                const auto& temp = mod.ident;
                const Skills sk = LookupSkill(temp);
                val = static_cast<unsigned int>(GetAvailSkill(sk));
                if (mod.modtype == AttribModItem::UNIT_LEVEL_HALF)
                {
                    val = ((val + 1)/2) * mod.val;
                }
                else if (mod.modtype == AttribModItem::CONSTANT)
                {
                    val = mod.val;
                }
                else
                {
                    val *= mod.val;
                }
            }
            else if (mod.flags & AttribModItem::ITEM)
            {
                val = 0;
                const auto& temp = mod.ident;
                const Items item = LookupItem(temp);
                if (item.isValid())
                {
                    if (ItemDefs[item].type & IT_MAGEONLY
                        && type != U_MAGE
                        && type != U_APPRENTICE
                        && type != U_GUARDMAGE)
                    {
                        // Ignore mage only items for non-mages
                    }
                    else if (mod.flags & AttribModItem::PERMAN)
                    {
                        const size_t men = GetMen();
                        if (men <= items.GetNum(item))
                        {
                            val = mod.val;
                        }
                    }
                    else
                    {
                        if (items.GetNum(item) > 0)
                        {
                            val = mod.val;
                        }
                    }
                }
            }
            else if (mod.flags & AttribModItem::FLAGGED)
            {
                const std::string temp = mod.ident;
                if (temp == "invis")
                {
                    val = (GetFlag(FLAG_INVIS) ? mod.val : 0);
                }
                if (temp == "guard")
                {
                    val = (guard == GUARD_GUARD ? mod.val : 0);
                }

            }
            if (mod.flags & AttribModItem::NOT)
            {
                val = ((val == 0) ? mod.val : 0);
            }
            if (val && mod.modtype == AttribModItem::FORCECONSTANT)
            {
                return val;
            }
            // Only flags can add to monster bonuses
            if (mod.flags & AttribModItem::FLAGGED)
            {
                if (ap.flags & AttribModType::CHECK_MONSTERS)
                {
                    monbonus += val;
                }
            }
            if (mod.flags & AttribModItem::CUMULATIVE)
            {
                base += val;
            }
            else if (val > bonus)
            {
                bonus = val;
            }
        }

        base += bonus;

        if (monbase.isValid())
        {
            monbase += monbonus;
            if (GetMen() > 0)
            {
                if (ap.flags & AttribModType::USE_WORST)
                {
                    base = (monbase < base) ? monbase : base;
                }
                else
                {
                    base = (monbase > base) ? monbase : base;
                }
            }
            else
            {
                base = monbase; // monster units have no men
            }
        }    
        return base;
    }
    catch(const NoSuchItemException&)
    {
        return 0;
    }
}

bool Unit::PracticeAttribute(char const *attrib)
{
    try
    {
        const AttribModType& ap = FindAttrib(attrib);
        for (const auto& mod: ap.mods)
        {
            if (mod.flags & AttribModItem::SKILL)
            {
                const auto& temp = mod.ident;
                const Skills sk = LookupSkill(temp);
                if (sk.isValid())
                {
                    if (Practice(sk))
                    {
                        return 1;
                    }
                }
            }
        }
        return 0;
    }
    catch(const NoSuchItemException&)
    {
        return 0;
    }
}

int Unit::GetProductionBonus(const Items& item)
{
    size_t bonus = 0;
    if (ItemDefs[item].mult_item.isValid())
    {
        bonus = items.GetNum(ItemDefs[item].mult_item);
    }
    else
    {
        bonus = GetMen();
    }
    if (bonus > GetMen())
    {
        bonus = GetMen();
    }
    return static_cast<int>(bonus) * ItemDefs[item].mult_val;
}

size_t Unit::SkillLevels()
{
    size_t levels = 0;
    for(const auto& s: skills)
    {
        levels += GetLevelByDays(s.days/GetMen());
    }
    return levels;
}

const Skill& Unit::GetSkillObject(const Skills& sk) const
{
    for(const auto& s: skills)
    {
        if (!s.type.isValid() && !sk.isValid())
        {
            return s;
        }
        if (s.type.isValid() && sk.isValid() && s.type == sk)
        {
            return s;
        }
    }

    throw NoSuchItemException();
}

Skill& Unit::GetSkillObject(const Skills& sk)
{
    for(auto& s: skills)
    {
        if (!s.type.isValid() && !sk.isValid())
        {
            return s;
        }
        if (s.type.isValid() && sk.isValid() && s.type == sk)
        {
            return s;
        }
    }

    throw NoSuchItemException();
}

void Unit::SkillStarvation()
{
    std::array<bool, Skills::size()> can_forget = {false};
    int count = 0;
    for (auto i = Skills::begin(); i != Skills::end(); ++i)
    {
        if (SkillDefs[*i].flags & SkillType::DISABLED)
        {
            can_forget[*i] = false;
            continue;
        }
        try
        {
            GetSkillObject(*i);
            can_forget[*i] = true;
            count++;
        }
        catch(const NoSuchItemException&)
        {
            can_forget[*i] = false;
        }
    }
    for (auto i = Skills::begin(); i != Skills::end(); ++i)
    {
        if (!can_forget[*i])
        {
            continue;
        }
        const Skill& si = GetSkillObject(*i);
        for (auto j = Skills::begin(); j  != Skills::end(); ++j) {
            if (SkillDefs[*j].flags & SkillType::DISABLED)
            {
                continue;
            }
            const Skill& sj = GetSkillObject(*j);
            unsigned int dependancy_level = 0;
            for (const auto& dep: SkillDefs[*i].depends)
            {
                const AString& skname = dep.skill;
                if (skname == SkillDefs[*j].abbr)
                {
                    dependancy_level = dep.level;
                    break;
                }
            }
            if (dependancy_level > 0)
            {
                if (GetLevelByDays(sj.days) == GetLevelByDays(si.days))
                {
                    can_forget[*j] = 0;
                    count--;
                }
            }
        }
    }
    if (!count)
    {
        for(const auto& i: items)
        {
            if (ItemDefs[i->type].type & IT_MAN)
            {
                count += static_cast<int>(items.GetNum(i->type));
                items.SetNum(i->type, 0);
            }
        }
        AString temp = AString(count) + " starve to death.";
        Error(temp);
        return;
    }
    count = getrandom(count) + 1;
    for (auto i = Skills::begin(); i != Skills::end(); ++i)
    {
        if (can_forget[*i])
        {
            if (--count == 0)
            {
                Skill& s = GetSkillObject(*i);
                AString temp = AString("Starves and forgets one level of ") + SkillDefs[*i].name + ".";
                Error(temp);
                switch(GetLevelByDays(s.days))
                {
                    case 1:
                        s.days -= 30;
                        if (s.days <= 0)
                        {
                            ForgetSkill(*i);
                        }
                        break;
                    case 2:
                        s.days -= 60;
                        break;
                    case 3:
                        s.days -= 90;
                        break;
                    case 4:
                        s.days -= 120;
                        break;
                    case 5:
                        s.days -= 150;
                        break;
                }
            }
        }
    }
    return;
}

int Unit::CanUseWeapon(const WeaponType& pWep, const Items& riding)
{
    if (!riding.isValid())
    {
        if (pWep.flags & WeaponType::NOFOOT)
        {
            return -1;
        }
    }
    else if (pWep.flags & WeaponType::NOMOUNT)
    {
        return -1;
    }
    return CanUseWeapon(pWep);
}

int Unit::CanUseWeapon(const WeaponType& pWep)
{
    int baseSkillLevel = 0;
    int tempSkillLevel = 0;

    Skills bsk, orsk;

    if (pWep.baseSkill != nullptr)
    {
        const auto& skname = pWep.baseSkill;
        bsk = LookupSkill(skname);
        if (bsk.isValid())
        {
            baseSkillLevel = static_cast<int>(GetSkill(bsk));
        }
    }

    if (pWep.orSkill != nullptr)
    {
        const auto& skname = pWep.orSkill;
        orsk = LookupSkill(skname);
        if (orsk.isValid())
        {
            tempSkillLevel = static_cast<int>(GetSkill(orsk));
        }
    }

    if (tempSkillLevel > baseSkillLevel)
    {
        baseSkillLevel = tempSkillLevel;
        Practice(orsk);
    }
    else
    {
        Practice(bsk);
    }

    if (pWep.flags & WeaponType::NEEDSKILL && !baseSkillLevel)
    {
        return -1;
    }

    return baseSkillLevel;
}
