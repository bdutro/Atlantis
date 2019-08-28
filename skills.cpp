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

#include <stdlib.h>
#include <cstring>

#include "game.h"
#include "skills.h"
#include "items.h"
#include "gamedata.h"

const RangeType& FindRange(char const *range)
{
    return RangeDefs.FindItemByKey(range);
}

const SpecialType& FindSpecial(char const *key)
{
    return SpecialDefs.FindItemByKey(key);
}

const EffectType& FindEffect(char const *effect)
{
    return EffectDefs.FindItemByName(effect);
}

const AttribModType& FindAttrib(char const *attrib)
{
    return AttribDefs.FindItemByKey(attrib);
}

const SkillType& FindSkill(char const *skname)
{
    return SkillDefs.FindItemByAbbr(skname);
}

bool FindSameSkills(char const* sk1, char const *sk2)
{
    if(sk1 == sk2)
    {
        return true;
    }
    else if(strcmp(sk1, sk2) == 0)
    {
        return true;
    }
    
    const SkillType* sk1_ptr;
    try
    {
        sk1_ptr = &FindSkill(sk1);
    }
    catch(const NoSuchItemException&)
    {
        sk1_ptr = nullptr;
    }

    const SkillType* sk2_ptr;
    try
    {
        sk2_ptr = &FindSkill(sk2);
    }
    catch(const NoSuchItemException&)
    {
        sk2_ptr = nullptr;
    }

    return sk1_ptr == sk2_ptr;
}

Skills LookupSkill(const AString& token)
{
    for (auto i = Skills::begin(); i != Skills::end(); ++i) {
        if (token == SkillDefs[*i].abbr)
        {
            return *i;
        }
    }
    return Skills();
}

Skills ParseSkill(AString *token)
{
    Skills r;
    for (auto i = Skills::begin(); i != Skills::end(); ++i) {
        if ((*token == SkillDefs[*i].name) || (*token == SkillDefs[*i].abbr)) {
            r = *i;
            break;
        }
    }
    if (r.isValid()) {
        if (SkillDefs[r].flags & SkillType::DISABLED)
        {
            r.invalidate();
        }
    }
    return r;
}

AString SkillStrs(const SkillType& pS)
{
    AString temp = AString(pS.name) + " [" + pS.abbr + "]";
    return temp;
}

AString SkillStrs(const Skills& type)
{
    return SkillStrs(SkillDefs[type]);
}

unsigned int SkillCost(const Skills& skill)
{
    return SkillDefs[skill].cost;
}

int SkillMax(char const *skill, const Items& race)
{
    try
    {
        const ManType& mt = FindRace(ItemDefs[race].abr);

        const SkillType& pS = FindSkill(skill);
        if (!Globals->MAGE_NONLEADERS) {
            if (pS.flags & SkillType::MAGIC) {
                if (!(ItemDefs[race].type & IT_LEADER))
                {
                    return 0;
                }
            }
        }

        const AString& skname = pS.abbr;
        AString mani = "MANI";
        for (const auto& c: mt.skills) {
            if (skname == c)
            {
                return mt.speciallevel;
            }
            // Allow MANI to act as a placeholder for all magical skills
            if ((pS.flags & SkillType::MAGIC) && mani == c)
            {
                return mt.speciallevel;
            }
        }
        return mt.defaultlevel;
    }
    catch(const NoSuchItemException&)
    {
        return 0;
    }
}

size_t GetLevelByDays(size_t dayspermen)
{
    size_t z = 30;
    size_t i = 0;
    while (dayspermen >= z) {
        i++;
        dayspermen -= z;
        z += 30;
    }
    return i;
}

size_t GetDaysByLevel(size_t level)
{
    size_t days = 0;

    for (; level>0; level--) {
        days += level * 30;
    }

    return days;
}

/* Returns the adjusted study rate,
 */
size_t StudyRateAdjustment(size_t days, size_t exp)
{
    size_t rate = 30;
    if (!Globals->REQUIRED_EXPERIENCE)
    {
        return rate;
    }
    size_t slope = 62;
    const unsigned int inc = Globals->REQUIRED_EXPERIENCE * 10;
    size_t cdays = inc;
    size_t prevd = 0;
    ssize_t diff = static_cast<ssize_t>(days - exp);
    if (diff <= 0) {
        rate += static_cast<size_t>(abs(diff) / 3);
    } else {
        size_t level = 0;
        size_t ctr = 0;
        while((((cdays + ctr) / slope + prevd) <= static_cast<size_t>(diff))
            && (rate > 0)) {
            rate -= 1;
            if (rate <= 5) {
                prevd += cdays / slope;
                ctr += cdays;
                cdays = 0;
                slope = (slope * 2)/3;
            }
            cdays += inc;
            size_t clevel = GetLevelByDays(cdays / slope);
            if ((clevel > level) && (rate > 5)) {
                level = clevel;
                switch(level) {
                    case 1: slope = 80;    
                        prevd += cdays / slope;
                        ctr += cdays;
                        cdays = 0;
                        break;
                    case 2: slope = 125;
                        prevd += cdays / slope;
                        ctr += cdays;
                        cdays = 0;
                        break;
                }    
            }
        }
    }
    return rate;
}

ShowSkill::ShowSkill(const Skills& s, unsigned int l)
{
    skill = s;
    level = l;
}

void Skill::Readin(Ainfile *f)
{
    AString *temp, *token;

    temp = f->GetStr();
    token = temp->gettoken();
    type = LookupSkill(*token);
    delete token;

    token = temp->gettoken();
    days = static_cast<unsigned int>(token->value());
    delete token;

    exp = 0;
    if (Globals->REQUIRED_EXPERIENCE) {
        token = temp->gettoken();
        exp = static_cast<unsigned int>(token->value());
        delete token;
    }

    delete temp;
}

void Skill::Writeout(Aoutfile *f) const
{
    AString temp;

    if (type.isValid()) {
        if (Globals->REQUIRED_EXPERIENCE) {
            temp = AString(SkillDefs[type].abbr) + " " + days + " " + exp;
        } else {
            temp = AString(SkillDefs[type].abbr) + " " + days;
        }
    } else {
        if (Globals->REQUIRED_EXPERIENCE) {
            temp = AString("NO_SKILL 0 0");
        } else {
            temp = AString("NO_SKILL 0");
        }
    }
    f->PutStr(temp);
}

Skill Skill::Split(size_t total, size_t leave)
{
    Skill temp;
    temp.type = type;
    temp.days = (days * leave) / total;
    days = days - temp.days;
    temp.exp = (exp * leave) / total;
    exp = exp - temp.exp;
    return temp;
}

size_t SkillList::GetDays(const Skills& skill)
{
    for(const auto& s: skills_) {
        if (s.type == skill) {
            return s.days;
        }
    }
    return 0;
}

void SkillList::SetDays(const Skills& skill, size_t days)
{
    for(auto it = skills_.begin(); it != skills_.end(); ++it) {
        auto& s = *it;
        if (s.type == skill) {
            if ((days == 0) && (s.exp <= 0)) {
                skills_.erase(it);
                return;
            } else {
                s.days = days;
                return;
            }
        }
    }
    if (days == 0) return;
    auto& s = skills_.emplace_back();
    s.type = skill;
    s.days = days;
    s.exp = 0;
}

size_t SkillList::GetExp(const Skills& skill)
{
    for(const auto& s: skills_) {
        if (s.type == skill) {
            return s.exp;
        }
    }
    return 0;
}

void SkillList::SetExp(const Skills& skill, size_t exp)
{
    for(auto& s: skills_) {
        if (s.type == skill) {
            s.exp = exp;
            return;
        }
    }
    if (exp == 0)
    {
        return;
    }
    auto& s = skills_.emplace_back();
    s.type = skill;
    s.days = 0;
    s.exp = exp;
}

SkillList SkillList::Split(size_t total, size_t leave)
{
    SkillList ret;
    auto it = skills_.begin();
    while(it != skills_.end()) {
        auto& s = *it;
        Skill n = s.Split(total, leave);
        ret.push_back(n);
        if ((s.days == 0) && (s.exp == 0))
        {
            it = skills_.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return ret;
}

void SkillList::Combine(const SkillList& b)
{
    for(const auto& s: b) {
        SetDays(s.type, GetDays(s.type) + s.days);
        SetExp(s.type, GetExp(s.type) + s.exp);
    }
}

/* Returns the rate of study (days/month and man)
 * for studying a skill
 */
size_t SkillList::GetStudyRate(const Skills& skill, size_t nummen)
{
    size_t days = 0;
    size_t exp = 0;
    if (nummen < 1)
    {
        return 0;
    }
    for(auto& s: skills_) {
        if (s.type == skill) {
            days = s.days / nummen;
            if (Globals->REQUIRED_EXPERIENCE)
                exp = s.exp / nummen;
        }
    }
    
    size_t rate = StudyRateAdjustment(days, exp);
    
    /*
     * if (nummen == 10) {
        AString temp = "Studying ";
        temp += SkillDefs[skill].abbr;
        temp += " with ";
        temp += exp + "XP at a rate of ";
        temp += rate + ".";
        Awrite(temp);
    }
    */
    return rate;
}

AString SkillList::Report(size_t nummen)
{
    AString temp;
    if (skills_.empty()) {
        temp += "none";
        return temp;
    }
    int i = 0;
    int displayed = 0;
    for(const auto& s: skills_) {
        if (s.days == 0) continue;
        displayed++;
        if (i) {
            temp += ", ";
        } else {
            i=1;
        }
        temp += SkillStrs(s.type);
        temp += AString(" ") + GetLevelByDays(s.days / nummen) +
            AString(" (") + AString(s.days/nummen);
        if (Globals->REQUIRED_EXPERIENCE) {
            temp += AString("+") + AString(GetStudyRate(s.type, nummen));
        }
        temp += AString(")");
    }
    if (!displayed) temp += "none";
    return temp;
}

void SkillList::Readin(Ainfile *f)
{
    const size_t n = f->GetInt<size_t>();
    for (size_t i = 0; i < n; ++i) {
        Skill s;
        s.Readin(f);
        if ((s.days == 0) && (s.exp==0))
        {
            continue;
        }

        skills_.push_back(s);
    }
}

void SkillList::Writeout(Aoutfile *f)
{
    f->PutInt(skills_.size());
    for(const auto& s: skills_)
    {
        s.Writeout(f);
    }
}
