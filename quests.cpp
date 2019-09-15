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

#include "quests.h"
#include "object.h"
#include <iterator>

QuestList quests;

Quest::Quest()
{
    type.invalidate();
    target = -1;
    objective.type.invalidate();
    objective.num = 0;
    building.invalidate();
    regionnum = -1;
    regionname = "-";
}

bool QuestList::ReadQuests(Ainfile& f)
{
    quests.clear();

    int count = f.GetInt<int>();
    if (count < 0)
    {
        return false;
    }

    AString name;
    while (count-- > 0) {
        Quest::Handle quest = std::make_shared<Quest>();
        quest->type = f.GetInt<Quests>();
        switch (quest->type.asEnum()) {
            case Quests::Types::SLAY:
                quest->target = f.GetInt<int>();
                break;
            case Quests::Types::HARVEST:
                quest->objective.Readin(f);
                quest->regionnum = f.GetInt<int>();
                break;
            case Quests::Types::BUILD:
                name = f.GetStr();
                quest->building = LookupObject(name);
                name = f.GetStr();
                quest->regionname = name;
                break;
            case Quests::Types::VISIT:
            {
                name = f.GetStr();
                quest->building = LookupObject(name);
                int dests = f.GetInt<int>();
                while (dests-- > 0) {
                    name = f.GetStr();
                    quest->destinations.insert(name.Str());
                }
                break;
            }
            case Quests::Types::DEMOLISH:
                quest->target = f.GetInt<int>();
                quest->regionnum = f.GetInt<int>();
                break;
            default:
            {
                quest->target = f.GetInt<int>();
                quest->objective.Readin(f);
                name = f.GetStr();
                quest->building = LookupObject(name);
                quest->regionnum = f.GetInt<int>();
                name = f.GetStr();
                quest->regionname = name;
                int dests = f.GetInt<int>();
                while (dests-- > 0) {
                    name = f.GetStr();
                    quest->destinations.insert(name.Str());
                }
                break;
            }
        }
        int rewards = f.GetInt<int>();
        while (rewards-- > 0) {
            Item::Handle item = std::make_shared<Item>();
            item->Readin(f);
            if (!item->type.isValid())
            {
                return false;
            }
            quest->rewards.push_back(item);
        }
        quests.push_back(quest);
    }

    return true;
}

void QuestList::WriteQuests(Aoutfile& f)
{
    std::set<std::string>::iterator it;

    f.PutInt(quests.size());
    for(const auto& q: *this) {
        f.PutInt(q->type);
        switch(q->type.asEnum()) {
            case Quests::Types::SLAY:
                f.PutInt(q->target);
                break;
            case Quests::Types::HARVEST:
                q->objective.Writeout(f);
                f.PutInt(q->regionnum);
                break;
            case Quests::Types::BUILD:
                if (q->building.isValid())
                    f.PutStr(ObjectDefs[q->building].name);
                else
                    f.PutStr("NO_OBJECT");
                f.PutStr(q->regionname);
                break;
            case Quests::Types::VISIT:
                if (q->building.isValid())
                {
                    f.PutStr(ObjectDefs[q->building].name);
                }
                else
                {
                    f.PutStr("NO_OBJECT");
                }
                f.PutInt(q->destinations.size());
                for (it = q->destinations.begin();
                        it != q->destinations.end();
                        it++) {
                    f.PutStr(it->c_str());
                }
                break;
            case Quests::Types::DEMOLISH:
                f.PutInt(q->target);
                f.PutInt(q->regionnum);
                break;
            default:
                f.PutInt(q->target);
                q->objective.Writeout(f);
                if (q->building.isValid())
                    f.PutStr(ObjectDefs[q->building].name);
                else
                    f.PutStr("NO_OBJECT");
                f.PutInt(q->regionnum);
                f.PutStr(q->regionname);
                f.PutInt(q->destinations.size());
                for (it = q->destinations.begin();
                        it != q->destinations.end();
                        it++) {
                    f.PutStr(it->c_str());
                }
                break;
        }
        f.PutInt(q->rewards.size());
        for(const auto& i: q->rewards) {
            i->Writeout(f);
        }
    }

    f.PutInt(0);

    return;
}

bool QuestList::CheckQuestKillTarget(const Unit::Handle& u, ItemList& reward)
{
    for(auto it = begin(); it != end(); ++it)
    {
        const auto& q = *it;
        if (q->type == Quests::Types::SLAY && q->target == static_cast<int>(u->num))
        {
            // This dead thing was the target of a quest!
            for(const auto& i: q->rewards)
            {
                reward.SetNum(i->type, reward.GetNum(i->type) + i->num);
            }
            erase(it);
            return true;
        }
    }

    return false;
}

bool QuestList::CheckQuestHarvestTarget(const ARegion::Handle& r,
                                        const Items& item,
                                        int harvested,
                                        int max,
                                        const Unit::Handle& u)
{
    for(auto it = begin(); it != end(); ++it) {
        const auto& q = *it;
        if (q->type == Quests::Types::HARVEST &&
                q->regionnum == static_cast<int>(r->num) &&
                q->objective.type == item) {
            if (getrandom(max) < harvested) {
                const auto u_fac = u->faction.lock();
                for(const auto& i: q->rewards) {
                    u->items.SetNum(i->type, u->items.GetNum(i->type) + i->num);
                    u_fac->DiscoverItem(i->type, 0, 1);
                }
                erase(it);
                return true;
            }
        }
    }

    return false;
}

bool QuestList::CheckQuestBuildTarget(const ARegion::Handle& r, const Objects& building, const Unit::Handle& u)
{
    for(auto it = begin(); it != end(); ++it) {
        const auto& q = *it;
        if (q->type == Quests::Types::BUILD &&
                q->building == building &&
                q->regionname == r->name) {
            const auto u_fac = u->faction.lock();
            for(const auto& i: q->rewards) {
                u->items.SetNum(i->type, u->items.GetNum(i->type) + i->num);
                u_fac->DiscoverItem(i->type, 0, 1);
            }
            erase(it);
            return true;
        }
    }

    return false;
}

bool QuestList::CheckQuestVisitTarget(const ARegion::Handle& r, const Unit::Handle& u)
{
    std::set<std::string> intersection;

    for(auto it = begin(); it != end(); ++it) {
        const auto& q = *it;
        if (q->type != Quests::Types::VISIT)
            continue;
        if (!q->destinations.count(r->name.Str()))
            continue;
        for(const auto& o: r->objects) {
            if (o->type == q->building) {
                u->visited.insert(r->name.Str());
                intersection.clear();
                std::set_intersection(
                    q->destinations.begin(),
                    q->destinations.end(),
                    u->visited.begin(),
                    u->visited.end(),
                    std::inserter(intersection, intersection.begin()),
                    std::less<std::string>()
                );
                if (intersection.size() == q->destinations.size()) {
                    // This unit has visited the
                    // required buildings in all those
                    // regions, so they win
                    const auto u_fac = u->faction.lock();
                    for(const auto& i: q->rewards) {
                        u->items.SetNum(i->type, u->items.GetNum(i->type) + i->num);
                        u_fac->DiscoverItem(i->type, 0, 1);
                    }
                    erase(it);
                    return true;
                }
            }
        }
    }

    return false;
}

bool QuestList::CheckQuestDemolishTarget(const ARegion::Handle& r, const Objects& building, const Unit::Handle& u)
{
    for(auto it = begin(); it != end(); ++it) {
        const auto& q = *it;
        if (q->type == Quests::Types::DEMOLISH &&
                q->regionnum == static_cast<int>(r->num) &&
                q->target == static_cast<int>(building)) {
            const auto u_fac = u->faction.lock();
            for (const auto& i: q->rewards) {
                u->items.SetNum(i->type, u->items.GetNum(i->type) + i->num);
                u_fac->DiscoverItem(i->type, 0, 1);
            }
            erase(it);
            return true;
        }
    }

    return false;
}

