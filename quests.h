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

#ifndef QUEST_CLASS
#define QUEST_CLASS

#include <set>
#include <string>
#include <algorithm>
#include <memory>

#include "ptrlist.h"
#include "astring.h"
#include "fileio.h"
#include "unit.h"
#include "items.h"
#include "validenum.h"

enum class _QuestTypes : size_t{
    SLAY,
    HARVEST,
    BUILD,
    VISIT,
    DELIVER,
    DEMOLISH,
    NQUESTS
};

using Quests = ValidEnum<_QuestTypes, _QuestTypes::NQUESTS>;

class Quest
{
    public:
        using Handle = std::shared_ptr<Quest>;

        Quest();
        ~Quest() = default;

        Quests    type;
        int    target;
        Item    objective;
        Objects    building;
        int    regionnum;
        AString    regionname;
        std::set<std::string> destinations;
        PtrList<Item>    rewards;
};

class QuestList
{
    private:
        using list_type = PtrList<Quest>;

    public:
        using iterator = list_type::iterator;
        using const_iterator = list_type::const_iterator;

        int ReadQuests(Ainfile& f);
        void WriteQuests(Aoutfile& f);

        bool CheckQuestKillTarget(const Unit::Handle& u, ItemList& reward);
        bool CheckQuestHarvestTarget(const ARegion::Handle& r,
                                     const Items& item,
                                     int harvested,
                                     int max,
                                     const Unit::Handle& u);
        bool CheckQuestBuildTarget(const ARegion::Handle& r, const Objects& building, const Unit::Handle& u);
        bool CheckQuestVisitTarget(const ARegion::Handle& r, const Unit::Handle& u);
        bool CheckQuestDemolishTarget(const ARegion::Handle& r, const Objects& building, const Unit::Handle& u);
        void clear()
        {
            quests_.clear();
        }

        void push_back(const Quest::Handle& quest)
        {
            quests_.push_back(quest);
        }

        iterator begin()
        {
            return quests_.begin();
        }

        iterator end()
        {
            return quests_.end();
        }

        const_iterator begin() const
        {
            return quests_.begin();
        }

        const_iterator end() const
        {
            return quests_.end();
        }

        const_iterator cbegin() const
        {
            return quests_.cbegin();
        }

        const_iterator cend() const
        {
            return quests_.cend();
        }

        size_t size() const
        {
            return quests_.size();
        }

    private:
        list_type quests_;
};

extern QuestList quests;

#endif
