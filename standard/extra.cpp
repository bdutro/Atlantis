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
//
// This file contains extra game-specific functions
//
#include "game.h"
#include "gamedata.h"

int Game::SetupFaction( const Faction::Handle& pFac )
{
    pFac->unclaimed = Globals->START_MONEY + TurnNumber() * 50;

    if (pFac->noStartLeader)
        return 1;

    //
    // Set up first unit.
    //
    Unit::Handle temp2 = GetNewUnit( pFac );
    temp2->SetMen(Items::Types::I_LEADERS, 1);
    pFac->DiscoverItem(Items::Types::I_LEADERS, 0, 1);
    temp2->reveal = REVEAL_FACTION;

    temp2->type = U_MAGE;
    temp2->Study(S_PATTERN, 30);
    temp2->Study(S_SPIRIT, 30);
    temp2->Study(S_GATE_LORE, 30);

    if (TurnNumber() >= 25) {
        temp2->Study(S_PATTERN, 60);
        temp2->Study(S_SPIRIT, 60);
        temp2->Study(S_FORCE, 90);
        temp2->Study(S_COMBAT, 30);
    }

    if (Globals->UPKEEP_MINIMUM_FOOD > 0)
    {
        if (!(ItemDefs[static_cast<size_t>(Items::Types::I_FOOD)].flags & ItemType::DISABLED)) {
            temp2->items.SetNum(Items::Types::I_FOOD, 6);
            pFac->DiscoverItem(Items::Types::I_FOOD, 0, 1);
        } else if (!(ItemDefs[static_cast<size_t>(Items::Types::I_FISH)].flags & ItemType::DISABLED)) {
            temp2->items.SetNum(Items::Types::I_FISH, 6);
            pFac->DiscoverItem(Items::Types::I_FISH, 0, 1);
        } else if (!(ItemDefs[static_cast<size_t>(Items::Types::I_LIVESTOCK)].flags & ItemType::DISABLED)) {
            temp2->items.SetNum(Items::Types::I_LIVESTOCK, 6);
            pFac->DiscoverItem(Items::Types::I_LIVESTOCK, 0, 1);
        } else if (!(ItemDefs[static_cast<size_t>(Items::Types::I_GRAIN)].flags & ItemType::DISABLED)) {
            temp2->items.SetNum(Items::Types::I_GRAIN, 2);
            pFac->DiscoverItem(Items::Types::I_GRAIN, 0, 1);
        }
        temp2->items.SetNum(Items::Types::I_SILVER, 10);
    }

    ARegion::WeakHandle reg;
    if (!pFac->pStartLoc.expired()) {
        reg = pFac->pStartLoc;
    } else if (!Globals->MULTI_HEX_NEXUS) {
        reg = regions.front();
    } else {
        auto pArr = regions.GetRegionArray(ARegionArray::LEVEL_NEXUS);
        while(reg.expired()) {
            reg = pArr->GetRegion(getrandom(pArr->x), getrandom(pArr->y));
        }
    }
    temp2->MoveUnit( reg.lock()->GetDummy() );

    if (Globals->LAIR_MONSTERS_EXIST || Globals->WANDERING_MONSTERS_EXIST) {
        // Try to auto-declare all player factions unfriendly
        // to Creatures, since all they do is attack you.
        pFac->SetAttitude(monfaction, A_UNFRIENDLY);
    }

    return( 1 );
}

Faction::WeakHandle Game::CheckVictory()
{
    for(const auto& region: regions) {
        for(const auto& obj: region->objects) {
            if (obj->type != Objects::Types::O_BKEEP){
                continue;
            }
            if (!obj->units.empty()){
                return Faction::WeakHandle();
            }
            // Now see find the first faction guarding the region
            for(const auto& o: region->objects) {
                for(const auto& u: o->units) {
                    if (u->guard == GUARD_GUARD){
                        return u->faction;
                    }
                }
            }
            break;
        }
    }
    return Faction::WeakHandle();
}

void Game::ModifyTablesPerRuleset(void)
{
    if (Globals->APPRENTICES_EXIST)
        EnableSkill(S_MANIPULATE);

    if (!Globals->GATES_EXIST)
        DisableSkill(S_GATE_LORE);

    if (Globals->FULL_TRUESEEING_BONUS) {
        ModifyAttribMod("observation", 1, AttribModItem::SKILL,
                "TRUE", AttribModItem::UNIT_LEVEL, 1);
    }
    if (Globals->IMPROVED_AMTS) {
        ModifyAttribMod("observation", 2, AttribModItem::ITEM,
                "AMTS", AttribModItem::CONSTANT, 3);
    }
    if (Globals->FULL_INVIS_ON_SELF) {
        ModifyAttribMod("stealth", 3, AttribModItem::SKILL,
                "INVI", AttribModItem::UNIT_LEVEL, 1);
    }

    if (Globals->NEXUS_IS_CITY && Globals->TOWNS_EXIST) {
        ClearTerrainRaces(Regions::Types::R_NEXUS);
        ModifyTerrainRace(Regions::Types::R_NEXUS, 0, Items::Types::I_HIGHELF);
        ModifyTerrainRace(Regions::Types::R_NEXUS, 1, Items::Types::I_VIKING);
        ModifyTerrainRace(Regions::Types::R_NEXUS, 2, Items::Types::I_PLAINSMAN);
        ClearTerrainItems(Regions::Types::R_NEXUS);
        ModifyTerrainItems(Regions::Types::R_NEXUS, 0, Items::Types::I_IRON, 100, 10);
        ModifyTerrainItems(Regions::Types::R_NEXUS, 1, Items::Types::I_WOOD, 100, 10);
        ModifyTerrainItems(Regions::Types::R_NEXUS, 2, Items::Types::I_STONE, 100, 10);
        ModifyTerrainEconomy(Regions::Types::R_NEXUS, 1000, 15, 50, 2);
    }

    EnableItem(Items::Types::I_PICK);
    EnableItem(Items::Types::I_SPEAR);
    EnableItem(Items::Types::I_AXE);
    EnableItem(Items::Types::I_HAMMER);
    EnableItem(Items::Types::I_MCROSSBOW);
    EnableItem(Items::Types::I_MWAGON);
    EnableItem(Items::Types::I_GLIDER);
    EnableItem(Items::Types::I_NET);
    EnableItem(Items::Types::I_LASSO);
    EnableItem(Items::Types::I_BAG);
    EnableItem(Items::Types::I_SPINNING);
    EnableItem(Items::Types::I_LEATHERARMOR);
    EnableItem(Items::Types::I_CLOTHARMOR);
    EnableItem(Items::Types::I_BOOTS);
    EnableItem(Items::Types::I_BAXE);
    EnableItem(Items::Types::I_MBAXE);
    EnableItem(Items::Types::I_IMARM);
    EnableItem(Items::Types::I_SUPERBOW);
    EnableItem(Items::Types::I_LANCE);
    EnableItem(Items::Types::I_JAVELIN);
    EnableItem(Items::Types::I_PIKE);

    EnableSkill(S_ARMORCRAFT);
    EnableSkill(S_WEAPONCRAFT);

    EnableObject(Objects::Types::O_ROADN);
    EnableObject(Objects::Types::O_ROADNE);
    EnableObject(Objects::Types::O_ROADNW);
    EnableObject(Objects::Types::O_ROADS);
    EnableObject(Objects::Types::O_ROADSE);
    EnableObject(Objects::Types::O_ROADSW);
    EnableObject(Objects::Types::O_TEMPLE);
    EnableObject(Objects::Types::O_MQUARRY);
    EnableObject(Objects::Types::O_AMINE);
    EnableObject(Objects::Types::O_PRESERVE);
    EnableObject(Objects::Types::O_SACGROVE);

    EnableObject(Objects::Types::O_ISLE);
    EnableObject(Objects::Types::O_DERELICT);
    EnableObject(Objects::Types::O_OCAVE);
    EnableObject(Objects::Types::O_WHIRL);
    EnableItem(Items::Types::I_PIRATES);
    EnableItem(Items::Types::I_KRAKEN);
    EnableItem(Items::Types::I_MERFOLK);
    EnableItem(Items::Types::I_ELEMENTAL);

    if ((Globals->UNDERDEEP_LEVELS > 0) || (Globals->UNDERWORLD_LEVELS > 1)) {
        EnableItem(Items::Types::I_MUSHROOM);
        EnableItem(Items::Types::I_HEALPOTION);
        EnableItem(Items::Types::I_ROUGHGEM);
        EnableItem(Items::Types::I_GEMS);
        EnableSkill(S_GEMCUTTING);
    }

    // Modify the various spells which are allowed to cross levels
    if (Globals->EASIER_UNDERWORLD) {
        ModifyRangeFlags("rng_teleport", RangeType::RNG_CROSS_LEVELS);
        ModifyRangeFlags("rng_farsight", RangeType::RNG_CROSS_LEVELS);
        ModifyRangeFlags("rng_clearsky", RangeType::RNG_CROSS_LEVELS);
        ModifyRangeFlags("rng_weather", RangeType::RNG_CROSS_LEVELS);
    }

    if (Globals->TRANSPORT & GameDefs::ALLOW_TRANSPORT) {
        EnableSkill(S_QUARTERMASTER);
        EnableObject(Objects::Types::O_CARAVANSERAI);
    }
    // XXX -- This is just here to preserve existing behavior
    ModifyItemProductionBooster(Items::Types::I_AXE, Items::Types::I_HAMMER, 1);
    return;
}
