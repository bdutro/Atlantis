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

#include "game.h"
#include "gamedata.h"
#include "quests.h"

void Game::RunOrders()
{
    //
    // Form and instant orders are handled during parsing
    //
    Awrite("Running FIND Orders...");
    RunFindOrders();
    Awrite("Running ENTER/LEAVE Orders...");
    RunEnterOrders(0);
    Awrite("Running PROMOTE/EVICT Orders...");
    RunPromoteOrders();
    Awrite("Running Combat...");
    DoAttackOrders();
    DoAutoAttacks();
    Awrite("Running STEAL/ASSASSINATE Orders...");
    RunStealOrders();
    Awrite("Running GIVE Orders...");
    DoGiveOrders();
    Awrite("Running ENTER NEW Orders...");
    RunEnterOrders(1);
    Awrite("Running EXCHANGE Orders...");
    DoExchangeOrders();
    Awrite("Running DESTROY Orders...");
    RunDestroyOrders();
    Awrite("Running PILLAGE Orders...");
    RunPillageOrders();
    Awrite("Running TAX Orders...");
    RunTaxOrders();
    Awrite("Running GUARD 1 Orders...");
    DoGuard1Orders();
    Awrite("Running Magic Orders...");
    ClearCastEffects();
    RunCastOrders();
    Awrite("Running SELL Orders...");
    RunSellOrders();
    Awrite("Running BUY Orders...");
    RunBuyOrders();
    Awrite("Running FORGET Orders...");
    RunForgetOrders();
    Awrite("Mid-Turn Processing...");
    MidProcessTurn();
    Awrite("Running QUIT Orders...");
    RunQuitOrders();
    Awrite("Removing Empty Units...");
    DeleteEmptyUnits();
    // SinkUncrewedFleets();
    // DrownUnits();
    if (Globals->ALLOW_WITHDRAW) {
        Awrite("Running WITHDRAW Orders...");
        DoWithdrawOrders();
    }
/*
    Awrite("Running Sail Orders...");
    RunSailOrders();
    Awrite("Running Move Orders...");
    RunMoveOrders();
*/
    Awrite("Running Consolidated Movement Orders...");
    RunMovementOrders();

    SinkUncrewedFleets();
    DrownUnits();
    FindDeadFactions();
    Awrite("Running Teach Orders...");
    RunTeachOrders();
    Awrite("Running Month-long Orders...");
    RunMonthOrders();
    RunTeleportOrders();
    if (Globals->TRANSPORT & GameDefs::ALLOW_TRANSPORT) {
        Awrite("Running Transport Orders...");
        CheckTransportOrders();
        RunTransportOrders();
    }
    Awrite("Assessing Maintenance costs...");
    AssessMaintenance();
    if (Globals->DYNAMIC_POPULATION) {
        Awrite("Processing Migration...");
        ProcessMigration();
    }
    Awrite("Post-Turn Processing...");
    PostProcessTurn();
    DeleteEmptyUnits();
    // EmptyHell(); moved to Game::RunGame()
    // to prevent dead mages from causing
    // a segfault with IMPROVED_FARSIGHT
    RemoveEmptyObjects();
}

void Game::ClearCastEffects()
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                u->SetFlag(FLAG_INVIS, 0);
            }
        }
    }
}

void Game::RunCastOrders()
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->castorders) {
                    RunACastOrder(r, o, u);
                    u->castorders.reset();
                }
            }
        }
    }
}

/* Moved to game.cpp, where it belongs...
int Game::CountMages(Faction *pFac)
{
    int i = 0;
    forlist(&regions) {
        ARegion *r = (ARegion *) elem;
        forlist(&r->objects) {
            Object *o = (Object *) elem;
            forlist(&o->units) {
                Unit *u = (Unit *) elem;
                if (u->faction == pFac && u->type == U_MAGE) i++;
            }
        }
    }
    return(i);
}
*/

bool Game::TaxCheck(const ARegion::Handle& pReg, const Faction::Handle& pFac)
{
    if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES) {
        const int allowed_taxes = AllowedTaxes(*pFac);
        if (allowed_taxes == -1) {
            //
            // No limit.
            //
            return true;
        }

        for(const auto& x: pFac->war_regions) {
            if (x.lock() == pReg) {
                //
                // This faction already performed a tax action in this
                // region.
                //
                return true;
            }
        }
        if (static_cast<int>(pFac->war_regions.size()) >= allowed_taxes) {
            //
            // Can't tax here.
            //
            return false;
        } else {
            //
            // Add this region to the faction's tax list.
            //
            pFac->war_regions.emplace_back(pReg);
            return true;
        }
    } else {
        //
        // No limit on taxing regions in this game.
        //
        return true;
    }
}

bool Game::TradeCheck(const ARegion::Handle& pReg, const Faction::Handle& pFac)
{
    if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES) {
        const int allowed_trades = AllowedTrades(*pFac);
        if (allowed_trades == -1) {
            //
            // No limit on trading on this faction.
            //
            return true;
        }

        for(const auto& x: pFac->trade_regions) {
            if (x.lock() == pReg) {
                //
                // This faction has already performed a trade action in this
                // region.
                //
                return true;
            }
        }
        if (static_cast<int>(pFac->trade_regions.size()) >= allowed_trades) {
            //
            // This faction is over its trade limit.
            //
            return false;
        } else {
            //
            // Add this region to the faction's trade list, and return 1.
            //
            pFac->trade_regions.emplace_back(pReg);
            return true;
        }
    } else {
        //
        // No limit on trade in this game.
        //
        return true;
    }
}

void Game::RunStealOrders()
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->stealorders) {
                    if (u->stealorders->type == Orders::Types::O_STEAL) {
                        Do1Steal(r, o, u);
                    } else if (u->stealorders->type == Orders::Types::O_ASSASSINATE) {
                        Do1Assassinate(r, o, u);
                    }
                    u->stealorders.reset();
                }
            }
        }
    }
}

WeakPtrList<Faction> Game::CanSeeSteal(const ARegion::Handle& r, const Unit::Handle& u)
{
    WeakPtrList<Faction> retval;
    for(const auto& f: factions)
    {
        if (r->Present(*f))
        {
            if (f->CanSee(*r, u, Globals->SKILL_PRACTICE_AMOUNT > 0))
            {
                retval.push_back(f);
            }
        }
    }
    return retval;
}

void Game::Do1Assassinate(const ARegion::Handle& r, const Object::Handle&, const Unit::Handle& u)
{
    const auto so = std::dynamic_pointer_cast<AssassinateOrder>(u->stealorders);
    const auto tar_w = r->GetUnitId(so->target, u->faction.lock()->num);

    if (tar_w.expired()) {
        u->Error("ASSASSINATE: Invalid unit given.");
        return;
    }

    const auto tar = tar_w.lock();
    if (!tar->IsAlive()) {
        u->Error("ASSASSINATE: Invalid unit given.");
        return;
    }

    // New rule -- You can only assassinate someone you can see
    if (!u->CanSee(*r, tar)) {
        u->Error("ASSASSINATE: Invalid unit given.");
        return;
    }

    if (tar->type == U_GUARD || tar->type == U_WMON ||
            tar->type == U_GUARDMAGE) {
        u->Error("ASSASSINATE: Can only assassinate other player's "
                "units.");
        return;
    }

    if (u->GetMen() != 1) {
        u->Error("ASSASSINATE: Must be executed by a 1-man unit.");
        return;
    }

    const WeakPtrList<Faction> seers = CanSeeSteal(r, u);
    int succ = 1;
    const auto tar_fac = tar->faction.lock();

    for(const auto& f_w: seers) {
        const auto f = f_w.lock();
        if (f == tar_fac) {
            succ = 0;
            break;
        }
        if (f->GetAttitude(tar_fac->num) == A_ALLY) {
            succ = 0;
            break;
        }
        if (f->num == guardfaction) {
            succ = 0;
            break;
        }
    }
    if (!succ) {
        AString temp = *(u->name) + " is caught attempting to assassinate " +
            *(tar->name) + " in " + *(r->name) + ".";
        for(const auto& f: seers) {
            f.lock()->Event(temp);
        }
        // One learns from one's mistakes.  Surviving them is another matter!
        u->PracticeAttribute("stealth");
        return;
    }

    int ass = 1;
    if (u->items.GetNum(Items::Types::I_RINGOFI)) {
        ass = 2; // Check if assassin has a ring.
        // New rule: if a target has an amulet of true seeing they
        // cannot be assassinated by someone with a ring of invisibility
        if (tar->AmtsPreventCrime(u)) {
            tar->Event("Assassination prevented by amulet of true seeing.");
            u->Event(AString("Attempts to assassinate ") + *(tar->name) +
                    ", but is prevented by amulet of true seeing.");
            return;
        }
    }
    u->PracticeAttribute("stealth");
    RunBattle(r, u, tar, ass);
}

void Game::Do1Steal(const ARegion::Handle& r, const Object::Handle&, const Unit::Handle& u)
{
    const auto so = std::dynamic_pointer_cast<StealOrder>(u->stealorders);
    const auto tar_w = r->GetUnitId(so->target, u->faction.lock()->num);

    if (tar_w.expired()) {
        u->Error("STEAL: Invalid unit given.");
        return;
    }

    const auto tar = tar_w.lock();

    // New RULE!! You can only steal from someone you can see.
    if (!u->CanSee(*r, tar)) {
        u->Error("STEAL: Invalid unit given.");
        return;
    }

    if (tar->type == U_GUARD || tar->type == U_WMON ||
            tar->type == U_GUARDMAGE) {
        u->Error("STEAL: Can only steal from other player's "
                "units.");
        return;
    }

    if (u->GetMen() != 1) {
        u->Error("STEAL: Must be executed by a 1-man unit.");
        return;
    }

    const auto tar_fac = tar->faction.lock();
    const WeakPtrList<Faction> seers = CanSeeSteal(r, u);
    bool succ = true;

    for(const auto& f_w: seers) {
        const auto f = f_w.lock();
        if (f == tar_fac) {
            succ = false;
            break;
        }
        if (f->GetAttitude(tar_fac->num) == A_ALLY) {
            succ = false;
            break;
        }
        if (f->num == guardfaction) {
            succ = false;
            break;
        }
    }

    if (!succ) {
        AString temp = *(u->name) + " is caught attempting to steal from " +
            *(tar->name) + " in " + *(r->name) + ".";
        for(const auto& f: seers) {
            f.lock()->Event(temp);
        }
        // One learns from one's mistakes.  Surviving them is another matter!
        u->PracticeAttribute("stealth");
        return;
    }

    //
    // New rule; if a target has an amulet of true seeing they can't be
    // stolen from by someone with a ring of invisibility
    //
    if (tar->AmtsPreventCrime(u)) {
        tar->Event("Theft prevented by amulet of true seeing.");
        u->Event(AString("Attempts to steal from ") + *(tar->name) + ", but "
                "is prevented by amulet of true seeing.");
        return;
    }

    size_t amt = 1;
    if (so->item == Items::Types::I_SILVER) {
        amt = tar->GetMoney();
        if (amt < 400) {
            amt = amt / 2;
        } else {
            amt = 200;
        }
    }

    if (tar->items.GetNum(so->item) < amt) amt = 0;

    u->items.SetNum(so->item, u->items.GetNum(so->item) + amt);
    tar->items.SetNum(so->item, tar->items.GetNum(so->item) - amt);

    {
        AString temp = *(u->name) + " steals " +
            ItemString(so->item, amt) + " from " + *(tar->name) + ".";
        for(const auto& f: seers) {
            f.lock()->Event(temp);
        }
    }

    tar->Event(AString("Has ") + ItemString(so->item, amt) + " stolen.");
    u->PracticeAttribute("stealth");
    return;
}

void Game::DrownUnits()
{
    for(const auto& r: regions) {
        if (TerrainDefs[r->type].similar_type == Regions::Types::R_OCEAN) {
            for(const auto& o: r->objects) {
                if (o->type != Objects::Types::O_DUMMY) continue;
                for(const auto& u: o->units) {
                    int drown = 0;
                    switch(Globals->FLIGHT_OVER_WATER) {
                        case GameDefs::WFLIGHT_UNLIMITED:
                            drown = !(u->CanSwim());
                            break;
                        case GameDefs::WFLIGHT_MUST_LAND:
                            drown = !u->CanReallySwim();
                            break;
                        case GameDefs::WFLIGHT_NONE:
                            drown = !(u->CanReallySwim());
                            break;
                        default: // Should never happen
                            drown = 1;
                            break;
                    }
                    if (drown) {
                        r->Kill(u);
                        u->Event("Drowns in the ocean.");
                    }
                }
            }
        }
    }
}

void Game::SinkUncrewedFleets()
{
    for(const auto& r: regions) {
        r->CheckFleets();
    }
}

void Game::RunForgetOrders()
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                for(const auto& fo: u->forgetorders) {
                    u->ForgetSkill(fo->skill);
                    u->Event(AString("Forgets ") + SkillStrs(fo->skill) + ".");
                }
                u->forgetorders.clear();
            }
        }
    }
}

void Game::RunQuitOrders()
{
    for(const auto& f: factions) {
        if (f->quit)
            Do1Quit(f);
    }
}

void Game::Do1Quit(const Faction::Handle& f)
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->faction.lock() == f) {
                    r->Kill(u);
                }
            }
        }
    }
}

void Game::RunDestroyOrders()
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            const auto u_w = o->GetOwner();
            if (!u_w.expired()) 
            {
                const auto u = u_w.lock();
                if (u->destroy)
                {
                    Do1Destroy(r, o, u);
                    continue;
                }
                else
                {
                    for(const auto& e: o->units)
                        e->destroy = 0;
                }
            }
        }
    }
}

void Game::Do1Destroy(const ARegion::Handle& r,
                      const Object::Handle& o,
                      const Unit::Handle& u)
{
    if (TerrainDefs[r->type].similar_type == Regions::Types::R_OCEAN) {
        u->Error("DESTROY: Can't destroy a ship while at sea.");
        for(const auto& u: o->units) {
            u->destroy = 0;
        }
        return;
    }

    if (!u->GetMen()) {
        u->Error("DESTROY: Empty units cannot destroy structures.");
        for(const auto& u: o->units) {
            u->destroy = 0;
        }
        return;
    }

    if (o->CanModify()) {
        u->Event(AString("Destroys ") + *(o->name) + ".");
        const auto dest = r->GetDummy();
        for(const auto& u: o->units) {
            u->destroy = 0;
            u->MoveUnit(dest);
        }
        if (quests.CheckQuestDemolishTarget(r, o->num, u)) {
            u->Event("You have completed a quest!");
        }
        r->RemoveObject(o);
    } else {
        u->Error("DESTROY: Can't destroy that.");
        for(const auto& u: o->units) {
            u->destroy = 0;
        }
    }
}

void Game::RunFindOrders()
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                RunFindUnit(u);
            }
        }
    }
}

void Game::RunFindUnit(const Unit::Handle& u)
{
    bool all = false;
    const auto u_fac = u->faction.lock();

    for(const auto& f: u->findorders) {
        if (f->find == 0) all = true;
        if (!all) {
            const auto fac = GetFaction(factions, static_cast<size_t>(f->find));
            if (fac) {
                u_fac->Event(AString("The address of ") + *(fac->name) +
                        " is " + *(fac->address) + ".");
            } else {
                u->Error(AString("FIND: ") + f->find + " is not a valid "
                        "faction number.");
            }
        } else {
            for(const auto& fac: factions) {
                if (fac) {
                    u_fac->Event(AString("The address of ") +
                            *(fac->name) + " is " + *(fac->address) + ".");
                }
            }
        }
    }
    u->findorders.clear();
}

void Game::RunTaxOrders()
{
    for(const auto& r: regions) {
        RunTaxRegion(r);
    }
}

int Game::FortTaxBonus(const Object::Handle& o, const Unit::Handle& u)
{
    int protect = ObjectDefs[o->type].protect;
    int fortbonus = 0;
    for(const auto& unit: o->units) {
        size_t men = unit->GetMen();
        if (unit->num == u->num) {
            if (unit->taxing == TAX_TAX) {
                int fortbonus = static_cast<int>(men);
                const size_t maxtax = unit->Taxers(1);
                if (fortbonus > protect)
                {
                    fortbonus = protect;
                }
                if (fortbonus > static_cast<int>(maxtax))
                {
                    fortbonus = static_cast<int>(maxtax);
                }
                fortbonus *= static_cast<int>(Globals->TAX_BONUS_FORT);
                return fortbonus;
            }
        }
        protect -= static_cast<int>(men);
        if (protect < 0) protect = 0;    
    }
    return fortbonus;
}

int Game::CountTaxes(const ARegion::Handle& reg)
{
    int t = 0;
    for(const auto& o: reg->objects) {
        int protect = ObjectDefs[o->type].protect;
        for(const auto& u: o->units) {
            if (u->GetFlag(FLAG_AUTOTAX) && !Globals->TAX_PILLAGE_MONTH_LONG)
            {
                u->taxing = TAX_TAX;
            }
            if (u->taxing == TAX_AUTO)
            {
                u->taxing = TAX_TAX;
            }
            if (u->taxing == TAX_TAX) {
                if (reg->Population() < 1) {
                    u->Error("TAX: No population to tax.");
                    u->taxing = TAX_NONE;
                } else if (!reg->CanTax(u)) {
                    u->Error("TAX: A unit is on guard.");
                    u->taxing = TAX_NONE;
                } else {
                    const size_t men = u->Taxers(0);
                    int u_men = static_cast<int>(u->GetMen());
                    int fortbonus = u_men;
                    if (fortbonus > protect)
                    {
                        fortbonus = protect;
                    }
                    protect -= u_men;
                    if (protect < 0)
                    {
                        protect = 0;
                    }
                    if (men) {
                        if (!TaxCheck(reg, u->faction.lock())) {
                            u->Error("TAX: Faction can't tax that many "
                                    "regions.");
                            u->taxing = TAX_NONE;
                        } else {
                            t += static_cast<int>(men) + fortbonus * static_cast<int>(Globals->TAX_BONUS_FORT);
                        }
                    } else {
                        u->Error("TAX: Unit cannot tax.");
                        u->taxing = TAX_NONE;
                        u->SetFlag(FLAG_AUTOTAX, 0);
                    }
                }
            }
        }
    }
    return t;
}

void Game::RunTaxRegion(const ARegion::Handle& reg)
{
    int desired = CountTaxes(reg);
    if (desired < reg->wealth) desired = reg->wealth;

    for(const auto& o: reg->objects) {
        for(const auto& u: o->units) {
            if (u->taxing == TAX_TAX) {
                int t = static_cast<int>(u->Taxers(0));
                t += FortTaxBonus(o, u);
                double fAmt = static_cast<double>(t) *
                    static_cast<double>(reg->wealth) / static_cast<double>(desired);
                int amt = static_cast<int>(fAmt);
                reg->wealth -= amt;
                desired -= t;
                u->SetMoney(u->GetMoney() + static_cast<size_t>(amt));
                u->Event(AString("Collects $") + amt + " in taxes in " +
                        reg->ShortPrint(regions) + ".");
                u->taxing = TAX_NONE;
            }
        }
    }
}

void Game::RunPillageOrders()
{
    for(const auto& r: regions) {
        RunPillageRegion(r);
    }
}

int Game::CountPillagers(const ARegion::Handle& reg)
{
    int p = 0;
    for(const auto& o: reg->objects) {
        for(const auto& u: o->units) {
            if (u->taxing == TAX_PILLAGE) {
                if (!reg->CanPillage(u)) {
                    u->Error("PILLAGE: A unit is on guard.");
                    u->taxing = TAX_NONE;
                } else {
                    int men = static_cast<int>(u->Taxers(1));
                    if (men) {
                        if (!TaxCheck(reg, u->faction.lock())) {
                            u->Error("PILLAGE: Faction can't tax that many "
                                    "regions.");
                            u->taxing = TAX_NONE;
                        } else {
                            p += men;
                        }
                    } else {
                        u->Error("PILLAGE: Not a combat ready unit.");
                        u->taxing = TAX_NONE;
                    }
                }
            }
        }
    }
    return p;
}

void Game::ClearPillagers(const ARegion::Handle& reg)
{
    for(const auto& o: reg->objects) {
        for(const auto& u: o->units) {
            if (u->taxing == TAX_PILLAGE) {
                u->Error("PILLAGE: Not enough men to pillage.");
                u->taxing = TAX_NONE;
            }
        }
    }
}

void Game::RunPillageRegion(const ARegion::Handle& reg)
{
    if (TerrainDefs[reg->type].similar_type == Regions::Types::R_OCEAN)
    {
        return;
    }
    if (reg->wealth < 1)
    {
        return;
    }
    if (reg->Wages() <= static_cast<int>(10 * Globals->MAINTENANCE_COST))
    {
        return;
    }

    /* First, count up pillagers */
    int pillagers = CountPillagers(reg);

    if (pillagers * 2 * static_cast<int>(Globals->TAX_BASE_INCOME) < reg->wealth) {
        ClearPillagers(reg);
        return;
    }

    const auto facs = reg->PresentFactions();
    int amt = reg->wealth * 2;
    for(const auto& o: reg->objects) {
        for(const auto& u: o->units) {
            if (u->taxing == TAX_PILLAGE) {
                u->taxing = TAX_NONE;
                int num = static_cast<int>(u->Taxers(1));
                int temp = (amt * num)/pillagers;
                amt -= temp;
                pillagers -= num;
                u->SetMoney(u->GetMoney() + static_cast<size_t>(temp));
                u->Event(AString("Pillages $") + temp + " from " +
                        reg->ShortPrint(regions) + ".");
                const auto u_fac = u->faction.lock();
                for(const auto& fp_w: facs) {
                    const auto fp = fp_w.lock();
                    if (fp != u_fac) {
                        fp->Event(*(u->name) + " pillages " +
                                *(reg->name) + ".");
                    }
                }
            }
        }
    }

    /* Destroy economy */
    reg->Pillage();
}

void Game::RunPromoteOrders()
{
    /* First, do any promote orders */
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            if (o->type != Objects::Types::O_DUMMY) {
                const auto u_w = o->GetOwner();
                if (!u_w.expired())
                {
                    const auto u = u_w.lock();
                    if(u->promote)
                    {
                        Do1PromoteOrder(o, u);
                        u->promote.reset();
                    }
                }
            }
        }
    }
    /* Now do any evict orders */
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            if (o->type != Objects::Types::O_DUMMY) {
                const auto u_w = o->GetOwner();
                if (!u_w.expired())
                {
                    const auto u = u_w.lock();
                    if(u->evictorders)
                    {
                        Do1EvictOrder(o, u);
                        u->evictorders.reset();
                    }
                }
            }
        }
    }

    /* Then, clear out other promote/evict orders */
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->promote) {
                    if (o->type != Objects::Types::O_DUMMY) {
                        u->Error("PROMOTE: Must be owner");
                        u->promote.reset();
                    } else {
                        u->Error("PROMOTE: Can only promote inside structures.");
                        u->promote.reset();
                    }
                }
                if (u->evictorders) {
                    if (o->type != Objects::Types::O_DUMMY) {
                        u->Error("EVICT: Must be owner");
                        u->evictorders.reset();
                    } else {
                        u->Error("EVICT: Can only evict inside structures.");
                        u->evictorders.reset();
                    }
                }
            }
        }
    }
}

void Game::Do1PromoteOrder(const Object::Handle& obj, const Unit::Handle& u)
{
    const auto tar_w = obj->GetUnitId(*u->promote, u->faction.lock()->num);
    if (tar_w.expired()) {
        u->Error("PROMOTE: Can't find target.");
        return;
    }

    const auto tar = tar_w.lock();
    obj->RemoveUnit(tar);
    obj->units.push_back(tar);
}

void Game::Do1EvictOrder(const Object::Handle& obj, const Unit::Handle& u)
{
    auto ord = u->evictorders;
    const size_t u_fac_num = u->faction.lock()->num;
    const auto obj_reg = obj->region.lock();

    obj_reg->DeduplicateUnitList(ord->targets, u_fac_num);
    while (ord && !ord->targets.empty()) {
        const auto& id = ord->targets.front();
        const auto tar_w = obj->GetUnitId(id, u_fac_num);
        ord->targets.pop_front();

        if (tar_w.expired())
        {
            continue;
        }

        const auto tar = tar_w.lock();
        if (obj->IsFleet() &&
            (TerrainDefs[obj_reg->type].similar_type == Regions::Types::R_OCEAN) &&
            (!tar->CanReallySwim() || tar->GetFlag(FLAG_NOCROSS_WATER))) {
            u->Error("EVICT: Cannot forcibly evict units over ocean.");
            continue;
        }
        const auto to = obj_reg->GetDummy();
        tar->MoveUnit(to);
        tar->Event(AString("Evicted from ") + *obj->name + " by " + *u->name);
        u->Event(AString("Evicted ") + *tar->name + " from " + *obj->name);
    }
}

/* RunEnterOrders is performed in TWO phases: one in the
 * instant orders phase for existing objects and one after
 * give orders for entering new objects (fleets).
 */
void Game::RunEnterOrders(int phase)
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                // normal enter phase or ENTER NEW / JOIN phase?
                if (phase == 0)
                {
                    if (u->enter > 0 || u->enter == -1)
                    {
                        Do1EnterOrder(r, o, u);
                    }
                }
                else
                {
                    if (u->joinorders)
                    {
                        Do1JoinOrder(r, o, u);
                    }
                }
            }
        }
    }
}

void Game::Do1EnterOrder(const ARegion::Handle& r,
                         const Object::Handle&,
                         const Unit::Handle& u)
{
    Object::Handle to;
    if (u->enter == -1) {
        to = r->GetDummy().lock();
        u->enter = 0;
        if ((TerrainDefs[r->type].similar_type == Regions::Types::R_OCEAN) &&
                (!u->CanSwim() || u->GetFlag(FLAG_NOCROSS_WATER))) {
            u->Error("LEAVE: Can't leave a ship in the ocean.");
            return;
        }
    } else {
        int on = u->enter;
        to = r->GetObject(on).lock();
        u->enter = 0;
        if (!to) {
            u->Error("ENTER: Can't enter that.");
            return;
        }
        if (!to->CanEnter(r, u)) {
            u->Error("ENTER: Can't enter that.");
            return;
        }
        if (!to->ForbiddenBy(r, u).expired()) {
            u->Error("ENTER: Is refused entry.");
            return;
        }
    }
    u->MoveUnit(to);
}

void Game::Do1JoinOrder(const ARegion::Handle& r,
                        const Object::Handle&,
                        const Unit::Handle& u)
{
    const auto jo = std::dynamic_pointer_cast<JoinOrder>(u->joinorders);
    const auto tar_w = r->GetUnitId(jo->target, u->faction.lock()->num);

    if (tar_w.expired()) {
        u->Error("JOIN: No such unit.");
        return;
    }

    const auto tar = tar_w.lock();
    const auto& to_w = tar->object;
    if (to_w.expired()) {
        u->Error("JOIN: Can't enter that.");
        return;
    }

    const auto to = to_w.lock();
    const auto u_obj = u->object.lock();
    if (u_obj == to) {
        // We're already there!
        return;
    }

    if (jo->merge) {
        if (!u_obj->IsFleet() ||
                u_obj->GetOwner().lock()->num != u->num) {
            u->Error("JOIN MERGE: Not fleet owner.");
            return;
        }
        if (!to->IsFleet()) {
            u->Error("JOIN MERGE: Target unit is not in a fleet.");
            return;
        }
        for(const auto& pass: u_obj->units) {
            if (!to->ForbiddenBy(r, pass).expired()) {
                u->Error("JOIN MERGE: A unit would be refused entry.");
                return;
            }
        }
        for(const auto& item: u_obj->ships) {
            GiveOrder go;
            UnitId id;
            go.amount = static_cast<int>(item->num);
            go.except = 0;
            go.item = static_cast<ssize_t>(item->type);
            id.unitnum = to->GetOwner().lock()->num;
            id.alias = 0;
            id.faction = 0;
            go.target = id;
            go.type = Orders::Types::O_GIVE;
            go.merge = 1;
            DoGiveOrder(r, u, go);
            go.target.invalidate();
        }
        for(const auto& pass: u_obj->units) {
            pass->MoveUnit(to);
        }

        return;
    }

    if (to == r->GetDummy().lock()) {
        if ((TerrainDefs[r->type].similar_type == Regions::Types::R_OCEAN) &&
                (!u->CanSwim() || u->GetFlag(FLAG_NOCROSS_WATER))) {
            u->Error("JOIN: Can't leave a ship in the ocean.");
            return;
        }
    } else {
        if (!to->CanEnter(r, u)) {
            u->Error("JOIN: Can't enter that.");
            return;
        }
        if (!to->ForbiddenBy(r, u).expired()) {
            u->Error("JOIN: Is refused entry.");
            return;
        }
        if (to->IsFleet() &&
                !jo->overload &&
                to->FleetCapacity() < static_cast<size_t>(to->FleetLoad()) + u->Weight()) {
            u->Event("JOIN: Fleet would be overloaded.");
            return;
        }
    }
    u->MoveUnit(to);
}

void Game::RemoveEmptyObjects()
{
    for(const auto& r: regions) {
        auto it = r->objects.begin();
        while(it != r->objects.end()) {
            const auto& o = *it;
            if ((o->IsFleet()) && 
                (TerrainDefs[r->type].similar_type != Regions::Types::R_OCEAN))
            {
                ++it;
                continue;
            }
            if (ObjectDefs[o->type].cost &&
                    o->incomplete >= ObjectDefs[o->type].cost) {
                for(const auto& u: o->units) {
                    u->MoveUnit(r->GetDummy());
                }
                it = r->objects.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void Game::EmptyHell()
{
    for(const auto& r: regions)
    {
        r->ClearHell();
    }
}

void Game::MidProcessUnit(const ARegion::Handle& r, const Unit::Handle& u)
{
    MidProcessUnitExtra(r, u);
}

void Game::PostProcessUnit(const ARegion::Handle& r, const Unit::Handle& u)
{
    PostProcessUnitExtra(r, u);
}

void Game::EndGame(const Faction::Handle& pVictor)
{
    for(const auto& pFac: factions) {
        pFac->exists = 0;
        if (pFac == pVictor)
            pFac->quit = QUIT_WON_GAME;
        else
            pFac->quit = QUIT_GAME_OVER;

        if (pVictor)
            pFac->Event(*(pVictor->name) + " has won the game!");
        else
            pFac->Event("The game has ended with no winner.");
    }

    gameStatus = GAME_STATUS_FINISHED;
}

void Game::MidProcessTurn()
{
    for(const auto& r: regions) {
        // r->MidTurn(); // Not yet implemented
        /* regional population dynamics */
        if (Globals->DYNAMIC_POPULATION)
        {
            r->Grow();
        }
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                MidProcessUnit(r, u);
            }
        }
    }
}

/* Process Migration if DYNAMIC_POPULATION
 * is set. */
void Game::ProcessMigration()
{
    // This return seems like a bug...
    //return;
    /* process two "phases" of migration
     * allowing a region to spread it's migration
     * between different destinations. */
    for (int phase = 1; phase <=2; phase++) {
        for(const auto& r: regions) {
            r->FindMigrationDestination(phase);
        }
        /* should always be true, but we need a
         * different scope for AList handling, anyway */
        if (Globals->DYNAMIC_POPULATION) {
            for(const auto& r: regions) {
                r->Migrate();
            }
        }
    }
}

void Game::PostProcessTurn()
{
    //
    // Check if there are any factions left.
    //
    bool livingFacs = false;
    for(const auto& pFac: factions) {
        if (pFac->exists) {
            livingFacs = true;
            break;
        }
    }

    if (!livingFacs)
    {
        EndGame(0);
    }
    else if (!(Globals->OPEN_ENDED)) {
        const auto pVictor = CheckVictory();
        if (!pVictor.expired())
        {
            EndGame(pVictor.lock());
        }
    }

    for(const auto& r: regions) {
        r->PostTurn(regions);

        if (Globals->CITY_MONSTERS_EXIST && (r->town || r->type == Regions::Types::R_NEXUS))
            AdjustCityMons(r);

        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                PostProcessUnit(r, u);
            }
        }
    }

    if (Globals->WANDERING_MONSTERS_EXIST) GrowWMons(Globals->WMON_FREQUENCY);

    if (Globals->LAIR_MONSTERS_EXIST) GrowLMons(Globals->LAIR_FREQUENCY);

    if (Globals->LAIR_MONSTERS_EXIST) GrowVMons();
}

void Game::DoAutoAttacks()
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->canattack && u->IsAlive())
                    DoAutoAttack(r, u);
            }
        }
    }
}

void Game::DoMovementAttacks(const PtrList<Location>& locs)
{
    for(const auto& l: locs) {
        if (!l->obj.expired()) {
            const auto l_obj = l->obj.lock();
            for(const auto& u: l_obj->units) {
                DoMovementAttack(l->region.lock(), u);
            }
        } else {
            DoMovementAttack(l->region.lock(), l->unit.lock());
        }
    }
}

void Game::DoMovementAttack(const ARegion::Handle& r, const Unit::Handle& u)
{
    if (u->canattack && u->IsAlive()) {
        DoAutoAttack(r, u);
        if (!u->canattack || !u->IsAlive()) {
            u->guard = GUARD_NONE;
        }
    }
    if (u->canattack && u->guard == GUARD_ADVANCE && u->IsAlive()) {
        DoAdvanceAttack(r, u);
        u->guard = GUARD_NONE;
    }
    if (u->IsAlive()) {
        DoAutoAttackOn(r, u);
        if (!u->canattack || !u->IsAlive()) {
            u->guard = GUARD_NONE;
        }
    }
}

void Game::DoAutoAttackOn(const ARegion::Handle& r, const Unit::Handle& t)
{
    for(const auto& o: r->objects) {
        for(const auto& u: o->units) {
            if (u->guard != GUARD_AVOID &&
                    (u->GetAttitude(*r, t) == A_HOSTILE) && u->IsAlive() &&
                    u->canattack)
            {
                AttemptAttack(r, u, t, 1);
            }
            if (!t->IsAlive())
            {
                return;
            }
        }
    }
}

void Game::DoAdvanceAttack(const ARegion::Handle& r, const Unit::Handle& u) {
    auto t_w = r->Forbidden(u);
    while (!t_w.expired() && u->canattack && u->IsAlive()) {
        const auto t = t_w.lock();
        AttemptAttack(r, u, t, 1, 1);
        t_w = r->Forbidden(u);
    }
}

void Game::DoAutoAttack(const ARegion::Handle& r, const Unit::Handle& u) {
    if (u->guard == GUARD_AVOID)
        return;
    for(const auto& o: r->objects) {
        for(const auto& t: o->units) {
            if (u->GetAttitude(*r, t) == A_HOSTILE) {
                AttemptAttack(r, u, t, 1);
            }
            if (u->canattack == 0 || u->IsAlive() == 0)
                return;
        }
    }
}

size_t Game::CountWMonTars(const ARegion::Handle& r, const Unit::Handle& mon) {
    size_t retval = 0;
    for(const auto& o: r->objects) {
        for(const auto& u: o->units) {
            if (u->type == U_NORMAL || u->type == U_MAGE ||
                    u->type == U_APPRENTICE) {
                if (mon->CanSee(*r, u) && mon->CanCatch(*r, u)) {
                    retval += u->GetMen();
                }
            }
        }
    }
    return retval;
}

Unit::WeakHandle Game::GetWMonTar(const ARegion::Handle& r,
                                  int tarnum,
                                  const Unit::Handle& mon) {
    for(const auto& o: r->objects) {
        for(const auto& u: o->units) {
            if (u->type == U_NORMAL || u->type == U_MAGE ||
                    u->type == U_APPRENTICE) {
                if (mon->CanSee(*r, u) && mon->CanCatch(*r, u)) {
                    size_t num = u->GetMen();
                    if (num && tarnum < static_cast<int>(num)) return u;
                    tarnum -= static_cast<int>(num);
                }
            }
        }
    }
    return Unit::WeakHandle();
}

void Game::CheckWMonAttack(const ARegion::Handle& r, const Unit::Handle& u) {
    int tars = static_cast<int>(CountWMonTars(r, u));
    if (!tars)
    {
        return;
    }

    int rand = 300 - tars;
    if (rand < 100)
    {
        rand = 100;
    }
    if (getrandom(rand) >= u->Hostile())
    {
        return;
    }

    const auto t = GetWMonTar(r, getrandom(tars), u);
    if (!t.expired())
    {
        AttemptAttack(r, u, t.lock(), 1);
    }
}

void Game::DoAttackOrders()
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->type == U_WMON) {
                    if (u->canattack && u->IsAlive()) {
                        CheckWMonAttack(r, u);
                    }
                } else {
                    if (u->attackorders && u->IsAlive()) {
                        const auto& ord = u->attackorders;
                        const size_t u_fac_num = u->faction.lock()->num;
                        r->DeduplicateUnitList(ord->targets, u_fac_num);
                        while (!ord->targets.empty()) {
                            const auto& id = ord->targets.front();
                            const auto t_w = r->GetUnitId(id, u_fac_num);
                            ord->targets.pop_front();
                            if (u->canattack && u->IsAlive()) {
                                if (!t_w.expired()) 
                                {
                                    const auto t = t_w.lock();
                                    AttemptAttack(r, u, t, 0);
                                }
                                else
                                {
                                    u->Error("ATTACK: Non-existent unit.");
                                }
                            }
                        }
                        u->attackorders.reset();
                    }
                }
            }
        }
    }
}

// Presume that u is alive, can attack, and wants to attack t.
// Check that t is alive, u can see t, and u has enough riding
// skill to catch t.
//
// Return 0 if success.
// 1 if t is already dead.
// 2 if u can't see t
// 3 if u lacks the riding to catch t
void Game::AttemptAttack(const ARegion::Handle& r,
                         const Unit::Handle& u,
                         const Unit::Handle& t,
                         int silent,
                         int adv)
{
    if (!t->IsAlive()) return;

    if (!u->CanSee(*r, t)) {
        if (!silent) u->Error("ATTACK: Non-existent unit.");
        return;
    }

    if (!u->CanCatch(*r, t)) {
        if (!silent) u->Error("ATTACK: Can't catch that unit.");
        return;
    }

    if (t->routed && Globals->ONLY_ROUT_ONCE) {
        if (!silent) u->Event("ATTACK: Target is already routed and scattered.");
        return;
    }

    RunBattle(r, u, t, 0, adv);
    return;
}

void Game::RunSellOrders()
{
    for(const auto& r: regions) {
        for(const auto& m: r->markets) {
            if (m->type == M_SELL)
            {
                DoSell(r, m);
            }
        }
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                for(size_t i = 0; i < u->sellorders.size(); ++i) {
                    u->Error("SELL: Can't sell that.");
                }
                u->sellorders.clear();
            }
        }
    }
}

int Game::GetSellAmount(const ARegion::Handle& r, const Market::Handle& m)
{
    int num = 0;
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            for(const auto& o: u->sellorders) {
                if (o->item == m->item) {
                    if (o->num == -1) {
                        o->num = static_cast<int>(u->items.CanSell(o->item));
                    }
                    if (m->amount != -1 && o->num > m->amount) {
                        o->num = m->amount;
                    }
                    if (o->num < 0) o->num = 0;
                    u->items.Selling(o->item, static_cast<size_t>(o->num));
                    num += o->num;
                }
            }
        }
    }
    return num;
}

void Game::DoSell(const ARegion::Handle& r, const Market::Handle& m)
{
    /* First, find the number of items being sold */
    int attempted = GetSellAmount(r, m);

    if (attempted < m->amount) attempted = m->amount;
    m->activity = 0;
    int oldamount = m->amount;
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            auto it = u->sellorders.begin();
            while(it != u->sellorders.end()) {
                const auto& o = *it;
                if (o->item == m->item) {
                    int temp = 0;
                    const int shared_num = static_cast<int>(u->GetSharedNum(o->item));
                    if (o->num > shared_num) {
                        o->num = shared_num;
                        u->Error("SELL: Unit attempted to sell "
                                "more than it had.");
                    }
                    if (attempted) {
                        temp = (m->amount *o->num + getrandom(attempted))
                            / attempted;
                        if (temp<0) temp = 0;
                    }
                    attempted -= o->num;
                    m->amount -= temp;
                    m->activity += temp;
                    u->ConsumeShared(o->item, static_cast<size_t>(temp));
                    u->SetMoney(u->GetMoney() + static_cast<unsigned int>(temp * m->price));
                    u->Event(AString("Sells ") + ItemString(o->item, temp)
                            + " at $" + m->price + " each.");
                    it = u->sellorders.erase(it);
                    continue;
                }
                ++it;
            }
        }
    }
    m->amount = oldamount;
}

void Game::RunBuyOrders()
{
    for(const auto& r: regions) {
        for(const auto& m: r->markets) {
            if (m->type == M_BUY)
            {
                DoBuy(r, m);
            }
        }
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                for(size_t i = 0; i < u->buyorders.size(); ++i) {
                    u->Error("BUY: Can't buy that.");
                }
                u->buyorders.clear();
            }
        }
    }
}

int Game::GetBuyAmount(const ARegion::Handle& r, const Market::Handle& m)
{
    int num = 0;
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            auto it = u->buyorders.begin();
            while(it != u->buyorders.end()) {
                const auto& o = *it;
                if (o->item == m->item) {
                    if (ItemDefs[o->item].type & IT_MAN) {
                        if (u->type == U_MAGE) {
                            u->Error("BUY: Mages can't recruit more men.");
                            o->num = 0;
                        }
                        if (u->type == U_APPRENTICE) {
                            AString temp = "BUY: ";
                            temp += static_cast<char>(toupper(Globals->APPRENTICE_NAME[0]));
                            temp += Globals->APPRENTICE_NAME + 1;
                            temp += "s can't recruit more men.";
                            u->Error(temp);
                            o->num = 0;
                        }
                        // XXX: there has to be a better way
                        if (u->GetSkill(Skills::Types::S_QUARTERMASTER)) {
                            u->Error("BUY: Quartermasters can't recruit more "
                                    "men.");
                            o->num = 0;
                        }
                        if (Globals->TACTICS_NEEDS_WAR &&
                                    u->GetSkill(Skills::Types::S_TACTICS) == 5) {
                            u->Error("BUY: Tacticians can't recruit more "
                                    "men.");
                            o->num = 0;
                        }
                        if (((ItemDefs[o->item].type & IT_LEADER) &&
                                u->IsNormal()) ||
                                (!(ItemDefs[o->item].type & IT_LEADER) &&
                                 u->IsLeader())) {
                            u->Error("BUY: Can't mix leaders and normal men.");
                            o->num = 0;
                        }
                    }
                    if (ItemDefs[o->item].type & IT_TRADE) {
                        if (!TradeCheck(r, u->faction.lock())) {
                            u->Error("BUY: Can't buy trade items in that "
                                    "many regions.");
                            o->num = 0;
                        }
                    }
                    if (o->num == -1) {
                        o->num = static_cast<int>(u->GetSharedMoney()) / m->price;
                        if (m->amount != -1 && o->num > m->amount) {
                            o->num = m->amount;
                        }
                    }
                    if (m->amount != -1 && o->num > m->amount) {
                        o->num = m->amount;
                        u->Error("BUY: Unit attempted to buy more than were for sale.");
                    }
                    if (o->num * m->price > static_cast<int>(u->GetSharedMoney())) {
                        o->num = static_cast<int>(u->GetSharedMoney()) / m->price;
                        u->Error("BUY: Unit attempted to buy more than it "
                                "could afford.");
                    }
                    num += o->num;
                }
                if (o->num < 1 && o->num != -1)
                {
                    it = u->buyorders.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }
    return num;
}

void Game::DoBuy(const ARegion::Handle& r, const Market::Handle& m)
{
    /* First, find the number of items being purchased */
    int attempted = GetBuyAmount(r, m);

    if (m->amount != -1)
        if (attempted < m->amount) attempted = m->amount;

    m->activity = 0;
    int oldamount = m->amount;
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            auto it = u->buyorders.begin();
            while(it != u->buyorders.end()) {
                const auto& o = *it;
                if (o->item == m->item) {
                    int temp = 0;
                    if (m->amount == -1) {
                        /* unlimited market */
                        temp = o->num;
                    } else {
                        if (attempted) {
                            temp = (m->amount * o->num +
                                    getrandom(attempted)) / attempted;
                            if (temp < 0) temp = 0;
                        }
                        attempted -= o->num;
                        m->amount -= temp;
                        m->activity += temp;
                    }
                    if (ItemDefs[o->item].type & IT_MAN) {
                        /* recruiting; must dilute skills */
                        u->AdjustSkills();
                        /* Setup specialized skill experience */
                        if (Globals->REQUIRED_EXPERIENCE) {
                            const auto& mt = FindRace(ItemDefs[o->item].abr);
                            int exp = static_cast<int>(mt.speciallevel - mt.defaultlevel);
                            if (exp > 0) {
                                const size_t exp2 = static_cast<size_t>(exp * temp) * GetDaysByLevel(1);
                                for (const auto& sname: mt.skills)
                                {
                                    const auto skill = LookupSkill(sname);
                                    if (!skill.isValid())
                                    {
                                        ++it;
                                        continue;
                                    }
                                    const size_t curxp = u->skills.GetExp(skill);
                                    u->skills.SetExp(skill, exp2 + curxp);
                                } 
                            }    
                        }
                        /* region economy effects */
                        r->Recruit(temp);
                    }
                    u->items.SetNum(o->item, u->items.GetNum(o->item) + static_cast<size_t>(temp));
                    u->faction.lock()->DiscoverItem(o->item, 0, 1);
                    u->ConsumeSharedMoney(static_cast<size_t>(temp * m->price));
                    u->Event(AString("Buys ") + ItemString(o->item, temp)
                            + " at $" + m->price + " each.");
                    it = u->buyorders.erase(it);
                    continue;
                }
                ++it;
            }
        }
    }

    m->amount = oldamount;
}

void Game::CheckUnitMaintenanceItem(const Items& item, int value, int consume)
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->needed > 0 && ((!consume) ||
                                (u->GetFlag(FLAG_CONSUMING_UNIT) ||
                                u->GetFlag(FLAG_CONSUMING_FACTION)))) {
                    int amount = static_cast<int>(u->items.GetNum(item));
                    if (amount) {
                        int eat = (u->needed + value - 1) / value;
                        if (eat > amount)
                            eat = amount;
                        if (ItemDefs[item].type & IT_FOOD) {
                            if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                                eat * value > u->stomach_space) {
                                eat = (u->stomach_space + value - 1) / value;
                                if (eat < 0)
                                    eat = 0;
                            }
                            u->hunger -= eat * value;
                            u->stomach_space -= eat * value;
                            if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                                u->stomach_space < 0) {
                                u->needed -= u->stomach_space;
                                u->stomach_space = 0;
                            }
                        }
                        u->needed -= eat * value;
                        u->items.SetNum(item, static_cast<size_t>(amount - eat));
                    }
                }
            }
        }
    }
}

void Game::CheckFactionMaintenanceItem(const Items& item, int value, int consume)
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->needed > 0 && ((!consume) ||
                        u->GetFlag(FLAG_CONSUMING_FACTION))) {
                    /* Go through all units again */
                    for(const auto& obj2: r->objects) {
                        for(const auto& u2: obj2->units) {
                            if (u->faction.lock() == u2->faction.lock() && u != u2) {
                                int amount = static_cast<int>(u2->items.GetNum(item));
                                if (amount) {
                                    int eat = (u->needed + value - 1) / value;
                                    if (eat > amount)
                                        eat = amount;
                                    if (ItemDefs[item].type & IT_FOOD) {
                                        if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                                            eat * value > u->stomach_space) {
                                            eat = (u->stomach_space + value - 1) / value;
                                            if (eat < 0)
                                                eat = 0;
                                        }
                                        u->hunger -= eat * value;
                                        u->stomach_space -= eat * value;
                                        if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                                            u->stomach_space < 0) {
                                            u->needed -= u->stomach_space;
                                            u->stomach_space = 0;
                                        }
                                    }
                                    u->needed -= eat * value;
                                    u2->items.SetNum(item, static_cast<size_t>(amount - eat));
                                }
                            }
                        }

                        if (u->needed < 1) break;
                    }
                }
            }
        }
    }
}

void Game::CheckAllyMaintenanceItem(const Items& item, int value)
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->needed > 0) {
                    /* Go through all units again */
                    for(const auto& obj2: r->objects) {
                        for(const auto& u2: obj2->units) {
                            if (u->faction.lock() != u2->faction.lock() &&
                                u2->GetAttitude(*r, u) == A_ALLY) {
                                int amount = static_cast<int>(u2->items.GetNum(item));
                                if (amount) {
                                    int eat = (u->needed + value - 1) / value;
                                    if (eat > amount)
                                        eat = amount;
                                    if (ItemDefs[item].type & IT_FOOD) {
                                        if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                                            eat * value > u->stomach_space) {
                                            eat = (u->stomach_space + value - 1) / value;
                                            if (eat < 0)
                                                eat = 0;
                                        }
                                        u->hunger -= eat * value;
                                        u->stomach_space -= eat * value;
                                        if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                                            u->stomach_space < 0) {
                                            u->needed -= u->stomach_space;
                                            u->stomach_space = 0;
                                        }
                                    }
                                    if (eat) {
                                        u->needed -= eat * value;
                                        u2->items.SetNum(item, static_cast<size_t>(amount - eat));
                                        u2->Event(*(u->name) + " borrows " +
                                                ItemString(item, eat) +
                                                " for maintenance.");
                                        u->Event(AString("Borrows ") +
                                                ItemString(item, eat) +
                                                " from " + *(u2->name) +
                                                " for maintenance.");
                                        u2->items.SetNum(item, static_cast<size_t>(amount - eat));
                                    }
                                }
                            }
                        }

                        if (u->needed < 1) break;
                    }
                }
            }
        }
    }
}

void Game::CheckUnitHungerItem(const Items& item, int value)
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->hunger > 0) {
                    int amount = static_cast<int>(u->items.GetNum(item));
                    if (amount) {
                        int eat = (u->hunger + value - 1) / value;
                        if (eat > amount)
                            eat = amount;
                        u->hunger -= eat * value;
                        u->stomach_space -= eat * value;
                        u->needed -= eat * value;
                        if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                            u->stomach_space < 0) {
                            u->needed -= u->stomach_space;
                            u->stomach_space = 0;
                        }
                        u->items.SetNum(item, static_cast<size_t>(amount - eat));
                    }
                }
            }
        }
    }
}

void Game::CheckFactionHungerItem(const Items& item, int value)
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->hunger > 0) {
                    /* Go through all units again */
                    for(const auto& obj2: r->objects) {
                        for(const auto& u2: obj2->units) {
                            if (u->faction.lock() == u2->faction.lock() && u != u2) {
                                int amount = static_cast<int>(u2->items.GetNum(item));
                                if (amount) {
                                    int eat = (u->hunger + value - 1) / value;
                                    if (eat > amount)
                                        eat = amount;
                                    u->hunger -= eat * value;
                                    u->stomach_space -= eat * value;
                                    u->needed -= eat * value;
                                    if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                                        u->stomach_space < 0) {
                                        u->needed -= u->stomach_space;
                                        u->stomach_space = 0;
                                    }
                                    u2->items.SetNum(item, static_cast<size_t>(amount - eat));
                                }
                            }
                        }

                        if (u->hunger < 1) break;
                    }
                }
            }
        }
    }
}

void Game::CheckAllyHungerItem(const Items& item, int value)
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->hunger > 0) {
                    /* Go through all units again */
                    for(const auto& obj2: r->objects) {
                        for(const auto& u2: obj2->units) {
                            if (u->faction.lock() != u2->faction.lock() &&
                                u2->GetAttitude(*r, u) == A_ALLY) {
                                int amount = static_cast<int>(u2->items.GetNum(item));
                                if (amount) {
                                    int eat = (u->hunger + value - 1) / value;
                                    if (eat > amount)
                                        eat = amount;
                                    u->hunger -= eat * value;
                                    u->stomach_space -= eat * value;
                                    u->needed -= eat * value;
                                    if (Globals->UPKEEP_MAXIMUM_FOOD >= 0 &&
                                        u->stomach_space < 0) {
                                        u->needed -= u->stomach_space;
                                        u->stomach_space = 0;
                                    }
                                    u2->items.SetNum(item, static_cast<size_t>(amount - eat));
                                        u2->Event(*(u->name) + " borrows " +
                                                ItemString(item, eat) +
                                                " to fend off starvation.");
                                        u->Event(AString("Borrows ") +
                                                ItemString(item, eat) +
                                                " from " + *(u2->name) +
                                                " to fend off starvation.");
                                        u2->items.SetNum(item, static_cast<size_t>(amount - eat));
                                }
                            }
                        }

                        if (u->hunger < 1) break;
                    }
                }
            }
        }
    }
}

void Game::AssessMaintenance()
{
    /* First pass: set needed */
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (!(u->faction.lock()->IsNPC())) {
                    r->visited = 1;
                    if (quests.CheckQuestVisitTarget(r, u)) {
                        u->Event("You have completed a pilgrimage!");
                    }
                }
                u->needed = static_cast<int>(u->MaintCost());
                u->hunger = static_cast<int>(u->GetMen() * Globals->UPKEEP_MINIMUM_FOOD);
                if (Globals->UPKEEP_MAXIMUM_FOOD < 0)
                {
                    u->stomach_space = -1;
                }
                else
                {
                    u->stomach_space = static_cast<int>(u->GetMen() * static_cast<unsigned int>(Globals->UPKEEP_MAXIMUM_FOOD));
                }
            }
        }
    }

    // Assess food requirements first
    if (Globals->UPKEEP_MINIMUM_FOOD > 0) {
        CheckUnitHunger();
        CheckFactionHunger();
        if (Globals->ALLOW_WITHDRAW) {
            // Can claim food for maintenance, so find the cheapest food
            Items i;
            ValidValue<size_t> cost;
            for (auto j = Items::begin(); j != Items::end(); ++j) {
                if (ItemDefs[*j].flags & ItemType::DISABLED)
                {
                    continue;
                }
                if (ItemDefs[*j].type & IT_FOOD) {
                    if (!i.isValid() ||
                            ItemDefs[i].baseprice > ItemDefs[*j].baseprice)
                    {
                        i = *j;
                    }
                }
            }
            if (i.isValid() && i > *Items::begin()) {
                cost = ItemDefs[i].baseprice * 5 / 2;
                for(const auto& r: regions) {
                    for(const auto& obj: r->objects) {
                        for(const auto& u: obj->units) {
                            size_t& uf_unclaimed = u->faction.lock()->unclaimed;
                            if (u->hunger > 0 && uf_unclaimed > cost) {
                                const int value = Globals->UPKEEP_FOOD_VALUE;
                                const int eat = (u->hunger + value - 1) / value;
                                /* Now see if faction has money */
                                if (uf_unclaimed >= static_cast<size_t>(eat) * cost) {
                                    u->Event(AString("Withdraws ") +
                                            ItemString(i, eat) +
                                            " for maintenance.");
                                    uf_unclaimed -= static_cast<size_t>(eat) * cost;
                                    u->hunger -= eat * value;
                                    u->stomach_space -= eat * value;
                                    u->needed -= eat * value;
                                } else {
                                    const size_t amount = uf_unclaimed / cost;
                                    u->Event(AString("Withdraws ") +
                                            ItemString(i, amount) +
                                            " for maintenance.");
                                    uf_unclaimed -= amount * cost;
                                    u->hunger -= static_cast<int>(amount) * value;
                                    u->stomach_space -= static_cast<int>(amount) * value;
                                    u->needed -= static_cast<int>(amount) * value;
                                }
                            }
                        }
                    }
                }
            }
        }
        CheckAllyHunger();
    }

    //
    // Check for CONSUMEing units.
    //
    if (Globals->FOOD_ITEMS_EXIST) {
        CheckUnitMaintenance(1);
        CheckFactionMaintenance(1);
    }

    //
    // Check the unit for money.
    //
    CheckUnitMaintenanceItem(Items::Types::I_SILVER, 1, 0);

    //
    // Check other units in same faction for money
    //
    CheckFactionMaintenanceItem(Items::Types::I_SILVER, 1, 0);

    if (Globals->FOOD_ITEMS_EXIST) {
        //
        // Check unit for possible food items.
        //
        CheckUnitMaintenance(0);

        //
        // Fourth pass; check other units in same faction for food items
        //
        CheckFactionMaintenance(0);
    }

    //
    // Check unclaimed money.
    //
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                size_t& uf_unclaimed = u->faction.lock()->unclaimed;
                if (u->needed > 0 && uf_unclaimed) {
                    /* Now see if faction has money */
                    if (static_cast<int>(uf_unclaimed) >= u->needed) {
                        u->Event(AString("Claims ") + u->needed +
                                 " silver for maintenance.");
                        uf_unclaimed -= static_cast<size_t>(u->needed);
                        u->needed = 0;
                    } else {
                        u->Event(AString("Claims ") +
                                uf_unclaimed +
                                " silver for maintenance.");
                        u->needed -= static_cast<int>(uf_unclaimed);
                        uf_unclaimed = 0;
                    }
                }
            }
        }
    }

    //
    // Check other allied factions for $$$.
    //
    CheckAllyMaintenanceItem(Items::Types::I_SILVER, 1);

    if (Globals->FOOD_ITEMS_EXIST) {
        //
        // Check other factions for food items.
        //
        CheckAllyMaintenance();
    }

    //
    // Last, if the unit still needs money, starve some men.
    //
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->needed > 0 || u->hunger > 0)
                    u->Short(u->needed, u->hunger);
            }
        }
    }
}

void Game::DoWithdrawOrders()
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                for(const auto& o: u->withdraworders) {
                    if (DoWithdrawOrder(r, u, o)) break;
                }
                u->withdraworders.clear();
            }
        }
    }
}

bool Game::DoWithdrawOrder(const ARegion::Handle& r, const Unit::Handle& u, const WithdrawOrder::Handle& o)
{
    const auto& itm = o->item;
    const auto& amt = o->amount;
    int cost = static_cast<int>(ItemDefs[itm].baseprice *5/2)*amt;

    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("WITHDRAW: Withdraw does not work in the Nexus.");
        return true;
    }

    const auto u_fac = u->faction.lock();
    if (cost > static_cast<int>(u_fac->unclaimed)) {
        u->Error(AString("WITHDRAW: Too little unclaimed silver to withdraw ")+
                ItemString(itm, amt)+".");
        return false;
    }

    if (ItemDefs[itm].max_inventory) {
        int cur = static_cast<int>(u->items.GetNum(itm)) + amt;
        if (cur > ItemDefs[itm].max_inventory) {
            u->Error(AString("WITHDRAW: Unit cannot have more than ")+
                    ItemString(itm, ItemDefs[itm].max_inventory));
            return false;
        }
    }

    u_fac->unclaimed -= static_cast<size_t>(cost);
    u->Event(AString("Withdraws ") + ItemString(o->item, amt) + ".");
    u->items.SetNum(itm, u->items.GetNum(itm) + static_cast<size_t>(amt));
    u_fac->DiscoverItem(itm, 0, 1);
    return false;
}

void Game::DoGiveOrders()
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                for(const auto& o: u->giveorders) {
                    if (o->item < 0) {
                        if (o->amount == -1) {
                            /* do 'give X unit' command */
                            DoGiveOrder(r, u, *o);
                        } else if (o->amount == -2) {
                            Object::Handle fleet;
                            Unit::Handle s;
                            if (o->type == Orders::Types::O_TAKE) {
                                const auto u_fac = u->faction.lock();
                                s = r->GetUnitId(o->target, u_fac->num).lock();
                                if (!s || !u->CanSee(*r, s)) {
                                    u->Error(AString("TAKE: Nonexistant target (") +
                                            o->target.get().Print() + ").");
                                    continue;
                                } else if (u_fac != s->faction.lock()) {
                                    u->Error(AString("TAKE: ") + o->target.get().Print() +
                                            " is not a member of your faction.");
                                    continue;
                                }
                                fleet = s->object.lock();
                            } else {
                                s = u;
                                fleet = obj;
                            }
                            /* do 'give all type' command */
                            if (fleet->IsFleet() && s == fleet->GetOwner().lock() &&
                                    !o->unfinished &&
                                    (o->item == -static_cast<ssize_t>(*Items::end()) || o->item == -IT_SHIP)) {
                                for(const auto& item: fleet->ships) {
                                    GiveOrder go;
                                    go.amount = static_cast<int>(item->num);
                                    go.except = 0;
                                    go.item = static_cast<ssize_t>(item->type);
                                    go.target = o->target;
                                    go.type = o->type;
                                    DoGiveOrder(r, u, go);
                                    go.target.invalidate();
                                }
                            }
                            for(const auto& item: s->items) {
                                if ((o->item == -static_cast<ssize_t>(*Items::end())) ||
                                    (ItemDefs[item->type].type & (-o->item))) {
                                    GiveOrder go;
                                    go.amount = static_cast<int>(item->num);
                                    go.except = 0;
                                    go.item = static_cast<ssize_t>(item->type);
                                    go.target = o->target;
                                    go.type = o->type;
                                    go.unfinished = o->unfinished;
                                    if (ItemDefs[item->type].type & IT_SHIP) {
                                        if (o->item == -static_cast<ssize_t>(*Items::end())) {
                                            go.unfinished = 1;
                                        }
                                        if (go.unfinished) {
                                            go.amount = 1;
                                        } else {
                                            go.amount = 0;
                                        }
                                    } else if (o->unfinished) {
                                        go.amount = 0;
                                    }
                                    if (go.amount) {
                                        DoGiveOrder(r, u, go);
                                    }
                                    go.target.invalidate();
                                }
                            }
                        } else {
                            if (o->type == Orders::Types::O_TAKE)
                            {
                                u->Error("TAKE: Invalid item.");
                            }
                            else
                            {
                                u->Error("GIVE: Invalid item.");
                            }
                        }
                    } else if (DoGiveOrder(r, u, *o)) {
                        break;
                    }
                }
                u->giveorders.clear();
            }
        }
    }
}

void Game::DoExchangeOrders()
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                for(const auto& o: u->exchangeorders) {
                    DoExchangeOrder(r, u, o);
                }
            }
        }
    }
}

void Game::DoExchangeOrder(const ARegion::Handle& r,
                           const Unit::Handle& u,
                           const ExchangeOrder::Handle& o)
{
    // Check if the destination unit exists
    const auto u_fac = u->faction.lock();
    const auto t_w = r->GetUnitId(o->target, u_fac->num);
    if (t_w.expired()) {
        u->Error(AString("EXCHANGE: Nonexistant target (") +
                o->target.Print() + ").");
        RemoveOrder(u->exchangeorders, o);
        return;
    }

    const auto t = t_w.lock();
    // Check each Item can be given
    if (ItemDefs[o->giveItem].flags & ItemType::CANTGIVE) {
        u->Error(AString("EXCHANGE: Can't trade ") +
                ItemDefs[o->giveItem].names + ".");
        RemoveOrder(u->exchangeorders, o);
        return;
    }

    if (ItemDefs[o->expectItem].flags & ItemType::CANTGIVE) {
        u->Error(AString("EXCHANGE: Can't trade ") +
                ItemDefs[o->expectItem].names + ".");
        RemoveOrder(u->exchangeorders, o);
        return;
    }

    if (ItemDefs[o->giveItem].type & IT_MAN) {
        u->Error("EXCHANGE: Exchange aborted.  Men may not be traded.");
        RemoveOrder(u->exchangeorders, o);
        return;
    }

    if (ItemDefs[o->expectItem].type & IT_MAN) {
        u->Error("EXCHANGE: Exchange aborted. Men may not be traded.");
        RemoveOrder(u->exchangeorders, o);
        return;
    }

    // New RULE -- Must be able to see unit to give something to them!
    if (!u->CanSee(*r, t)) {
        u->Error(AString("EXCHANGE: Nonexistant target (") +
                o->target.Print() + ").");
        return;
    }
    // Check other unit has enough to give
    int amtRecieve = o->expectAmount;
    if (amtRecieve > static_cast<int>(t->GetSharedNum(o->expectItem))) {
        t->Error(AString("EXCHANGE: Not giving enough. Expecting ") +
                ItemString(o->expectItem, o->expectAmount) + ".");
        u->Error(AString("EXCHANGE: Exchange aborted.  Not enough ") +
                "recieved. Expecting " +
            ItemString(o->expectItem, o->expectAmount) + ".");
        o->exchangeStatus = 0;
        return;
    }

    int exchangeOrderFound = 0;
    const auto t_fac = t->faction.lock();
    // Check if other unit has a reciprocal exchange order
    for(const auto& tOrder: t->exchangeorders) {
        const auto ptrUnitTemp = r->GetUnitId(tOrder->target, t_fac->num);
        if (ptrUnitTemp.lock() == u) {
            if (tOrder->expectItem == o->giveItem) {
                if (tOrder->giveItem == o->expectItem) {
                    exchangeOrderFound = 1;
                    if (tOrder->giveAmount < o->expectAmount) {
                        t->Error(AString("EXCHANGE: Not giving enough. ") +
                                "Expecting " +
                                ItemString(o->expectItem, o->expectAmount) +
                                ".");
                        u->Error(AString("EXCHANGE: Exchange aborted. ") +
                                "Not enough recieved. Expecting " +
                                ItemString(o->expectItem, o->expectAmount) +
                                ".");
                        tOrder->exchangeStatus = 0;
                        o->exchangeStatus = 0;
                        return;
                    } else if (tOrder->giveAmount > o->expectAmount) {
                        t->Error(AString("EXCHANGE: Exchange aborted. Too ") +
                                "much given. Expecting " +
                                ItemString(o->expectItem, o->expectAmount) +
                                ".");
                        u->Error(AString("EXCHANGE: Exchange aborted. Too ") +
                                "much offered. Expecting " +
                                ItemString(o->expectItem, o->expectAmount) +
                                ".");
                        tOrder->exchangeStatus = 0;
                        o->exchangeStatus = 0;
                    } else if (tOrder->giveAmount == o->expectAmount)
                        o->exchangeStatus = 1;

                    if ((o->exchangeStatus == 1) &&
                            (tOrder->exchangeStatus == 1)) {
                        u->Event(AString("Exchanges ") +
                                ItemString(o->giveItem, o->giveAmount) +
                                " with " + *t->name + " for " +
                                ItemString(tOrder->giveItem,
                                    tOrder->giveAmount) +
                                ".");
                        t->Event(AString("Exchanges ") +
                                ItemString(tOrder->giveItem,
                                    tOrder->giveAmount) + " with " +
                                *u->name + " for " +
                                ItemString(o->giveItem, o->giveAmount) + ".");
                        u->ConsumeShared(o->giveItem,
                                         static_cast<size_t>(o->giveAmount));
                        t->items.SetNum(o->giveItem,
                                t->items.GetNum(o->giveItem) + static_cast<size_t>(o->giveAmount));
                        t->ConsumeShared(tOrder->giveItem,
                                         static_cast<size_t>(tOrder->giveAmount));
                        u->items.SetNum(tOrder->giveItem,
                                u->items.GetNum(tOrder->giveItem) +
                                static_cast<size_t>(tOrder->giveAmount));
                        u_fac->DiscoverItem(tOrder->giveItem, 0, 1);
                        t_fac->DiscoverItem(o->giveItem, 0, 1);
                        RemoveOrder(u->exchangeorders, o);
                        RemoveOrder(t->exchangeorders, tOrder);
                        return;
                    } else if ((o->exchangeStatus >= 0) &&
                            (tOrder->exchangeStatus >= 0)) {
                        RemoveOrder(u->exchangeorders, o);
                        RemoveOrder(t->exchangeorders, tOrder);
                    }
                }
            }
        }
    }
    if (!exchangeOrderFound) {
        if (!u->CanSee(*r, t)) {
            u->Error(AString("EXCHANGE: Nonexistant target (") +
                    o->target.Print() + ").");
            RemoveOrder(u->exchangeorders, o);
            return;
        } else {
            u->Error("EXCHANGE: target unit did not issue a matching "
                    "exchange order.");
            RemoveOrder(u->exchangeorders, o);
            return;
        }
    }
}

int Game::DoGiveOrder(const ARegion::Handle& r,
                      const Unit::Handle& u,
                      GiveOrder& o)
{
    size_t num;
    int amt, newfleet, cur;
    int notallied;
    AString temp, ord;

    if (o.type == Orders::Types::O_TAKE)
    {
        ord = "TAKE";
    }
    else
    {
        ord = "GIVE";
    }

    const size_t o_item = static_cast<size_t>(o.item);
    const auto o_target = o.target.get();
    auto u_fac = u->faction.lock();
    /* Transfer/GIVE ship items: */
    if (o.item >= 0)
    {
        if(ItemDefs[o_item].type & IT_SHIP) {
            // GIVE 0
            if (!o_target.unitnum.isValid()) {
                bool hasitem = u->items.GetNum(o_item);
                temp = "Abandons ";
                const auto u_obj = u->object.lock();
                // discard unfinished ships from inventory
                if (o.unfinished) {
                    if (!hasitem || o.amount > 1) {
                        u->Error("GIVE: not enough.");
                        return 0;
                    }
                    ssize_t ship = -1;
                    for(const auto& it: u->items) {
                        if (it->type == o_item) {
                            u->Event(temp + it->Report(1) + ".");
                            ship = static_cast<ssize_t>(it->type);
                        }
                    }
                    if (ship > 0) u->items.SetNum(ship,0);
                    return 0;
                // abandon fleet ships
                } else if (!(u_obj->IsFleet()) || 
                    (u->num != u_obj->GetOwner().lock()->num)) {
                    u->Error("GIVE: only fleet owner can give ships.");
                    return 0;
                }

                // Check amount
                num = u_obj->GetNumShips(o_item);
                if (num < 1) {
                    u->Error("GIVE: no such ship in fleet.");
                    return 0;
                }
                if ((static_cast<int>(num) < o.amount) && (o.amount != -2)) {
                    u->Error("GIVE: not enough ships.");
                    o.amount = static_cast<int>(num);
                }

                // Check we're not dumping passengers in the ocean
                if (TerrainDefs[r->type].similar_type == Regions::Types::R_OCEAN) {
                    size_t shipcount = 0;
                    for(const auto& sh: u_obj->ships) {
                        shipcount += sh->num;
                    }
                    if (static_cast<int>(shipcount) <= o.amount) {
                        for(const auto& p: u_obj->units) {
                            if ((!p->CanSwim() || p->GetFlag(FLAG_NOCROSS_WATER))) {
                                u->Error("GIVE: Can't abandon our last ship in the ocean.");
                                return 0;
                            }
                        }
                    }
                }

                u_obj->SetNumShips(o_item, num - static_cast<size_t>(o.amount));
                u->Event(AString(temp) + ItemString(o_item, num - static_cast<size_t>(o.amount)) + ".");
                return 0;
            }
        }
        // GIVE to unit:    
        const auto t_w = r->GetUnitId(o.target, u_fac->num);
        if (t_w.expired()) {
            u->Error(ord + ": Nonexistant target (" +
                o_target.Print() + ").");
            return 0;
        }
        auto t = t_w.lock();
        auto t_fac = t->faction.lock();
        if (o.type == Orders::Types::O_TAKE && u_fac != t_fac) {
            u->Error(ord + ": " + o_target.Print() +
                    " is not a member of your faction.");
            return 0;
        } else if (u_fac != t_fac && t_fac->IsNPC()) {
            u->Error(ord + ": Can't give to non-player unit (" +
                o_target.Print() + ").");
            return 0;
        }
        if (u == t) {
            if (o.type == Orders::Types::O_TAKE)
                u->Error(ord + ": Attempt to take " +
                    ItemDefs[o_item].names + " from self.");
            else
                u->Error(ord + ": Attempt to give " +
                    ItemDefs[o_item].names + " to self.");
            return 0;
        }
        if (!u->CanSee(*r, t) &&
            (t_fac->GetAttitude(u_fac->num) < A_FRIENDLY)) {
                u->Error(ord + ": Nonexistant target (" +
                    o_target.Print() + ").");
                return 0;
        }
        if (t_fac->GetAttitude(u_fac->num) < A_FRIENDLY) {
                u->Error(ord + ": Target is not a member of a friendly faction.");
                return 0;
        }
        if (ItemDefs[o_item].flags & ItemType::CANTGIVE) {
            if (o.type == Orders::Types::O_TAKE)
                u->Error(ord + ": Can't take " +
                    ItemDefs[o_item].names + ".");
            else
                u->Error(ord + ": Can't give " +
                    ItemDefs[o_item].names + ".");
            return 0;
        }
        auto s = u;
        if (o.type == Orders::Types::O_TAKE) {
            s = t;
            t = u;
            t_fac = t->faction.lock();
        }

        const auto s_obj = s->object.lock();
        const auto s_fac = s->faction.lock();
        // Check amount
        if (o.unfinished) {
            num = s->items.GetNum(o_item) > 0;
        } else if (!(s_obj->IsFleet()) || 
            (s->num != s_obj->GetOwner().lock()->num)) {
            u->Error(ord + ": only fleet owner can transfer ships.");
            return 0;
        } else {
            num = s_obj->GetNumShips(o_item);
            if (num < 1) {
                u->Error(ord + ": no such ship in fleet.");
                return 0;
            }
        }
        amt = o.amount;
        if (amt != -2 && amt > static_cast<int>(num)) {
            u->Error(ord + ": Not enough.");
            amt = static_cast<int>(num);
        } else if (amt == -2) {
            amt = static_cast<int>(num);
            if (o.except) {
                if (o.except > amt) {
                    amt = 0;
                    u->Error(ord + ": EXCEPT value greater than amount on hand.");
                } else {
                    amt = amt - o.except;
                }
            }
        }
        if (!amt) {
            // giving 0 things is easy
            return 0;
        }

        if (o.unfinished) {
            if (t->items.GetNum(o_item) > 0) {
                if (o.type == Orders::Types::O_TAKE)
                {
                    u->Error(ord + ": Already have an unfinished ship of that type.");
                }
                else
                {
                    u->Error(ord + ": Target already has an unfinished ship of that type.");
                }
                return 0;
            }
            const auto it = std::make_shared<Item>();
            it->type = o_item;
            it->num = s->items.GetNum(o_item);
            if (o.type == Orders::Types::O_TAKE) {
                u->Event(AString("Takes ") + it->Report(1) +
                    " from " + *s->name + ".");
            } else {
                u->Event(AString("Gives ") + it->Report(1) +
                    " to " + *t->name + ".");
                if (s_fac != t_fac) {
                    t->Event(AString("Receives ") + it->Report(1) +
                        " from " + *s->name + ".");
                }
            }
            s->items.SetNum(o_item, 0);
            t->items.SetNum(o_item, it->num);
            t_fac->DiscoverItem(o_item, 0, 1);
        } else {
            // Check we're not dumping passengers in the ocean
            if (TerrainDefs[r->type].similar_type == Regions::Types::R_OCEAN &&
                    !o.merge) {
                size_t shipcount = 0;
                for(const auto& sh: s_obj->ships) {
                    shipcount += sh->num;
                }
                if (static_cast<int>(shipcount) <= amt) {
                    for(const auto& p: s_obj->units) {
                        if ((!p->CanSwim() || p->GetFlag(FLAG_NOCROSS_WATER))) {
                            u->Error(ord + ": Can't give away our last ship in the ocean.");
                            return 0;
                        }
                    }
                }
            }

            // give into existing fleet or form new fleet?
            newfleet = 0;
            const auto t_obj = t->object.lock();
            // target is not in fleet or not fleet owner
            if (!(t_obj->IsFleet()) ||
                (t->num != t_obj->GetOwner().lock()->num)) newfleet = 1;
            if (newfleet == 1) {
                // create a new fleet
                const auto fleet = std::make_shared<Object>(r);
                fleet->type = Objects::Types::O_FLEET;
                fleet->num = shipseq++;
                fleet->name = new AString(AString("Fleet [") + fleet->num + "]");
                t_obj->region.lock()->AddFleet(fleet);
                t->MoveUnit(fleet);
            }
            if (ItemDefs[o_item].max_inventory) {
                cur = static_cast<int>(t_obj->GetNumShips(o_item)) + amt;
                if (cur > ItemDefs[o_item].max_inventory) {
                    u->Error(ord + ": Fleets cannot have more than "+
                        ItemString(o_item, ItemDefs[o_item].max_inventory) +".");
                    return 0;
                }
            }
            s->Event(AString("Transfers ") + ItemString(o_item, amt) + " to " +
                *t_obj->name + ".");
            if (s_fac != t_fac) {
                t->Event(AString("Receives ") + ItemString(o_item, amt) +
                    " from " + *s_obj->name + ".");
            }
            s_obj->SetNumShips(o_item, s_obj->GetNumShips(o_item) - static_cast<size_t>(amt));
            t_obj->SetNumShips(o_item, t_obj->GetNumShips(o_item) + static_cast<size_t>(amt));
            t_fac->DiscoverItem(o_item, 0, 1);
        }
        return 0;
    }
    
    if (!o_target.unitnum.isValid()) {
        /* Give 0 */
        // Check there is enough to give
        amt = o.amount;
        const int shared_num = static_cast<int>(u->GetSharedNum(o.item));
        if (amt != -2 && amt > shared_num) {
            u->Error(ord + ": Not enough.");
            amt = shared_num;
        } else if (amt == -2) {
            amt = static_cast<int>(u->items.GetNum(o.item));
            if (o.except) {
                if (o.except > amt) {
                    amt = 0;
                    u->Error("GIVE: EXCEPT value greater than amount on hand.");
                } else {
                    amt = amt - o.except;
                }
            }
        }
        if (amt == -1) {
            u->Error("Can't discard a whole unit.");
            return 0;
        }

        if (amt < 0) {
            u->Error("Cannot give a negative number of items.");
            return 0;
        }

        temp = "Discards ";
        if (ItemDefs[o_item].type & IT_MAN) {
            u->SetMen(o.item, u->GetMen(o.item) - static_cast<size_t>(amt));
            r->DisbandInRegion(o.item, amt);
            temp = "Disbands ";
        } else if (Globals->RELEASE_MONSTERS &&
                (ItemDefs[o_item].type & IT_MONSTER)) {
            temp = "Releases ";
            u->items.SetNum(o.item, u->items.GetNum(o.item) - static_cast<size_t>(amt));
            if (Globals->WANDERING_MONSTERS_EXIST) {
                const auto mfac = GetFaction(factions, monfaction);
                const auto mon = GetNewUnit(mfac, 0);
                const auto& mp = FindMonster(ItemDefs[o_item].abr,
                        (ItemDefs[o_item].type & IT_ILLUSION));
                mon->MakeWMon(mp.name, o.item, static_cast<size_t>(amt));
                mon->MoveUnit(r->GetDummy());
                // This will result in 0 unless MONSTER_NO_SPOILS or
                // MONSTER_SPOILS_RECOVERY are set.
                mon->free = Globals->MONSTER_NO_SPOILS +
                    Globals->MONSTER_SPOILS_RECOVERY;
            }
        } else {
            u->ConsumeShared(o.item, static_cast<size_t>(amt));
        }
        u->Event(temp + ItemString(o.item, amt) + ".");
        return 0;
    }

    amt = o.amount;
    const auto t_w = r->GetUnitId(o.target, u_fac->num);
    if (t_w.expired()) {
        u->Error(ord + ": Nonexistant target (" +
                o_target.Print() + ").");
        return 0;
    }
    
    auto t = t_w.lock();
    auto t_fac = t->faction.lock();
    if (o.type == Orders::Types::O_TAKE && u_fac != t_fac) {
        u->Error(ord + ": " + o_target.Print() +
                " is not a member of your faction.");
        return 0;
    } else if (u_fac != t_fac && t_fac->IsNPC()) {
        u->Error(ord + ": Can't give to non-player unit (" +
                o_target.Print() + ").");
        return 0;
    }
    if (amt == -1 && u_fac == t_fac) {
        u->Error(ord + ": Unit already belongs to our faction!");
        return 0;
    }
    if (u == t) {
        if (o.type == Orders::Types::O_TAKE)
        {
            u->Error(ord + ": Attempt to take " + 
                    ItemString(o.item, amt) +
                    " from self.");
        }
        else
        {
            u->Error(ord + ": Attempt to give " + 
                    ItemString(o.item, amt) +
                    " to self.");
        }
        return 0;
    }
    // New RULE -- Must be able to see unit to give something to them!
    if (!u->CanSee(*r, t) &&
            (t_fac->GetAttitude(u_fac->num) < A_FRIENDLY)) {
        u->Error(ord + ": Nonexistant target (" +
                o_target.Print() + ").");
        return 0;
    }

    auto s = u;
    if (o.type == Orders::Types::O_TAKE) {
        s = t;
        t = u;
        t_fac = t->faction.lock();
    }

    const auto s_fac = s->faction.lock();
    const int shared_num = static_cast<int>(s->GetSharedNum(o.item));
    // Check there is enough to give
    if (amt != -2 && amt > shared_num) {
        u->Error(ord + ": Not enough.");
        amt = shared_num;
    } else if (amt == -2) {
        amt = static_cast<int>(s->items.GetNum(o.item));
        if (o.except) {
            if (o.except > amt) {
                amt = 0;
                u->Error(ord + ": EXCEPT value greater than amount on hand.");
            } else {
                amt = amt - o.except;
            }
        }
    }

    if ((o.item < 0 || Items(o.item) != Items::Types::I_SILVER) &&
            t_fac->GetAttitude(s_fac->num) < A_FRIENDLY) {
        u->Error("GIVE: Target is not a member of a friendly faction.");
        return 0;
    }

    if (amt == -1) {
        /* Give unit */
        if (u->type == U_MAGE) {
            if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_UNLIMITED) {
                if (CountMages(t_fac) >= AllowedMages(*t_fac)) {
                    u->Error("GIVE: Faction has too many mages.");
                    return 0;
                }
            }
        }
        if (u->type == U_APPRENTICE) {
            if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_UNLIMITED) {
                if (CountApprentices(t_fac) >= AllowedApprentices(*t_fac)) {
                    temp = "GIVE: Faction has too many ";
                    temp += Globals->APPRENTICE_NAME;
                    temp += "s.";
                    u->Error(temp);
                    return 0;
                }
            }
        }

        if (u->GetSkill(Skills::Types::S_QUARTERMASTER)) {
            if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES) {
                if (Globals->TRANSPORT & GameDefs::ALLOW_TRANSPORT) {
                    if (CountQuarterMasters(t_fac) >= AllowedQuarterMasters(*t_fac)) {
                        u->Error("GIVE: Faction has too many quartermasters.");
                        return 0;
                    }
                }
            }
        }

        if (u->GetSkill(Skills::Types::S_TACTICS) == 5) {
            if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES) {
                if (Globals->TACTICS_NEEDS_WAR) {
                    if (CountTacticians(t_fac) >= AllowedTacticians(*t_fac)) {
                        u->Error("GIVE: Faction has too many tacticians.");
                        return 0;
                    }
                }
            }
        }

        notallied = 1;
        if (t_fac->GetAttitude(u_fac->num) == A_ALLY) {
            notallied = 0;
        }

        u->Event(AString("Gives unit to ") + *(t_fac->name) + ".");
        u_fac = t_fac;
        u->Event("Is given to your faction.");

        if (notallied && u->monthorders && u->monthorders->type == Orders::Types::O_MOVE &&
                std::dynamic_pointer_cast<MoveOrder>(u->monthorders)->advancing) {
            u->Error("Unit cannot advance after being given.");
            u->monthorders.reset();
        }

        /* Check if any new skill reports have to be shown */
        for(const auto& skill: u->skills) {
            const size_t newlvl = u->GetRealSkill(skill.type);
            const size_t oldlvl = u_fac->skills.GetDays(skill.type);
            if (newlvl > oldlvl) {
                for (size_t i=oldlvl+1; i<=newlvl; i++) {
                    u_fac->shows.emplace_back(std::make_shared<ShowSkill>(skill.type, i));
                }
                u_fac->skills.SetDays(skill.type, newlvl);
            }
        }

        // Okay, now for each item that the unit has, tell the new faction
        // about it in case they don't know about it yet.
        for(const auto& it: u->items) {
            u_fac->DiscoverItem(it->type, 0, 1);
        }

        return notallied;
    }

    /* If the item to be given is a man, combine skills */
    if (ItemDefs[o_item].type & IT_MAN) {
        if (s->type == U_MAGE || s->type == U_APPRENTICE ||
                t->type == U_MAGE || t->type == U_APPRENTICE) {
            u->Error(ord + ": Magicians can't transfer men.");
            return 0;
        }
        if (Globals->TACTICS_NEEDS_WAR) {
            if (s->GetSkill(Skills::Types::S_TACTICS) == 5 ||
                    t->GetSkill(Skills::Types::S_TACTICS) == 5) {
                u->Error(ord + ": Tacticians can't transfer men.");
                return 0;
            }
        }
        if (s->GetSkill(Skills::Types::S_QUARTERMASTER) > 0 || t->GetSkill(Skills::Types::S_QUARTERMASTER) > 0) {
            u->Error(ord + ": Quartermasters can't transfer men.");
            return 0;
        }

        if ((ItemDefs[o_item].type & IT_LEADER) && t->IsNormal()) {
            u->Error(ord + ": Can't mix leaders and normal men.");
            return 0;
        } else {
            if (!(ItemDefs[o_item].type & IT_LEADER) && t->IsLeader()) {
                u->Error(ord + ": Can't mix leaders and normal men.");
                return 0;
            }
        }
        // Small hack for Ceran
        if (o.item >= 0 && Items(o_item) == Items::Types::I_MERC && t->GetMen()) {
            u->Error("GIVE: Can't mix mercenaries with other men.");
            return 0;
        }

        if (u_fac != t_fac) {
            u->Error(ord + ": Can't give men to another faction.");
            return 0;
        }

        if (s->nomove) t->nomove = 1;

        const auto skills = s->skills.Split(s->GetMen(), static_cast<size_t>(amt));
        t->skills.Combine(skills);
    }

    if (ItemDefs[o_item].flags & ItemType::CANTGIVE) {
        u->Error(ord + ": Can't give " + ItemDefs[o_item].names + ".");
        return 0;
    }

    if (ItemDefs[o_item].max_inventory) {
        cur = static_cast<int>(t->items.GetNum(o.item)) + amt;
        if (cur > ItemDefs[o_item].max_inventory) {
            u->Error(ord + ": Unit cannot have more than "+
                    ItemString(o.item, ItemDefs[o_item].max_inventory));
            return 0;
        }
    }

    if (o.type == Orders::Types::O_TAKE) {
        u->Event(AString("Takes ") + ItemString(o.item, amt) +
                " from " + *s->name + ".");
    } else {
        u->Event(AString("Gives ") + ItemString(o.item, amt) + " to " +
                *t->name + ".");
        if (s_fac != t_fac) {
            t->Event(AString("Receives ") + ItemString(o.item, amt) +
                    " from " + *s->name + ".");
        }
    }
    s->ConsumeShared(o.item, static_cast<size_t>(amt));
    t->items.SetNum(o.item, t->items.GetNum(o.item) + static_cast<size_t>(amt));
    t_fac->DiscoverItem(o.item, 0, 1);

    if (ItemDefs[o_item].type & IT_MAN) {
        t->AdjustSkills();
    }
    return 0;
}

void Game::DoGuard1Orders()
{
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->guard == GUARD_SET || u->guard == GUARD_GUARD) {
                    if (!u->Taxers(1)) {
                        u->guard = GUARD_NONE;
                        u->Error("Must be combat ready to be on guard.");
                        continue;
                    }
                    if (u->type != U_GUARD && r->HasCityGuard()) {
                        u->guard = GUARD_NONE;
                        u->Error("Is prevented from guarding by the "
                                "Guardsmen.");
                        continue;
                    }
                    u->guard = GUARD_GUARD;
                }
            }
        }
    }
}

void Game::FindDeadFactions()
{
    for(const auto& f: factions) {
        f->CheckExist(regions);
    }
}

void Game::DeleteEmptyUnits()
{
    for(const auto& region: regions) {
        DeleteEmptyInRegion(region);
    }
}

void Game::DeleteEmptyInRegion(const ARegion::Handle& region)
{
    for(const auto& obj: region->objects) {
        for(const auto& unit: obj->units) {
            if (unit->IsAlive() == 0) {
                region->Kill(unit);
            }
        }
    }
}

void Game::CheckTransportOrders()
{
    if (!(Globals->TRANSPORT & GameDefs::ALLOW_TRANSPORT))
        return;

    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                const auto u_fac = u->faction.lock();
                for(const auto& o: u->transportorders) {
                    // make sure target exists
                    AString ordertype =
                        (o->type == Orders::Types::O_DISTRIBUTE) ? "DISTRIBUTE" :
                        "TRANSPORT";
                    if (!o->target.isValid() || !o->target.get().unitnum.isValid()) {
                        u->Error(ordertype + ": Target does not exist.");
                        o->type = *Orders::end();
                        continue;
                    }

                    const auto tar = regions.GetUnitId(o->target,
                            u_fac->num, *r);
                    if (!tar) {
                        u->Error(ordertype + ": Target does not exist.");
                        o->type = *Orders::end();
                        continue;
                    }

                    const auto tar_unit = tar->unit.lock();
                    // Make sure target isn't self
                    if (tar_unit == u) {
                        u->Error(ordertype + ": Target is self.");
                        o->type = *Orders::end();
                        continue;
                    }

                    // Make sure the target and unit are at least friendly
                    if (tar_unit->faction.lock()->GetAttitude(u_fac->num) < A_FRIENDLY) {
                        u->Error(ordertype +
                                ": Target is not a member of a friendly "
                                "faction.");
                        o->type = *Orders::end();
                        continue;
                    }

                    // Make sure the target of a transport order is a unit
                    // with the quartermaster skill who owns a transport
                    // structure
                    if (o->type == Orders::Types::O_TRANSPORT) {
                        if (tar_unit->GetSkill(Skills::Types::S_QUARTERMASTER) == 0) {
                            u->Error(ordertype +
                                    ": Target is not a quartermaster");
                            o->type = *Orders::end();
                            continue;
                        }
                        const auto tar_obj = tar->obj.lock();
                        if ((tar_obj->GetOwner().lock() != tar_unit) ||
                                !(ObjectDefs[tar_obj->type].flags &
                                    ObjectType::TRANSPORT) ||
                                (tar_obj->incomplete > 0)) {
                            u->Error(ordertype + ": Target does not own "
                                    "a transport structure.");
                            o->type = *Orders::end();
                            continue;
                        }
                    }

                    // make sure target is in range.
                    int dist, maxdist;
                    if ((o->type == Orders::Types::O_TRANSPORT) &&
                        (u == obj->GetOwner().lock()) &&
                        (ObjectDefs[obj->type].flags & ObjectType::TRANSPORT))
                    {
                        maxdist = Globals->NONLOCAL_TRANSPORT;
                        if (maxdist > 0 &&
                            Globals->TRANSPORT & GameDefs::QM_AFFECT_DIST) {
                            const int level = static_cast<int>(u->GetSkill(Skills::Types::S_QUARTERMASTER));
                            maxdist += ((level + 1)/3);
                        }
                    }
                    else
                    {
                        maxdist = static_cast<int>(Globals->LOCAL_TRANSPORT);
                    }

                    int penalty = 10000000;
                    try
                    {
                        const auto& rt = FindRange("rng_transport");
                        penalty = rt.crossLevelPenalty;
                    }
                    catch(const NoSuchItemException&)
                    {
                    }

                    const auto tar_region = tar->region.lock();
                    if (maxdist > 0) {
                        // 0 maxdist represents unlimited range
                        dist = static_cast<int>(regions.GetPlanarDistance(r, tar_region, penalty, static_cast<unsigned int>(maxdist)));
                        if (dist > maxdist) {
                            u->Error(ordertype + ": Recipient is too far away.");
                            o->type = *Orders::end();
                            continue;
                        }
                    } else {
                        dist = static_cast<int>(regions.GetPlanarDistance(r, tar_region, penalty, Globals->LOCAL_TRANSPORT));
                    }

                    // On long range transport or distribute, make sure the
                    // issuer is a quartermaster and is owner of a structure
                    if ((o->type == Orders::Types::O_DISTRIBUTE) ||
                        ((dist > static_cast<int>(Globals->LOCAL_TRANSPORT)) &&
                         (o->type == Orders::Types::O_TRANSPORT))) {
                        if (u->GetSkill(Skills::Types::S_QUARTERMASTER) == 0) {
                            u->Error(ordertype +
                                    ": Unit is not a quartermaster");
                            o->type = *Orders::end();
                            continue;
                        }
                        if ((u != obj->GetOwner().lock()) ||
                            !(ObjectDefs[obj->type].flags &
                                ObjectType::TRANSPORT) ||
                            (obj->incomplete > 0)) {
                        u->Error(ordertype +
                                ": Unit does not own transport structure.");
                        o->type = *Orders::end();
                        continue;
                        }
                    }

                    // make sure amount is available (all handled later)
                    if (o->amount > 0 && o->amount > static_cast<int>(u->GetSharedNum(o->item))) {
                        u->Error(ordertype + ": Not enough.");
                        o->type = *Orders::end();
                        continue;
                    }

                    if (o->amount > 0 && ItemDefs[o->item].max_inventory) {
                        int cur = static_cast<int>(tar_unit->items.GetNum(o->item)) + o->amount;
                        if (cur > ItemDefs[o->item].max_inventory) {
                            u->Error(ordertype +
                                    ": Target cannot have more than " +
                                    ItemString(o->item,
                                        ItemDefs[o->item].max_inventory));
                            o->type = *Orders::end();
                            continue;
                        }
                    }

                    // Check if we have a trade hex
                    if (!TradeCheck(r, u_fac)) {
                        u->Error(ordertype + ": Faction cannot transport or "
                                "distribute in that many hexes.");
                        o->type = *Orders::end();
                        continue;
                    }
                }
            }
        }
    }
}

void Game::RunTransportOrders()
{
    if (!(Globals->TRANSPORT & GameDefs::ALLOW_TRANSPORT))
    {
        return;
    }
    for(const auto& r: regions) {
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                const auto u_fac = u->faction.lock();
                for(const auto& t: u->transportorders) {
                    if (t->type != Orders::Types::O_TRANSPORT && t->type != Orders::Types::O_DISTRIBUTE)
                    {
                        continue;
                    }
                    const auto tar = regions.GetUnitId(t->target, u_fac->num, *r);
                    if (!tar)
                    {
                        continue;
                    }
                    AString ordertype = (t->type == Orders::Types::O_TRANSPORT) ?
                        "TRANSPORT" : "DISTRIBUTE";

                    int amt = t->amount;
                    if (amt < 0)
                    {
                        amt = static_cast<int>(u->items.GetNum(t->item));
                        if (t->except) {
                            if (t->except > amt) {
                                amt = 0;
                                u->Error(ordertype +
                                        ": EXCEPT value greater than amount "
                                        "on hand.");
                            } else {
                                amt = amt - t->except;
                            }
                        }
                    }
                    else
                    {
                        const int shared_num = static_cast<int>(u->GetSharedNum(t->item));
                        if (amt > shared_num)
                        {
                            u->Error(ordertype + ": Not enough.");
                            amt = shared_num;
                        }
                    }

                    const auto tar_unit = tar->unit.lock();
                    if (ItemDefs[t->item].max_inventory) {
                        int cur = static_cast<int>(tar_unit->items.GetNum(t->item)) + amt;
                        if (cur > ItemDefs[t->item].max_inventory) {
                            u->Error(ordertype +
                                    ": Target cannot have more than " +
                                    ItemString(t->item,
                                        ItemDefs[t->item].max_inventory));
                            continue;
                        }
                    }

                    u->ConsumeShared(t->item, static_cast<size_t>(amt));
                    // now see if the unit can pay for shipping
                    int penalty = 10000000;
                    try
                    {
                        const auto& rt = FindRange("rng_transport");
                        penalty = rt.crossLevelPenalty;
                    }
                    catch(const NoSuchItemException&)
                    {
                    }
                    int dist = static_cast<int>(regions.GetPlanarDistance(r, tar->region.lock(), penalty, Globals->LOCAL_TRANSPORT));
                    size_t weight = ItemDefs[t->item].weight * static_cast<size_t>(amt);
                    if (weight == 0 && Globals->FRACTIONAL_WEIGHT > 0)
                        weight = (static_cast<size_t>(amt) / Globals->FRACTIONAL_WEIGHT) + 1;
                    size_t cost = 0;
                    if (dist > static_cast<int>(Globals->LOCAL_TRANSPORT)) {
                        cost = Globals->SHIPPING_COST * weight;
                        if (Globals->TRANSPORT & GameDefs::QM_AFFECT_COST)
                            cost *= (4 - ((u->GetSkill(Skills::Types::S_QUARTERMASTER)+1)/2));
                    }

                    // if not, give it back
                    if (cost > u->GetSharedMoney()) {
                        u->Error(ordertype + ": Cannot afford shipping cost.");
                        u->items.SetNum(t->item, u->items.GetNum(t->item) + static_cast<size_t>(amt));
                        continue;
                    }
                    u->ConsumeSharedMoney(cost);

                    ordertype = (t->type == Orders::Types::O_TRANSPORT) ?
                        "Transports " : "Distributes ";
                    u->Event(ordertype + ItemString(t->item, amt) + " to " +
                            *tar_unit->name + " for $" + cost + ".");

                    const auto tu_fac = tar_unit->faction.lock();
                    if (u_fac != tu_fac) {
                        tar_unit->Event(AString("Recieves ") +
                                ItemString(t->item, amt) + " from " +
                                *u->name + ".");
                    }

                    tar_unit->items.SetNum(t->item,
                                           tar_unit->items.GetNum(t->item) + static_cast<size_t>(amt));
                    tu_fac->DiscoverItem(t->item, 0, 1);

                    u->Practice(Skills::Types::S_QUARTERMASTER);

                }
                u->transportorders.clear();
            }
        }
    }
}
