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

#include "gamedata.h"
#include "game.h"

const EnumArray<Attitudes, const char *, Attitudes::size()> AttitudeStrs = {{
    "Hostile",
    "Unfriendly",
    "Neutral",
    "Friendly",
    "Ally"
}};

const EnumArray<Factions, const char *, Factions::size()> FactionStrs = {{
    "War",
    "Trade",
    "Magic"
}};

// LLS - fix up the template strings
const EnumArray<Templates, const char*, Templates::size()> TemplateStrs = {{
    "off",
    "short",
    "long",
    "map"
}};

Templates ParseTemplate(const AString& token)
{
    for (auto i = Templates::begin(); i != Templates::end(); ++i)
    {
        if (token == TemplateStrs[*i])
        {
            return *i;
        }
    }
    return Templates();
}

Attitudes ParseAttitude(const AString& token)
{
    for (auto i = Attitudes::begin(); i != Attitudes::end(); ++i)
    {
        if (token == AttitudeStrs[*i])
        {
            return *i;
        }
    }
    return Attitudes();
}

FactionVector::FactionVector(size_t size)
{
    vector.resize(size);
    vectorsize = size;
    ClearVector();
}

void FactionVector::ClearVector()
{
    vector.clear();
}

void FactionVector::SetFaction(size_t x, const Faction::WeakHandle& fac)
{
    vector[x] = fac;
}

Faction::WeakHandle FactionVector::GetFaction(size_t x)
{
    return vector[x];
}

void Attitude::Writeout(Aoutfile& f)
{
    f.PutInt(factionnum);
    f.PutInt(attitude);
}

void Attitude::Readin(Ainfile& f, ATL_VER)
{
    factionnum = f.GetInt<size_t>();
    attitude = f.GetInt<int>();
}

Faction::Faction()
{
    exists = true;
    type.fill(1);
    lastchange = -6;
    times = 0;
    showunitattitudes = 0;
    temformat = Templates::Types::TEMPLATE_OFF;
    quit = QuitReason::QUIT_NONE;
    defaultattitude = Attitudes::Types::A_NEUTRAL;
    unclaimed = 0;
    noStartLeader = 0;
    startturn = 0;
}

Faction::Faction(size_t n)
{
    exists = true;
    num = n;
    type.fill(1);
    lastchange = -6;
    name = AString("Faction (") + AString(num) + AString(")");
    address = AString("NoAddress");
    password = AString("none");
    times = 1;
    showunitattitudes = 0;
    temformat = Templates::Types::TEMPLATE_LONG;
    defaultattitude = Attitudes::Types::A_NEUTRAL;
    quit = QuitReason::QUIT_NONE;
    unclaimed = 0;
    noStartLeader = 0;
    startturn = 0;
}

void Faction::Writeout(Aoutfile& f)
{
    f.PutInt(num);

    for (const auto& t: type)
    {
        f.PutInt(t);
    }

    f.PutInt(lastchange);
    f.PutInt(lastorders);
    f.PutInt(unclaimed);
    f.PutStr(name);
    f.PutStr(address);
    f.PutStr(password);
    f.PutInt(times);
    f.PutBool(showunitattitudes);
    f.PutInt(temformat);

    skills.Writeout(f);
    items.Writeout(f);
    f.PutInt(defaultattitude);
    f.PutInt(attitudes.size());
    for(const auto& a: attitudes)
    {
        a->Writeout(f);
    }
}

void Faction::Readin(Ainfile& f, ATL_VER v)
{
    num = f.GetInt<size_t>();

    for (auto& t: type)
    {
        t = f.GetInt<int>();
    }

    lastchange = f.GetInt<int>();
    lastorders = f.GetInt<size_t>();
    unclaimed = f.GetInt<size_t>();

    name = f.GetStr();
    address = f.GetStr();
    password = f.GetStr();
    times = f.GetInt<int>();
    showunitattitudes = f.GetInt<int>();
    temformat = f.GetInt<Templates>();

    skills.Readin(f);
    items.Readin(f);

    defaultattitude = f.GetInt<int>();
    int n = f.GetInt<int>();
    for (int i = 0; i < n; ++i) {
        Attitude::Handle a = std::make_shared<Attitude>();
        a->Readin(f, v);
        if (a->factionnum != num)
        {
            attitudes.push_back(a);
        }
    }

    // if (skills.GetDays(S_BUILDING) > 1)
    //    shows.Add(new ShowSkill(S_BUILDING, 2));
}

void Faction::View()
{
    AString temp;
    temp = AString("Faction ") + num + AString(" : ") + name;
    Awrite(temp);
}

void Faction::SetName(const AString& s)
{
    if (s.Len()) {
        AString newname = s.getlegal();
        if (!newname.Len())
        {
            return;
        }
        newname += AString(" (") + num + ")";
        name = newname;
    }
}

void Faction::SetNameNoChange(const AString& s)
{
    if (s.Len()) {
        name = s;
    }
}

void Faction::SetAddress(const AString &strNewAddress)
{
    address = strNewAddress;
}

AString Faction::FactionTypeStr()
{
    AString temp;
    if (IsNPC()) return AString("NPC");

    if (Globals->FACTION_LIMIT_TYPE == GameDefs::FactionLimits::FACLIM_UNLIMITED) {
        return (AString("Unlimited"));
    } else if (Globals->FACTION_LIMIT_TYPE == GameDefs::FactionLimits::FACLIM_MAGE_COUNT) {
        return(AString("Normal"));
    } else if (Globals->FACTION_LIMIT_TYPE == GameDefs::FactionLimits::FACLIM_FACTION_TYPES) {
        int comma = 0;
        for (auto i = Factions::begin(); i != Factions::end(); ++i) {
            if (type[*i]) {
                if (comma) {
                    temp += ", ";
                } else {
                    comma = 1;
                }
                temp += AString(FactionStrs[*i]) + " " + type[*i];
            }
        }
        if (!comma) return AString("none");
    }
    return temp;
}

void Faction::WriteReport(Areport& f, const Game& pGame)
{
    if (IsNPC() && num == 1) {
        if (Globals->GM_REPORT || (pGame.month == 0 && pGame.year == 1)) {
            // Put all skills, items and objects in the GM report
            shows.clear();
            for (auto i = Skills::begin(); i != Skills::end(); ++i) {
                for (unsigned int j = 1; j < 6; j++) {
                    shows.emplace_back(*i, j);
                }
            }
            if (!shows.empty()) {
                f.PutStr("Skill reports:");
                for(const auto& s: shows) {
                    AString string = s->Report(*this);
                    if (string.Len()) {
                        f.PutStr("");
                        f.PutStr(string);
                    }
                }
                shows.clear();
                f.EndLine();
            }

            itemshows.clear();
            for (size_t i = 0; i < Items::size(); i++) {
                AString::Handle show = ItemDescription(static_cast<int>(i), 1);
                if (show)
                {
                    itemshows.push_back(show);
                }
            }
            if (!itemshows.empty())
            {
                f.PutStr("Item reports:");
                for(const auto& elem: itemshows)
                {
                    f.PutStr("");
                    f.PutStr(*elem);
                }
                itemshows.clear();
                f.EndLine();
            }

            objectshows.clear();
            for (size_t i = 0; i < Objects::size(); i++)
            {
                AString::Handle show = ObjectDescription(static_cast<int>(i));
                if (show)
                {
                    objectshows.push_back(show);
                }
            }
            if (!objectshows.empty())
            {
                f.PutStr("Object reports:");
                for(const auto& elem: objectshows)
                {
                    f.PutStr("");
                    f.PutStr(*elem);
                }
                objectshows.clear();
                f.EndLine();
            }

            present_regions.clear();
            for(const auto& reg: pGame.regions)
            {
                reg->WriteReport(f, *this, pGame.month, pGame.regions);
            }
        }
        errors.clear();
        events.clear();
        battles.clear();
        return;
    }

    f.PutStr("Atlantis Report For:");
    if ((Globals->FACTION_LIMIT_TYPE == GameDefs::FactionLimits::FACLIM_MAGE_COUNT) ||
            (Globals->FACTION_LIMIT_TYPE == GameDefs::FactionLimits::FACLIM_UNLIMITED))
    {
        f.PutStr(name);
    } else if (Globals->FACTION_LIMIT_TYPE == GameDefs::FactionLimits::FACLIM_FACTION_TYPES)
    {
        f.PutStr(name + " (" + FactionTypeStr() + ")");
    }
    f.PutStr(AString(MonthNames[ pGame.month ]) + ", Year " + pGame.year);
    f.EndLine();

    f.PutStr(AString("Atlantis Engine Version: ") +
            ATL_VER_STRING(CURRENT_ATL_VER));
    f.PutStr(AString(Globals->RULESET_NAME) + ", Version: " +
            ATL_VER_STRING(Globals->RULESET_VERSION));
    f.EndLine();

    if (!times) {
        f.PutStr("Note: The Times is not being sent to you.");
        f.EndLine();
    }

    if (!password.Len() || (password == "none")) {
        f.PutStr("REMINDER: You have not set a password for your faction!");
        f.EndLine();
    }

    if (Globals->MAX_INACTIVE_TURNS != -1) {
        int cturn = static_cast<int>(pGame.TurnNumber() - lastorders);
        if ((cturn >= (Globals->MAX_INACTIVE_TURNS - 3)) && !IsNPC()) {
            cturn = Globals->MAX_INACTIVE_TURNS - cturn;
            f.PutStr(AString("WARNING: You have ") + cturn +
                    AString(" turns until your faction is automatically ")+
                    AString("removed due to inactivity!"));
            f.EndLine();
        }
    }

    if (!exists) {
        if (quit == QuitReason::QUIT_AND_RESTART) {
            f.PutStr("You restarted your faction this turn. This faction "
                    "has been removed, and a new faction has been started "
                    "for you. (Your new faction report will come in a "
                    "separate message.)");
        } else if (quit == QuitReason::QUIT_GAME_OVER) {
            f.PutStr("I'm sorry, the game has ended. Better luck in "
                    "the next game you play!");
        } else if (quit == QuitReason::QUIT_WON_GAME) {
            f.PutStr("Congratulations, you have won the game!");
        } else {
            f.PutStr("I'm sorry, your faction has been eliminated.");
            // LLS
            f.PutStr("If you wish to restart, please let the "
                    "Gamemaster know, and you will be restarted for "
                    "the next available turn.");
        }
        f.PutStr("");
    }

    f.PutStr("Faction Status:");
    if (Globals->FACTION_LIMIT_TYPE == GameDefs::FactionLimits::FACLIM_MAGE_COUNT) {
        f.PutStr(AString("Mages: ") + nummages + " (" +
                pGame.AllowedMages(*this) + ")");
        if (Globals->APPRENTICES_EXIST) {
            AString temp;
            temp = static_cast<char>(toupper(Globals->APPRENTICE_NAME[0]));
            temp += Globals->APPRENTICE_NAME + 1;
            temp += "s: ";
            temp += numapprentices;
            temp += " (";
            temp += pGame.AllowedApprentices(*this);
            temp += ")";
            f.PutStr(temp);
        }
    } else if (Globals->FACTION_LIMIT_TYPE == GameDefs::FactionLimits::FACLIM_FACTION_TYPES) {
        f.PutStr(AString("Tax Regions: ") + war_regions.size() + " (" +
                pGame.AllowedTaxes(*this) + ")");
        f.PutStr(AString("Trade Regions: ") + trade_regions.size() + " (" +
                pGame.AllowedTrades(*this) + ")");
        if (Globals->TRANSPORT.isSet(GameDefs::TransportOptions::ALLOW_TRANSPORT)) {
            f.PutStr(AString("Quartermasters: ") + numqms + " (" +
                    pGame.AllowedQuarterMasters(*this) + ")");
        }
        if (Globals->TACTICS_NEEDS_WAR) {
            f.PutStr(AString("Tacticians: ") + numtacts + " (" +
                    pGame.AllowedTacticians(*this) + ")");
        }
        f.PutStr(AString("Mages: ") + nummages + " (" +
                pGame.AllowedMages(*this) + ")");
        if (Globals->APPRENTICES_EXIST) {
            AString temp;
            temp = static_cast<char>(toupper(Globals->APPRENTICE_NAME[0]));
            temp += Globals->APPRENTICE_NAME + 1;
            temp += "s: ";
            temp += numapprentices;
            temp += " (";
            temp += pGame.AllowedApprentices(*this);
            temp += ")";
            f.PutStr(temp);
        }
    }
    f.PutStr("");

    if (!errors.empty())
    {
        f.PutStr("Errors during turn:");
        for(const auto& elem: errors)
        {
            f.PutStr(elem);
        }
        errors.clear();
        f.EndLine();
    }

    if (!battles.empty())
    {
        f.PutStr("Battles during turn:");
        for(const auto& b: battles)
        {
            b.lock()->Report(f, *this);
        }
        battles.clear();
    }

    if (!events.empty())
    {
        f.PutStr("Events during turn:");
        for(const auto& elem: events)
        {
            f.PutStr(elem);
        }
        events.clear();
        f.EndLine();
    }

    if (!shows.empty())
    {
        f.PutStr("Skill reports:");
        for(const auto& s: shows) {
            AString string = s->Report(*this);
            if (string.Len()) {
                f.PutStr("");
                f.PutStr(string);
            }
        }
        shows.clear();
        f.EndLine();
    }

    if (!itemshows.empty())
    {
        f.PutStr("Item reports:");
        for(const auto& elem: itemshows)
        {
            f.PutStr("");
            f.PutStr(*elem);
        }
        itemshows.clear();
        f.EndLine();
    }

    if (!objectshows.empty())
    {
        f.PutStr("Object reports:");
        for(const auto& elem: objectshows)
        {
            f.PutStr("");
            f.PutStr(*elem);
        }
        objectshows.clear();
        f.EndLine();
    }

    /* Attitudes */
    AString temp = AString("Declared Attitudes (default ") +
        AttitudeStrs[defaultattitude] + "):";
    f.PutStr(temp);
    for (auto i = Attitudes::begin(); i != Attitudes::end(); ++i) {
        bool j = false;
        temp = AString(AttitudeStrs[*i]) + " : ";
        for(const auto& a: attitudes) {
            if (a->attitude == *i) {
                if (j) temp += ", ";
                temp += GetFaction(pGame.factions,
                            a->factionnum)->name;
                j = true;
            }
        }
        if (!j) temp += "none";
        temp += ".";
        f.PutStr(temp);
    }
    f.EndLine();

    temp = AString("Unclaimed silver: ") + unclaimed + ".";
    f.PutStr(temp);
    f.PutStr("");

    for(const auto& r: present_regions) {
        r.lock()->WriteReport(f, *this, pGame.month, pGame.regions);
    } 
        // LLS - maybe we don't want this -- I'll assume not, for now 
    //f.PutStr("#end");
    f.EndLine();

}

// LLS - write order template
void Faction::WriteTemplate(Areport& f, const Game& pGame)
{
    AString temp;
    if (temformat == Templates::Types::TEMPLATE_OFF)
    {
        return;
    }
    if (!IsNPC()) {
        f.PutStr("");
        switch (temformat.asEnum()) {
            case Templates::Types::TEMPLATE_SHORT:
                f.PutStr("Orders Template (Short Format):");
                break;
            case Templates::Types::TEMPLATE_LONG:
                f.PutStr("Orders Template (Long Format):");
                break;
            // DK
            case Templates::Types::TEMPLATE_MAP:
                f.PutStr("Orders Template (Map Format):");
                break;
            default:
                break;
        }

        f.PutStr("");
        temp = AString("#atlantis ") + num;
        if (password != "none") {
            temp += AString(" \"") + password + "\"";
        }
        f.PutStr(temp);
        for(const auto& r: present_regions) {
            // DK
            r.lock()->WriteTemplate(f, *this, pGame.regions, pGame.month);
        }

        f.PutStr("");
        f.PutStr("#end");
        f.EndLine();
    }
}

void Faction::WriteFacInfo(Aoutfile& file)
{
    file.PutStr(AString("Faction: ") + num);
    file.PutStr(AString("Name: ") + name);
    file.PutStr(AString("Email: ") + address);
    file.PutStr(AString("Password: ") + password);
    file.PutStr(AString("LastOrders: ") + lastorders);
    file.PutStr(AString("FirstTurn: ") + startturn);
    file.PutStr(AString("SendTimes: ") + times);

    // LLS - write template info to players file
    file.PutStr(AString("Template: ") + TemplateStrs[temformat]);

    for(const auto& pStr: extraPlayers)
    {
        file.PutStr(pStr);
    }

    extraPlayers.clear();
}

void Faction::CheckExist(const ARegionList& regs)
{
    if (IsNPC())
    {
        return;
    }
    exists = false;
    for(const auto& reg: regs)
    {
        if (reg->Present(*this))
        {
            exists = true;
            return;
        }
    }
}

void Faction::Error(const AString &s)
{
    if (IsNPC())
    {
        return;
    }
    if (errors.size() > 1000)
    {
        if (errors.size() == 1001)
        {
            errors.emplace_back("Too many errors!");
        }
        return;
    }

    errors.emplace_back(s);
}

void Faction::Event(const AString &s)
{
    if (IsNPC())
    {
        return;
    }
    events.emplace_back(s);
}

void Faction::RemoveAttitude(size_t f)
{
    for(auto it = attitudes.begin(); it != attitudes.end(); ++it) {
        const auto& a = *it;
        if (a->factionnum == f) {
            attitudes.erase(it);
            return;
        }
    }
}

Attitudes Faction::GetAttitude(size_t n) const
{
    if (n == num)
    {
        return Attitudes::Types::A_ALLY;
    }
    for(const auto& a: attitudes)
    {
        if (a->factionnum == n)
        {
            return a->attitude;
        }
    }
    return defaultattitude;
}

void Faction::SetAttitude(size_t faction_num, const Attitudes& att)
{
    for(auto it = attitudes.begin(); it != attitudes.end(); ++it)
    {
        const auto& a = *it;
        if (a->factionnum == faction_num)
        {
            if (!att.isValid())
            {
                attitudes.erase(it);
                return;
            }
            else
            {
                a->attitude = att;
                return;
            }
        }
    }
    if (att.isValid())
    {
        auto& a = attitudes.emplace_back();
        a->factionnum = faction_num;
        a->attitude = att;
    }
}

bool Faction::CanCatch(const ARegion& r, const Unit::Handle& t)
{
    if (TerrainDefs[r.type].similar_type == Regions::Types::R_OCEAN)
    {
        return true;
    }

    size_t def = t->GetDefenseRiding();

    for(const auto& o: r.objects)
    {
        for(const auto& u: o->units)
        {
            if (u == t && o->type != Objects::Types::O_DUMMY)
            {
                return true;
            }
            if (u->faction.lock().get() == this && u->GetAttackRiding() >= def)
            {
                return true;
            }
        }
    }
    return false;
}

unsigned int Faction::CanSee(const ARegion& r, const Unit::Handle& u, int practice)
{
    bool detfac = false;
    if (u->faction.lock().get() == this)
    {
        return 2;
    }
    if (u->reveal == UnitReveal::REVEAL_FACTION)
    {
        return 2;
    }

    unsigned int retval = 0;
    if (u->reveal == UnitReveal::REVEAL_UNIT)
    {
        retval = 1;
    }
    if (u->guard == UnitGuard::GUARD_GUARD)
    {
        retval = 1;
    }
    for(const auto& obj: r.objects)
    {
        int dummy = 0;
        if (obj->type == Objects::Types::O_DUMMY)
        {
            dummy = 1;
        }
        for(const auto& temp: obj->units)
        {
            if (u == temp && dummy == 0)
            {
                retval = 1;
            }

            // penalty of 2 to stealth if assassinating and 1 if stealing
            // TODO: not sure about the reasoning behind the IMPROVED_AMTS part
            int stealpenalty = 0;
            if (Globals->HARDER_ASSASSINATION && u->stealorders)
            {
                if (u->stealorders->type == Orders::Types::O_STEAL)
                {
                    stealpenalty = 1;
                }
                else if (u->stealorders->type == Orders::Types::O_ASSASSINATE)
                {
                    if (Globals->IMPROVED_AMTS)
                    {
                        stealpenalty = 1;
                    }
                    else
                    {
                        stealpenalty = 2;
                    }
                }
            }

            if (temp->faction.lock().get() == this)
            {
                if (static_cast<int>(temp->GetAttribute("observation")) >
                        static_cast<int>(u->GetAttribute("stealth")) - stealpenalty)
                {
                    if (practice)
                    {
                        temp->PracticeAttribute("observation");
                        retval = 2;
                    }
                    else
                    {
                        return 2;
                    }
                }
                else
                {
                    if (static_cast<int>(temp->GetAttribute("observation")) ==
                            static_cast<int>(u->GetAttribute("stealth")) - stealpenalty)
                    {
                        if (practice)
                        {
                            temp->PracticeAttribute("observation");
                        }
                        if (retval < 1)
                        {
                            retval = 1;
                        }
                    }
                }
                if (temp->GetSkill(Skills::Types::S_MIND_READING) > 2)
                {
                    detfac = true;
                }
            }
        }
    }
    if (retval == 1 && detfac)
    {
        return 2;
    }
    return retval;
}

void Faction::DefaultOrders()
{
    war_regions.clear();
    trade_regions.clear();
    numshows = 0;
}

void Faction::TimesReward()
{
    if (Globals->TIMES_REWARD) {
        Event(AString("Times reward of ") + Globals->TIMES_REWARD + " silver.");
        unclaimed += Globals->TIMES_REWARD;
    }
}

void Faction::SetNPC()
{
    type.fill(-1);
}

bool Faction::IsNPC() const
{
    if (type[Factions::Types::F_WAR] == -1)
    {
        return true;
    }
    return false;
}

Faction::Handle GetFaction(const PtrList<Faction>& facs, size_t n)
{
    for(const auto& f: facs)
    {
        if (f->num == n)
        {
            return f;
        }
    }
    return nullptr;
}

Faction::WeakHandle GetFaction2(const WeakPtrList<Faction>& facs, size_t n)
{
    for(const auto& f: facs)
    {
        if (f.lock()->num == n)
        {
            return f;
        }
    }
    return Faction::WeakHandle();
}

void Faction::DiscoverItem(const Items& item, int force, int full)
{
    Skills skill;
    size_t seen;
    AString skname;

    seen = items.GetNum(item);
    if (!seen) {
        if (full) {
            items.SetNum(item, 2);
        } else {
            items.SetNum(item, 1);
        }
        force = 1;
    } else {
        if (seen == 1) {
            if (full) {
                items.SetNum(item, 2);
            }
            force = 1;
        } else {
            full = 1;
        }
    }
    if (force) {
        itemshows.emplace_back(ItemDescription(static_cast<int>(item), full));
        if (!full)
            return;
        // If we've found an item that grants a skill, give a
        // report on the skill granted (if we haven't seen it
        // before)
        skname = ItemDefs[item].grantSkill;
        skill = LookupSkill(skname);
        if (skill.isValid() && !SkillDefs[skill].flags.isSet(SkillType::SkillFlags::DISABLED)) {
            for (size_t i = 1; i <= ItemDefs[item].maxGrant; i++) {
                if (i > skills.GetDays(skill)) {
                    skills.SetDays(skill, i);
                    shows.emplace_back(skill, i);
                }
            }
        }
    }
}
