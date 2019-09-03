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
#include <utility>
#include "game.h"
#include "battle.h"
#include "army.h"
#include "gamedefs.h"
#include "gamedata.h"
#include "quests.h"

void Battle::FreeRound(const Army::Handle& att, const Army::Handle& def, int ass)
{
    /* Write header */
    AddLine(att->leader.lock()->name + " gets a free round of attacks.");

    /* Update both army's shields */
    att->shields.clear();
    UpdateShields(att);

    def->shields.clear();
    UpdateShields(def);

    //
    // Update the attacking armies round counter
    //
    att->round++;

    /* Run attacks until done */
    size_t alv = def->NumAlive();
    while (att->CanAttack() && def->NumAlive()) {
        size_t num = getrandom(att->CanAttack());
        bool behind;
        const auto a = att->GetAttacker(num, behind);
        DoAttack(att->round, a, att, def, behind, ass);
    }

    /* Write losses */
    def->Regenerate(*this);
    alv -= def->NumAlive();
    AddLine(def->leader.lock()->name + " loses " + alv + ".");
    AddLine("");
    att->Reset();
}

void Battle::DoAttack(int round, const Soldier::Handle& a, const Army::Handle& attackers, const Army::Handle& def,
        bool behind, int ass)
{
    DoSpecialAttack(round, a, attackers, def, behind);
    if (!def->NumAlive()) return;

    if (!behind && a->riding.isValid()) {
        const auto& pMt = FindMount(ItemDefs[a->riding].abr);
        if (pMt.mountSpecial != NULL) {
            int num, tot = -1;
            const auto& spd = FindSpecial(pMt.mountSpecial);
            for (const auto& damage: spd.damage) {
                int times = damage.value;
                if (spd.effectflags & SpecialType::FX_USE_LEV)
                    times *= static_cast<int>(pMt.specialLev);
                int realtimes = damage.minnum + getrandom(times) +
                    getrandom(times);
                num = def->DoAnAttack(pMt.mountSpecial, realtimes,
                        damage.type, static_cast<int>(pMt.specialLev),
                        damage.flags, damage.dclass,
                        damage.effect, 0, a);
                if (num != -1) {
                    if (tot == -1) tot = num;
                    else tot += num;
                }
            }
            if (tot != -1) {
                AddLine(a->name + " " + spd.spelldesc + ", " +
                        spd.spelldesc2 + tot + spd.spelltarget + ".");
            }
        }
    }
    if (!def->NumAlive()) return;

    int numAttacks = a->attacks;
    if (a->attacks < 0) {
        if (round % ( -1 * a->attacks ) == 1)
            numAttacks = 1;
        else
            numAttacks = 0;
    } else if (ass && (Globals->MAX_ASSASSIN_FREE_ATTACKS > 0) &&
            (numAttacks > Globals->MAX_ASSASSIN_FREE_ATTACKS)) {
        numAttacks = Globals->MAX_ASSASSIN_FREE_ATTACKS;
    }

    for (int i = 0; i < numAttacks; i++) {
        int flags = 0;
        int attackType = ATTACK_COMBAT;
        int mountBonus = 0;
        int attackClass = SLASHING;

        try
        {
            const auto& pWep = FindWeapon(ItemDefs[a->weapon].abr);
            if (!(pWep.flags & WeaponType::RANGED))
            {
                break;
            }

            flags = pWep.flags;
            attackType = pWep.attackType;
            mountBonus = pWep.mountBonus;
            attackClass = pWep.weapClass;
        }
        catch(const NoSuchItemException&)
        {
            if(behind)
            {
                break;
            }
        }

        def->DoAnAttack(NULL, 1, attackType, a->askill, flags, attackClass, NULL, mountBonus, a);
        if (!def->NumAlive()) break;
    }

    a->ClearOneTimeEffects();
}

void Battle::NormalRound(int round, const Army::Handle& a, const Army::Handle& b)
{
    /* Write round header */
    AddLine(AString("Round ") + round + ":");

    /* Update both army's shields */
    UpdateShields(a);
    UpdateShields(b);

    /* Initialize variables */
    a->round++;
    b->round++;
    size_t aalive = a->NumAlive();
    size_t aialive = aalive;
    size_t balive = b->NumAlive();
    size_t bialive = balive;
    size_t aatt = a->CanAttack();
    size_t batt = b->CanAttack();

    /* Run attacks until done */
    while (aalive && balive && (aatt || batt))
    {
        size_t num = getrandom(aatt + batt);
        bool behind;
        if (num >= aatt)
        {
            num -= aatt;
            auto s = b->GetAttacker(num, behind);
            DoAttack(b->round, s, b, a, behind);
        }
        else
        {
            auto s = a->GetAttacker(num, behind);
            DoAttack(a->round, s, a, b, behind);
        }
        aalive = a->NumAlive();
        balive = b->NumAlive();
        aatt = a->CanAttack();
        batt = b->CanAttack();
    }

    /* Finish round */
    a->Regenerate(*this);
    b->Regenerate(*this);
    aialive -= aalive;
    AddLine(a->leader.lock()->name + " loses " + aialive + ".");
    bialive -= balive;
    AddLine(b->leader.lock()->name + " loses " + bialive + ".");
    AddLine("");
    a->Reset();
    b->Reset();
}

void Battle::GetSpoils(const PtrList<Location>& losers, ItemList& spoils, int ass)
{
    ItemList ships;
    for(const auto& l: losers) {
        const auto& u = l->unit.lock();
        size_t numalive = u->GetSoldiers();
        size_t numdead = u->losses;
        if (!numalive) {
            if (quests.CheckQuestKillTarget(u, spoils)) {
                AddLine("Quest completed!");
            }
        }
        for(const auto& i: u->items) {
            if (IsSoldier(i->type)) continue;
            // ignore incomplete ships
            if (ItemDefs[i->type].type & IT_SHIP) continue;
            // New rule:  Assassins with RINGS cannot get AMTS in spoils
            // This rule is only meaningful with Proportional AMTS usage
            // is enabled, otherwise it has no effect.
            if ((ass == 2) && (i->type == Items::Types::I_AMULETOFTS)) continue;
            float percent = static_cast<float>(numdead)/static_cast<float>(numalive+numdead);
            // incomplete ships:
            if (ItemDefs[i->type].type & IT_SHIP) {
                if (getrandom<float>(100) < percent) {
                    u->items.SetNum(i->type, 0);
                    if (i->num < ships.GetNum(i->type))
                        ships.SetNum(i->type, i->num);
                }
            } else {
                size_t num = static_cast<size_t>(static_cast<float>(i->num) * percent);
                size_t num2 = (num + getrandom(2UL))/2;
                if (ItemDefs[i->type].type & IT_ALWAYS_SPOIL) {
                    num2 = num;
                }
                if (ItemDefs[i->type].type & IT_NEVER_SPOIL) {
                    num2 = 0;
                }
                spoils.SetNum(i->type, spoils.GetNum(i->type) + num2);
                u->items.SetNum(i->type, i->num - num);
            }
        }
    }
    // add incomplete ships to spoils...
    for (size_t sh = 0; sh < Items::size(); sh++) {
        if (ItemDefs[sh].type & IT_SHIP) {
            spoils.SetNum(sh, ships.GetNum(sh));
        }
    }
}

int Battle::Run(const ARegion::Handle& region,
                const Unit::Handle& att,
                const PtrList<Location>& atts,
                const Unit::Handle& tar,
                const PtrList<Location>& defs,
                int ass,
                const ARegionList& pRegs)
{
    AString temp;
    assassination = ASS_NONE;
    attacker = att->faction;

    std::pair<Army::Handle, Army::Handle> armies(
        std::make_shared<Army>(att, atts, region->type, ass),
        std::make_shared<Army>(tar, defs, region->type, ass)
    );

    if (ass) {
        FreeRound(armies.first, armies.second, ass);
    } else {
        if (armies.first->tac > armies.second->tac) FreeRound(armies.first, armies.second);
        if (armies.second->tac > armies.first->tac) FreeRound(armies.second, armies.first);
    }

    int round = 1;
    while (!armies.first->Broken() && !armies.second->Broken() && round < 101) {
        NormalRound(round++, armies.first, armies.second);
    }

    if ((armies.first->Broken() && !armies.second->Broken()) ||
        (!armies.first->NumAlive() && armies.second->NumAlive())) {
        if (ass) assassination = ASS_FAIL;

        if (armies.first->NumAlive()) {
            AddLine(armies.first->leader.lock()->name + " is routed!");
            FreeRound(armies.second, armies.first);
        } else {
            AddLine(armies.first->leader.lock()->name + " is destroyed!");
        }
        AddLine("Total Casualties:");
        ItemList spoils;
        armies.first->Lose(*this, spoils);
        GetSpoils(atts, spoils, ass);
        if(!spoils.empty()) {
            temp = AString("Spoils: ") + spoils.Report(2,0,1) + ".";
        } else {
            temp = "Spoils: none.";
        }
        armies.second->Win(*this, spoils);
        AddLine("");
        AddLine(temp);
        AddLine("");
        return BATTLE_LOST;
    }

    if ((armies.second->Broken() && !armies.first->Broken()) ||
        (!armies.second->NumAlive() && armies.first->NumAlive())) {
        const auto second_leader = armies.second->leader.lock();
        if (ass) {
            assassination = ASS_SUCC;
            asstext = second_leader->name +
                        " is assassinated in " +
                        region->ShortPrint( pRegs ) +
                        "!";
        }
        if (armies.second->NumAlive()) {
            AddLine(second_leader->name + " is routed!");
            FreeRound(armies.first, armies.second);
        } else {
            AddLine(second_leader->name + " is destroyed!");
        }
        AddLine("Total Casualties:");
        ItemList spoils;
        armies.second->Lose(*this, spoils);
        GetSpoils(defs, spoils, ass);
        if (spoils.size()) {
            temp = AString("Spoils: ") + spoils.Report(2,0,1) + ".";
        } else {
            temp = "Spoils: none.";
        }
        armies.first->Win(*this, spoils);
        AddLine("");
        AddLine(temp);
        AddLine("");
        return BATTLE_WON;
    }

    AddLine("The battle ends indecisively.");
    AddLine("");
    AddLine("Total Casualties:");
    armies.first->Tie(*this);
    armies.second->Tie(*this);
    temp = "Spoils: none.";
    AddLine("");
    AddLine(temp);
    AddLine("");
    return BATTLE_DRAW;
}

void Battle::WriteSides(const ARegion::Handle& r,
                        const Unit::Handle& att,
                        const Unit::Handle& tar,
                        const PtrList<Location>& atts,
                        const PtrList<Location>& defs,
                        int ass,
                        const ARegionList& pRegs )
{
    if (ass) {
        AddLine(att->name + " attempts to assassinate " + tar->name
                + " in " + r->ShortPrint( pRegs ) + "!");
    } else {
        AddLine(att->name + " attacks " + tar->name + " in " +
                r->ShortPrint( pRegs ) + "!");
    }
    AddLine("");

    unsigned int dobs = 0;
    unsigned int aobs = 0;
    {
        for(const auto& l: defs) {
            unsigned int a = l->unit.lock()->GetAttribute("observation");
            if (a > dobs)
            {
                dobs = a;
            }
        }
    }

    AddLine("Attackers:");
    {
        for(const auto& l: atts) {
            const auto& u = l->unit.lock();
            unsigned int a = u->GetAttribute("observation");
            if (a > aobs)
            {
                aobs = a;
            }
            AString temp = u->BattleReport(dobs);
            AddLine(temp);
        }
    }
    AddLine("");
    AddLine("Defenders:");
    {
        for(const auto& l: defs) {
            AString temp = l->unit.lock()->BattleReport(aobs);
            AddLine(temp);
        }
    }
    AddLine("");
}

void Battle::Report(Areport * f, const Faction& fac) {
    if (assassination == ASS_SUCC && &fac != attacker.lock().get()) {
        f->PutStr(asstext);
        f->PutStr("");
        return;
    }
    for(const auto& s: text) {
        f->PutStr(*s);
    }
}

void Battle::AddLine(const AString & s) {
    text.emplace_back(std::make_unique<AString>(s));
}

void Game::GetDFacs(const ARegion::Handle& r, const Unit::Handle& t, WeakPtrList<Faction>& facs)
{
    int AlliesIncluded = 0;
    
    // First, check whether allies should assist in this combat
    if (Globals->ALLIES_NOAID == 0) {
        AlliesIncluded = 1;
    } else {
        // Check whether any of the target faction's
        // units aren't set to noaid
        for(const auto& obj: r->objects) {
            for(const auto& u: obj->units) {
                if (u->IsAlive()) {
                    if (u->faction.lock() == t->faction.lock() &&
                        (u->GetFlag(FLAG_NOAID) == 0)) {
                        AlliesIncluded = 1;
                        break;
                    }
                }
            }
            if (AlliesIncluded == 1) break; // forlist (objects)
        }
    }
    
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            if (u->IsAlive()) {
                if (u->faction.lock() == t->faction.lock() ||
                    (AlliesIncluded == 1 && 
                     u->guard != GUARD_AVOID &&
                     u->GetAttitude(*r, t) == Attitudes::Types::A_ALLY) ) {

                    const auto ufac = u->faction.lock();
                    if (GetFaction2(facs, ufac->num).expired()) 
                    {
                        facs.push_back(u->faction);
                    }
                }
            }
        }
    }
}

void Game::GetAFacs(const ARegion::Handle& r,
                    const Unit::Handle& att,
                    const Unit::Handle& tar,
                    WeakPtrList<Faction>& dfacs,
                    WeakPtrList<Faction>& afacs,
                    PtrList<Location>& atts)
{
    for(const auto& obj: r->objects) {
        for(const auto& u: obj->units) {
            if (u->canattack && u->IsAlive()) {
                bool add = false;
                if ((u->faction.lock() == att->faction.lock() || u->GetAttitude(*r,tar) == Attitudes::Types::A_HOSTILE) &&
                    (u->guard != GUARD_AVOID || u == att))
                {
                    add = true;
                }
                else
                {
                    if (u->guard == GUARD_ADVANCE && u->GetAttitude(*r,tar) != Attitudes::Types::A_ALLY)
                    {
                        add = true;
                    }
                    else
                    {
                        if (u->attackorders)
                        {
                            const auto uf = u->faction.lock();
                            const auto tf = tar->faction.lock();
                            for(const auto& id: u->attackorders->targets) {
                                Unit::WeakHandle t_w = r->GetUnitId(id, uf->num);
                                if (t_w.expired())
                                {
                                    continue;
                                }
                                const auto t = t_w.lock();
                                if (t == tar)
                                {
                                    u->attackorders->targets.remove(id);
                                }
                                if (t->faction.lock() == tf)
                                {
                                    add = true;
                                }
                            }
                        }
                    }
                }

                if (add) {
                    const auto ufac = u->faction.lock();
                    const size_t ufac_num = ufac->num;
                    if (GetFaction2(dfacs, ufac_num).expired()) {
                        auto& l = atts.emplace_back(std::make_shared<Location>());
                        l->unit = u;
                        l->obj = obj;
                        l->region = r;
                        if (GetFaction2(afacs, ufac_num).expired()) {
                            afacs.push_back(u->faction);
                        }
                    }
                }
            }
        }
    }
}

bool Game::CanAttack(const ARegion::Handle& r, const WeakPtrList<Faction>& afacs, const Unit::Handle& u)
{
    bool see = false;
    bool ride = false;
    for(const auto& f_w: afacs)
    {
        const auto f = f_w.lock();
        if (f->CanSee(*r, u) == 2)
        {
            if (ride)
            {
                return true;
            }
            see = true;
        }
        if (f->CanCatch(*r, u))
        {
            if (see)
            {
                return true;
            }
            ride = true;
        }
    }
    return false;
}

void Game::GetSidesForRegion_(const ARegion::Handle& r,
                              const ARegion::Handle& r2,
                              WeakPtrList<Faction>& afacs,
                              WeakPtrList<Faction>& dfacs,
                              PtrList<Location>& atts,
                              PtrList<Location>& defs,
                              const Unit::Handle& att,
                              const Unit::Handle& tar,
                              bool adv,
                              bool& noaida,
                              bool& noaidd,
                              bool first)
{
    static constexpr int ADD_ATTACK = 1;
    static constexpr int ADD_DEFENSE = 2;

    for(const auto& o: r2->objects) {
        for(const auto& u: o->units) {
            int add = 0;
            /* First, can the unit be involved in the battle at all? */
            if ((first || u->GetFlag(FLAG_HOLDING) == 0) && u->IsAlive()) {
                const auto ufac = u->faction.lock();
                const auto ufac_num = ufac->num;
                if (!GetFaction2(afacs, ufac_num).expired()) {
                    /*
                     * The unit is on the attacking side, check if the
                     * unit should be in the battle
                     */
                    if (first || (!noaida)) {
                        if (u->canattack &&
                                (u->guard != GUARD_AVOID || u==att) &&
                                u->CanMoveTo(*r2, *r) &&
                                !::GetUnit(atts, u->num).expired()) {
                            add = ADD_ATTACK;
                        }
                    }
                } else {
                    /* The unit is not on the attacking side */
                    /*
                     * First, check for the noaid flag; if it is set,
                     * only units from this region will join on the
                     * defensive side
                     */
                    if (!(!first && noaidd)) {
                        if (u->type == U_GUARD) {
                            /* The unit is a city guardsman */
                            if (first && !adv)
                            {
                                add = ADD_DEFENSE;
                            }
                        } else if (u->type == U_GUARDMAGE) {
                            /* the unit is a city guard support mage */
                            if (first && !adv)
                            {
                                add = ADD_DEFENSE;
                            }
                        } else {
                            /*
                             * The unit is not a city guardsman, check if
                             * the unit is on the defensive side
                             */
                            if (!GetFaction2(dfacs, ufac_num).expired()) {
                                if (u->guard == GUARD_AVOID) {
                                    /*
                                     * The unit is avoiding, and doesn't
                                     * want to be in the battle if he can
                                     * avoid it
                                     */
                                    if (u == tar ||
                                            (first &&
                                             ufac == tar->faction.lock() &&
                                             CanAttack(r, afacs, u))) {
                                        add = ADD_DEFENSE;
                                    }
                                } else {
                                    /*
                                     * The unit is not avoiding, and wants
                                     * to defend, if it can
                                     */
                                    if (u->CanMoveTo(*r2, *r)) {
                                        add = ADD_DEFENSE;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (add == ADD_ATTACK) {
                auto& l = atts.emplace_back(std::make_shared<Location>());
                l->unit = u;
                l->obj = o;
                l->region = r2;
            } else if (add == ADD_DEFENSE) {
                auto& l = defs.emplace_back(std::make_shared<Location>());
                l->unit = u;
                l->obj = o;
                l->region = r2;
            }
        }
    }
    //
    // If we are in the original region, check for the noaid status of
    // the units involved
    //
    if (first) {
        noaida = true;
        for(const auto& l: atts) {
            if (!l->unit.lock()->GetFlag(FLAG_NOAID)) {
                noaida = false;
                break;
            }
        }
    }

    noaidd = true;
    for(const auto& l: defs) {
        if (!l->unit.lock()->GetFlag(FLAG_NOAID)) {
            noaidd = false;
            break;
        }
    }
}

void Game::GetSides(const ARegion::Handle& r,
                    WeakPtrList<Faction>& afacs,
                    WeakPtrList<Faction>& dfacs,
                    PtrList<Location>& atts,
                    PtrList<Location>& defs,
                    const Unit::Handle& att,
                    const Unit::Handle& tar,
                    int ass,
                    bool adv)
{
    if (ass) {
        /* Assassination attempt */
        {
            auto& l = atts.emplace_back(std::make_shared<Location>());
            l->unit = att;
            l->obj = r->GetDummy();
            l->region = r;
        }

        {
            auto& l = defs.emplace_back(std::make_shared<Location>());
            l->unit = tar;
            l->obj = r->GetDummy();
            l->region = r;
        }

        return;
    }

    bool noaida = false, noaidd = false;

    for(const auto& o: r->objects) {
        /* Set building capacity */
        if (o->incomplete < 1 && o->IsBuilding()) {
            o->capacity = static_cast<size_t>(ObjectDefs[o->type].protect);
            o->shipno = 0;
        } else if (o->IsFleet()) {
            o->capacity = 0;
            o->shipno = 0;
        }
    }

    GetSidesForRegion_(r,
                       r,
                       afacs,
                       dfacs,
                       atts,
                       defs,
                       att,
                       tar,
                       adv,
                       noaida,
                       noaidd,
                       true);

    for(const auto& r2_w: r->neighbors)
    {
        if (r2_w.expired())
        {
            continue;
        }
        const auto r2 = r2_w.lock();
        for(const auto& obj: r2->objects) {
            /* Can't get building bonus in another region */
            obj->capacity = 0;
            obj->shipno = obj->ships.size();
        }
        GetSidesForRegion_(r,
                           r2,
                           afacs,
                           dfacs,
                           atts,
                           defs,
                           att,
                           tar,
                           adv,
                           noaida,
                           noaidd);
    }
}

size_t Game::KillDead(const Location::Handle& l, const Battle::Handle& b)
{
    size_t uncontrolled = 0;

    const auto l_unit = l->unit.lock();
    if (!l_unit->IsAlive()) {
        l->region.lock()->Kill(l_unit);
        uncontrolled += l_unit->raised;
        l_unit->raised = 0;
    } else {
        if (!l_unit->advancefrom.expired()) {
            l_unit->MoveUnit( l_unit->advancefrom.lock()->GetDummy() );
        }
        if (l_unit->raised > 0) {
            size_t undead = getrandom(l_unit->raised * 2 / 3 + 1); // This is always <= l_unit->raised as long as l_unit->raised > 0
            size_t skel = l_unit->raised - undead;
            AString tmp = ItemString(Items::Types::I_SKELETON, static_cast<int>(skel));
            if (undead > 0) {
                tmp += " and ";
                tmp += ItemString(Items::Types::I_UNDEAD, static_cast<int>(undead));
            }
            tmp += " rise";
            if ((skel + undead) == 1)
                tmp += "s";
            tmp += " from the grave to join ";
            tmp += l_unit->name;
            tmp += ".";
            l_unit->items.SetNum(Items::Types::I_SKELETON, l_unit->items.GetNum(Items::Types::I_SKELETON) + skel);
            l_unit->items.SetNum(Items::Types::I_UNDEAD, l_unit->items.GetNum(Items::Types::I_UNDEAD) + undead);
            b->AddLine(tmp);
            b->AddLine("");
            l_unit->raised = 0;
        }
    }

    return uncontrolled;
}

int Game::RunBattle(const ARegion::Handle& r, const Unit::Handle& attacker, const Unit::Handle& target, int ass, bool adv)
{
    WeakPtrList<Faction> afacs;
    WeakPtrList<Faction> dfacs;
    PtrList<Location> atts;
    PtrList<Location> defs;
    int result;

    const auto attacker_faction = attacker->faction.lock();
    if (ass) {
        if (attacker->GetAttitude(*r, target) == Attitudes::Types::A_ALLY) {
            attacker->Error("ASSASSINATE: Can't assassinate an ally.");
            return BATTLE_IMPOSSIBLE;
        }
        /* Assassination attempt */
        afacs.push_back(attacker->faction);
        dfacs.push_back(target->faction);
    } else {
        if ( r->IsSafeRegion() ) {
            attacker->Error("ATTACK: No battles allowed in safe regions.");
            return BATTLE_IMPOSSIBLE;
        }
        if (attacker->GetAttitude(*r, target) == Attitudes::Types::A_ALLY) {
            attacker->Error("ATTACK: Can't attack an ally.");
            return BATTLE_IMPOSSIBLE;
        }
        GetDFacs(r, target, dfacs);
        if (!GetFaction2(dfacs, attacker_faction->num).expired()) {
            attacker->Error("ATTACK: Can't attack an ally.");
            return BATTLE_IMPOSSIBLE;
        }
        GetAFacs(r,attacker,target,dfacs,afacs,atts);
    }

    GetSides(r, afacs, dfacs, atts, defs, attacker, target, ass, adv);

    if (atts.empty()) {
        // This shouldn't happen, but just in case
        Awrite(AString("Cannot find any attackers!"));
        return BATTLE_IMPOSSIBLE;
    }
    if (defs.empty()) {
        // This shouldn't happen, but just in case
        Awrite(AString("Cannot find any defenders!"));
        return BATTLE_IMPOSSIBLE;
    }

    auto& b = battles.emplace_back(std::make_shared<Battle>());
    b->WriteSides(r, attacker, target, atts, defs, ass, regions);

    for(const auto& f: factions) {
        if (!GetFaction2(afacs,f->num).expired() || !GetFaction2(dfacs,f->num).expired() || r->Present(*f)) {
            f->battles.push_back(b);
        }
    }
    result = b->Run(r, attacker, atts, target, defs, ass, regions);

    /* Remove all dead units */
    size_t uncontrolled = 0;
    for(const auto& l: atts) {
        uncontrolled += KillDead(l, b);
    }
    for(const auto& l: defs) {
        uncontrolled += KillDead(l, b);
    }
    if (uncontrolled > 0 && monfaction > 0) {
        size_t undead = getrandom(uncontrolled * 2 / 3 + 1); // This is always <= uncontrolled as long as uncontrolled > 0
        size_t skel = uncontrolled - undead;
        AString tmp = ItemString(Items::Types::I_SKELETON, static_cast<int>(skel));
        if (undead > 0) {
            tmp += " and ";
            tmp += ItemString(Items::Types::I_UNDEAD, static_cast<int>(undead));
        }
        tmp += " rise";
        if ((skel + undead) == 1)
            tmp += "s";
        tmp += " from the grave to seek vengeance.";
        Faction::Handle monfac = GetFaction(factions, monfaction);
        Unit::Handle u = GetNewUnit(monfac, 0);
        u->MakeWMon("Undead", Items::Types::I_SKELETON, skel);
        u->items.SetNum(Items::Types::I_UNDEAD, undead);
        u->MoveUnit(r->GetDummy());
        b->AddLine(tmp);
        b->AddLine("");
    }
    return result;
}
