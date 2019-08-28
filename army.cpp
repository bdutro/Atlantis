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
#include "army.h"
#include "gameio.h"
#include "gamedata.h"

enum {
    WIN_NO_DEAD,
    WIN_DEAD,
    LOSS
};

Soldier::Soldier(const Unit::Handle& u, const Object::Handle& o, const Regions& regtype, const Items& r, int ass)
{
    AString abbr;
    Items item;

    race = r;
    unit = u;
    building = *Objects::begin();

    healing = 0;
    healtype = 0;
    healitem.invalidate();
    canbehealed = 1;
    regen = 0;

    armor.invalidate();
    riding.invalidate();
    weapon.invalidate();

    attacks = 1;
    attacktype = ATTACK_COMBAT;

    special = NULL;
    slevel = 0;

    askill = 0;

    dskill[ATTACK_COMBAT] = 0;
    dskill[ATTACK_ENERGY] = -2;
    dskill[ATTACK_SPIRIT] = -2;
    dskill[ATTACK_WEATHER] = -2;
    dskill[ATTACK_RIDING] = 0;
    dskill[ATTACK_RANGED] = 0;
    for (unsigned int i=0; i<NUM_ATTACK_TYPES; i++)
        protection[i] = 0;
    damage = 0;
    hits = u->GetAttribute("toughness");
    if (hits < 1) hits = 1;
    maxhits = hits;
    amuletofi = 0;
    battleItems = 0;

    /* Special case to allow protection from ships */
    if (o->IsFleet() && o->capacity < 1 && o->shipno < o->ships.size()) {
        Objects objectno;

        unsigned int i = 0;
        for(const auto& ship: o->ships) {
            if (o->shipno == i) {
                abbr = ItemDefs[ship->type].name;
                objectno = LookupObject(&abbr);
                if (objectno.isValid() && ObjectDefs[objectno].protect > 0) {
                    o->capacity = static_cast<size_t>(ObjectDefs[objectno].protect) * ship->num;
                    o->type = objectno;
                }
                o->shipno++;
            }
            i++;
            if (o->capacity > 0) break;
        }
    }
    /* Building bonus */
    if (o->capacity) {
        building = o->type;
        //should the runes spell be a base or a bonus?
        for (int i=0; i<NUM_ATTACK_TYPES; i++) {
            if (Globals->ADVANCED_FORTS) {
                protection[i] += ObjectDefs[o->type].defenceArray[i];
            } else
                dskill[i] += ObjectDefs[o->type].defenceArray[i];
        }
        if (o->runes) {
            dskill[ATTACK_ENERGY] = std::max(dskill[ATTACK_ENERGY], o->runes);
            dskill[ATTACK_SPIRIT] = std::max(dskill[ATTACK_SPIRIT], o->runes);
        }
        o->capacity--;
    }

    /* Is this a monster? */
    if (ItemDefs[r].type & IT_MONSTER) {
        const auto& mp = FindMonster(ItemDefs[r].abr, ItemDefs[r].type & IT_ILLUSION);
        if (u->type == U_WMON)
            name = AString(mp.name) + " in " + *(u->name);
        else
            name = AString(mp.name) + " controlled by " + *(u->name);
        askill = mp.attackLevel;
        dskill[ATTACK_COMBAT] += mp.defense[ATTACK_COMBAT];
        if (mp.defense[ATTACK_ENERGY] > dskill[ATTACK_ENERGY]) {
            dskill[ATTACK_ENERGY] = mp.defense[ATTACK_ENERGY];
        }
        if (mp.defense[ATTACK_SPIRIT] > dskill[ATTACK_SPIRIT]) {
            dskill[ATTACK_SPIRIT] = mp.defense[ATTACK_SPIRIT];
        }
        if (mp.defense[ATTACK_WEATHER] > dskill[ATTACK_WEATHER]) {
            dskill[ATTACK_WEATHER] = mp.defense[ATTACK_WEATHER];
        }
        dskill[ATTACK_RIDING] += mp.defense[ATTACK_RIDING];
        dskill[ATTACK_RANGED] += mp.defense[ATTACK_RANGED];
        damage = 0;
        hits = mp.hits;
        if (hits < 1) hits = 1;
        maxhits = hits;
        attacks = mp.numAttacks;
        if (!attacks) attacks = 1;
        special = mp.special;
        slevel = mp.specialLevel;
        if (Globals->MONSTER_BATTLE_REGEN) {
            regen = mp.regen;
            if (regen < 0) regen = 0;
        }
        return;
    }

    name = *(u->name);

    SetupHealing();

    SetupSpell();
    SetupCombatItems();

    // Set up armor
    for (unsigned int i = 0; i < MAX_READY; i++) {
        // Check preferred armor first.
        item = u->readyArmor[i];
        if (!item.isValid()) break;
        abbr = ItemDefs[item].abr;
        item = u->GetArmor(abbr, ass);
        if (item.isValid()) {
            armor = item;
            break;
        }
    }
    if (!armor.isValid()) {
        for (size_t armorType = 1; armorType < ArmorDefs.size(); ++armorType) {
            abbr = ArmorDefs[armorType].abbr;
            item = u->GetArmor(abbr, ass);
            if (item.isValid()) {
                armor = item;
                break;
            }
        }
    }

    //
    // Check if this unit is mounted
    //
    int terrainflags = TerrainDefs[regtype].flags;
    int canFly = (terrainflags & TerrainType::FLYINGMOUNTS);
    int canRide = (terrainflags & TerrainType::RIDINGMOUNTS);
    int ridingBonus = 0;
    if (canFly || canRide) {
        //
        // Mounts of some type _are_ allowed in this region
        //
        for (size_t mountType = 1; mountType < MountDefs.size(); ++mountType) {
            abbr = MountDefs[mountType].abbr;
            item = u->GetMount(abbr, canFly, canRide, ridingBonus);
            if (!item.isValid()) continue;
            // Defer adding the combat bonus until we know if the weapon
            // allows it.  The defense bonus for riding can be added now
            // however.
            dskill[ATTACK_RIDING] += ridingBonus;
            riding = item;
            break;
        }
    }

    //
    // Find the correct weapon for this soldier.
    //
    int attackBonus = 0;
    int defenseBonus = 0;
    int numAttacks = 1;
    for (unsigned int i = 0; i < MAX_READY; i++) {
        // Check the preferred weapon first.
        item = u->readyWeapon[i];
        if (!item.isValid()) break;
        abbr = ItemDefs[item].abr;
        item = u->GetWeapon(abbr,
                            riding,
                            ridingBonus,
                            attackBonus,
                            defenseBonus,
                            numAttacks);
        if (item.isValid()) {
            weapon = item;
            break;
        }
    }
    if (!weapon.isValid()) {
        for (size_t weaponType = 1; weaponType < WeaponDefs.size(); ++weaponType) {
            abbr = WeaponDefs[weaponType].abbr;
            item = u->GetWeapon(abbr, riding, ridingBonus, attackBonus,
                    defenseBonus, numAttacks);
            if (item.isValid()) {
                weapon = item;
                break;
            }
        }
    }
    // If we did not get a weapon, set attack and defense bonuses to
    // combat skill (and riding bonus if applicable).
    if (!weapon.isValid()) {
        attackBonus = u->GetAttribute("combat") + ridingBonus;
        defenseBonus = attackBonus;
        numAttacks = 1;
    } else {
        // Okay.  We got a weapon.  If this weapon also has a special
        // and we don't have a special set, use that special.
        // Weapons (like Runeswords) which are both weapons and battle
        // items will be skipped in the battle items setup and handled
        // here.
        if ((ItemDefs[weapon].type & IT_BATTLE) && special == NULL) {
            const auto& pBat = FindBattleItem(ItemDefs[weapon].abr);
            special = pBat.special;
            slevel = pBat.skillLevel;
        }
    }

    u->PracticeAttribute("combat");

    // Set the attack and defense skills
    // These will include the riding bonus if they should be included.
    askill += attackBonus;
    dskill[ATTACK_COMBAT] += defenseBonus;
    attacks = numAttacks;
}

void Soldier::SetupSpell()
{
    const auto unit_sp = unit.lock();
    if (unit_sp->type != U_MAGE && unit_sp->type != U_GUARDMAGE) return;

    if (unit_sp->combat.isValid()) {
        slevel = unit_sp->GetSkill(unit_sp->combat);
        if (!slevel) {
            //
            // The unit can't cast this spell!
            //
            unit_sp->combat.invalidate();
            return;
        }

        const auto& pST = SkillDefs[static_cast<size_t>(unit_sp->combat)];
        if (!(pST.flags & SkillType::COMBAT)) {
            //
            // This isn't a combat spell!
            //
            unit_sp->combat.invalidate();
            return;
        }

        special = pST.special;
        unit_sp->Practice(unit_sp->combat);
    }
}

void Soldier::SetupCombatItems()
{
    int exclusive = 0;
    
    const auto unit_sp = unit.lock();

    for (size_t battleType = 1; battleType < BattleItemDefs.size(); ++battleType) {
        const auto& pBat = BattleItemDefs[battleType];

        AString abbr = pBat.abbr;
        Items item = unit_sp->GetBattleItem(abbr);
        if (!item.isValid()) continue;

        // If we are using the ready command, skip this item unless
        // it's the right one, or unless it is a shield which doesn't
        // need preparing.
        if (!Globals->USE_PREPARE_COMMAND ||
                (!unit_sp->readyItem.isValid() &&
                 (Globals->USE_PREPARE_COMMAND == GameDefs::PREPARE_NORMAL)) ||
                (item == unit_sp->readyItem) ||
                (pBat.flags & BattleItemType::SHIELD)) {
            if ((pBat.flags & BattleItemType::SPECIAL) && special != NULL) {
                // This unit already has a special attack so give the item
                // back to the unit as they aren't going to use it.
                unit_sp->items.SetNum(item, unit_sp->items.GetNum(item)+1);
                continue;
            }
            if (pBat.flags & BattleItemType::MAGEONLY &&
                    unit_sp->type != U_MAGE && unit_sp->type != U_GUARDMAGE &&
                    unit_sp->type != U_APPRENTICE) {
                // Only mages/apprentices can use this item so give the
                // item back to the unit as they aren't going to use it.
                unit_sp->items.SetNum(item, unit_sp->items.GetNum(item)+1);
                continue;
            }

            if (pBat.flags & BattleItemType::EXCLUSIVE) {
                if (exclusive) {
                    // Can only use one exclusive item, and we already
                    // have one, so give the extras back.
                    unit_sp->items.SetNum(item, unit_sp->items.GetNum(item)+1);
                    continue;
                }
                exclusive = 1;
            }

            if (pBat.flags & BattleItemType::MAGEONLY) {
                unit_sp->Practice(Skills::Types::S_MANIPULATE);
            }

            /* Make sure amulets of invulnerability are marked */
            if (item == Items::Types::I_AMULETOFI) {
                amuletofi = 1;
            }

            SET_BIT(battleItems, battleType);

            if (pBat.flags & BattleItemType::SPECIAL) {
                special = pBat.special;
                slevel = pBat.skillLevel;
            }

            if (pBat.flags & BattleItemType::SHIELD) {
                const auto& sp = FindSpecial(pBat.special);
                for (const auto shield: sp.shield) {
                    if (shield == NUM_ATTACK_TYPES) {
                        for (int j = 0; j < NUM_ATTACK_TYPES; j++) {
                            if (dskill[j] < static_cast<int>(pBat.skillLevel))
                                dskill[j] = static_cast<int>(pBat.skillLevel);
                        }
                    } else if (shield >= 0) {
                        if (dskill[shield] < static_cast<int>(pBat.skillLevel))
                            dskill[shield] = static_cast<int>(pBat.skillLevel);
                    }
                }
            }
        } else {
            // We are using prepared items and this item is NOT the one
            // we have prepared, so give it back to the unit as they won't
            // use it.
            unit_sp->items.SetNum(item, unit_sp->items.GetNum(item)+1);
            continue;
        }
    }
}

int Soldier::HasEffect(char const *eff)
{
    if (eff == NULL) return 0;

    return effects[eff];
}

void Soldier::SetEffect(char const *eff)
{
    if (eff == NULL) return;

    try
    {
        const auto& e = FindEffect(eff);

        askill += e.attackVal;

        for (const auto& def_mod: e.defMods) {
            if (def_mod.type != -1)
            {
                dskill[def_mod.type] += def_mod.val;
            }
        }

        if (e.cancelEffect != NULL) ClearEffect(e.cancelEffect);

        if (!(e.flags & EffectType::EFF_NOSET)) effects[eff] = 1;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Soldier::ClearEffect(char const *eff)
{
    if (eff == NULL) return;

    try
    {
        const auto& e = FindEffect(eff);

        askill -= e.attackVal;

        for (const auto& def_mod: e.defMods) {
            if (def_mod.type != -1)
            {
                dskill[def_mod.type] -= def_mod.val;
            }
        }

        effects[eff] = 0;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Soldier::ClearOneTimeEffects(void)
{
    for (const auto& i: EffectDefs) {
        if (HasEffect(i.name) && (i.flags & EffectType::EFF_ONESHOT))
            ClearEffect(i.name);
    }
}

bool Soldier::ArmorProtect(int weaponClass)
{
    //
    // Return 1 if the armor is successful
    //
    try
    {
        if (!armor.isValid() || armor == 0)
        {
            return false;
        }
        const auto& pArm = FindArmor(ItemDefs[armor].abr);
        int chance = pArm.saves[weaponClass];

        if (chance <= 0)
        {
            return false;
        }
        if (chance > getrandom(pArm.from))
        {
            return true;
        }

    }
    catch(const NoSuchItemException&)
    {
    }

    return false;
}

void Soldier::RestoreItems()
{
    const auto unit_sp = unit.lock();
    if (healing && healitem.isValid()) {
        if (healitem == Items::Types::I_HERBS) {
            unit_sp->items.SetNum(healitem,
                    unit_sp->items.GetNum(healitem) + healing);
        } else if (healitem == Items::Types::I_HEALPOTION) {
            unit_sp->items.SetNum(healitem,
                    unit_sp->items.GetNum(healitem)+healing/10);
        }
    }
    if (weapon.isValid())
    {
        unit_sp->items.SetNum(weapon,unit_sp->items.GetNum(weapon) + 1);
    }
    if (armor.isValid())
    {
        unit_sp->items.SetNum(armor,unit_sp->items.GetNum(armor) + 1);
    }
    if (riding.isValid())
    {
        unit_sp->items.SetNum(riding,unit_sp->items.GetNum(riding) + 1);
    }

    for (size_t battleType = 1; battleType < BattleItemDefs.size(); ++battleType) {
        const BattleItemType& pBat = BattleItemDefs[battleType];

        if (GET_BIT(battleItems, battleType)) {
            AString itm(pBat.abbr);
            Items item = LookupItem(itm);
            unit_sp->items.SetNum(item, unit_sp->items.GetNum(item) + 1);
        }
    }
}

void Soldier::Alive(int state)
{
    RestoreItems();
    const auto unit_sp = unit.lock();
    if (state == LOSS) {
        unit_sp->canattack = 0;
        unit_sp->routed = 1;
        /* Guards with amuletofi will not go off guard */
        if (!amuletofi &&
            (unit_sp->guard == GUARD_GUARD || unit_sp->guard == GUARD_SET)) {
            unit_sp->guard = GUARD_NONE;
        }
    } else {
        unit_sp->advancefrom.reset();
    }

    if (state == WIN_DEAD) {
        unit_sp->canattack = 0;
        unit_sp->nomove = 1;
    }
}

void Soldier::Dead()
{
    RestoreItems();

    const auto unit_sp = unit.lock();
    unit_sp->SetMen(race,unit_sp->GetMen(race) - 1);
}

size_t Army::BuildArmy_(const PtrList<Location>& locs, int regtype, int ass)
{
    size_t x = 0;
    size_t y = count;

    for(const auto& l: locs) {
        const auto u = l->unit.lock();
        const auto obj = l->obj.lock();
        if (ass) {
            for(const auto& it: u->items) {
                if (it) {
                    if (ItemDefs[ it->type ].type & IT_MAN) {
                            soldiers[x] = std::make_shared<Soldier>(u, obj, regtype,
                                                                    it->type, ass);
                            hitstotal = soldiers[x]->hits;
                            ++x;
                            return x;
                    }
                }
            }
        } else {
            auto ii = u->items.begin();
            do {
                const auto& it = *ii;
                if (IsSoldier(it->type)) {
                    for (unsigned int i = 0; i < it->num; i++) {
                        if ((ItemDefs[ it->type ].type & IT_MAN) &&
                                u->GetFlag(FLAG_BEHIND)) {
                            --y;
                            soldiers[y] = std::make_shared<Soldier>(u, obj, regtype,
                                                                    it->type);
                            hitstotal += soldiers[y]->hits;
                        } else {
                            soldiers[x] = std::make_shared<Soldier>(u, obj, regtype,
                                                                    it->type);
                            hitstotal += soldiers[x]->hits;
                            ++x;
                        }
                    }
                }
                ++ii;
            } while(ii != u->items.end());
        }
    }

    return x;
}

Army::Army(const Unit::Handle& ldr, const PtrList<Location>& locs, int regtype, int ass)
{
    int tacspell = 0;
    Unit::Handle tactician = ldr;

    leader = ldr;
    round = 0;
    tac = ldr->GetAttribute("tactics");
    count = 0;
    hitstotal = 0;

    if (ass) {
        count = 1;
        ldr->losses = 0;
    } else {
        for(const auto& l: locs) {
            const auto u = l->unit.lock();
            count += u->GetSoldiers();
            u->losses = 0;
            int temp = u->GetAttribute("tactics");
            if (temp > tac) {
                tac = temp;
                tactician = u;
            }
        }
    }
    // If TACTICS_NEEDS_WAR is enabled, we don't want to push leaders 
    // from tact-4 to tact-5! Also check that we have skills, otherwise
    // we get a nasty core dump ;)
    if (Globals->TACTICS_NEEDS_WAR && !tactician->skills.empty()) {
        size_t currskill = tactician->skills.GetDays(Skills::Types::S_TACTICS) / tactician->GetMen();
        if (currskill < 450 - Globals->SKILL_PRACTICE_AMOUNT) {
            tactician->PracticeAttribute("tactics");
        }
    } else { // Only Globals->TACTICS_NEEDS_WAR == 0
        tactician->PracticeAttribute("tactics");
    }
    soldiers.resize(count);
    canfront = BuildArmy_(locs, regtype, ass);

    tac = tac + tacspell;

    canbehind = count;
    notfront = count;
    notbehind = count;

    hitsalive = hitstotal;

    if (!NumFront()) {
        canfront = canbehind;
        notfront = notbehind;
    }
}

void Army::Reset() {
    canfront = notfront;
    canbehind = notbehind;
    notfront = notbehind;
}

void Army::WriteLosses(Battle& b) {
    b.AddLine(*(leader.lock()->name) + " loses " + (count - NumAlive()) + ".");

    if (notbehind != count) {
        WeakPtrList<Unit> units;
        for (size_t i = notbehind; i<count; i++) {
            const auto& u_w = soldiers[i]->unit;
            const auto up = u_w.lock();
            if (GetUnitList(units, up).expired()) {
                units.push_back(u_w);
            }
        }

        int comma = 0;
        AString damaged;
        for(const auto& u: units) {
            if (comma) {
                damaged += AString(", ") + AString(u.lock()->num);
            } else {
                damaged = AString("Damaged units: ") + AString(u.lock()->num);
                comma = 1;
            }
        }

        b.AddLine(damaged + ".");
    }
}

void Army::GetMonSpoils(ItemList& spoils, const Items& monitem, size_t free)
{
    if ((Globals->MONSTER_NO_SPOILS > 0) &&
            (free >= Globals->MONSTER_SPOILS_RECOVERY)) {
        // This monster is in it's period of absolutely no spoils.
        return;
    }

    /* First, silver */
    const auto& mp = FindMonster(ItemDefs[monitem].abr, ItemDefs[monitem].type & IT_ILLUSION);
    size_t silv = mp.silver;
    if ((Globals->MONSTER_NO_SPOILS > 0) && (free > 0)) {
        // Adjust the spoils for length of freedom.
        silv *= (Globals->MONSTER_SPOILS_RECOVERY-free);
        silv /= Globals->MONSTER_SPOILS_RECOVERY;
    }
    spoils.SetNum(Items::Types::I_SILVER,
                  spoils.GetNum(Items::Types::I_SILVER) + getrandom(silv));

    if (mp.spoiltype == -1) return;

    int thespoil = mp.spoiltype;

    if (thespoil == IT_NORMAL && getrandom(2)) thespoil = IT_TRADE;

    int count = 0;
    for (auto i = Items::begin(); i != Items::end(); ++i) {
        if ((ItemDefs[*i].type & thespoil) &&
                !(ItemDefs[*i].type & IT_SPECIAL) &&
                !(ItemDefs[*i].type & IT_SHIP) &&
                !(ItemDefs[*i].flags & ItemType::DISABLED)) {
            count ++;
        }
    }
    if (count == 0) return;
    count = getrandom(count) + 1;

    for (auto i = Items::begin(); i != Items::end(); ++i) {
        if ((ItemDefs[*i].type & thespoil) &&
                !(ItemDefs[*i].type & IT_SPECIAL) &&
                !(ItemDefs[*i].type & IT_SHIP) &&
                !(ItemDefs[*i].flags & ItemType::DISABLED)) {
            count--;
            if (count == 0) {
                thespoil = static_cast<int>(*i);
                break;
            }
        }
    }

    size_t val = getrandom(mp.silver * 2);
    if ((Globals->MONSTER_NO_SPOILS > 0) && (free > 0)) {
        // Adjust for length of monster freedom.
        val *= (Globals->MONSTER_SPOILS_RECOVERY-free);
        val /= Globals->MONSTER_SPOILS_RECOVERY;
    }

    auto baseprice = ItemDefs[static_cast<size_t>(thespoil)].baseprice;
    spoils.SetNum(thespoil, spoils.GetNum(thespoil) + (val + getrandom(baseprice)) / baseprice);
}

void Army::Regenerate(Battle& b)
{
    for (size_t i = 0; i < count; i++) {
        const auto& s = soldiers[i];
        if (i < notbehind) {
            int diff = s->maxhits - s->hits;
            if (diff > 0) {
                AString aName = s->name;

                if (s->damage != 0) {
                    b.AddLine(aName + " takes " + s->damage +
                            " hits bringing it to " + s->hits + "/" +
                            s->maxhits + ".");
                    s->damage = 0;
                } else {
                    b.AddLine(aName + " takes no hits leaving it at " +
                            s->hits + "/" + s->maxhits + ".");
                }
                if (s->regen) {
                    int regen = s->regen;
                    if (regen > diff) regen = diff;
                    s->hits += regen;
                    b.AddLine(aName + " regenerates " + regen +
                            " hits bringing it to " + s->hits + "/" +
                            s->maxhits + ".");
                }
            }
        }
    }
}

void Army::Lose(Battle& b, ItemList& spoils)
{
    WriteLosses(b);
    for (size_t i=0; i<count; i++) {
        auto& s = soldiers[i];
        if (i < notbehind) {
            s->Alive(LOSS);
        } else {
            const auto& up = s->unit.lock();
            if ((up->type==U_WMON) && (ItemDefs[s->race].type&IT_MONSTER))
                GetMonSpoils(spoils, s->race, up->free);
            s->Dead();
        }
        s.reset();
    }
}

void Army::Tie(Battle& b)
{
    WriteLosses(b);
    for (size_t x=0; x < count; x++) {
        auto& s = soldiers[x];
        if (x<NumAlive()) {
            s->Alive(WIN_DEAD);
        } else {
            s->Dead();
        }
        s.reset();
    }
}

bool Army::CanBeHealed()
{
    for (size_t i = notbehind; i<count; i++) {
        const auto& temp = soldiers[i];
        if (temp->canbehealed)
        {
            return true;
        }
    }
    return false;
}

void Army::DoHeal(Battle& b)
{
    // Do magical healing
    for (size_t i = 5; i > 0; --i)
        DoHealLevel(b, i, 0);
    // Do Normal healing
    DoHealLevel(b, 1, 1);
}

void Army::DoHealLevel(Battle& b, size_t type, int useItems)
{
    unsigned int rate = HealDefs[type].rate;

    for (size_t i = 0; i < NumAlive(); i++) {
        const auto& s = soldiers[i];
        int n = 0;
        if (!CanBeHealed()) break;
        if (s->healtype <= 0) continue;
        size_t s_healtype = static_cast<size_t>(s_healtype);
        // This should be here.. Use the best healing first
        if (s_healtype != type) continue;
        if (!s->healing) continue;
        if (useItems) {
            if (!s->healitem.isValid())
            {
                continue;
            }
            if (s->healitem != Items::Types::I_HEALPOTION)
            {
                s->unit.lock()->Practice(Skills::Types::S_HEALING);
            }
        } else {
            if (s->healitem.isValid()) continue;
            s->unit.lock()->Practice(Skills::Types::S_MAGICAL_HEALING);
        }

        while (s->healing) {
            if (!CanBeHealed()) break;
            size_t j = getrandom(count - NumAlive()) + notbehind;
            const auto& temp = soldiers[j];
            if (temp->canbehealed) {
                s->healing--;
                if (getrandom(100U) < rate) {
                    n++;
                    soldiers[j] = soldiers[notbehind];
                    soldiers[notbehind] = temp;
                    notbehind++;
                } else
                    temp->canbehealed = 0;
            }
        }
        b.AddLine(*(s->unit.lock()->name) + " heals " + n + ".");
    }
}

void Army::Win(Battle& b, const ItemList& spoils)
{
    int wintype;

    DoHeal(b);

    WriteLosses(b);

    size_t na = NumAlive();

    if (count - na) wintype = WIN_DEAD;
    else wintype = WIN_NO_DEAD;

    WeakPtrList<Unit> units;

    for (size_t x = 0; x < count; x++) {
        const auto& s = soldiers[x];
        if (x < NumAlive())
        {
            s->Alive(wintype);
        }
        else
        {
            s->Dead();
        }
    }

    for(const auto& i: spoils) {
        if (i && na) {
            size_t ns;

            do {
                units.clear();
                // Make a list of units who can get this type of spoil
                for (size_t x = 0; x < na; x++) {
                    const auto& u_w = soldiers[x]->unit;
                    const auto u = u_w.lock();
                    if (u->CanGetSpoil(i)) {
                        units.push_back(u_w);
                    }
                }

                ns = units.size();
                if (ItemDefs[i->type].type & IT_SHIP) {
                    size_t t = getrandom(ns);
                    auto it = std::next(units.begin(), static_cast<ssize_t>(t));
                    const auto& up_w = *it;
                    if (!up_w.expired())
                    {
                        const auto up = up_w.lock();
                        if(up->CanGetSpoil(i)) {
                            up->items.SetNum(i->type, i->num);
                            up->faction.lock()->DiscoverItem(i->type, 0, 1);
                            i->num = 0;
                        }
                    }
                    break;
                }
                while (ns > 0 && i->num >= ns) {
                    size_t chunk = 1;
                    if (!ItemDefs[i->type].weight) {
                        chunk = i->num / ns;
                    }
                    auto it = units.begin();
                    while(it != units.end())
                    {
                        const auto& up = it->lock();
                        if (up->CanGetSpoil(i)) {
                            up->items.SetNum(i->type,
                                             up->items.GetNum(i->type) + chunk);
                            up->faction.lock()->DiscoverItem(i->type, 0, 1);
                            i->num -= chunk;
                            ++it;
                        } else {
                            it = units.erase(it);
                            ns--;
                        }
                    }
                }
                while (ns > 0 && i->num > 0) {
                    size_t t = getrandom(ns);
                    auto it = std::next(units.begin(), static_cast<ssize_t>(t));
                    const auto& up_w = *it;
                    if (!up_w.expired())
                    {
                        const auto up = up_w.lock();
                        if(up->CanGetSpoil(i)) {
                            up->items.SetNum(i->type,
                                             up->items.GetNum(i->type) + 1);
                            up->faction.lock()->DiscoverItem(i->type, 0, 1);
                            i->num--;
                        }
                    } else {
                        units.erase(it);
                        ns--;
                    }
                }
                units.clear();
            } while (ns > 0 && i->num > 0);
        }
    }
    for(auto& s: soldiers)
    {
        s.reset();
    }
}

bool Army::Broken()
{
    if (Globals->ARMY_ROUT == GameDefs::ARMY_ROUT_FIGURES) {
        if ((NumAlive() << 1) < count) return true;
    } else {
        if ((hitsalive << 1) < hitstotal) return true;
    }
    return false;
}

size_t Army::NumSpoilers()
{
    size_t na = NumAlive();
    size_t count = 0;
    for (size_t x=0; x < na; x++) {
        const auto u = soldiers[x]->unit.lock();
        if (!(u->flags & FLAG_NOSPOILS)) count++;
    }
    return count;
}

size_t Army::NumAlive()
{
    return notbehind;
}

size_t Army::CanAttack()
{
    return canbehind;
}

size_t Army::NumFront()
{
    return (canfront + notfront - canbehind);
}

Soldier::Handle Army::GetAttacker(size_t i, bool &behind)
{
    const auto& retval = soldiers[i];
    if (i < canfront) {
        soldiers[i] = soldiers[canfront-1];
        soldiers[canfront-1] = soldiers[canbehind-1];
        soldiers[canbehind-1] = retval;
        canfront--;
        canbehind--;
        behind = 0;
        return retval;
    }
    soldiers[i] = soldiers[canbehind-1];
    soldiers[canbehind-1] = soldiers[notfront-1];
    soldiers[notfront-1] = retval;
    canbehind--;
    notfront--;
    behind = 1;
    return retval;
}

ssize_t Army::GetTargetNum(char const *special)
{
    size_t tars = NumFront();
    if (tars == 0) {
        canfront = canbehind;
        notfront = notbehind;
        tars = NumFront();
        if (tars == 0) return -1;
    }

    
    try
    {
        const auto& sp = FindSpecial(special);

        if (!sp.targflags) {
            throw NoSuchItemException();
        }

        int validtargs = 0;
        bool found_start = false;
        size_t start;

        for (size_t i = 0; i < canfront; i++) {
            if (CheckSpecialTarget(special, i)) {
                validtargs++;
                // slight scan optimisation - skip empty initial sequences
                if (!found_start)
                {
                    found_start = true;
                    start = i;
                }
            }
        }
        for (size_t i = canbehind; i < notfront; i++) {
            if (CheckSpecialTarget(special, i)) {
                validtargs++;
                // slight scan optimisation - skip empty initial sequences
                if (!found_start)
                {
                    found_start = true;
                    start = i;
                }
            }
        }
        if (validtargs) {
            int targ = getrandom(validtargs);
            for (size_t i = start; i < notfront; i++) {
                if (i == canfront) i = canbehind;
                if (CheckSpecialTarget(special, i)) {
                    if (!targ--)
                    {
                        return static_cast<ssize_t>(i);
                    }
                }
            }
        }
    }
    catch(const NoSuchItemException&)
    {
        size_t i = getrandom(tars);
        if (i < canfront)
        {
            return static_cast<ssize_t>(i);
        }
        return static_cast<ssize_t>(i + canbehind - canfront);
    }

    return -1;
}

ssize_t Army::GetEffectNum(char const *effect)
{
    size_t validtargs = 0;
    bool found_start = false;
    size_t start;

    for (size_t i = 0; i < canfront; i++) {
        if (soldiers[i]->HasEffect(effect)) {
            validtargs++;
            // slight scan optimisation - skip empty initial sequences
            if (!found_start)
            {
                found_start = true;
                start = i;
            }
        }
    }
    for (size_t i = canbehind; i < notfront; i++) {
        if (soldiers[i]->HasEffect(effect)) {
            validtargs++;
            // slight scan optimisation - skip empty initial sequences
            if (!found_start)
            {
                found_start = true;
                start = i;
            }
        }
    }
    if (validtargs) {
        size_t targ = getrandom(validtargs);
        for (size_t i = start; i < notfront; i++) {
            if (i == canfront)
            {
                i = canbehind;
            }
            if (soldiers[i]->HasEffect(effect)) {
                if (!targ--)
                {
                    return static_cast<ssize_t>(i);
                }
            }
        }
    }
    return -1;
}

Soldier::Handle Army::GetTarget(size_t i)
{
    return soldiers[i];
}

template<typename T>
bool Hits(T a, T d)
{
    T tohit = 1,tomiss = 1;
    if (a > d) {
        tohit = 1 << (a-d);
    } else if (d > a) {
        tomiss = 1 << (d-a);
    }
    if (getrandom(tohit+tomiss) < tohit) return true;
    return false;
}

template<typename T, typename U>
typename std::enable_if<std::is_signed<T>::value && !std::is_signed<U>::value, bool>::type
Hits(T a, U b)
{
    return Hits(a, static_cast<T>(b));
}

template<typename T, typename U>
typename std::enable_if<!std::is_signed<T>::value && std::is_signed<U>::value, bool>::type
Hits(T a, U b)
{
    return Hits(static_cast<U>(a), b);
}

int Army::RemoveEffects(int num, char const *effect)
{
    int ret = 0;
    for (int i = 0; i < num; i++) {
        //
        // Try to find a target unit.
        //
        ssize_t tarnum = GetEffectNum(effect);
        if (tarnum == -1) continue;
        auto tar = GetTarget(static_cast<size_t>(tarnum));

        //
        // Remove the effect
        //
        tar->ClearEffect(effect);
        ret++;
    }
    return(ret);
}

int Army::DoAnAttack(char const *special, int numAttacks, int attackType,
        int attackLevel, int flags, int weaponClass, char const *effect,
        int mountBonus, const Soldier::Handle& attacker)
{
    /* 1. Check against Global effects (not sure how yet) */
    /* 2. Attack shield */
    int combat = 0;
    int canShield = 0;
    switch(attackType) {
        case ATTACK_RANGED:
            canShield = 1;
            // fall through
        case ATTACK_COMBAT:
        case ATTACK_RIDING:
            combat = 1;
            break;
        case ATTACK_ENERGY:
        case ATTACK_WEATHER:
        case ATTACK_SPIRIT:
            canShield = 1;
            break;
    }

    if (canShield) {
        int shieldType = attackType;

        const auto hi = shields.GetHighShield(shieldType);
        if (hi != shields.end()) {
            /* Check if we get through shield */
            if (!Hits(attackLevel, hi->shieldskill)) {
                return -1;
            }

            if (effect != NULL && !combat) {
                /* We got through shield... if killing spell, destroy shield */
                shields.erase(hi);
            }
        }
    }

    //
    // Now, loop through and do attacks
    //
    int ret = 0;
    for (int i = 0; i < numAttacks; i++) {
        /* 3. Get the target */
        ssize_t tarnum_signed = GetTargetNum(special);
        if (tarnum_signed == -1) continue;
        size_t tarnum = static_cast<size_t>(tarnum_signed);
        auto tar = GetTarget(tarnum);
        int tarFlags = 0;
        if (tar->weapon.isValid()) {
            const auto& pw = FindWeapon(ItemDefs[tar->weapon].abr);
            tarFlags = pw.flags;
        }

        /* 4. Add in any effects, if applicable */
        int tlev = 0;
        if (attackType != NUM_ATTACK_TYPES)
            tlev = tar->dskill[ attackType ];
        if (special != NULL) {
            const auto& sp = FindSpecial(special);
            if ((sp.effectflags & SpecialType::FX_NOBUILDING) && tar->building)
                tlev -= 2;
        }

        /* 4.1 Check whether defense is allowed against this weapon */
        if ((flags & WeaponType::NODEFENSE) && (tlev > 0)) tlev = 0;

        if (!(flags & WeaponType::RANGED)) {
            /* 4.2 Check relative weapon length */
            int attLen = 1;
            int defLen = 1;
            if (flags & WeaponType::LONG) attLen = 2;
            else if (flags & WeaponType::SHORT) attLen = 0;
            if (tarFlags & WeaponType::LONG) defLen = 2;
            else if (tarFlags & WeaponType::SHORT) defLen = 0;
            if (attLen > defLen) attackLevel++;
            else if (defLen > attLen) tlev++;
        }

        /* 4.3 Add bonuses versus mounted */
        if (tar->riding.isValid()) attackLevel += mountBonus;

        /* 5. Attack soldier */
        if (attackType != NUM_ATTACK_TYPES) {
            if (!(flags & WeaponType::ALWAYSREADY)) {
                int failchance = 2;
                if (Globals->ADVANCED_FORTS) {
                    failchance += (tar->protection[attackType]+1)/2;
                }
                if (getrandom(failchance)) {
                    continue;
                }
            }

            if (!Hits(attackLevel,tlev)) {
                continue;
            }
        }

        /* 6. If attack got through, apply effect, or kill */
        if (effect == NULL) {
            /* 7. Last chance... Check armor */
            if (tar->ArmorProtect(weaponClass)) {
                continue;
            }

            /* 8. Seeya! */
            Kill(tarnum);
            ret++;
            if ((ItemDefs[tar->race].type & IT_MAN) &&
                (ItemDefs[attacker->race].type & IT_UNDEAD)) {
                if (getrandom(100) < Globals->UNDEATH_CONTAGION) {
                    attacker->unit.lock()->raised++;
                    tar->canbehealed = 0;
                }
            }
        } else {
            if (tar->HasEffect(effect)) {
                continue;
            }
            tar->SetEffect(effect);
            ret++;
        }
    }
    return ret;
}

void Army::Kill(size_t killed)
{
    const auto& temp = soldiers[killed];

    if (temp->amuletofi) return;

    if (Globals->ARMY_ROUT == GameDefs::ARMY_ROUT_HITS_INDIVIDUAL)
        hitsalive--;
    temp->hits--;
    temp->damage++;
    if (temp->hits > 0) return;
    temp->unit.lock()->losses++;
    if (Globals->ARMY_ROUT == GameDefs::ARMY_ROUT_HITS_FIGURE) {
        if (ItemDefs[temp->race].type & IT_MONSTER) {
            const auto& mp = FindMonster(ItemDefs[temp->race].abr, ItemDefs[temp->race].type & IT_ILLUSION);
            hitsalive -= mp.hits;
        } else {
            // Assume everything that is a solder and isn't a monster is a
            // man.
            hitsalive--;
        }
    }

    if (killed < canfront) {
        soldiers[killed] = soldiers[canfront-1];
        soldiers[canfront-1] = temp;
        killed = canfront - 1;
        canfront--;
    }

    if (killed < canbehind) {
        soldiers[killed] = soldiers[canbehind-1];
        soldiers[canbehind-1] = temp;
        killed = canbehind-1;
        canbehind--;
    }

    if (killed < notfront) {
        soldiers[killed] = soldiers[notfront-1];
        soldiers[notfront-1] = temp;
        killed = notfront-1;
        notfront--;
    }

    soldiers[killed] = soldiers[notbehind-1];
    soldiers[notbehind-1] = temp;
    notbehind--;
}
