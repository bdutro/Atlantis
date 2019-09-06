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
// MODIFICATIONS
// Date            Person                Comments
// ----            ------                --------
// 2000/MAR/14    Larry Stanbery        Added production enhancement.
// 2000/MAR/21    Azthar Septragen    Added roads.
// 2001/Feb/21    Joseph Traub        Added FACLIM_UNLIMITED

#include <cstdlib>

#include "game.h"
#include "gamedata.h"
#include "quests.h"

void Game::RunMovementOrders()
{
    int error;
    PtrList<Location> locs;
    AString order;

    for (size_t phase = 0; phase < Globals->MAX_SPEED; ++phase) {
        for(const auto& r: regions) {
            for(const auto& o: r->objects) {
                for(const auto& u: o->units) {
                    Object::Handle tempobj = o;
                    DoMoveEnter(u, r, tempobj);
                }
            }
        }
        for(const auto& r: regions) {
            for(const auto& o: r->objects) {
                error = 1;
                if (o->IsFleet()) {
                    const auto u_w = o->GetOwner();
                    if (u_w.expired())
                    {
                        continue;
                    }
                    const auto u = u_w.lock();
                    if (u->phase.isValid() && u->phase >= phase)
                    {
                        continue;
                    }
                    if (!u->nomove &&
                            u->monthorders &&
                            u->monthorders->type == Orders::Types::O_SAIL)  {
                        u->phase = phase;
                        if (o->incomplete < 50) {
                            const auto l = Do1SailOrder(r, o, u);
                            if (l)
                            {
                                locs.push_back(l);
                            }
                            error = 0;
                        } else
                            error = 3;
                    } else
                        error = 2;
                }
                if (error > 0) {
                    for(const auto& u: o->units) {
                        if (u && u->monthorders &&
                                u->monthorders->type == Orders::Types::O_SAIL) {
                            switch (error) {
                                case 1:
                                    u->Error("SAIL: Must be on a ship.");
                                    break;
                                case 2:
                                    u->Error("SAIL: Owner must issue fleet directions.");
                                    break;
                                case 3:
                                    u->Error("SAIL: Fleet is too damaged to sail.");
                                    break;
                            }
                            u->monthorders.reset();
                        }
                    }
                }
            }
        }
        for(const auto& r: regions) {
            for(const auto& o: r->objects) {
                for(const auto& u: o->units) {
                    if (u->phase.isValid() && u->phase >= phase)
                    {
                        continue;
                    }
                    u->phase = phase;
                    if (u && !u->nomove && u->monthorders &&
                            (u->monthorders->type == Orders::Types::O_MOVE ||
                            u->monthorders->type == Orders::Types::O_ADVANCE)) {
                        auto l = DoAMoveOrder(u, r, o);
                        if (l)
                        {
                            locs.push_back(l);
                        }
                    }
                }
            }
        }
        DoMovementAttacks(locs);
        locs.clear();
    }

    // Do a final round of Enters after the phased movement is done,
    // in case such a thing is at the end of a move chain
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                Object::Handle tempobj = o;
                DoMoveEnter(u, r, tempobj);
            }
        }
    }

    // Queue remaining moves
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                const auto mo = std::dynamic_pointer_cast<MoveOrder>(u->monthorders);
                if (!u->nomove &&
                        u->monthorders &&
                        (u->monthorders->type == Orders::Types::O_MOVE ||
                        u->monthorders->type == Orders::Types::O_ADVANCE) &&
                        !mo->dirs.empty()) {
                    const auto& d = mo->dirs.front();
                    if (u->savedmovedir != d->dir)
                        u->savedmovement = 0;
                    u->savedmovement += static_cast<int>(u->movepoints / Globals->MAX_SPEED);
                    u->savedmovedir = d->dir;
                } else {
                    u->savedmovement = 0;
                    u->savedmovedir.invalidate();
                }
                if (u->monthorders && 
                        (u->monthorders->type == Orders::Types::O_MOVE ||
                        u->monthorders->type == Orders::Types::O_ADVANCE)) {
                    if (!mo->dirs.empty()) {
                        auto& tOrder = u->oldorders.emplace_front(std::make_shared<AString>());
                        if (mo->advancing)
                            *tOrder = "ADVANCE";
                        else
                            *tOrder = "MOVE";
                        u->Event(*tOrder + ": Unit has insufficient movement points;"
                                " remaining moves queued.");
                        for(const auto& d: mo->dirs) {
                            *tOrder += " ";
                            if (d->dir.isRegularDirection())
                            {
                                *tOrder += DirectionAbrs[d->dir];
                            }
                            else if (d->dir.isMoveIn())
                            {
                                *tOrder += "IN";
                            }
                            else if (d->dir.isMoveOut())
                            {
                                *tOrder += "OUT";
                            }
                            else if (d->dir.isMovePause())
                            {
                                *tOrder += "P";
                            }
                            else
                            {
                                *tOrder += d->dir.getMoveObject();
                            }
                        }
                    }
                }
            }
            auto u_w = o->GetOwner();
            if(!u_w.expired())
            {
                auto u = u_w.lock();
                if (o->IsFleet() && !u->nomove &&
                        u->monthorders && 
                        u->monthorders->type == Orders::Types::O_SAIL) {
                    auto so = std::dynamic_pointer_cast<SailOrder>(u->monthorders);
                    if (!so->dirs.empty()) {
                        u->Event("SAIL: Can't sail that far;"
                            " remaining moves queued.");
                        auto& tOrder = u->oldorders.emplace_front(std::make_shared<AString>("SAIL"));
                        for(const auto& d: so->dirs) {
                            *tOrder += " ";
                            if (d->dir.isMovePause())
                            {
                                *tOrder += "P";
                            }
                            else if(!d->dir.isMoveInOutOrEnter())
                            {
                                *tOrder += DirectionAbrs[d->dir];
                            }
                        }
                    }
                }
            }
        }
    }
}

Location::Handle Game::Do1SailOrder(ARegion::Handle reg, const Object::Handle& fleet, const Unit::Handle& cap)
{
    const auto o = std::dynamic_pointer_cast<SailOrder>(cap->monthorders);
    WeakPtrList<Faction> facs;

    fleet->movepoints += fleet->GetFleetSpeed(0);
    bool stop = false;
    size_t wgt = 0;
    size_t slr = 0;
    bool nomove = false;

    for(const auto& unit: fleet->units) {
        if (GetFaction2(facs, unit->faction.lock()->num).expired()) {
            facs.emplace_back(unit->faction);
        }
        wgt += unit->Weight();
        if (unit->nomove) {
            // If any unit on-board was in a fight (and
            // suffered casualties), then halt movement
            nomove = true;
        }
        if (unit->monthorders && unit->monthorders->type == Orders::Types::O_SAIL) {
            slr += static_cast<size_t>(unit->GetSkill(Skills::Types::S_SAILING)) * unit->GetMen();
        }
    }

    if (nomove) {
        stop = true;
    } else if (wgt > fleet->FleetCapacity()) {
        cap->Error("SAIL: Fleet is overloaded.");
        stop = true;
    } else if (slr < fleet->GetFleetSize()) {
        cap->Error("SAIL: Not enough sailors.");
        stop = true;
    } else if (o->dirs.empty()) {
        // no more moves?
        stop = true;
    } else {
        const auto& x = o->dirs.front();
        ARegion::Handle newreg;
        if (x->dir.isMovePause()) {
            newreg = reg;
        } else {
            newreg = reg->neighbors[x->dir].lock();
        }
        unsigned int cost = 1;
        if (Globals->WEATHER_EXISTS) {
            if (newreg && newreg->weather != Weather::Types::W_NORMAL &&
                    !newreg->clearskies)
                cost = 2;
        }
        if (x->dir.isMovePause()) {
            cost = 1;
        }
        // We probably shouldn't see terrain-based errors until
        // we accumulate enough movement points to get there
        if (fleet->movepoints < cost * Globals->MAX_SPEED)
            return 0;
        if (!newreg) {
            cap->Error("SAIL: Can't sail that way.");
            stop = true;
        } else if (x->dir.isMovePause()) {
            // Can always do maneuvers
        } else if (fleet->flying < 1 && !newreg->IsCoastalOrLakeside()) {
            cap->Error("SAIL: Can't sail inland.");
            stop = true;
        } else if ((fleet->flying < 1) &&
            (TerrainDefs[reg->type].similar_type != Regions::Types::R_OCEAN) &&
            (TerrainDefs[newreg->type].similar_type != Regions::Types::R_OCEAN)) {
            cap->Error("SAIL: Can't sail inland.");
            stop = true;
        } else if (Globals->PREVENT_SAIL_THROUGH &&
                (TerrainDefs[reg->type].similar_type != Regions::Types::R_OCEAN) &&
                (fleet->flying < 1) &&
                (fleet->prevdir.isValid()) &&
                (fleet->prevdir != x->dir)) {
            // Check to see if sailing THROUGH land!
            // always allow retracing steps
            bool blocked1 = false;
            bool blocked2 = false;
            Directions d1 = fleet->prevdir;
            Directions d2 = x->dir;
            if (d1 > d2) {
                std::swap(d1, d2);
            }
            for (auto k = std::next(d1.asIter()); k != d2.asIter(); ++k) {
                const auto& land1 = reg->neighbors[*k];
                if ((land1.expired()) ||
                        (TerrainDefs[land1.lock()->type].similar_type !=
                         Regions::Types::R_OCEAN))
                    blocked1 = true;
            }
            ssize_t sides = static_cast<ssize_t>(Directions::size()) - 2 - (static_cast<ssize_t>(d2) - static_cast<ssize_t>(d1) - 1);
            ssize_t start = static_cast<ssize_t>(d2) + 1;
            ssize_t end = static_cast<ssize_t>(d2) + sides;
            for (ssize_t l = start; l <= end; l++) {
                Directions dl;
                const size_t l_u = static_cast<size_t>(l);
                if(l_u >= Directions::size())
                {
                    dl = l_u - Directions::size();
                }
                else
                {
                    dl = l_u;
                }
                const auto& land2 = reg->neighbors[dl];
                if (land2.expired() ||
                        (TerrainDefs[land2.lock()->type].similar_type !=
                         Regions::Types::R_OCEAN))
                    blocked2 = true;
            }
            if (blocked1 && blocked2)
            {
                cap->Error(AString("SAIL: Could not sail ") +
                        DirectionStrs[x->dir] + AString(" from ") +
                        reg->ShortPrint(regions) +
                        ". Cannot sail through land.");
                stop = true;
            }
        }

        if (!stop) {
            fleet->movepoints -= cost * Globals->MAX_SPEED;
            if (!x->dir.isMovePause()) {
                fleet->MoveObject(newreg);
                fleet->SetPrevDir(reg->GetRealDirComp(x->dir));
            }
            for(const auto& unit: fleet->units) {
                unit->moved += cost;
                if (unit->guard == GUARD_GUARD)
                    unit->guard = GUARD_NONE;
                unit->alias = 0;
                unit->PracticeAttribute("wind");
                if (unit->monthorders) {
                    if (unit->monthorders->type == Orders::Types::O_SAIL)
                        unit->Practice(Skills::Types::S_SAILING);
                    if (unit->monthorders->type == Orders::Types::O_MOVE) {
                        unit->monthorders.reset();
                    }
                }
                unit->DiscardUnfinishedShips();
                if (GetFaction2(facs, unit->faction.lock()->num).expired()) {
                    facs.push_back(unit->faction);
                }
            }

            for(const auto& f_w: facs) {
                auto f = f_w.lock();
                if (x->dir.isMovePause()) {
                    f->Event(fleet->name +
                        AString(" performs maneuvers in ") +
                        reg->ShortPrint(regions) +
                        AString("."));
                } else {
                    f->Event(fleet->name +
                        AString(" sails from ") +
                        reg->ShortPrint(regions) +
                        AString(" to ") +
                        newreg->ShortPrint(regions) +
                        AString("."));
                }
            }
            if (Globals->TRANSIT_REPORT != GameDefs::REPORT_NOTHING &&
                    !x->dir.isMovePause()) {
                if (!(cap->faction.lock()->IsNPC())) newreg->visited = 1;
                for(const auto& unit: fleet->units) {
                    // Everyone onboard gets to see the sights

                    // Note the hex being left
                    for(const auto& f: reg->passers) {
                        if (f->unit.lock() == unit) {
                            // We moved into here this turn
                            f->exits_used[x->dir] = true;
                        }
                    }
                    // And mark the hex being entered
                    auto& f = newreg->passers.emplace_back(std::make_shared<Farsight>());
                    f->faction = unit->faction;
                    f->level = 0;
                    f->unit = unit;
                    f->exits_used[reg->GetRealDirComp(x->dir)] = true;
                }
            }
            reg = newreg;
            if (newreg->ForbiddenShip(fleet)) {
                cap->faction.lock()->Event(fleet->name +
                    AString(" is stopped by guards in ") +
                    newreg->ShortPrint(regions) + 
                    AString("."));
                stop = true;
            }
            o->dirs.pop_front();
        }
    }

    if (stop) {
        // Clear out everyone's orders
        for(const auto& unit: fleet->units) {
            if (unit->monthorders &&
                    unit->monthorders->type == Orders::Types::O_SAIL) {
                unit->monthorders.reset();
            }
        }
    }

    auto loc = std::make_shared<Location>();
    loc->unit = cap;
    loc->region = reg;
    loc->obj = fleet;
    return loc;
}

void Game::RunTeachOrders()
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->monthorders) {
                    if (u->monthorders->type == Orders::Types::O_TEACH) {
                        Do1TeachOrder(r, u);
                        u->monthorders.reset();
                    }
                }
            }
        }
    }
}

void Game::Do1TeachOrder(const ARegion::Handle& reg, const Unit::Handle& unit)
{
    /* First pass, find how many to teach */
    if (Globals->LEADERS_EXIST && !unit->IsLeader()) {
        /* small change to handle Ceran's mercs */
        if (!unit->GetMen(Items::Types::I_MERC)) {
            // Mercs can teach even though they are not leaders.
            // They cannot however improve their own skills
            unit->Error("TEACH: Only leaders can teach.");
            return;
        }
    }

    size_t students = 0;
    const auto order = std::dynamic_pointer_cast<TeachOrder>(unit->monthorders);
    const auto ufac = unit->faction.lock();
    reg->DeduplicateUnitList(order->targets, ufac->num);

    auto it = order->targets.begin();
    while(it != order->targets.end()) {
        const auto id = *it;
        const auto target_w = reg->GetUnitId(id, ufac->num);
        if (target_w.expired()) {
            it = order->targets.erase(it);
            unit->Error("TEACH: No such unit.");
            continue;
        } else {
            const auto target = target_w.lock();
            if (target->faction.lock()->GetAttitude(ufac->num).isNotFriendly()) {
                unit->Error(AString("TEACH: ") + target->name +
                            " is not a member of a friendly faction.");
                it = order->targets.erase(it);
                continue;
            } else {
                if (!target->monthorders ||
                    target->monthorders->type != Orders::Types::O_STUDY) {
                    unit->Error(AString("TEACH: ") + target->name +
                                " is not studying.");
                    it = order->targets.erase(it);
                    continue;
                } else {
                    const auto& sk = std::dynamic_pointer_cast<StudyOrder>(target->monthorders)->skill;
                    if (unit->GetRealSkill(sk) <= target->GetRealSkill(sk)) {
                        unit->Error(AString("TEACH: ") +
                                    target->name + " is not studying "
                                    "a skill you can teach.");
                        it = order->targets.erase(it);
                        continue;
                    } else {
                        // Check whether it's a valid skill to teach
                        if (SkillDefs[sk].flags & SkillType::NOTEACH) {
                            unit->Error(AString("TEACH: ") + 
                                    AString(SkillDefs[sk].name) + 
                                    " cannot be taught.");
                            return;
                        } else {
                            students += target->GetMen();
                        }
                    }
                }
            }
        }
        ++it;
    }
    if (!students) return;

    size_t days = (30 * unit->GetMen() * Globals->STUDENTS_PER_TEACHER);

    /* We now have a list of valid targets */
    for(const auto& id: order->targets) {
        const auto u = reg->GetUnitId(id, ufac->num).lock();

        size_t umen = u->GetMen();
        size_t tempdays = (umen * days) / students;
        if (tempdays > 30 * umen) tempdays = 30 * umen;
        days -= tempdays;
        students -= umen;

        const auto o = std::dynamic_pointer_cast<StudyOrder>(u->monthorders);
        o->days += tempdays;
        if (o->days > 30 * umen)
        {
            days += o->days - 30 * umen;
            o->days = 30 * umen;
        }
        unit->Event(AString("Teaches ") + SkillDefs[o->skill].name +
                    " to " + u->name + ".");
        // The TEACHER may learn something in this process!
        unit->Practice(o->skill);
    }
}

void Game::Run1BuildOrder(const ARegion::Handle& r, const Object::Handle& obj, const Unit::Handle& u)
{
    int questcomplete = 0;

    if (!TradeCheck(r, u->faction.lock())) {
        u->Error("BUILD: Faction can't produce in that many regions.");
        u->monthorders.reset();
        return;
    }

    auto buildobj = r->GetObject(u->build).lock();
    // plain "BUILD" order needs to check that the unit is in something
    // that can be built AFTER enter/leave orders have executed
    if (!buildobj || buildobj->type == Objects::Types::O_DUMMY) {
        buildobj = obj;
    }
    if (!buildobj || buildobj->type == Objects::Types::O_DUMMY) {
        u->Error("BUILD: Nothing to build.");
        u->monthorders.reset();
        return;
    }
    AString skname = ObjectDefs[buildobj->type].skill;
    Skills sk = LookupSkill(skname);
    if (!sk.isValid()) {
        u->Error("BUILD: Can't build that.");
        u->monthorders.reset();
        return;
    }

    size_t usk = u->GetSkill(sk);
    if (usk < ObjectDefs[buildobj->type].level) {
        u->Error("BUILD: Can't build that.");
        u->monthorders.reset();
        return;
    }
    
    int needed = buildobj->incomplete;
    const auto& type = buildobj->type;
    // AS
    if (((ObjectDefs[type].flags & ObjectType::NEVERDECAY) || !Globals->DECAY) &&
            needed < 1) {
        u->Error("BUILD: Object is finished.");
        u->monthorders.reset();
        return;
    }

    // AS
    if (needed <= -(ObjectDefs[type].maxMaintenance)) {
        u->Error("BUILD: Object does not yet require maintenance.");
        u->monthorders.reset();
        return;
    }

    const auto& it = ObjectDefs[type].item;
    size_t itn;
    if (it.isWoodOrStone()) {
        itn = u->GetSharedNum(Items::Types::I_WOOD) + u->GetSharedNum(Items::Types::I_STONE);
    } else {
        itn = u->GetSharedNum(it);
    }

    if (itn == 0) {
        u->Error("BUILD: Don't have the required materials.");
        u->monthorders.reset();
        return;
    }

    size_t num = u->GetMen() * static_cast<size_t>(usk);

    // AS
    AString job;
    if (needed < 1) {
        // This looks wrong, but isn't.
        // If a building has a maxMaintainence of 75 and the road is at
        // -70 (ie, 5 from max) then we want the value of maintMax to be
        // 5 here.  Then we divide by maintFactor (some things are easier
        // to refix than others) to get how many items we need to fix it.
        // Then we fix it by that many items * maintFactor
        size_t maintMax = static_cast<size_t>((ObjectDefs[type].maxMaintenance + needed) / ObjectDefs[type].maintFactor);
        if (num > maintMax) num = maintMax;
        if (itn < num) num = itn;
        job = "Performs maintenance on ";
        buildobj->incomplete -= (static_cast<int>(num) * ObjectDefs[type].maintFactor);
        if (buildobj->incomplete < -(ObjectDefs[type].maxMaintenance))
            buildobj->incomplete = -(ObjectDefs[type].maxMaintenance);
    } else if (needed > 0) {
        if (num > static_cast<size_t>(needed)) num = static_cast<size_t>(needed);
        if (itn < num) num = itn;
        job = "Performs construction on ";
        buildobj->incomplete -= static_cast<int>(num);
        if (buildobj->incomplete == 0) {
            job = "Completes construction of ";
            buildobj->incomplete = -(ObjectDefs[type].maxMaintenance);
            if (quests.CheckQuestBuildTarget(r, type, u)) {
                questcomplete = 1;
            }
        }
    }

    /* Perform the build */
    
    if (obj != buildobj)
        u->MoveUnit(buildobj);

    if (it.isWoodOrStone()) {
        size_t num_shared_stone = u->GetSharedNum(Items::Types::I_STONE);
        if (num > num_shared_stone) {
            num -= num_shared_stone;
            u->ConsumeShared(Items::Types::I_STONE, num_shared_stone);
            u->ConsumeShared(Items::Types::I_WOOD, num);
        } else {
            u->ConsumeShared(Items::Types::I_STONE, num);
        }
    } else {
        u->ConsumeShared(it, num);
    }
    
    /* Regional economic improvement */
    r->improvement += static_cast<int>(num);

    // AS
    u->Event(job + buildobj->name);
    if (questcomplete)
        u->Event("You have completed a quest!");
    u->Practice(sk);

    u->monthorders.reset();
}

/* Alternate processing for building item-type ship
 * objects and instantiating fleets.
 */
void Game::RunBuildShipOrder(const ARegion::Handle& r, const Object::Handle&, const Unit::Handle& u)
{
    size_t unfinished;
    AString skname;

    Items ship(abs(u->build));
    skname = ItemDefs[ship].pSkill;
    size_t skill = LookupSkill(skname);
    size_t level = u->GetSkill(skill);

    unsigned int output;

    // get needed to complete
    size_t maxbuild = 0;
    if ((u->monthorders) && 
        (u->monthorders->type == Orders::Types::O_BUILD)) {
            auto border = std::dynamic_pointer_cast<BuildOrder>(u->monthorders);
            maxbuild = static_cast<size_t>(border->needtocomplete);
    }
    if (maxbuild < 1) {
        // Our helpers have already finished the hard work, so
        // just put the finishing touches on the new vessel
        unfinished = 0;
    } else {
        output = ShipConstruction(r, u, u, level, maxbuild, ship);
        
        if (output < 1) return;
        
        // are there unfinished ship items of the given type?
        unfinished = u->items.GetNum(ship);
        
        if (unfinished == 0) {
            // Start a new ship's construction from scratch
            unfinished = static_cast<size_t>(ItemDefs[ship].pMonths);
            u->items.SetNum(ship, unfinished);    
        }

        // Now reduce unfinished by produced amount
        if (unfinished < output)
        {
            unfinished = 0;
        }
        else
        {
            unfinished -= output;
        }
    }
    u->items.SetNum(ship, unfinished);

    // practice
    u->Practice(skill);

    if (unfinished == 0) {
        u->Event(AString("Finishes building a ") + ItemDefs[ship].name + " in " +
            r->ShortPrint(regions) + ".");
        CreateShip(r, u, ship);
    } else {
        unsigned int percent = 100 * output / static_cast<unsigned int>(ItemDefs[ship].pMonths);
        u->Event(AString("Performs construction work on a ") + 
            ItemDefs[ship].name + " (" + percent + "%) in " +
            r->ShortPrint(regions) + ".");
    }

    u->monthorders.reset();
}

void Game::RunBuildHelpers(const ARegion::Handle& r)
{
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            if (u->monthorders) {
                if (u->monthorders->type == Orders::Types::O_BUILD) {
                    auto o = std::dynamic_pointer_cast<BuildOrder>(u->monthorders);
                    Object::WeakHandle tarobj;
                    if (o->target.isValid()) {
                        const auto ufac_num = u->faction.lock()->num;
                        auto target_w = r->GetUnitId(o->target, ufac_num);
                        if (target_w.expired()) {
                            u->Error("BUILD: No such unit to help.");
                            u->monthorders.reset();
                            continue;
                        }
                        const auto target = target_w.lock();
                        // Make sure that unit is building
                        if (!target->monthorders ||
                                target->monthorders->type != Orders::Types::O_BUILD) {
                            u->Error("BUILD: Unit isn't building.");
                            u->monthorders.reset();
                            continue;
                        }
                        // Make sure that unit considers you friendly!
                        if (target->faction.lock()->GetAttitude(ufac_num).isNotFriendly()) {
                            u->Error("BUILD: Unit you are helping rejects "
                                    "your help.");
                            u->monthorders.reset();
                            continue;
                        }
                        if (target->build == 0) {
                            // Help with whatever building the target is in
                            tarobj = target->object;
                            u->build = tarobj.lock()->num;
                        } else if (target->build > 0) {
                            u->build = target->build;
                            tarobj = r->GetObject(target->build);
                        } else {
                            // help build ships
                            Items ship(abs(target->build));
                            AString skname = ItemDefs[ship].pSkill;
                            Skills skill = LookupSkill(skname);
                            size_t level = u->GetSkill(skill);
                            size_t needed = 0;
                            if ((target->monthorders) && 
                                    (target->monthorders->type == Orders::Types::O_BUILD)) {
                                        const auto border = std::dynamic_pointer_cast<BuildOrder>(target->monthorders);
                                        needed = static_cast<size_t>(border->needtocomplete);
                            }
                            if (needed < 1) {
                                u->Error("BUILD: Construction is already complete.");
                                u->monthorders.reset();
                                continue;
                            }
                            unsigned int output = ShipConstruction(r, u, target, level, needed, ship);
                            if (output < 1) continue;
                            
                            size_t unfinished = target->items.GetNum(ship);
                            if (unfinished == 0) {
                                // Start construction on a new ship
                                unfinished = static_cast<size_t>(ItemDefs[ship].pMonths);
                                target->items.SetNum(ship, unfinished);    
                            }
                            unfinished -= output;

                            // practice
                            u->Practice(skill);
                            
                            if (unfinished > 0) {
                                target->items.SetNum(ship, unfinished);
                                if ((target->monthorders) && 
                                    (target->monthorders->type == Orders::Types::O_BUILD)) {
                                        const auto border = std::dynamic_pointer_cast<BuildOrder>(target->monthorders);
                                        border->needtocomplete = static_cast<int>(unfinished);
                                }
                            } else {
                                // CreateShip(r, target, ship);
                                // don't create the ship yet; leave that for the unit we're helping
                                target->items.SetNum(ship, 1);
                                if ((target->monthorders) && 
                                    (target->monthorders->type == Orders::Types::O_BUILD)) {
                                        const auto border = std::dynamic_pointer_cast<BuildOrder>(target->monthorders);
                                        border->needtocomplete = 0;
                                }
                            } 
                            unsigned int percent = 100 * output / static_cast<unsigned int>(ItemDefs[ship].pMonths);
                            u->Event(AString("Helps ") +
                                target->name + " with construction of a " + 
                                ItemDefs[ship].name + " (" + percent + "%) in " +
                                r->ShortPrint(regions) + ".");                            
                        }
                        // no need to move unit if item-type ships
                        // are being built. (leave this commented out)
                        // if (tarobj == NULL) tarobj = target->object;
                        if (!tarobj.expired() && (u->object.lock() != tarobj.lock()))
                            u->MoveUnit(tarobj);
                    } else {
                        Object::WeakHandle buildobj;
                        if (u->build > 0) {
                            buildobj = r->GetObject(u->build);
                            if (!buildobj.expired())
                            {
                                const auto buildobj_s  = buildobj.lock();
                                if(buildobj_s != r->GetDummy().lock() &&
                                   buildobj_s != u->object.lock())
                                {
                                    u->MoveUnit(buildobj);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Creates a new ship - either by adding it to a 
 * compatible fleet object or creating a new fleet
 * object with Unit u as owner consisting of exactly
 * ONE ship of the given type.
 */
void Game::CreateShip(const ARegion::Handle& r, const Unit::Handle& u, const Items& ship)
{
    const auto obj = u->object.lock();
    // Do we need to create a new fleet?
    bool newfleet = true;
    if (obj->IsFleet()) {
        newfleet = false;
        int flying = obj->flying;
        // are the fleets compatible?
        if ((flying > 0) && (ItemDefs[ship].fly < 1)) newfleet = true;
        if ((flying < 1) && (ItemDefs[ship].fly > 0)) newfleet = true;
    }
    if (newfleet) {
        // create a new fleet
        auto& fleet = obj->region.lock()->objects.emplace_back(std::make_shared<Object>(r));
        fleet->type = Objects::Types::O_FLEET;
        fleet->num = shipseq++;
        fleet->name = AString("Ship [") + fleet->num + "]";
        fleet->AddShip(ship);
        u->MoveUnit(fleet);
    } else {
        obj->AddShip(ship);
    }
}

/* Checks and returns the amount of ship construction,
 * handles material use and practice for both the main
 * shipbuilders and the helpers.
 */
unsigned int Game::ShipConstruction(const ARegion::Handle& r,
                                    const Unit::Handle& u,
                                    const Unit::Handle& target,
                                    size_t level,
                                    size_t needed,
                                    const Items& ship)
{
    if (!TradeCheck(r, u->faction.lock())) {
        u->Error("BUILD: Faction can't produce in that many regions.");
        u->monthorders.reset();
        return 0;
    }

    if (level < ItemDefs[ship].pLevel) {
        u->Error("BUILD: Can't build that.");
        u->monthorders.reset();
        return 0;
    }

    // are there unfinished ship items of the given type?
    size_t unfinished = target->items.GetNum(ship);

    size_t number = u->GetMen() * level + static_cast<size_t>(u->GetProductionBonus(ship));

    // find the max we can possibly produce based on man-months of labor
    size_t maxproduced;
    if (ItemDefs[ship].flags & ItemType::SKILLOUT)
        maxproduced = u->GetMen();
    else
        // don't adjust for pMonths
        // - pMonths represents total requirement
        maxproduced = number;
    
    // adjust maxproduced for items needed until completion
    if (needed < maxproduced) maxproduced = needed;
        
    // adjust maxproduced for unfinished ships
    if ((unfinished > 0) && (maxproduced > unfinished))
        maxproduced = unfinished;

    if (ItemDefs[ship].flags & ItemType::ORINPUTS) {
        // Figure out the max we can produce based on the inputs
        size_t count = 0;
        for (const auto& input: ItemDefs[ship].pInput) {
            const auto& i = input.item;
            if (i.isValid())
            {
                count += u->GetSharedNum(i) / static_cast<size_t>(input.amt);
            }
        }
        if (maxproduced > count)
            maxproduced = count;
        count = maxproduced;
        
        // no required materials?
        if (count < 1) {
            u->Error("BUILD: Don't have the required materials.");
            u->monthorders.reset();
            return 0;
        }
        
        /* regional economic improvement */
        r->improvement += static_cast<int>(count);

        // Deduct the items spent
        for (const auto& input: ItemDefs[ship].pInput) {
            const auto& i = input.item;
            const unsigned int a = static_cast<unsigned int>(input.amt);
            if (i.isValid()) {
                const size_t amt = u->GetSharedNum(i);
                const size_t amt_divided = amt / a;
                if (count > amt_divided) {
                    count -= amt_divided;
                    u->ConsumeShared(i, amt_divided * a);
                } else {
                    u->ConsumeShared(i, count * a);
                    count = 0;
                }
            }
        }
    }
    else {
        // Figure out the max we can produce based on the inputs
        for (const auto& input: ItemDefs[ship].pInput) {
            const auto& i = input.item;
            if (i.isValid()) {
                const size_t amt = u->GetSharedNum(i);
                const size_t amt_divided = amt / static_cast<unsigned int>(input.amt);
                if (amt_divided < maxproduced) {
                    maxproduced = amt_divided;
                }
            }
        }
        
        // no required materials?
        if (maxproduced < 1) {
            u->Error("BUILD: Don't have the required materials.");
            u->monthorders.reset();
            return 0;
        }
        
        /* regional economic improvement */
        r->improvement += static_cast<int>(maxproduced);

        // Deduct the items spent
        for (const auto& input: ItemDefs[ship].pInput) {
            const auto& i = input.item;
            const unsigned int a = static_cast<unsigned int>(input.amt);
            if (i.isValid()) {
                u->ConsumeShared(i, maxproduced*a);
            }
        }
    }
    unsigned int output = static_cast<unsigned int>(maxproduced) * static_cast<unsigned int>(ItemDefs[ship].pOut);
    if (ItemDefs[ship].flags & ItemType::SKILLOUT)
    {
        output *= static_cast<unsigned int>(level);
    }

    u->monthorders.reset();
    
    return output;
}

void Game::RunMonthOrders()
{
    for(const auto& r: regions) {
        RunIdleOrders(r);
        RunStudyOrders(r);
        RunBuildHelpers(r);
        RunProduceOrders(r);
    }
}

void Game::RunUnitProduce(const ARegion::Handle& r, const Unit::Handle& u)
{
    auto o = std::dynamic_pointer_cast<ProduceOrder>(u->monthorders);

    for(const auto& p: r->products) {
        // PRODUCE orders for producing goods from the land
        // are shared among factions, and therefore handled
        // specially by the RunAProduction() function
        if (o->skill == p->skill && o->item == p->itemtype)
            return;
    }

    if (o->item == Items::Types::I_SILVER) {
        if (!o->quiet)
            u->Error("Can't do that in this region.");
        u->monthorders.reset();
        return;
    }

    if (!o->item.isValid() || ItemDefs[o->item].flags & ItemType::DISABLED) {
        u->Error("PRODUCE: Can't produce that.");
        u->monthorders.reset();
        return;
    }

    const auto& input = ItemDefs[o->item].pInput[0].item;
    if (!input.isValid()) {
        u->Error("PRODUCE: Can't produce that.");
        u->monthorders.reset();
        return;
    }

    size_t level = u->GetSkill(o->skill);
    if (level < ItemDefs[o->item].pLevel) {
        u->Error("PRODUCE: Can't produce that.");
        u->monthorders.reset();
        return;
    }

    // LLS
    int number = static_cast<int>(u->GetMen() * level) + u->GetProductionBonus(o->item);

    if (!TradeCheck(r, u->faction.lock())) {
        u->Error("PRODUCE: Faction can't produce in that many regions.");
        u->monthorders.reset();
        return;
    }

    // find the max we can possibly produce based on man-months of labor
    size_t maxproduced;
    if (ItemDefs[o->item].flags & ItemType::SKILLOUT)
        maxproduced = u->GetMen();
    else
        maxproduced = static_cast<size_t>(number / ItemDefs[o->item].pMonths);

    if (o->target > 0)
    {
        const size_t o_target = static_cast<size_t>(o->target);
        if(maxproduced > o_target)
        {
            maxproduced = o_target;
        }
    }

    if (ItemDefs[o->item].flags & ItemType::ORINPUTS) {
        // Figure out the max we can produce based on the inputs
        size_t count = 0;
        for (const auto& input: ItemDefs[o->item].pInput) {
            const auto& i = input.item;
            if (i.isValid())
            {
                count += u->GetSharedNum(i) / static_cast<size_t>(input.amt);
            }
        }
        if (maxproduced > count)
            maxproduced = count;
        count = maxproduced;

        /* regional economic improvement */
        r->improvement += static_cast<int>(count);

        // Deduct the items spent
        for (const auto& input: ItemDefs[o->item].pInput) {
            const auto& i = input.item;
            const size_t a = static_cast<size_t>(input.amt);
            if (i.isValid()) {
                const size_t amt = u->GetSharedNum(i);
                const size_t amt_divided = amt / a;
                if (count > amt_divided) {
                    count -= amt_divided;
                    u->ConsumeShared(i, amt_divided * a);
                } else {
                    u->ConsumeShared(i, count * a);
                    count = 0;
                }
            }
        }
    }
    else {
        // Figure out the max we can produce based on the inputs
        for (const auto& input: ItemDefs[o->item].pInput) {
            const auto& i = input.item;
            if (i.isValid()) {
                const size_t amt = u->GetSharedNum(i);
                const size_t amt_divided = amt / static_cast<unsigned int>(input.amt);
                if (amt_divided < maxproduced) {
                    maxproduced = amt_divided;
                }
            }
        }
        
        /* regional economic improvement */
        r->improvement += static_cast<int>(maxproduced);
        
        // Deduct the items spent
        for (const auto& input: ItemDefs[o->item].pInput) {
            const auto& i = input.item;
            const size_t a = static_cast<size_t>(input.amt);
            if (i.isValid()) {
                u->ConsumeShared(i, maxproduced*a);
            }
        }
    }

    // Now give the items produced
    size_t output = maxproduced * static_cast<size_t>(ItemDefs[o->item].pOut);
    if (ItemDefs[o->item].flags & ItemType::SKILLOUT)
    {
        output *= static_cast<size_t>(level);
    }
    u->items.SetNum(o->item,u->items.GetNum(o->item) + output);
    u->Event(AString("Produces ") + ItemString(o->item, static_cast<int>(output)) + " in " +
            r->ShortPrint(regions) + ".");
    u->Practice(o->skill);
    o->target -= static_cast<int>(output);
    if (o->target > 0) {
        auto& tOrder = u->turnorders.emplace_front(std::make_shared<TurnOrder>());
        AString order;
        tOrder->repeating = 0;
        order = "PRODUCE ";
        order += o->target;
        order += " ";
        order += ItemDefs[o->item].abr;
        tOrder->turnOrders.emplace_back(std::make_shared<AString>(order));
    }
    u->monthorders.reset();
}

void Game::RunProduceOrders(const ARegion::Handle& r)
{
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            if (u->monthorders) {
                if (u->monthorders->type == Orders::Types::O_PRODUCE) {
                    RunUnitProduce(r,u);
                } else {
                    if (u->monthorders->type == Orders::Types::O_BUILD) {
                        if (u->build >= 0) {
                            Run1BuildOrder(r,obj,u);
                        } else {
                            RunBuildShipOrder(r,obj,u);
                        }
                    }
                }
            }
        }
    }
    for(const auto& elem: r->products)
    {
        RunAProduction(r, elem);
    }
}

int Game::ValidProd(const Unit::Handle& u,
                    const ARegion::Handle& r,
                    const Production::Handle& p)
{
    if (u->monthorders->type != Orders::Types::O_PRODUCE) return 0;

    const auto po = std::dynamic_pointer_cast<ProduceOrder>(u->monthorders);
    if (p->itemtype == po->item && p->skill == po->skill) {
        if (!p->skill.isValid()) {
            /* Factor for fractional productivity: 10 */
            po->productivity = static_cast<int>(static_cast<float>(u->GetMen() * static_cast<size_t>(p->productivity)) / 10.0);
            return po->productivity;
        }
        size_t level = u->GetSkill(p->skill);
        if (level < ItemDefs[p->itemtype].pLevel) {
            u->Error("PRODUCE: Unit isn't skilled enough.");
            u->monthorders.reset();
            return 0;
        }

        //
        // Check faction limits on production. If the item is silver, then the
        // unit is entertaining or working, and the limit does not apply
        //
        if (p->itemtype != Items::Types::I_SILVER && !TradeCheck(r, u->faction.lock())) {
            u->Error("PRODUCE: Faction can't produce in that many regions.");
            u->monthorders.reset();
            return 0;
        }

        /* check for bonus production */
        // LLS
        int bonus = u->GetProductionBonus(p->itemtype);
        /* Factor for fractional productivity: 10 */
        po->productivity = static_cast<int>(static_cast<float>(u->GetMen() * level * static_cast<size_t>(p->productivity)) / 10.0) + bonus;
        if (po->target > 0 && po->productivity > po->target)
            po->productivity = po->target;
        return po->productivity;
    }
    return 0;
}

int Game::FindAttemptedProd(const ARegion::Handle& r, const Production::Handle& p)
{
    int attempted = 0;
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            if ((u->monthorders) && (u->monthorders->type == Orders::Types::O_PRODUCE)) {
                attempted += ValidProd(u,r,p);
            }
        }
    }
    return attempted;
}

void Game::RunAProduction(const ARegion::Handle& r, const Production::Handle& p)
{
    bool questcomplete;
    p->activity = 0;
    if (p->amount == 0) return;

    /* First, see how many units are trying to work */
    int attempted = FindAttemptedProd(r,p);
    int amt = p->amount;
    if (attempted < amt) attempted = amt;
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            questcomplete = false;
            if (!u->monthorders || u->monthorders->type != Orders::Types::O_PRODUCE)
                continue;

            const auto po = std::dynamic_pointer_cast<ProduceOrder>(u->monthorders);
            if (po->skill != p->skill || po->item != p->itemtype)
                continue;

            /* We need to implement a hack to avoid overflowing */
            int uatt, ubucks;

            uatt = po->productivity;
            if (uatt && amt && attempted)
            {
                double dUbucks = static_cast<double>(amt) * static_cast<double>(uatt)
                    / static_cast<double>(attempted);
                ubucks = static_cast<int>(dUbucks);
                questcomplete = quests.CheckQuestHarvestTarget(r, po->item, ubucks, amt, u);
            }
            else
            {
                ubucks = 0;
            }

            amt -= ubucks;
            attempted -= uatt;
            u->items.SetNum(po->item, u->items.GetNum(po->item) + static_cast<size_t>(ubucks));
            u->faction.lock()->DiscoverItem(po->item, 0, 1);
            p->activity += ubucks;
            po->target -= ubucks;
            if (po->target > 0) {
                auto& tOrder = u->turnorders.emplace_front(std::make_shared<TurnOrder>());
                auto& order = *(tOrder->turnOrders.emplace_back(std::make_shared<AString>()));
                tOrder->repeating = 0;
                order = "PRODUCE ";
                order += po->target;
                order += " ";
                order += ItemDefs[po->item].abr;
            }

            /* Show in unit's events section */
            if (po->item == Items::Types::I_SILVER)
            {
                //
                // WORK
                //
                if (!po->skill.isValid())
                {
                    u->Event(AString("Earns ") + ubucks + " silver working in "
                             + r->ShortPrint(regions) + ".");
                }
                else
                {
                    //
                    // ENTERTAIN
                    //
                    u->Event(AString("Earns ") + ubucks
                             + " silver entertaining in " +
                             r->ShortPrint(regions)
                             + ".");
                    // If they don't have PHEN, then this will fail safely
                    u->Practice(Skills::Types::S_PHANTASMAL_ENTERTAINMENT);
                    u->Practice(Skills::Types::S_ENTERTAINMENT);
                }
            }
            else
            {
                /* Everything else */
                u->Event(AString("Produces ") + ItemString(po->item,ubucks) +
                         " in " + r->ShortPrint(regions) + ".");
                u->Practice(po->skill);
            }
            u->monthorders.reset();
            if (questcomplete)
            {
                u->Event("You have completed a quest!");
            }
        }
    }
}

void Game::RunStudyOrders(const ARegion::Handle& r)
{
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            if (u->monthorders) {
                if (u->monthorders->type == Orders::Types::O_STUDY) {
                    Do1StudyOrder(u,obj);
                    u->monthorders.reset();
                }
            }
        }
    }
}

void Game::RunIdleOrders(const ARegion::Handle& r)
{
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            if (u->monthorders && u->monthorders->type == Orders::Types::O_IDLE) {
                u->Event("Sits idle.");
                u->monthorders.reset();
            }
        }
    }
}

void Game::Do1StudyOrder(const Unit::Handle& u, const Object::Handle& obj)
{
    const auto o = std::dynamic_pointer_cast<StudyOrder>(u->monthorders);
    int reset_man, taughtdays, days;
    AString str;

    reset_man = -1;
    const Skills& sk = o->skill;
    if (!sk.isValid() || SkillDefs[sk].flags & SkillType::DISABLED ||
            (SkillDefs[sk].flags & SkillType::APPRENTICE &&
                !Globals->APPRENTICES_EXIST)) {
        u->Error("STUDY: Can't study that.");
        return;
    }

    // Check that the skill can be studied
    if (SkillDefs[sk].flags & SkillType::NOSTUDY) {
        u->Error( AString("STUDY: ") + AString(SkillDefs[sk].name) + " cannot be studied.");
        return;
    }
    
    // Small patch for Ceran Mercs
    if (u->GetMen(Items::Types::I_MERC)) {
        u->Error("STUDY: Mercenaries are not allowed to study.");
        return;
    }

    if (o->level.isValid()) {
        unsigned int skmax = u->GetSkillMax(sk);
        if (skmax < o->level) {
            o->level = skmax;
            if (u->GetRealSkill(sk) >= o->level) {
                str = "STUDY: Cannot study ";
                str += SkillDefs[sk].name;
                str += " beyond level ";
                str += o->level;
                str += ".";
                u->Error(str);
                return;
            } else {
                str = "STUDY: set study goal for ";
                str += SkillDefs[sk].name;
                str += " to the maximum achievable level (";
                str += o->level;
                str += ").";
                u->Error(str);
            }
        }
        if (u->GetRealSkill(sk) >= o->level) {
            u->Error("STUDY: already reached specified level; nothing to study.");
            return;
        }
    }

    const unsigned int cost = SkillCost(sk) * static_cast<unsigned int>(u->GetMen());
    if (cost > u->GetSharedMoney()) {
        u->Error("STUDY: Not enough funds.");
        return;
    }

    const auto u_fac = u->faction.lock();
    if ((SkillDefs[sk].flags & SkillType::MAGIC) && u->type != U_MAGE) {
        if (u->type == U_APPRENTICE) {
            u->Error(AString("STUDY: An ") +
                Globals->APPRENTICE_NAME +
                " cannot be made into a mage.");
            return;
        }
        if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_UNLIMITED) {
            if (CountMages(u_fac) >= AllowedMages(*u_fac)) {
                u->Error("STUDY: Can't have another magician.");
                return;
            }
        }
        if (u->GetMen() != 1) {
            u->Error("STUDY: Only 1-man units can be magicians.");
            return;
        }
        if (!(Globals->MAGE_NONLEADERS)) {
            if (u->GetLeaders() != 1) {
                u->Error("STUDY: Only leaders may study magic.");
                return;
            }
        }
        reset_man = u->type;
        u->type = U_MAGE;
    }

    if ((SkillDefs[sk].flags&SkillType::APPRENTICE) &&
            u->type != U_APPRENTICE) {
        if (u->type == U_MAGE) {
            u->Error(AString("STUDY: A mage cannot be made into an ") +
                Globals->APPRENTICE_NAME + ".");
            return;
        }

        if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_UNLIMITED) {
            if (CountApprentices(u_fac) >= AllowedApprentices(*u_fac)) {
                u->Error(AString("STUDY: Can't have another ") +                    Globals->APPRENTICE_NAME + ".");
                return;
            }
        }
        if (u->GetMen() != 1) {
            u->Error(AString("STUDY: Only 1-man units can be ") +
                Globals->APPRENTICE_NAME + "s.");
            return;
        }
        if (!(Globals->MAGE_NONLEADERS)) {
            if (u->GetLeaders() != 1) {
                u->Error(AString("STUDY: Only leaders may be ") +
                    Globals->APPRENTICE_NAME + "s.");
                return;
            }
        }
        reset_man = u->type;
        u->type = U_APPRENTICE;
    }

    if ((Globals->TRANSPORT & GameDefs::ALLOW_TRANSPORT) &&
            (sk == Skills::Types::S_QUARTERMASTER) && (u->GetSkill(Skills::Types::S_QUARTERMASTER) == 0) &&
            (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES)) {
        if (CountQuarterMasters(u_fac) >= AllowedQuarterMasters(*u_fac)) {
            u->Error("STUDY: Can't have another quartermaster.");
            return;
        }
        if (u->GetMen() != 1) {
            u->Error("STUDY: Only 1-man units can be quartermasters.");
            return;
        }
    }

    // If TACTICS_NEEDS_WAR is enabled, and the unit is trying to study to tact-5,
    // check that there's still space...
    if (Globals->TACTICS_NEEDS_WAR && sk == Skills::Types::S_TACTICS && 
            u->GetSkill(sk) == 4 && u->skills.GetDays(sk)/u->GetMen() >= 300) {
        if (CountTacticians(u_fac) >= AllowedTacticians(*u_fac)) {
            u->Error("STUDY: Can't start another level 5 tactics leader.");
            return;
        }
        if (u->GetMen() != 1) {
            u->Error("STUDY: Only 1-man units can study to level 5 in tactics.");
            return;
        }
        
    } // end tactics check
    
    // adjust teaching for study rate
    const size_t study_rate = u->skills.GetStudyRate(sk, u->GetMen());
    taughtdays = static_cast<int>((static_cast<size_t>(o->days) * study_rate) / 30);

    days = static_cast<int>(u->skills.GetStudyRate(sk, u->GetMen()) * u->GetMen() + static_cast<size_t>(taughtdays));

    if ((SkillDefs[sk].flags & SkillType::MAGIC) && u->GetSkill(sk) >= 2) {
        if (obj->incomplete > 0 || obj->type == Objects::Types::O_DUMMY) {
            u->Error("Warning: Magic study rate outside of a building "
                    "cut in half above level 2.");
            days /= 2;
        } else if (obj->mages < 1) {
            if (!Globals->LIMITED_MAGES_PER_BUILDING ||
                    (!obj->IsFleet() &&
                    !ObjectDefs[obj->type].maxMages)) {
                u->Error("Warning: Magic study rate cut in half above level 2 due "
                        "to unsuitable building.");
            } else {
                u->Error("Warning: Magic study rate cut in half above level 2 due "
                        "to number of mages studying in structure.");
            }
            days /= 2;
        } else if (Globals->LIMITED_MAGES_PER_BUILDING) {
            obj->mages--;
        }
    }

    if (SkillDefs[sk].flags & SkillType::SLOWSTUDY) {
        days /= 2;
    }

    if (u->Study(sk,days)) {
        u->ConsumeSharedMoney(cost);
        str = "Studies ";
        str += SkillDefs[sk].name;
        taughtdays = taughtdays / static_cast<int>(u->GetMen());
        if (taughtdays) {
            str += " and was taught for ";
            str += taughtdays;
            str += " days";
        }
        str += ".";
        u->Event(str);
        // study to level order
        if (o->level.isValid()) {
            if (u->GetSkill(sk) < o->level) {
                auto& tOrder = u->turnorders.emplace_front(std::make_shared<TurnOrder>());
                auto& order = *(tOrder->turnOrders.emplace_back(std::make_shared<AString>()));
                tOrder->repeating = 0;
                order = AString("STUDY ") + SkillDefs[sk].abbr + " " + static_cast<int>(o->level);
            } else {
                AString msg("Completes study to level ");
                msg += static_cast<int>(o->level);
                msg += " in ";
                msg += SkillDefs[sk].name;
                msg += ".";
                u->Event(msg);
            }    
        }
    } else {
        // if we just tried to become a mage or apprentice, but
        // were unable to study, reset unit to whatever it was before.
        if (reset_man != -1)
            u->type = reset_man;
    }
}

void Game::DoMoveEnter(const Unit::Handle& unit, const ARegion::Handle& region, Object::Handle& obj)
{
    if (!unit->monthorders ||
            ((unit->monthorders->type != Orders::Types::O_MOVE) &&
             (unit->monthorders->type != Orders::Types::O_ADVANCE)))
        return;
    const auto o = std::dynamic_pointer_cast<MoveOrder>(unit->monthorders);

    while (!o->dirs.empty()) {
        const auto& x = o->dirs.front();
        const auto& i = x->dir;
        if (!i.isMoveOut() && !i.isMoveEnter())
        {
            return;
        }
        o->dirs.pop_front();

        if (i.isMoveEnter()) {
            const auto to_w = region->GetObject(i.getMoveObject());
            if (to_w.expired()) {
                unit->Error("MOVE: Can't find object.");
                continue;
            }

            const auto to = to_w.lock();
            if (!to->CanEnter(region, unit)) {
                unit->Error("ENTER: Can't enter that.");
                continue;
            }

            auto forbid = to->ForbiddenBy(region, unit);
            if (!forbid.expired() && !o->advancing) {
                unit->Error("ENTER: Is refused entry.");
                continue;
            }

            if (!forbid.expired() && region->IsSafeRegion())
            {
                unit->Error("ENTER: No battles allowed in safe regions.");
                continue;
            }

            if (!forbid.expired() && !(unit->canattack && unit->IsAlive())) {
                unit->Error(AString("ENTER: Unable to attack ") +
                        forbid.lock()->name);
                continue;
            }

            bool done = false;
            while (!forbid.expired())
            {
                const auto forbid_p = forbid.lock();
                int result = RunBattle(region, unit, forbid_p, 0, 0);
                if (result == BATTLE_IMPOSSIBLE) {
                    unit->Error(AString("ENTER: Unable to attack ")+
                            forbid_p->name);
                    done = true;
                    break;
                }
                if (!unit->canattack || !unit->IsAlive()) {
                    done = true;
                    break;
                }
                forbid = to->ForbiddenBy(region, unit);
            }
            if (done) continue;

            unit->MoveUnit(to);
            unit->Event(AString("Enters ") + to->name + ".");
            obj = to;
        } else {
            if (i.isMoveOut()) {
                if (TerrainDefs[region->type].similar_type == Regions::Types::R_OCEAN &&
                        (!unit->CanSwim() ||
                         unit->GetFlag(FLAG_NOCROSS_WATER)))
                {
                    unit->Error("MOVE: Can't leave ship.");
                    continue;
                }

                auto to = region->GetDummy();
                unit->MoveUnit(to);
                obj = to.lock();
            }
        }
    }
}

Location::Handle Game::DoAMoveOrder(const Unit::Handle& unit,
                                    const ARegion::Handle& region,
                                    const Object::Handle& obj)
{
    const auto o = std::dynamic_pointer_cast<MoveOrder>(unit->monthorders);
    AString road, temp;

    if (o->dirs.empty()) {
        unit->monthorders.reset();
        return nullptr;
    }

    const auto& x = o->dirs.front();

    ARegion::Handle newreg;

    if (x->dir.isMoveIn()) {
        if (obj->inner == -1) {
            unit->Error("MOVE: Can't move IN there.");
            unit->monthorders.reset();
            return nullptr;
        }
        newreg = regions.GetRegion(static_cast<size_t>(obj->inner)).lock();
        if (obj->type == Objects::Types::O_GATEWAY) {
            // Gateways should only exist in the nexus, and move the
            // user to a semi-random instance of the target terrain
            // type, so select where they will actually move to.
            const auto level = regions.GetRegionArray(newreg->zloc);
            // match levels to try for, in order:
            // 0 - completely empty towns
            // 1 - towns with only guardsmen
            // 2 - towns with guardsmen and other players
            // 3 - completely empty hexes
            // 4 - anywhere that matches terrain (out of options)
            int match = 0;
            int candidates = 0;
            while (!candidates && match < 5) {
                for (unsigned int x = 0; x < level->x; x++)
                    for (unsigned int y = 0; y < level->y; y++) {
                        const auto scanReg_w = level->GetRegion(x, y);
                        if (scanReg_w.expired())
                        {
                            continue;
                        }
                        const auto scanReg = scanReg_w.lock();
                        if (TerrainDefs[scanReg->type].similar_type != TerrainDefs[newreg->type].similar_type)
                            continue;
                        if (match < 3 && !scanReg->town)
                            continue;
                        if (match == 4) {
                            candidates++;
                            continue;
                        }
                        bool guards = false;
                        bool others = false;
                        for(const auto& o: scanReg->objects) {
                            for(const auto& u: o->units) {
                                if (u->faction.lock()->num == guardfaction)
                                {
                                    guards = true;
                                }
                                else
                                {
                                    others = true;
                                }
                            }
                        }
                        switch (match) {
                            case 0:
                                if (guards || others)
                                {
                                    continue;
                                }
                                break;
                            case 1:
                                if (!guards || others)
                                {
                                    continue;
                                }
                                break;
                            case 2:
                                if (!guards)
                                {
                                    continue;
                                }
                                break;
                            case 3:
                                if (others)
                                {
                                    continue;
                                }
                                break;
                        }
                        candidates++;
                    }
                if (!candidates)
                    match++;
            }
            if (candidates) {
                candidates = getrandom(candidates);
                for (unsigned int x = 0; x < level->x; x++)
                    for (unsigned int y = 0; y < level->y; y++) {
                        const auto scanReg_w = level->GetRegion(x, y);
                        if (scanReg_w.expired())
                        {
                            continue;
                        }
                        const auto scanReg = scanReg_w.lock();
                        if (TerrainDefs[scanReg->type].similar_type != TerrainDefs[newreg->type].similar_type)
                            continue;
                        if (match < 3 && !scanReg->town)
                            continue;
                        if (match == 4) {
                            candidates++;
                            continue;
                        }
                        bool guards = false;
                        bool others = false;
                        for(const auto& o: scanReg->objects) {
                            for(const auto& u: o->units) {
                                if (u->faction.lock()->num == guardfaction)
                                {
                                    guards = true;
                                }
                                else
                                {
                                    others = true;
                                }
                            }
                        }
                        switch (match) {
                            case 0:
                                if (guards || others)
                                {
                                    continue;
                                }
                                break;
                            case 1:
                                if (!guards || others)
                                {
                                    continue;
                                }
                                break;
                            case 2:
                                if (!guards)
                                {
                                    continue;
                                }
                                break;
                            case 3:
                                if (others)
                                {
                                    continue;
                                }
                                break;
                        }
                        if (!candidates--) {
                            newreg = scanReg;
                        }
                    }
            }
        }
    } else if (x->dir.isMovePause()) {
        newreg = region;
    } else {
        newreg = region->neighbors[x->dir].lock();
    }

    if (!newreg) {
        unit->Error(AString("MOVE: Can't move that direction."));
        unit->monthorders.reset();
        return nullptr;
    }

    unit->movepoints += unit->CalcMovePoints(region);

    road = "";
    int startmove = 0;
    int movetype = unit->MoveType(region);
    unsigned int cost = newreg->MoveCost(movetype, *region, x->dir, road);
    if (x->dir.isMovePause())
    {
        cost = 1;
    }
    if (region->type == Regions::Types::R_NEXUS) {
        cost = 1;
        startmove = 1;
    }
    if ((TerrainDefs[region->type].similar_type == Regions::Types::R_OCEAN) &&
            (!unit->CanSwim() ||
            unit->GetFlag(FLAG_NOCROSS_WATER))) {
        unit->Error("MOVE: Can't move while in the ocean.");
        unit->monthorders.reset();
        return nullptr;
    }
    size_t weight = unit->items.Weight();
    if ((TerrainDefs[region->type].similar_type == Regions::Types::R_OCEAN) &&
            (TerrainDefs[newreg->type].similar_type != Regions::Types::R_OCEAN) &&
            !unit->CanWalk(weight) &&
            !unit->CanRide(weight) &&
            !unit->CanFly(weight)) {
        unit->Error("Must be able to walk to climb out of the ocean.");
        unit->monthorders.reset();
        return nullptr;
    }
    if (movetype == M_NONE) {
        unit->Error("MOVE: Unit is overloaded and cannot move.");
        unit->monthorders.reset();
        return nullptr;
    }

    // If we're moving in the same direction as last month and
    // have stored movement points, then add in those stored
    // movement points, but make sure that these are only used
    // towards entering the hex we were trying to enter
    if (!unit->moved &&
            unit->movepoints >= Globals->MAX_SPEED &&
            unit->movepoints < cost * Globals->MAX_SPEED &&
            x->dir == unit->savedmovedir) {
        while (unit->savedmovement > 0 &&
                unit->movepoints < cost * Globals->MAX_SPEED) {
            unit->movepoints += Globals->MAX_SPEED;
            unit->savedmovement--;
        }
        unit->savedmovement = 0;
        unit->savedmovedir = -1;
    }

    if (unit->movepoints < cost * Globals->MAX_SPEED)
    {
        return nullptr;
    }

    if (x->dir.isMovePause()) {
        unit->Event(AString("Pauses to admire the scenery in ") + region->ShortPrint(regions) + ".");
        unit->movepoints -= cost * Globals->MAX_SPEED;
        unit->moved += cost;
        o->dirs.pop_front();
        return nullptr;
    }

    if ((TerrainDefs[newreg->type].similar_type == Regions::Types::R_OCEAN) &&
            (!unit->CanSwim() ||
            unit->GetFlag(FLAG_NOCROSS_WATER))) {
        unit->Event(AString("Discovers that ") +
                newreg->ShortPrint(regions) + " is " +
                TerrainDefs[newreg->type].name + ".");
        unit->monthorders.reset();
        return nullptr;
    }

    if (unit->type == U_WMON && newreg->town && newreg->IsGuarded()) {
        unit->Event("Monsters don't move into guarded towns.");
        unit->monthorders.reset();
        return nullptr;
    }

    if (unit->guard == GUARD_ADVANCE) {
        const auto ally = newreg->ForbiddenByAlly(unit);
        if (!ally.expired() && !startmove) {
            unit->Event(AString("Can't ADVANCE: ") + newreg->name +
                        " is guarded by " + ally.lock()->name + ", an ally.");
            unit->monthorders.reset();
            return nullptr;
        }
    }

    if (o->advancing) unit->guard = GUARD_ADVANCE;

    const auto forbid_w = newreg->Forbidden(unit);
    if (!forbid_w.expired() && !startmove && unit->guard != GUARD_ADVANCE) {
        const auto forbid = forbid_w.lock();
        unsigned int obs = unit->GetAttribute("observation");
        unit->Event(AString("Is forbidden entry to ") +
                    newreg->ShortPrint(regions) + " by " +
                    forbid->GetName(obs) + ".");
        obs = forbid->GetAttribute("observation");
        forbid->Event(AString("Forbids entry to ") +
                    unit->GetName(obs) + ".");
        unit->monthorders.reset();
        return nullptr;
    }

    if (unit->guard == GUARD_GUARD) unit->guard = GUARD_NONE;

    unit->alias = 0;
    unit->movepoints -= cost * Globals->MAX_SPEED;
    unit->moved += cost;
    unit->MoveUnit(newreg->GetDummy());
    unit->DiscardUnfinishedShips();

    switch (movetype) {
        case M_WALK:
        default:
            temp = AString("Walks ") + road;
            break;
        case M_RIDE:
            temp = AString("Rides ") + road;
            unit->Practice(Skills::Types::S_RIDING);
            break;
        case M_FLY:
            temp = "Flies ";
            unit->Practice(Skills::Types::S_SUMMON_WIND);
            unit->Practice(Skills::Types::S_RIDING);
            break;
        case M_SWIM:
            temp = AString("Swims ");
            break;
    }
    unit->Event(temp + AString("from ") + region->ShortPrint(regions)
            + AString(" to ") + newreg->ShortPrint(regions) +
            AString("."));

    if (!forbid_w.expired()) {
        unit->advancefrom = region;
    }

    // TODO: Should we get a transit report on the starting region?
    if (Globals->TRANSIT_REPORT != GameDefs::REPORT_NOTHING) {
        if (!(unit->faction.lock()->IsNPC())) newreg->visited = 1;
        // Update our visit record in the region we are leaving.
        for(const auto& f: region->passers) {
            if (f->unit.lock() == unit) {
                // We moved into here this turn
                if (!x->dir.isMoveInOutOrEnter()) {
                    f->exits_used[x->dir] = true;
                }
            }
        }
        // And mark the hex being entered
        auto& f= newreg->passers.emplace_back(std::make_shared<Farsight>());
        f->faction = unit->faction;
        f->level = 0;
        f->unit = unit;
        if (!x->dir.isMoveInOutOrEnter()) {
            f->exits_used[region->GetRealDirComp(x->dir)] = true;
        }
    }

    o->dirs.pop_front();

    auto loc = std::make_shared<Location>();
    loc->unit = unit;
    loc->region = newreg;
    loc->obj.reset();
    return loc;

}
