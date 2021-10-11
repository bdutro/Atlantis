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
// Date        Person          Comment
// ----        ------          -------
// 2001/Jul/08 Joseph Traub    Moved functions t here from game.cpp
//
#include "game.h"
#include "gamedata.h"

static ManType& FindRace_(char const *abbr);

static ManType& FindRace_(char const *abbr)
{
    return ManDefs.FindItemByAbbr(abbr);
}

static MonType& FindMonster_(char const *abbr, int illusion);

static MonType& FindMonster_(char const *abbr, int illusion)
{
    return MonDefs.FindItemByAbbr(GetMonsterTag(abbr, illusion));
}

static ArmorType& FindArmor_(char const *abbr);

static ArmorType& FindArmor_(char const *abbr)
{
    return ArmorDefs.FindItemByAbbr(abbr);
}

static WeaponType& FindWeapon_(char const *abbr);

static WeaponType& FindWeapon_(char const *abbr)
{
    return WeaponDefs.FindItemByAbbr(abbr);
}

static MountType& FindMount_(char const *abbr);

static MountType& FindMount_(char const *abbr)
{
    return MountDefs.FindItemByAbbr(abbr);
}

static BattleItemType& FindBattleItem_(char const *abbr);

static BattleItemType& FindBattleItem_(char const *abbr)
{
    return BattleItemDefs.FindItemByAbbr(abbr);
}

static SpecialType& FindSpecial_(char const *key);

static SpecialType& FindSpecial_(char const *key)
{
    return SpecialDefs.FindItemByKey(key);
}

static EffectType& FindEffect_(char const *effect);

static EffectType& FindEffect_(char const *effect)
{
    return EffectDefs.FindItemByName(effect);
}

static RangeType& FindRange_(char const *range);

static RangeType& FindRange_(char const *range)
{
    return RangeDefs.FindItemByKey(range);
}

static AttribModType& FindAttrib_(char const *attrib);

static AttribModType& FindAttrib_(char const *attrib)
{
    return AttribDefs.FindItemByKey(attrib);
}

void Game::EnableSkill(const Skills& sk)
{
    if (!sk.isValid())
    {
        return;
    }
    SkillDefs[sk].flags.clear(SkillType::SkillFlags::DISABLED);
}

void Game::DisableSkill(const Skills& sk)
{
    if (!sk.isValid())
    {
        return;
    }
    SkillDefs[sk].flags.set(SkillType::SkillFlags::DISABLED);
}

void Game::ModifySkillDependancy(const Skills& sk, int i, char const *dep, int lev)
{
    if (i < 0)
    {
        return;
    }
    if (lev < 0)
    {
        return;
    }
    if (!sk.isValid())
    {
        return;
    }
    auto& skill_def = SkillDefs[sk];

    const size_t i_u = static_cast<size_t>(i);
    if(i_u >= skill_def.depends.size())
    {
        return;
    }
    try
    {
        FindSkill(dep);
        skill_def.depends[i_u].skill = dep;
        skill_def.depends[i_u].level = static_cast<unsigned int>(lev);
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySkillCost(const Skills& sk, int cost)
{
    if (cost < 0)
    {
        return;
    }
    if (!sk.isValid())
    {
        return;
    }
    SkillDefs[sk].cost = static_cast<unsigned int>(cost);
}

void Game::ModifySkillSpecial(const Skills& sk, char const *special)
{
    if (!sk.isValid())
    {
        return;
    }
    try
    {
        FindSpecial_(special);
        SkillDefs[sk].special = special;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySkillRange(const Skills& sk, char const *range)
{
    if (!sk.isValid())
    {
        return;
    }
    try
    {
        FindRange_(range);
        SkillDefs[sk].range = range;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::EnableItem(const Items& item)
{
    if (!item.isValid())
    {
        return;
    }
    ItemDefs[item].flags &= ~ItemType::DISABLED;
}

void Game::DisableItem(const Items& item)
{
    if (!item.isValid())
    {
        return;
    }
    ItemDefs[item].flags |= ItemType::DISABLED;
}

void Game::ModifyItemFlags(const Items& it, int flags)
{
    if (!it.isValid())
    {
        return;
    }
    ItemDefs[it].flags = flags;
}

void Game::ModifyItemType(const Items& it, int type)
{
    if (!it.isValid())
    {
        return;
    }
    ItemDefs[it].type = type;
}

void Game::ModifyItemWeight(const Items& it, int weight)
{
    if (!it.isValid())
    {
        return;
    }
    if (weight < 0)
    {
        weight = 0;
    }
    ItemDefs[it].weight = static_cast<size_t>(weight);
}

void Game::ModifyItemBasePrice(const Items& it, int price)
{
    if (!it.isValid())
    {
        return;
    }
    if (price < 0)
    {
        price = 0;
    }
    ItemDefs[it].baseprice = static_cast<size_t>(price);
}

void Game::ModifyItemCapacities(const Items& it, int wlk, int rid, int fly, int swm)
{
    if (!it.isValid())
    {
        return;
    }
    if (wlk < 0) wlk = 0;
    if (rid < 0) rid = 0;
    if (fly < 0) fly = 0;
    if (swm < 0) swm = 0;

    auto& item_def = ItemDefs[it];
    item_def.walk = static_cast<unsigned int>(wlk);
    item_def.ride = static_cast<unsigned int>(rid);
    item_def.fly = static_cast<unsigned int>(fly);
    item_def.swim = static_cast<unsigned int>(swm);
}

void Game::ModifyItemSpeed(const Items& it, int speed)
{
    if (!it.isValid())
    {
        return;
    }
    if (speed < 0) speed = 0;
    ItemDefs[it].speed = static_cast<unsigned int>(speed);
}

void Game::ModifyItemProductionBooster(const Items& it, const Items& item, int bonus)
{
    if (!it.isValid())
    {
        return;
    }

    auto& item_def = ItemDefs[it];
    item_def.mult_item = item;
    item_def.mult_val = bonus;
}

void Game::ModifyItemHitch(const Items& it, const Items& item, int capacity)
{
    if (!it.isValid())
    {
        return;
    }
    if (capacity < 0)
    {
        return;
    }

    auto& item_def = ItemDefs[it];
    item_def.hitchItem = item;
    item_def.hitchwalk = static_cast<unsigned int>(capacity);
}

void Game::ModifyItemProductionSkill(const Items& it, char *sk, size_t lev)
{
    if (!it.isValid())
    {
        return;
    }
    try
    {
        FindSkill(sk);

        auto& item_def = ItemDefs[it];
        item_def.pSkill = sk;
        item_def.pLevel = lev;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyItemProductionOutput(const Items& it, int months, int count)
{
    if (!it.isValid())
    {
        return;
    }
    if (count < 0) count = 0;
    if (months < 0) months = 0;

    auto& item_def = ItemDefs[it];
    item_def.pMonths = months;
    item_def.pOut = count;
}

void Game::ModifyItemProductionInput(const Items& it, int i, const Items& input, int amount)
{
    if (i < 0)
    {
        return;
    }
    if (!it.isValid())
    {
        return;
    }
    if(!input.isValid())
    {
        return;
    }
    auto& item_def = ItemDefs[it];
    const size_t i_u = static_cast<size_t>(i);
    if(i_u >= item_def.pInput.size())
    {
        return;
    }
    if (amount < 0)
    {
        amount = 0;
    }
    auto& pinput = item_def.pInput[i_u];
    pinput.item = input;
    pinput.amt = amount;
}

void Game::ModifyItemMagicSkill(const Items& it, char *sk, size_t lev)
{
    if (!it.isValid())
    {
        return;
    }

    try
    {
        FindSkill(sk);

        auto& item_def = ItemDefs[it];
        item_def.mSkill = sk;
        item_def.mLevel = lev;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyItemMagicOutput(const Items& it, int count)
{
    if (!it.isValid())
    {
        return;
    }

    if (count < 0) count = 0;
    ItemDefs[it].mOut = count;
}

void Game::ModifyItemMagicInput(const Items& it, int i, const Items& input, int amount)
{
    if(i < 0)
    {
        return;
    }
    if (!it.isValid())
    {
        return;
    }
    if(!input.isValid())
    {
        return;
    }

    const size_t i_u = static_cast<size_t>(i);
    auto& item_def = ItemDefs[it];
    if (i_u >= item_def.mInput.size())
    {
        return;
    }
    if (amount < 0)
    {
        amount = 0;
    }
    auto& minput = item_def.mInput[i_u];
    minput.item = input;
    minput.amt = amount;
}

void Game::ModifyItemEscape(const Items& it, int escape, char const *skill, int val)
{
    if (!it.isValid())
    {
        return;
    }

    auto& item_def = ItemDefs[it];
    item_def.escape = escape;
    item_def.esc_skill = skill;
    item_def.esc_val = static_cast<size_t>(val);
}

void Game::ModifyRaceSkillLevels(char const *r, int spec, int def)
{
    try
    {
        auto& mt = FindRace_(r);
        if (spec < 0)
        {
            spec = 0;
        }
        if (def < 0)
        {
            def = 0;
        }
        mt.speciallevel = static_cast<unsigned int>(spec);
        mt.defaultlevel = static_cast<unsigned int>(def);
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyRaceSkills(char const *r, int i, char const *sk)
{
    if(i < 0)
    {
        return;
    }

    try
    {
        auto& mt = FindRace_(r);
        size_t i_u = static_cast<size_t>(i);
        FindSkill(sk);
        mt.skills[i_u] = sk;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMonsterAttackLevel(char const *mon, int lev)
{
    if (lev < 0)
    {
        return;
    }
    try
    {
        auto& pM = FindMonster_(mon, 0);
        pM.attackLevel = lev;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMonsterDefense(char const *mon, int defenseType, int level)
{
    if(defenseType < 0)
    {
        return;
    }

    try
    {
        auto& pM = FindMonster_(mon, 0);
        auto& defense = pM.defense;
        const size_t defenseType_u = static_cast<size_t>(defenseType);
        if(defenseType_u >= defense.size())
        {
            return;
        }
        defense[defenseType_u] = level;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMonsterAttacksAndHits(char const *mon, int num, int hits, int regen)
{
    if (num < 0)
    {
        return;
    }
    if (hits < 0)
    {
        return;
    }
    if (regen < 0)
    {
        return;
    }

    try
    {
        auto& pM = FindMonster_(mon, 0);
        pM.numAttacks = num;
        pM.hits = static_cast<unsigned int>(hits);
        pM.regen = regen;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMonsterSkills(char const *mon, int tact, int stealth, int obs)
{
    if (tact < 0)
    {
        return;
    }
    if (stealth < 0)
    {
        return;
    }
    if (obs < 0)
    {
        return;
    }

    try
    {
        auto& pM = FindMonster_(mon, 0);
        pM.tactics = tact;
        pM.stealth = stealth;
        pM.obs = obs;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMonsterSpecial(char const *mon, char const *special, int lev)
{
    if (lev < 0)
    {
        return;
    }

    try
    {
        auto& pM = FindMonster_(mon, 0);
        FindSpecial_(special);
        pM.special = special;
        pM.specialLevel = static_cast<size_t>(lev);
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMonsterSpoils(char const *mon, int silver, int spoilType)
{
    if (spoilType < -1)
    {
        return;
    }
    if (silver < 0)
    {
        return;
    }

    try
    {
        auto& pM = FindMonster_(mon, 0);
        pM.silver = static_cast<size_t>(silver);
        pM.spoiltype = spoilType;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMonsterThreat(char const *mon, int num, int hostileChance)
{
    if (num < 0)
    {
        return;
    }
    if (hostileChance < 0 || hostileChance > 100)
    {
        return;
    }

    try
    {
        auto& pM = FindMonster_(mon, 0);
        pM.hostile = hostileChance;
        pM.number = static_cast<unsigned int>(num);
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyWeaponSkills(char const *weap, char *baseSkill, char *orSkill)
{
    try
    {
        auto& pw = FindWeapon_(weap);
        FindSkill(baseSkill);
        FindSkill(orSkill);
        pw.baseSkill = baseSkill;
        pw.orSkill = orSkill;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyWeaponFlags(char const *weap, int flags)
{
    try
    {
        auto& pw = FindWeapon_(weap);
        pw.flags = flags;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyWeaponAttack(char const *weap, int wclass, int attackType,
        int numAtt)
{
    if (wclass < 0 || wclass > (NUM_WEAPON_CLASSES - 1))
    {
        return;
    }
    if (attackType < 0 || attackType > (NUM_ATTACK_TYPES - 1))
    {
        return;
    }
    try
    {
        auto& pw = FindWeapon_(weap);
        pw.weapClass = wclass;
        pw.attackType = attackType;
        pw.numAttacks = numAtt;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyWeaponBonuses(char const *weap, int attack, int defense, int vsMount)
{
    try
    {
        auto& pw = FindWeapon_(weap);
        pw.attackBonus = attack;
        pw.defenseBonus = defense;
        pw.mountBonus = vsMount;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyArmorFlags(char const *armor, int flags)
{
    try
    {
        auto& pa = FindArmor_(armor);
        pa.flags = flags;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyArmorSaveFrom(char const *armor, int from)
{
    if (from < 0)
    {
        return;
    }

    try
    {
        auto& pa = FindArmor_(armor);
        pa.from = from;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyArmorSaveValue(char const *armor, int wclass, int val)
{
    if (wclass < 0 || wclass > (NUM_WEAPON_CLASSES - 1))
    {
        return;
    }
    if (val < 0)
    {
        return;
    }

    try
    {
        auto& pa = FindArmor_(armor);
        if(val > pa.from)
        {
            return;
        }
        pa.saves[wclass] = val;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMountSkill(char const *mount, char *skill)
{
    try
    {
        auto& pm = FindMount_(mount);
        FindSkill(skill);
        pm.skill = skill;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMountBonuses(char const *mount, int min, int max, int hampered)
{
    if (min < 0)
    {
        return;
    }
    if (max < 0)
    {
        return;
    }
    if (hampered < min)
    {
        return;
    }
    try
    {
        auto& pm = FindMount_(mount);
        pm.minBonus = min;
        pm.maxBonus = max;
        pm.maxHamperedBonus = hampered;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyMountSpecial(char const *mount, char const *special, int level)
{
    if (level < 0)
    {
        return;
    }

    try
    {
        auto& pm = FindMount_(mount);
        FindSpecial_(special);
        pm.mountSpecial = special;
        pm.specialLev = static_cast<unsigned int>(level);
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::EnableObject(const Objects& obj)
{
    if (!obj.isValid())
    {
        return;
    }
    ObjectDefs[obj].flags &= ~ObjectType::DISABLED;
}

void Game::DisableObject(const Objects& obj)
{
    if (!obj.isValid())
    {
        return;
    }
    ObjectDefs[obj].flags |= ObjectType::DISABLED;
}

void Game::ModifyObjectFlags(const Objects& ob, int flags)
{
    if (!ob.isValid())
    {
        return;
    }
    ObjectDefs[ob].flags = flags;
}

void Game::ModifyObjectDecay(const Objects& ob, int maxMaint, int maxMonthDecay, int mFact)
{
    if (maxMonthDecay > maxMaint)
    {
        return;
    }
    if (maxMaint < 0)
    {
        return;
    }
    if (maxMonthDecay < 0)
    {
        return;
    }
    if (mFact < 0)
    {
        return;
    }
    if (!ob.isValid())
    {
        return;
    }
    ObjectDefs[ob].maxMaintenance = maxMaint;
    ObjectDefs[ob].maxMonthlyDecay = maxMonthDecay;
    ObjectDefs[ob].maintFactor = mFact;
}

void Game::ModifyObjectProduction(const Objects& ob, const Items& it)
{
    if (!ob.isValid())
    {
        return;
    }
    ObjectDefs[ob].productionAided = it;
}

void Game::ModifyObjectMonster(const Objects& ob, const Items& monster)
{
    if (!ob.isValid())
    {
        return;
    }
    ObjectDefs[ob].monster = monster;
}

void Game::ModifyObjectConstruction(const Objects& ob, const ObjectTypeItems& it, int num, char const *sk, int lev)
{
    if (num < 0)
    {
        return;
    }
    if (lev < 0)
    {
        return;
    }
    if (!ob.isValid())
    {
        return;
    }
    if (!it.isValid())
    {
        return;
    }

    try
    {
        FindSkill(sk);
        ObjectDefs[ob].item = it;
        ObjectDefs[ob].cost = num;
        ObjectDefs[ob].skill = sk;
        ObjectDefs[ob].level = static_cast<unsigned int>(lev);
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyObjectManpower(const Objects& ob, int prot, int cap, int sail, int mages)
{
    if (prot < 0)
    {
        return;
    }
    if (cap < 0)
    {
        return;
    }
    if (sail < 0)
    {
        return;
    }
    if (mages < 0)
    {
        return;
    }
    if (!ob.isValid())
    {
        return;
    }
    ObjectDefs[ob].protect = prot;
    ObjectDefs[ob].capacity = cap;
    ObjectDefs[ob].sailors = sail;
    ObjectDefs[ob].maxMages = static_cast<unsigned int>(mages);
}

void Game::ModifyObjectDefence(const Objects& ob, int co, int en, int sp, int we, int ri, int ra)
{
    if (!ob.isValid())
    {
        return;
    }
    //if (val < 0) return;    // we could conceivably have a negative value 
                                // associated with a structure
    ObjectDefs[ob].defenceArray[0] = co;
    ObjectDefs[ob].defenceArray[1] = en;
    ObjectDefs[ob].defenceArray[2] = sp;
    ObjectDefs[ob].defenceArray[3] = we;
    ObjectDefs[ob].defenceArray[4] = ri;
    ObjectDefs[ob].defenceArray[5] = ra;
}

void Game::ModifyObjectName(const Objects& ob, char const *name)
{
    if (!ob.isValid())
    {
        return;
    }
    ObjectDefs[ob].name = name;
}

void Game::ClearTerrainRaces(const Regions& t)
{
    if (!t.isValid())
    {
        return;
    }

    auto& terrain_def = TerrainDefs[t];
    for (auto& c: terrain_def.races) {
        c.invalidate();
    }
    for (auto& c: terrain_def.coastal_races) {
        c.invalidate();
    }
}

void Game::ModifyTerrainRace(const Regions& t, int i, Items r)
{
    if (i < 0)
    {
        return;
    }
    if (!t.isValid())
    {
        return;
    }

    auto& terrain_def = TerrainDefs[t];

    const size_t i_u = static_cast<size_t>(i);

    if(i_u >= terrain_def.races.size())
    {
        return;
    }

    if (r.isValid() && !(ItemDefs[r].type & IT_MAN))
    {
        r.invalidate();
    }
    TerrainDefs[t].races[i_u] = r;
}

void Game::ModifyTerrainCoastRace(const Regions& t, int i, Items r)
{
    if (i < 0)
    {
        return;
    }
    if (!t.isValid())
    {
        return;
    }

    auto& terrain_def = TerrainDefs[t];

    const size_t i_u = static_cast<size_t>(i);

    if(i_u >= terrain_def.coastal_races.size())
    {
        return;
    }

    if (r.isValid() && !(ItemDefs[r].type & IT_MAN))
    {
        r.invalidate();
    }
    TerrainDefs[t].coastal_races[i_u] = r;
}

void Game::ClearTerrainItems(const Regions& terrain)
{
    if (!terrain.isValid())
    {
        return;
    }

    for (auto& c: TerrainDefs[terrain].prods) {
        c.product.invalidate();
        c.chance = 0;
        c.amount = 0;
    }
}

void Game::ModifyTerrainItems(const Regions& terrain, int i, const Items& p, int c, int a)
{
    if (i < 0)
    {
        return;
    }
    if (!terrain.isValid())
    {
        return;
    }

    auto& terrain_defs = TerrainDefs[terrain];
    const size_t i_u = static_cast<size_t>(i);

    if(i_u >= terrain_defs.prods.size())
    {
        return;
    }

    if (c < 0 || c > 100)
    {
        c = 0;
    }
    if (a < 0)
    {
        a = 0;
    }

    auto& prod = terrain_defs.prods[i_u];
    prod.product = p;
    prod.chance = c;
    prod.amount = a;
}

void Game::ModifyTerrainWMons(const Regions& t, int freq, const Items& smon, const Items& bigmon, const Items& hum)
{
    if(!t.isValid())
    {
        return;
    }
    if (freq < 0)
    {
        freq = 0;
    }

    auto& terrain_def = TerrainDefs[t];
    terrain_def.wmonfreq = freq;
    terrain_def.smallmon = smon;
    terrain_def.bigmon = bigmon;
    terrain_def.humanoid = hum;
}

void Game::ModifyTerrainLairChance(const Regions& t, int chance)
{
    if(!t.isValid())
    {
        return;
    }
    if (chance < 0 || chance > 100)
    {
        chance = 0;
    }
    // Chance is percent out of 100 that should have some lair
    TerrainDefs[t].lairChance = chance;
}

void Game::ModifyTerrainLair(const Regions& t, int i, const Objects& l)
{
    if (i < 0)
    {
        return;
    }
    if (!t.isValid())
    {
        return;
    }

    auto& terrain_def = TerrainDefs[t];
    const size_t i_u = static_cast<size_t>(i);

    if(i_u >= terrain_def.lairs.size())
    {
        return;
    }

    terrain_def.lairs[i_u] = l;
}

void Game::ModifyTerrainEconomy(const Regions& t, int pop, int wages, int econ, int move)
{
    if(!t.isValid())
    {
        return;
    }
    if (pop < 0)
    {
        pop = 0;
    }
    if (wages < 0)
    {
        wages = 0;
    }
    if (econ < 0)
    {
        econ = 0;
    }
    if (move < 1)
    {
        move = 1;
    }

    auto& terrain_def = TerrainDefs[t];
    terrain_def.pop = pop;
    terrain_def.wages = wages;
    terrain_def.economy = static_cast<unsigned int>(econ);
    terrain_def.movepoints = move;
}

void Game::ModifyBattleItemFlags(char const *item, int flags)
{
    try
    {
        auto& pb = FindBattleItem_(item);
        pb.flags = flags;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyBattleItemSpecial(char const *item, char const *special, int level)
{
    if (level < 0)
    {
        return;
    }

    try
    {
        auto& pb = FindBattleItem_(item);
        FindSpecial(special);
        pb.special = special;
        pb.skillLevel = static_cast<size_t>(level);
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySpecialTargetFlags(char const *special, int targetflags)
{
    try
    {
        auto& sp = FindSpecial_(special);
        sp.targflags = targetflags;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySpecialTargetObjects(char const *special, int index, const Objects& obj)
{
    if (index < 0)
    {
        return;
    }
    if ((obj.isValid() && (obj == *Objects::begin())) || (obj == *Objects::end()) || obj.overflowed())
    {
        return;
    }
    try
    {
        auto& sp = FindSpecial_(special);
        const size_t index_u = static_cast<size_t>(index);
        if(index_u >= sp.buildings.size())
        {
            return;
        }
        sp.buildings[index_u] = obj;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySpecialTargetItems(char const *special, int index, const Items& item)
{
    if (index < 0)
    {
        return;
    }

    if(item == *Items::end() || item.overflowed())
    {
        return;
    }

    try
    {
        auto& sp = FindSpecial_(special);
        const size_t index_u = static_cast<size_t>(index);
        if(index_u >= sp.targets.size())
        {
            return;
        }
        sp.targets[index_u] = item;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySpecialTargetEffects(char const *special, int index, char const *effect)
{
    if (index < 0)
    {
        return;
    }
    try
    {
        auto& sp = FindSpecial_(special);
        const size_t index_u = static_cast<size_t>(index);
        if(index_u >= sp.effects.size())
        {
            return;
        }
        FindEffect(effect);
        sp.effects[index_u] = effect;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySpecialEffectFlags(char const *special, int effectflags)
{
    try
    {
        auto& sp = FindSpecial_(special);
        sp.effectflags = effectflags;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySpecialShields(char const *special, int index, int type)
{
    if (index < 0)
    {
        return;
    }

    if (type < -1 || type > (NUM_ATTACK_TYPES))
    {
        return;
    }

    try
    {
        auto& sp = FindSpecial_(special);
        const size_t index_u = static_cast<size_t>(index);
        if (index_u >= sp.shield.size())
        {
            return;
        }
        sp.shield[index_u] = type;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySpecialDefenseMods(char const *special, int index, int type, int val)
{
    if(index < 0)
    {
        return;
    }

    if (type < -1 || type > (NUM_ATTACK_TYPES))
    {
        return;
    }

    try
    {
        auto& sp = FindSpecial_(special);
        const size_t index_u = static_cast<size_t>(index);
        if (index_u >= sp.defs.size())
        {
            return;
        }
        auto& def = sp.defs[index_u];
        def.type = type;
        def.val = val;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifySpecialDamage(char const *special, int index, int type, int min,
        int val, int flags, int cls, char const *effect)
{
    if (index < 0)
    {
        return;
    }
    if (type < -1 || type > NUM_ATTACK_TYPES)
    {
        return;
    }
    if (cls < -1 || cls > (NUM_WEAPON_CLASSES-1))
    {
        return;
    }
    if (min < 0)
    {
        return;
    }

    try
    {
        auto& sp = FindSpecial_(special);
        FindEffect(effect);

        const size_t index_u = static_cast<size_t>(index);
        if (index_u >= sp.damage.size())
        {
            return;
        }
        auto& damage = sp.damage[index_u];
        damage.type = type;
        damage.minnum = min;
        damage.value = val;
        damage.flags = flags;
        damage.dclass = cls;
        damage.effect = effect;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyEffectFlags(char const *effect, int flags)
{
    try
    {
        auto& ep = FindEffect_(effect);
        ep.flags = flags;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyEffectAttackMod(char const *effect, int val)
{
    try
    {
        auto& ep = FindEffect_(effect);
        ep.attackVal = val;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyEffectDefenseMod(char const *effect, int index, int type, int val)
{
    if (index < 0)
    {
        return;
    }

    if (type < 0 || type > NUM_ATTACK_TYPES)
    {
        return;
    }

    try
    {
        auto& ep = FindEffect_(effect);
        const size_t index_u = static_cast<size_t>(index);
        if (index_u >= ep.defMods.size())
        {
            return;
        }

        auto& def_mod = ep.defMods[index_u];
        def_mod.type = type;
        def_mod.val = val;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyEffectCancelEffect(char const *effect, char *uneffect)
{
    try
    {
        auto& ep = FindEffect_(effect);
        FindEffect(uneffect);
        ep.cancelEffect = uneffect;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyRangeFlags(char const *range, int flags)
{
    try
    {
        auto& rp = FindRange_(range);
        rp.flags = flags;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyRangeClass(char const *range, int rclass)
{
    if (rclass < 0 || rclass > (RangeType::NUMRANGECLASSES-1))
    {
        return;
    }

    try
    {
        auto& rp = FindRange_(range);
        rp.rangeClass = rclass;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyRangeMultiplier(char const *range, int mult)
{
    if (mult < 1)
    {
        return;
    }

    try
    {
        auto& rp = FindRange_(range);
        rp.rangeMult = static_cast<unsigned int>(mult);
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyRangeLevelPenalty(char const *range, int pen)
{
    if (pen < 0)
    {
        return;
    }
    try
    {
        auto& rp = FindRange_(range);
        rp.crossLevelPenalty = pen;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyAttribMod(char const *mod_str, int index, int flags, char const *ident,
        int type, unsigned int val)
{
    if (!ident)
    {
        return;
    }
    if (index < 0)
    {
        return;
    }
    if (type < 0 || type > AttribModItem::NUMMODTYPE-1)
    {
        return;
    }
    try
    {
        auto& mp = FindAttrib_(mod_str);
        const size_t index_u = static_cast<size_t>(index);
        if (index_u >= mp.mods.size())
        {
            return;
        }

        auto& mod = mp.mods[index_u];
        mod.flags = flags;
        mod.ident = ident;
        mod.modtype = type;
        mod.val = val;
    }
    catch(const NoSuchItemException&)
    {
    }
}

void Game::ModifyHealing(int level, int patients, int success)
{
    if (level < 1)
    {
        return;
    
    }
    const size_t level_u = static_cast<size_t>(level);
    if(level_u >= HealDefs.size())
    {
        return;
    }
    auto& heal_def = HealDefs[level_u];
    heal_def.num = static_cast<unsigned int>(patients);
    heal_def.rate = static_cast<unsigned int>(success);
}

