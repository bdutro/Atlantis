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
#ifndef BATTLE_CLASS
#define BATTLE_CLASS

class Battle;

#include <memory>
#include <list>
#include "astring.h"
#include "fileio.h"
#include "army.h"
#include "items.h"

enum {
    ASS_NONE,
    ASS_SUCC,
    ASS_FAIL
};

enum {
    BATTLE_IMPOSSIBLE,
    BATTLE_LOST,
    BATTLE_WON,
    BATTLE_DRAW
};

class Battle
{
    public:
        using Handle = std::shared_ptr<Battle>;
        Battle() = default;
        ~Battle() = default;

        void Report(Areport*, const Faction&);
        void AddLine(const AString &);

        int Run(const std::shared_ptr<ARegion>&,
                const std::shared_ptr<Unit>&,
                const PtrList<Location>&,
                const std::shared_ptr<Unit>&,
                const PtrList<Location>&,
                int ass,
                const ARegionList& pRegs);
        void FreeRound(const std::shared_ptr<Army>&, const std::shared_ptr<Army>&, int ass = 0);
        void NormalRound(int, const std::shared_ptr<Army>&, const std::shared_ptr<Army>&);
        void DoAttack(int round,
                      const std::shared_ptr<Soldier>& a,
                      const std::shared_ptr<Army>& attackers,
                      const std::shared_ptr<Army>& def,
                      bool behind,
                      int ass = 0);

        void GetSpoils(const PtrList<Location>&, ItemList&, int);

        //
        // These functions should be implemented in specials.cpp
        //
        void UpdateShields(const std::shared_ptr<Army>& );
        void DoSpecialAttack(int round,
                             const std::shared_ptr<Soldier>& a,
                             const std::shared_ptr<Army>& attackers,
                             const std::shared_ptr<Army>& def,
                             bool behind);

        void WriteSides(const std::shared_ptr<ARegion>&,
                        const std::shared_ptr<Unit>&,
                        const std::shared_ptr<Unit>&,
                        const PtrList<Location>&,
                        const PtrList<Location>&,
                        int,
                        const ARegionList& pRegs);

        int assassination;
        std::weak_ptr<Faction> attacker; /* Only matters in the case of an assassination */
        AString asstext;
        UniquePtrList<AString> text;
};

#endif
