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
#ifndef SPELLS_H
#define SPELLS_H

#include "unit.h"
#include "astring.h"
#include "orders.h"
#include "skills.h"
#include "orderscheck.h"

template<typename BaseGame>
class SpellsGame : public BaseGame
{
    private:
        template<typename T>
        static inline T square(T a)
        {
            return a * a;
        }

        static int RandomiseSummonAmount(int num)
        {
            int retval, i;
        
            retval = 0;
        
            for (i = 0; i < 2 * num; i++)
            {
                if (getrandom(2))
                    retval++;
            }
            if (retval < 1 && num > 0)
                retval = 1;
        
            return retval;
        }

    public:
        virtual void RunACastOrder(const ARegion::Handle&, const Object::Handle&, const Unit::Handle&) override;
        virtual void ProcessCastOrder(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck) override;
        virtual void RunTeleportOrders() override;

        //
        // Spell parsing - generic
        //
        void ProcessGenericSpell(const Unit::Handle&, const Skills&, const OrdersCheck::Handle& pCheck);
        void ProcessRegionSpell(const Unit::Handle&, AString &, const Skills&, const OrdersCheck::Handle& pCheck);
        
        //
        // Spell parsing - specific
        //
        void ProcessCastGateLore(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck );
        void ProcessCastPortalLore(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck );
        void ProcessPhanBeasts(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck );
        void ProcessPhanUndead(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck );
        void ProcessPhanDemons(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck );
        void ProcessInvisibility(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck );
        void ProcessBirdLore(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck );
        void ProcessMindReading(const Unit::Handle&, AString&, const OrdersCheck::Handle& pCheck );
        void ProcessLacandonTeleport(const Unit::Handle&, AString &, const OrdersCheck::Handle& pCheck);
        void ProcessTransmutation(const Unit::Handle&, AString &, const OrdersCheck::Handle& pCheck);
        
        //
        // Spell helpers
        //
        bool GetRegionInRange(const ARegion::Handle& r, const ARegion::Handle& tar, const Unit::Handle& u, const Skills& spell);
        
        //
        // Spell running
        //
        bool RunDetectGates(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
        bool RunFarsight(const ARegion::Handle&,const Unit::Handle&);
        bool RunGateJump(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
        bool RunTeleport(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
        bool RunLacandonTeleport(const ARegion::Handle&, const Object::Handle&, const Unit::Handle&);
        bool RunPortalLore(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
        bool RunEarthLore(const ARegion::Handle&,const Unit::Handle&);
        bool RunWeatherLore(const ARegion::Handle&, const Unit::Handle&);
        bool RunClearSkies(const ARegion::Handle&,const Unit::Handle&);
        bool RunPhanBeasts(const ARegion::Handle&,const Unit::Handle&);
        bool RunPhanUndead(const ARegion::Handle&,const Unit::Handle&);
        bool RunPhanDemons(const ARegion::Handle&,const Unit::Handle&);
        bool RunInvisibility(const ARegion::Handle&,const Unit::Handle&);
        bool RunWolfLore(const ARegion::Handle&,const Unit::Handle&);
        bool RunBirdLore(const ARegion::Handle&,const Unit::Handle&);
        bool RunDragonLore(const ARegion::Handle&,const Unit::Handle&);
        bool RunSummonSkeletons(const ARegion::Handle&,const Unit::Handle&);
        bool RunRaiseUndead(const ARegion::Handle&,const Unit::Handle&);
        bool RunSummonLich(const ARegion::Handle&,const Unit::Handle&);
        bool RunSummonImps(const ARegion::Handle&,const Unit::Handle&);
        bool RunSummonDemon(const ARegion::Handle&,const Unit::Handle&);
        bool RunSummonBalrog(const ARegion::Handle&,const Unit::Handle&);
        bool RunCreateArtifact(const ARegion::Handle&, const Unit::Handle&, const Skills&, const Items&);
        bool RunEngraveRunes(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
        bool RunConstructGate(const ARegion::Handle&, const Unit::Handle&, const Skills&);
        bool RunEnchant(const ARegion::Handle&,const Unit::Handle&, const Skills&, const Items&);
        bool RunMindReading(const ARegion::Handle&,const Unit::Handle&);
        bool RunTransmutation(const ARegion::Handle&,const Unit::Handle&);
        bool RunBlasphemousRitual(const ARegion::Handle&,const Unit::Handle&);
};

#endif
