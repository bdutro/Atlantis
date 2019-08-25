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
#ifndef ARMY_CLASS
#define ARMY_CLASS

#include <functional>
#include <map>
#include <vector>
#include <list>
#include <memory>

class Soldier;
class Army;
class Location;

#include "unit.h"
#include "alist.h"
#include "items.h"
#include "object.h"
#include "shields.h"
#include "helper.h"
#include "skilltype.h"

class Soldier {
    public:
        using Handle = std::shared_ptr<Soldier>;
        Soldier(const Unit::Handle& unit, const std::shared_ptr<Object>& object, const Regions& regType, const Items& race, int ass=0);

        void SetupSpell();
        void SetupCombatItems();

        //
        // SetupHealing is actually game-specific, and appears in specials.cpp
        //
        void SetupHealing();

        int HasEffect(char const *);
        void SetEffect(char const *);
        void ClearEffect(char const *);
        void ClearOneTimeEffects(void);
        bool ArmorProtect(int weaponClass );

        void RestoreItems();
        void Alive(int);
        void Dead();

        /* Unit info */
        AString name;
        Unit::WeakHandle unit;
        Items race;
        Items riding;
        Objects building;

        /* Healing information */
        unsigned int healing;
        int healtype;
        Items healitem;
        int canbehealed;
        int regen;

        /* Attack info */
        Items weapon;
        int attacktype;
        int askill;
        int attacks;
        char const *special;
        size_t slevel;

        /* Defense info */
        int dskill[NUM_ATTACK_TYPES];
        int protection[NUM_ATTACK_TYPES];
        Items armor;
        int hits;
        int maxhits;
        int damage;

        BITFIELD battleItems;
        int amuletofi;

        /* Effects */
        std::map< char const *, int > effects;
};

class Army
{
    public:
        using Handle = std::shared_ptr<Army>;

        Army(const Unit::Handle&, const std::list<std::shared_ptr<Location>>&, int, int = 0);
        ~Army() = default;

        void WriteLosses(Battle&);
        void Lose(Battle&, ItemList&);
        void Win(Battle&, const ItemList&);
        void Tie(Battle&);
        bool CanBeHealed();
        void DoHeal(Battle&);
        void DoHealLevel(Battle&, size_t, int useItems );
        void Regenerate(Battle&);

        void GetMonSpoils(ItemList&, const Items&, size_t);

        bool Broken();
        size_t NumAlive();
        size_t NumSpoilers();
        size_t CanAttack();
        size_t NumFront();
        Soldier::Handle GetAttacker(size_t, bool &);
        ssize_t GetEffectNum(char const *effect);
        ssize_t GetTargetNum(char const *special = NULL);
        Soldier::Handle GetTarget( size_t );
        int RemoveEffects(int num, char const *effect);
        int DoAnAttack(char const *special, int numAttacks, int attackType,
                int attackLevel, int flags, int weaponClass, char const *effect,
                int mountBonus, const Soldier::Handle& attacker);
        void Kill(size_t);
        void Reset();

        //
        // These funcs are in specials.cpp
        //
        bool CheckSpecialTarget(char const *, size_t);

        std::vector<Soldier::Handle> soldiers;
        Unit::WeakHandle leader;
        ShieldList shields;
        int round;
        int tac;
        size_t canfront;
        size_t canbehind;
        size_t notfront;
        size_t notbehind;
        size_t count;

        int hitsalive; // current number of "living hits"
        int hitstotal; // Number of hits at start of battle.

    private:
        size_t BuildArmy_(const std::list<std::shared_ptr<Location>>& locs, int regtype, int ass);
};

#endif
