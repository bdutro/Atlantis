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
#include "battle.h"
#include "gamedata.h"

void Soldier::SetupHealing()
{
    const auto unit_s = unit.lock();
    if (unit_s->type == U_MAGE ||
            unit_s->type == U_APPRENTICE ||
            unit_s->type == U_GUARDMAGE) {
        healtype = unit_s->GetSkill(Skills::Types::S_MAGICAL_HEALING);
        if (healtype > 5) healtype = 5;
        if (healtype > 0) {
            healing = HealDefs[healtype].num;
            healitem.invalidate();
            return;
        }
    }

    if (unit_s->items.GetNum(Items::Types::I_HEALPOTION)) {
        healtype = 1;
        unit_s->items.SetNum(Items::Types::I_HEALPOTION,
                             unit_s->items.GetNum(Items::Types::I_HEALPOTION)-1);
        healing = 10;
        healitem = Items::Types::I_HEALPOTION;
    } else {
        healing = unit_s->GetSkill(Skills::Types::S_HEALING) * Globals->HEALS_PER_MAN;
        if (healing) {
            healtype = 1;
            const size_t herbs = unit_s->items.GetNum(Items::Types::I_HERBS);
            if (herbs < healing) healing = static_cast<unsigned int>(herbs);
            unit_s->items.SetNum(Items::Types::I_HERBS,herbs - healing);
            healitem = Items::Types::I_HERBS;
        }
    }
}

bool Army::CheckSpecialTarget(char const *special, size_t tar)
{
    const auto& spd = FindSpecial(special);
    bool match = false;

    const auto& s_tar = soldiers[tar];
    if (spd.targflags & SpecialType::HIT_BUILDINGIF) {
        match = false;
        if (!s_tar->building)
        {
            return false;
        }
        for (const auto& b: spd.buildings) {
            if (s_tar->building && (b == s_tar->building))
            {
                match = true;
                break;
            }
        }
        if (!match)
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_BUILDINGEXCEPT) {
        if(spd.targflags & SpecialType::HIT_BUILDINGIF)
        {
            if(match)
            {
                return false;
            }
        }

        match = false;
        if (!s_tar->building)
        {
            return false;
        }
        for (const auto& b: spd.buildings) {
            if (s_tar->building && (b == s_tar->building))
            {
                match = true;
                break;
            }
        }
        if (match)
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_SOLDIERIF) {
        match = false;
        if (!s_tar->race.isValid())
        {
            return false;
        }
        for (const auto& t: spd.targets) {
            if (s_tar->race == t)
            {
                match = true;
                break;
            }
        }
        if (!match)
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_SOLDIEREXCEPT) {
        if (spd.targflags & SpecialType::HIT_SOLDIERIF) {
            if(match)
            {
                return false;
            }
        }
        match = false;
        if (!s_tar->race.isValid())
        {
            return false;
        }
        for (const auto& t: spd.targets) {
            if (s_tar->race == t)
            {
                match = true;
                break;
            }
        }
        if (match)
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_EFFECTIF) {
        match = false;
        for (const auto& e: spd.effects) {
            if (s_tar->HasEffect(e))
            {
                match = true;
                break;
            }
        }
        if (!match)
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_EFFECTEXCEPT) {
        if (spd.targflags & SpecialType::HIT_EFFECTIF) {
            if(match)
            {
                return false;
            }
        }
        match = false;
        for (const auto& e: spd.effects) {
            if (s_tar->HasEffect(e))
            {
                match = true;
                break;
            }
        }
        if (match)
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_MOUNTIF) {
        match = false;
        if (!s_tar->riding.isValid())
        {
            return false;
        }
        for (const auto& t: spd.targets) {
            if (s_tar->riding == t)
            {
                match = true;
                break;
            }
        }
        if (!match)
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_MOUNTEXCEPT) {
        if (spd.targflags & SpecialType::HIT_MOUNTIF) {
            if(match)
            {
                return false;
            }
        }
        match = false;
        if (!s_tar->riding.isValid())
        {
            return false;
        }
        for (const auto& t: spd.targets) {
            if (s_tar->riding == t)
            {
                match = true;
                break;
            }
        }
        if (match)
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_ILLUSION) {
        // All illusions are of type monster, so lets make sure we get it
        // right.  If we ever have other types of illusions, we can change
        // this.
        if (!(ItemDefs[s_tar->race].type & IT_MONSTER))
        {
            return false;
        }
        if (!(ItemDefs[s_tar->race].type & IT_ILLUSION))
        {
            return false;
        }
    }

    if (spd.targflags & SpecialType::HIT_NOMONSTER) {
        if (ItemDefs[s_tar->race].type & IT_MONSTER)
        {
            return false;
        }
    }
    return true;
}

void Battle::UpdateShields(const Army::Handle& a)
{
    for (size_t i = 0; i < a->notbehind; i++) {
        if (a->soldiers[i]->special == nullptr)
        {
            continue;
        }
        const auto& spd = FindSpecial(a->soldiers[i]->special);

        if (!(spd.effectflags & SpecialType::FX_SHIELD) &&
                !(spd.effectflags & SpecialType::FX_DEFBONUS)) continue;

        if (spd.effectflags & SpecialType::FX_SHIELD) {
            for (const auto& shtype: spd.shield) {
                if (shtype == -1)
                {
                    continue;
                }
                auto& sh = a->shields.emplace_back();
                sh.shieldtype = shtype;
                sh.shieldskill = a->soldiers[i]->slevel;
            }
        }

        if (spd.effectflags & SpecialType::FX_DEFBONUS && a->round == 0) {
            for (const auto& shtype: spd.defs) {
                if (shtype.type == -1)
                {
                    continue;
                }
                int bonus = shtype.val;
                if (spd.effectflags & SpecialType::FX_USE_LEV)
                {
                    bonus *= static_cast<int>(a->soldiers[i]->slevel);
                }
                a->soldiers[i]->dskill[shtype.type] += bonus;
            }
        }

        AddLine(*(a->soldiers[i]->unit.lock()->name) + " casts " +
                spd.shielddesc + ".");
    }
}

void Battle::DoSpecialAttack(int, const Soldier::Handle& a, const Army::Handle& , const Army::Handle& def, bool)
{
    int num, tot = -1;
    AString results[4];
    int dam = 0;

    if (a->special == nullptr)
    {
        return;
    }
    const auto& spd = FindSpecial(a->special);

    if (!(spd.effectflags & SpecialType::FX_DAMAGE))
    {
        return;
    }

    for (const auto& d: spd.damage) {
        if (d.type == -1)
        {
            continue;
        }
        int times = d.value;
        if (spd.effectflags & SpecialType::FX_USE_LEV)
        {
            times *= static_cast<int>(a->slevel);
        }
        int realtimes = d.minnum + getrandom(times) + getrandom(times);
        num = def->DoAnAttack(a->special, realtimes,
                d.type, static_cast<int>(a->slevel),
                d.flags, d.dclass,
                d.effect, 0, a);
        if (spd.effectflags & SpecialType::FX_DONT_COMBINE && num != -1) {
            if (d.effect == nullptr) {
                results[dam] = AString("killing ") + num;
                dam++;
            } else {
                results[dam] = AString(spd.spelldesc2) + num;
            }
        }
        if (num != -1) {
            if (tot == -1)
            {
                tot = num;
            }
            else
            {
                tot += num;
            }
        }
    }
    if (tot == -1) {
        AddLine(a->name + " " + spd.spelldesc + ", but it is deflected.");
    } else {
        if (spd.effectflags & SpecialType::FX_DONT_COMBINE) {
            AString temp = a->name + " " + spd.spelldesc;
            for (int i = 0; i < dam; i++) {
                if (i)
                {
                    temp += ", ";
                }
                if (i == dam - 1)
                {
                    temp += " and ";
                }
                temp += results[dam];
            }
            temp += AString(spd.spelltarget) + ".";
            AddLine(temp);
        } else {
            AddLine(a->name + " " + spd.spelldesc + ", " + spd.spelldesc2 +
                    tot + spd.spelltarget + ".");
        }
    }
}

