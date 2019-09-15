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
#include <functional>

#include "game.h"
#include "gamedata.h"

void Game::CreateCityMons()
{
    if (!Globals->CITY_MONSTERS_EXIST) return;

    for(const auto& r: regions) {
        if ((r->type == Regions::Types::R_NEXUS) || r->IsStartingCity() || r->town) {
            CreateCityMon(r, 100, 1);
        }
    }
}

void Game::CreateWMons()
{
    if (!Globals->WANDERING_MONSTERS_EXIST) return;

    GrowWMons(50);
}

void Game::CreateLMons()
{
    if (!Globals->LAIR_MONSTERS_EXIST) return;

    GrowLMons(50);
}

void Game::GrowWMons(int rate)
{
    //
    // Now, go through each 8x8 block of the map, and make monsters if
    // needed.
    //
    for (size_t level = 0; level < regions.numLevels; level++) {
        const auto& pArr = regions.pRegionArrays[level];
        for (unsigned int xsec=0; xsec< pArr->x / 8; xsec++) {
            for (unsigned int ysec=0; ysec< pArr->y / 16; ysec++) {
                /* OK, we have a sector. Count mons, and wanted */
                int mons=0;
                int wanted=0;
                for (unsigned int x=0; x<8; x++) {
                    for (unsigned int y=0; y<16; y+=2) {
                        const auto reg_w = pArr->GetRegion(x+xsec*8, y+ysec*16+x%2);
                        if (!reg_w.expired())
                        {
                            const auto reg = reg_w.lock();
                            if(!reg->IsGuarded()) {
                                mons += reg->CountWMons();
                                /*
                                 * Make sure there is at least one monster type
                                 * enabled for this region
                                 */
                                bool avail = false;
                                const auto& smallmon = TerrainDefs[reg->type].smallmon;
                                if (!(!smallmon.isValid() ||
                                     (ItemDefs[smallmon].flags & ItemType::DISABLED)))
                                    avail = true;
                                const auto& bigmon = TerrainDefs[reg->type].bigmon;
                                if (!(!bigmon.isValid() ||
                                     (ItemDefs[bigmon].flags & ItemType::DISABLED)))
                                    avail = true;
                                const auto& humanoid = TerrainDefs[reg->type].humanoid;
                                if (!(!humanoid.isValid() ||
                                     (ItemDefs[humanoid].flags & ItemType::DISABLED)))
                                    avail = true;

                                if (avail)
                                    wanted += TerrainDefs[reg->type].wmonfreq;
                            }
                        }
                    }
                }

                wanted /= 10;
                wanted -= mons;
                wanted = (wanted*rate + getrandom(100))/100;
                if (wanted > 0) {
                    for (int i=0; i< wanted;) {
                        const unsigned int m = getrandom(8U);
                        const unsigned int n = getrandom(8U) * 2 + m % 2;
                        const auto reg_w = pArr->GetRegion(m+xsec*8, n+ysec*16);
                        if (!reg_w.expired())
                        {
                            const auto reg = reg_w.lock();
                            if(!reg->IsGuarded() && MakeWMon(reg)) {
                                i++;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Game::GrowLMons(int rate)
{
    for(const auto& r: regions) {
        //
        // Don't make lmons in guarded regions
        //
        if (r->IsGuarded()) continue;
        
        for(const auto& obj: r->objects) {
            if (!obj->units.empty()) continue;
            const auto& montype = ObjectDefs[obj->type].monster;
            int grow=!(ObjectDefs[obj->type].flags&ObjectType::NOMONSTERGROWTH);
            if (montype.isValid() && grow) {
                if (getrandom(100) < rate) {
                    MakeLMon(obj);
                }
            }
        }
    }
}

bool Game::MakeWMon(const ARegion::Handle& pReg)
{
    if (!Globals->WANDERING_MONSTERS_EXIST) return false;

    if (TerrainDefs[pReg->type].wmonfreq == 0) return false;

    auto montype = TerrainDefs[pReg->type].smallmon;
    if (getrandom(2) && (TerrainDefs[pReg->type].humanoid.isValid()))
        montype = TerrainDefs[pReg->type].humanoid;
    if (TerrainDefs[pReg->type].bigmon.isValid() && !getrandom(8)) {
        montype = TerrainDefs[pReg->type].bigmon;
    }
    if (!montype.isValid() || (ItemDefs[montype].flags & ItemType::DISABLED))
        return false;

    const auto& mp = FindMonster(ItemDefs[montype].abr,
            (ItemDefs[montype].type & IT_ILLUSION));
    const auto monfac = GetFaction(factions, monfaction);
    const auto u = GetNewUnit(monfac, 0);
    u->MakeWMon(mp.name, montype, (mp.number+getrandom(mp.number)+1)/2);
    u->MoveUnit(pReg->GetDummy());
    return true;
}

void Game::MakeLMon(const Object::Handle& pObj)
{
    if (!Globals->LAIR_MONSTERS_EXIST) return;
    if (ObjectDefs[pObj->type].flags & ObjectType::NOMONSTERGROWTH) return;

    Items montype = ObjectDefs[pObj->type].monster;

    if (montype == Items::Types::I_TRENT)
    {
        montype = TerrainDefs[pObj->region.lock()->type].bigmon;
    }

    if (montype == Items::Types::I_CENTAUR)
    {
        montype = TerrainDefs[pObj->region.lock()->type].humanoid;
    }

    if (!montype.isValid() || (ItemDefs[montype].flags & ItemType::DISABLED))
        return;

    std::reference_wrapper<const MonType> mp = FindMonster(ItemDefs[montype].abr,
                                                           ItemDefs[montype].type & IT_ILLUSION);
    const auto monfac = GetFaction(factions, monfaction);
    auto u = GetNewUnit(monfac, 0);
    switch(montype.asEnum()) {
        case Items::Types::I_IMP:
            u->MakeWMon("Demons", Items::Types::I_IMP, getrandom(mp.get().number + 1));

            mp = FindMonster(ItemDefs[Items::Types::I_DEMON].abr,
                    (ItemDefs[Items::Types::I_DEMON].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_DEMON, getrandom(mp.get().number + 1));

            mp = FindMonster(ItemDefs[Items::Types::I_BALROG].abr,
                    (ItemDefs[Items::Types::I_BALROG].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_BALROG, getrandom(mp.get().number + 1));
            break;
        case Items::Types::I_SKELETON:
            u->MakeWMon("Undead", Items::Types::I_SKELETON, getrandom(mp.get().number + 1));

            mp = FindMonster(ItemDefs[Items::Types::I_UNDEAD].abr,
                    (ItemDefs[Items::Types::I_UNDEAD].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_UNDEAD, getrandom(mp.get().number + 1));

            mp = FindMonster(ItemDefs[Items::Types::I_LICH].abr,
                    (ItemDefs[Items::Types::I_LICH].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_LICH, getrandom(mp.get().number + 1));
            break;
        case Items::Types::I_MAGICIANS:
            u->MakeWMon("Evil Mages", Items::Types::I_MAGICIANS,
                    (mp.get().number + getrandom(mp.get().number) + 1) / 2);

            mp = FindMonster(ItemDefs[Items::Types::I_SORCERERS].abr,
                    (ItemDefs[Items::Types::I_SORCERERS].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_SORCERERS,
                    getrandom(mp.get().number + 1));
            u->SetFlag(FLAG_BEHIND, 1);
            u->MoveUnit(pObj);

            u = GetNewUnit(monfac, 0);

            mp = FindMonster(ItemDefs[Items::Types::I_WARRIORS].abr,
                    (ItemDefs[Items::Types::I_WARRIORS].type & IT_ILLUSION));
            u->MakeWMon(mp.get().name, Items::Types::I_WARRIORS,
                    (mp.get().number + getrandom(mp.get().number) + 1) / 2);

            break;
        case Items::Types::I_DARKMAGE:
            u->MakeWMon("Dark Mages", Items::Types::I_DARKMAGE, (getrandom(mp.get().number) + 1));

            mp = FindMonster(ItemDefs[Items::Types::I_MAGICIANS].abr,
                    (ItemDefs[Items::Types::I_MAGICIANS].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_MAGICIANS,
                    (mp.get().number + getrandom(mp.get().number) + 1) / 2);

            mp = FindMonster(ItemDefs[Items::Types::I_SORCERERS].abr,
                    (ItemDefs[Items::Types::I_SORCERERS].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_SORCERERS, getrandom(mp.get().number + 1));

            mp = FindMonster(ItemDefs[Items::Types::I_DARKMAGE].abr,
                    (ItemDefs[Items::Types::I_DARKMAGE].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_DARKMAGE, getrandom(mp.get().number + 1));
            u->SetFlag(FLAG_BEHIND, 1);
            u->MoveUnit(pObj);

            u = GetNewUnit(monfac, 0);

            mp = FindMonster(ItemDefs[Items::Types::I_DROW].abr,
                    (ItemDefs[Items::Types::I_DROW].type & IT_ILLUSION));
            u->MakeWMon(mp.get().name, Items::Types::I_DROW,
                    (mp.get().number + getrandom(mp.get().number) + 1) / 2);

            break;
        case Items::Types::I_ILLYRTHID:
            u->MakeWMon(mp.get().name, Items::Types::I_ILLYRTHID,
                    (mp.get().number + getrandom(mp.get().number) + 1) / 2);
            u->SetFlag(FLAG_BEHIND, 1);
            u->MoveUnit(pObj);

            u = GetNewUnit(monfac, 0);

            mp = FindMonster(ItemDefs[Items::Types::I_SKELETON].abr,
                    (ItemDefs[Items::Types::I_SKELETON].type & IT_ILLUSION));
            u->MakeWMon("Undead", Items::Types::I_SKELETON, getrandom(mp.get().number + 1));

            mp = FindMonster(ItemDefs[Items::Types::I_UNDEAD].abr,
                    (ItemDefs[Items::Types::I_UNDEAD].type & IT_ILLUSION));
            u->items.SetNum(Items::Types::I_UNDEAD, getrandom(mp.get().number + 1));
            break;
        case Items::Types::I_STORMGIANT:
            if (getrandom(3) < 1) {
                montype = Items::Types::I_CLOUDGIANT;
                mp = FindMonster(ItemDefs[montype].abr,
                        (ItemDefs[montype].type & IT_ILLUSION));
            }
            u->MakeWMon(mp.get().name, montype,
                    (mp.get().number + getrandom(mp.get().number) + 1) / 2);
            break;
        default:
            u->MakeWMon(mp.get().name, montype,
                    (mp.get().number + getrandom(mp.get().number) + 1) / 2);
            break;
    }
    u->MoveUnit(pObj);
}

Unit::Handle Game::MakeManUnit(const Faction::Handle& fac, const Items& mantype, size_t num, size_t level, int weaponlevel, int armor, int behind)
{
    const auto u = GetNewUnit(fac);
    const auto& men = FindRace(ItemDefs[mantype].abr);

    // Check skills:
    unsigned int scomb = men.defaultlevel;
    unsigned int sxbow = men.defaultlevel;
    unsigned int slbow = men.defaultlevel;
    for (const auto& sk: men.skills) {
        if (sk == nullptr)
        {
            continue;
        }
        if (FindSkill(sk) == FindSkill("COMB"))
        {
            scomb = men.speciallevel;
        }
        if (FindSkill(sk) == FindSkill("XBOW"))
        {
            sxbow = men.speciallevel;
        }
        if (FindSkill(sk) == FindSkill("LBOW"))
        {
            slbow = men.speciallevel;
        }
    }
    unsigned int combat = scomb;
    AString s("COMB");
    auto sk = LookupSkill(s);
    if (behind) {
        if (slbow >= sxbow) {
            s = AString("LBOW");
            sk = LookupSkill(s);
            combat = slbow;
        } else {
            s = AString("XBOW");
            sk = LookupSkill(s);
            combat = sxbow;
        }
    }
    if (combat < level)
    {
        weaponlevel += static_cast<int>(level - combat);
    }
    ValidValue<size_t> weapon;
    Items witem;
    std::vector<int> fitting(WeaponDefs.size(), 0);
    while (!weapon.isValid()) {
        int n = 0;
        for (size_t i = 0; i < WeaponDefs.size(); ++i) {
            const AString it(WeaponDefs[i].abbr);
            if (ItemDefs[LookupItem(it)].flags & ItemType::DISABLED) continue;
            // disregard picks!
            const AString ps("PICK");
            if (LookupItem(it) == LookupItem(ps)) continue;
            
            // Sort out the more exotic weapons!
            int producelevel = static_cast<int>(ItemDefs[LookupItem(it)].pLevel);
            if (ItemDefs[LookupItem(it)].pSkill != FindSkill("WEAP").abbr)
            {
                continue;
            }

            const AString s1(WeaponDefs[i].baseSkill);
            const AString s2(WeaponDefs[i].orSkill);            
            if ((WeaponDefs[i].flags & WeaponType::RANGED) && (!behind))
            {
                continue;
            }
            int attack = WeaponDefs[i].attackBonus;
            if (attack < (producelevel-1)) attack = producelevel-1;
            if ((LookupSkill(s1) == sk)
                || (LookupSkill(s2) == sk))
            {
                if ((behind) && (attack + static_cast<int>(combat) <= weaponlevel))
                {
                    if (WeaponDefs[i].attackBonus == weaponlevel)
                    {
                        fitting[i] = 5;
                    }
                    else
                    {
                        fitting[i] = 1;
                    }
                    n += fitting[i];
                }
                else if ((!behind) && (attack == weaponlevel))
                {
                    fitting[i] = 1;
                    //if (WeaponDefs[i].attackBonus == weaponlevel) fitting[i] = 5;
                    n += fitting[i];
                }
                else
                {
                    continue;
                }
            }
            else
            {
                // make Javelins possible
                const AString cs("COMB");
                if ((behind) && (scomb > combat))
                {
                    if ((WeaponDefs[i].flags & WeaponType::RANGED)
                        && ((LookupSkill(s1) == LookupSkill(cs))
                            || (LookupSkill(s2) == LookupSkill(cs))))
                    {
                            fitting[i] = 1;
                            n++;
                    }
                }
            }
        }
        
        if (n < 1) {
            weaponlevel++;
            continue;
        } else {
            int secondtry = -1;
            while(secondtry <= 0) {
                weapon.invalidate();
                int w = getrandom(n);
                /*
                Awrite(AString("Roll: ") + w);
                */
                n = -1;
                for (size_t i = 0; i < fitting.size(); i++) {
                    if (fitting[i]) {
                        n += fitting[i];
                        /*
                        Awrite(WeaponDefs[i].abbr);
                        */
                        if ((n >= w) && (!weapon.isValid()))
                            weapon = i;
                    }
                }
                if (weapon.isValid()) {
                    const AString ws(WeaponDefs[weapon].abbr);
                    witem = LookupItem(ws);
                    secondtry++;
                    if (men.CanUse(witem)) break;
                }
            }
        }
    }
    // Check again which skills the weapon uses
    const AString ws1(WeaponDefs[weapon].baseSkill);
    const AString ws2(WeaponDefs[weapon].orSkill);
    if ((LookupSkill(ws1) != sk) && (LookupSkill(ws2) != sk))
    {
        sk = LookupSkill(ws1);
    }
    unsigned int maxskill = men.defaultlevel;
    bool special = false;
    for (const auto& ms: men.skills)
    {
        if (FindSkill(ms) == FindSkill(SkillDefs[sk].abbr))
        {
            special = true;
        }        
    }
    if (special) maxskill = men.speciallevel;
    if (level > maxskill) level = maxskill;
    u->SetMen(mantype, num);
    /*
    Awrite(AString("Unit (") + u->num + ") -> chose " + ItemDefs[witem].name);
    */
    u->items.SetNum(witem, num);
    u->SetSkill(sk, level);
    if (behind) u->SetFlag(FLAG_BEHIND,1);
    if (armor) {
        Items ar = Items::Types::I_PLATEARMOR;
        if (!men.CanUse(ar)) ar = Items::Types::I_CHAINARMOR;
        if (!men.CanUse(ar)) ar = Items::Types::I_LEATHERARMOR;
        u->items.SetNum(ar, num);
    }
    return u;
}
