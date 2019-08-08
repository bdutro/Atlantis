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

#include <stdio.h>
#include <string.h>
#include "game.h"
#include "gamedata.h"

int ARegion::CheckSea(Directions dir, unsigned int range, int remainocean)
{
    static constexpr int NDIRS = static_cast<int>(ALL_DIRECTIONS.size());
    if (type != Regions::Types::R_OCEAN) return 0;
    if (range-- < 1) return 1;
    for (int d2 = -1; d2< 2; d2++) {
        size_t direc = static_cast<size_t>((static_cast<int>(dir) + d2 + NDIRS) % NDIRS);
        const auto& newregion = neighbors[direc];
        if (newregion.expired()) continue;
        remainocean += newregion.lock()->CheckSea(dir, range, remainocean);
        if (remainocean) break;
    }
    return remainocean;
}


void ARegionList::CreateAbyssLevel(unsigned int level, char const *name)
{
    MakeRegions(level, 4, 4);
    pRegionArrays[level]->SetName(name);
    pRegionArrays[level]->levelType = ARegionArray::LEVEL_NEXUS;

    ARegion::WeakHandle reg;
    for (unsigned int x = 0; x < 4; x++) {
        for (unsigned int y = 0; y < 4; y++) {
            reg = pRegionArrays[level]->GetRegion(x, y);
            if (reg.expired()) continue;
            const auto reg_sp = reg.lock();
            reg_sp->SetName("Abyssal Plains");
            reg_sp->type = Regions::Types::R_DESERT;
            reg_sp->wages = -2;
        }
    }

    unsigned int tempx, tempy;
    if (Globals->GATES_EXIST) {
        bool gateset = false;
        do {
            tempx = getrandom(4U);
            tempy = getrandom(4U);
            reg = pRegionArrays[level]->GetRegion(tempx, tempy);
            if (!reg.expired()) {
                gateset = true;
                numberofgates++;
                reg.lock()->gate = -1;
            }
        } while(!gateset);
    }

    FinalSetup(pRegionArrays[level]);

    ARegion::WeakHandle lair;
    do {
        tempx = getrandom(4U);
        tempy = getrandom(4U);
        lair = pRegionArrays[level]->GetRegion(tempx, tempy);
    } while(lair.expired() || lair.lock() == reg.lock());

    const auto lair_sp = lair.lock();
    auto& o = lair_sp->objects.emplace_back(std::make_shared<Object>(lair));
    o->num = lair_sp->buildingseq++;
    o->name = new AString(AString(ObjectDefs[static_cast<size_t>(Objects::Types::O_BKEEP)].name)+" ["+o->num+"]");
    o->type = Objects::Types::O_BKEEP;
    o->incomplete = 0;
    o->inner = -1;
}


void ARegionList::CreateNexusLevel(unsigned int level, unsigned int xSize, unsigned int ySize, char const *name)
{
    MakeRegions(level, xSize, ySize);

    pRegionArrays[level]->SetName(name);
    pRegionArrays[level]->levelType = ARegionArray::LEVEL_NEXUS;

    AString nex_name = Globals->WORLD_NAME;
    nex_name += " Nexus";

    for (unsigned int y = 0; y < ySize; y++) {
        for (unsigned int x = 0; x < xSize; x++) {
            ARegion::WeakHandle reg = pRegionArrays[level]->GetRegion(x, y);
            if (!reg.expired()) {
                const auto reg_sp = reg.lock();
                reg_sp->SetName(nex_name.Str());
                reg_sp->type = Regions::Types::R_NEXUS;
            }
        }
    }

    FinalSetup(pRegionArrays[level]);

    for (unsigned int y = 0; y < ySize; y++) {
        for (unsigned int x = 0; x < xSize; x++) {
            ARegion::WeakHandle reg = pRegionArrays[level]->GetRegion(x, y);
            if (!reg.expired() && Globals->NEXUS_IS_CITY && Globals->TOWNS_EXIST) {
                reg.lock()->MakeStartingCity();
                if (Globals->GATES_EXIST) {
                    numberofgates++;
                }
            }
        }
    }
}

void ARegionList::CreateSurfaceLevel(unsigned int level, unsigned int xSize, unsigned int ySize, char const *name)
{
    if (Globals->ICOSAHEDRAL_WORLD) {
        MakeIcosahedralRegions(level, xSize, ySize);
    } else {
        MakeRegions(level, xSize, ySize);
    }

    pRegionArrays[level]->SetName(name);
    pRegionArrays[level]->levelType = ARegionArray::LEVEL_SURFACE;
    unsigned int sea = Globals->OCEAN;
    if (Globals->SEA_LIMIT)
    {
        // Original line: sea = sea * (100 + 2 * Globals->SEA_LIMIT) / 100;
        sea = sea * (1 + Globals->SEA_LIMIT / 50);
    }
            
    MakeLand(pRegionArrays[level], sea, Globals->CONTINENT_SIZE);
    
    CleanUpWater(pRegionArrays[level]);

    SetupAnchors(pRegionArrays[level]);
    
    GrowTerrain(pRegionArrays[level], false);
    
    AssignTypes(pRegionArrays[level]);

    SeverLandBridges(pRegionArrays[level]);

    if (Globals->LAKES) RemoveCoastalLakes(pRegionArrays[level]);
    
    if (Globals->GROW_RACES) GrowRaces(pRegionArrays[level]);

    FinalSetup(pRegionArrays[level]);
}

void ARegionList::CreateIslandLevel(unsigned int level, unsigned int nPlayers, char const *name)
{
    unsigned int xSize, ySize;
    xSize = 20 + (nPlayers + 3) / 4 * 6 - 2;
    ySize = xSize;

    MakeRegions(level, xSize, ySize);

    pRegionArrays[level]->SetName(name);
    pRegionArrays[level]->levelType = ARegionArray::LEVEL_SURFACE;

    MakeCentralLand(pRegionArrays[level]);
    MakeIslands(pRegionArrays[level], nPlayers);
    RandomTerrain(pRegionArrays[level]);

    if (Globals->GROW_RACES) GrowRaces(pRegionArrays[level]);

    FinalSetup(pRegionArrays[level]);
}

void ARegionList::CreateUnderworldLevel(unsigned int level, unsigned int xSize, unsigned int ySize,
        char const *name)
{
    if (Globals->ICOSAHEDRAL_WORLD) {
        MakeIcosahedralRegions(level, xSize, ySize);
    } else {
        MakeRegions(level, xSize, ySize);
    }

    pRegionArrays[level]->SetName(name);
    pRegionArrays[level]->levelType = ARegionArray::LEVEL_UNDERWORLD;

    SetRegTypes(pRegionArrays[level], Regions::Types::R_NUM);

    SetupAnchors(pRegionArrays[level]);

    GrowTerrain(pRegionArrays[level], true);

    AssignTypes(pRegionArrays[level]);

    MakeUWMaze(pRegionArrays[level]);

    if (Globals->GROW_RACES) GrowRaces(pRegionArrays[level]);

    FinalSetup(pRegionArrays[level]);
}

void ARegionList::CreateUnderdeepLevel(unsigned int level, unsigned int xSize, unsigned int ySize,
        char const *name)
{
    if (Globals->ICOSAHEDRAL_WORLD) {
        MakeIcosahedralRegions(level, xSize, ySize);
    } else {
        MakeRegions(level, xSize, ySize);
    }

    pRegionArrays[level]->SetName(name);
    pRegionArrays[level]->levelType = ARegionArray::LEVEL_UNDERDEEP;

    SetRegTypes(pRegionArrays[level], Regions::Types::R_NUM);

    SetupAnchors(pRegionArrays[level]);

    GrowTerrain(pRegionArrays[level], true);

    AssignTypes(pRegionArrays[level]);

    MakeUWMaze(pRegionArrays[level]);

    if (Globals->GROW_RACES) GrowRaces(pRegionArrays[level]);

    FinalSetup(pRegionArrays[level]);
}

void ARegionList::MakeRegions(unsigned int level, unsigned int xSize, unsigned int ySize)
{
    Awrite("Making a level...");

    ARegionArray::Handle arr = std::make_shared<ARegionArray>(xSize, ySize);
    pRegionArrays[level] = arr;

    //
    // Make the regions themselves
    //
    for (unsigned int y = 0; y < ySize; y++) {
        for (unsigned int x = 0; x < xSize; x++) {
            if (!((x + y) % 2)) {
                auto& reg = regions_.emplace_back(std::make_shared<ARegion>());
                reg->SetLoc(x, y, level);
                reg->num = size() - 1;

                //
                // Some initial values; these will get reset
                //
                reg->type.invalidate();
                reg->race.invalidate();
                reg->wages = -1;

                arr->SetRegion(x, y, reg);
            }
        }
    }

    SetupNeighbors(arr);

    Awrite("");
}

void ARegionList::SetupNeighbors(const ARegionArray::Handle& pRegs)
{
    for (unsigned int x = 0; x < pRegs->x; x++) {
        for (unsigned int y = 0; y < pRegs->y; y++) {
            ARegion::WeakHandle reg = pRegs->GetRegion(x, y);
            if (reg.expired()) continue;
            NeighSetup(reg.lock(), pRegs);
        }
    }
}

void ARegionList::MakeIcosahedralRegions(unsigned int level, unsigned int xSize, unsigned int ySize)
{
    unsigned int scale, x2, y2;

    Awrite("Making an icosahedral level...");

    scale = xSize / 10;
    if (scale < 1) {
        Awrite("Can't create an icosahedral level with xSize < 10!");
        return;
    }
    if (ySize < scale * 10) {
        Awrite("ySize must be at least xSize!");
        return;
    }

    // Create the arrays as the specified size, as some code demands that
    // the RegionArray be multiples of 8 in each direction
    ARegionArray::Handle arr = std::make_shared<ARegionArray>(xSize, ySize);
    pRegionArrays[level] = arr;

    // but we'll only use up to multiples of 10, as that is required
    // by the geometry of the resulting icosahedron.  The best choice
    // would be to satisfy both criteria by choosing a multiple of 40,
    // of course (remember that sublevels are halved in size though)!
    xSize = scale * 10;
    ySize = xSize;

    //
    // Make the regions themselves
    //
    for (unsigned int y = 0; y < ySize; y++) {
        for (unsigned int x = 0; x < xSize; x++) {
            if (!((x + y) % 2)) {
                // These cases remove all the hexes that are cut out to
                // make the world join up into a big icosahedron (d20).
                if (y < 2) {
                    if (x)
                        continue;
                }
                else if (y <= 3 * scale) {
                    x2 = x % (2 * scale);
                    if (y < 3 * x2 && y <= 3 * (2 * scale - x2))
                        continue;
                }
                else if (y < 7 * scale - 1) {
                    // Include all of this band
                }
                else if (y < 10 * scale - 2) {
                    x2 = (x + 2 * scale + 1) % (2 * scale);
                    y2 = 10 * scale - 1 - y;
                    if (y2 < 3 * x2 && y2 <= 3 * (2 * scale - x2))
                        continue;
                }
                else {
                    if (x != 10 * scale - 1)
                        continue;
                }

                auto& reg = regions_.emplace_back(std::make_shared<ARegion>());
                reg->SetLoc(x, y, level);
                reg->num = size() - 1;

                //
                // Some initial values; these will get reset
                //
                reg->type.invalidate();
                reg->race.invalidate(); // 
                reg->wages = -1; // initially store: name
                reg->population = -1; // initially used as flag
                reg->elevation = -1;

                arr->SetRegion(x, y, reg);
            }
        }
    }

    SetupIcosahedralNeighbors(arr);

    Awrite("");
}

void ARegionList::SetupIcosahedralNeighbors(const ARegionArray::Handle& pRegs)
{
    for (unsigned int x = 0; x < pRegs->x; x++) {
        for (unsigned int y = 0; y < pRegs->y; y++) {
            ARegion::WeakHandle reg = pRegs->GetRegion(x, y);
            if (reg.expired()) continue;
            IcosahedralNeighSetup(reg.lock(), pRegs);
        }
    }
}

void ARegionList::MakeLand(const ARegionArray::Handle& pRegs, unsigned int percentOcean, unsigned int continentSize)
{
    static constexpr size_t NDIRS = ALL_DIRECTIONS.size();
    unsigned int total, ocean;

    total = 0;
    for (unsigned int x=0; x < pRegs->x; x++)
    {
        for (unsigned int y=0; y < pRegs->y; y++)
        {
            if (!pRegs->GetRegion(x, y).expired())
            {
                total++;
            }
        }
    }
    ocean = total;

    Awrite("Making land");
    while (ocean > (total * percentOcean) / 100) {
        unsigned int sz = getrandom(continentSize);
        sz = sz * sz;

        unsigned int tempx = getrandom(pRegs->x);
        unsigned int yoff = pRegs->y / 40;
        unsigned int yband = pRegs->y / 2 - 2 * yoff;
        unsigned int tempy = (getrandom(yband)+yoff) * 2 + tempx % 2;

        ARegion::WeakHandle reg_w = pRegs->GetRegion(tempx, tempy);
        if (reg_w.expired()) continue;
        ARegion::WeakHandle newreg = reg_w;
        ARegion::WeakHandle seareg = reg_w;

        auto reg = reg_w.lock();
        // Archipelago or Continent?
        if (getrandom(100) < Globals->ARCHIPELAGO) {
            // Make an Archipelago:
            sz = sz / 5 + 1;
            bool first = true;
            unsigned int tries = 0;
            for (unsigned int i=0; i < sz; i++) {
                size_t direc = getrandom(NDIRS);
                newreg = reg->neighbors[direc];
                while (newreg.expired()) {
                    direc = getrandom(NDIRS);
                    newreg = reg->neighbors[direc];
                }
                tries++;
                for (unsigned int m = 0; m < 2; m++) {
                    seareg = newreg;
                    newreg = seareg.lock()->neighbors[direc];
                    if (newreg.expired())
                    {
                        break;
                    }
                }
                if (newreg.expired())
                {
                    break;
                }

                seareg = newreg;
                newreg = seareg.lock()->neighbors[getrandom(NDIRS)];
                if (newreg.expired()) break;
                // island start point (~3 regions away from last island)
                seareg = newreg;
                if (first) {
                    seareg = reg;
                    first = false;
                }
                if (!seareg.lock()->type.isValid()) {
                    reg = seareg.lock();
                    tries = 0;
                    reg->type = Regions::Types::R_NUM;
                    ocean--;
                } else {
                    if (tries > 5)
                    {
                        break;
                    }
                    continue;
                }
                int growit = getrandom(20);
                int growth = 0;
                int growch = 2;
                const Directions direc_dir = static_cast<Directions>(direc);
                // grow this island
                while (growit > growch) {
                    growit = getrandom(20);
                    tries = 0;
                    size_t newdir = getrandom(NDIRS);
                    while (direc_dir == reg->GetRealDirComp(static_cast<Directions>(newdir)))
                    {
                        newdir = getrandom(NDIRS);
                    }
                    newreg = reg->neighbors[newdir];
                    while (newreg.expired() && (tries < 36)) {
                        while (direc_dir == reg->GetRealDirComp(static_cast<Directions>(newdir)))
                        {
                            newdir = getrandom(NDIRS);
                        }
                        newreg = reg->neighbors[newdir];
                        tries++;
                    }
                    if (newreg.expired()) continue;
                    reg = newreg.lock();
                    tries = 0;
                    if (!reg->type.isValid()) {
                        reg->type = Regions::Types::R_NUM;
                        growth++;
                        if (growth > growch) growch = growth;
                        ocean--;
                    } else continue;
                }
            }
        } else {
            // make a continent
            if (!reg->type.isValid()) {
                reg->type = Regions::Types::R_NUM;
                ocean--;
            }
            for (unsigned int i=0; i<sz; i++) {
                size_t dir = getrandom(NDIRS);
                if ((reg->yloc < yoff*2) && ((dir < 2) || (dir == NDIRS-1))
                    && (getrandom(4) < 3))
                {
                    continue;
                }
                if ((reg->yloc > (yband+yoff)*2) && ((dir < 5) && (dir > 1))
                    && (getrandom(4) < 3))
                {
                    continue;
                }
                ARegion::WeakHandle newreg_w = reg->neighbors[dir];
                if (newreg_w.expired())
                {
                    break;
                }
                const auto newreg = newreg_w.lock();
                bool polecheck = false;
                for (const auto& creg: newreg->neighbors) {
                    if (creg.expired())
                    {
                        polecheck = true;
                    }
                }
                if (polecheck)
                {
                    break;
                }
                reg = newreg;
                if (!reg->type.isValid()) {
                    reg->type = Regions::Types::R_NUM;
                    ocean--;
                }
            }
        }
    }

    // At this point, go back through and set all the rest to ocean
    SetRegTypes(pRegs, Regions::Types::R_OCEAN);
    Awrite("");
}

void ARegionList::MakeCentralLand(const ARegionArray::Handle& pRegs)
{
    for (unsigned int i = 0; i < pRegs->x; i++) {
        for (unsigned int j = 0; j < pRegs->y; j++) {
            ARegion::WeakHandle reg_w = pRegs->GetRegion(i, j);
            if (reg_w.expired()) continue;
            const auto reg = reg_w.lock();
            // Initialize region to ocean.
            reg->type = Regions::Types::R_OCEAN;
            // If the region is close to the edges, it stays ocean
            if (i < 8 || i >= pRegs->x - 8 || j < 8 || j >= pRegs->y - 8)
                continue;
            // If the region is within 10 of the edges, it has a 50%
            // chance of staying ocean.
            if (i < 10 || i >= pRegs->x - 10 || j < 10 || j >= pRegs->y - 10) {
                if (getrandom(100) > 50) continue;
            }

            // Otherwise, set the region to land.
            reg->type = Regions::Types::R_NUM;
        }
    }
}

void ARegionList::MakeIslands(const ARegionArray::Handle& pArr, unsigned int nPlayers)
{
    // First, make the islands along the top.
    unsigned int nRow = (nPlayers + 3) / 4;
    for (unsigned int i = 0; i < nRow; i++)
    {
        MakeOneIsland(pArr, 10 + i * 6, 2);
    }
    // Next, along the left.
    nRow = (nPlayers + 2) / 4;
    for (unsigned int i = 0; i < nRow; i++)
    {
        MakeOneIsland(pArr, 2, 10 + i * 6);
    }
    // The islands along the bottom.
    nRow = (nPlayers + 1) / 4;
    for (unsigned int i = 0; i < nRow; i++)
    {
        MakeOneIsland(pArr, 10 + i * 6, pArr->y - 6);
    }
    // And the islands on the right.
    nRow = nPlayers / 4;
    for (unsigned int i = 0; i < nRow; i++)
    {
        MakeOneIsland(pArr, pArr->x - 6, 10 + i * 6);
    }
}

void ARegionList::MakeOneIsland(const ARegionArray::Handle& pRegs, unsigned int xx, unsigned int yy)
{
    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            ARegion::WeakHandle pReg = pRegs->GetRegion(i + xx, j + yy);
            if (pReg.expired()) continue;
            pReg.lock()->type = Regions::Types::R_NUM;
        }
    }
}

void ARegionList::CleanUpWater(const ARegionArray::Handle& pRegs)
{
    Awrite("Converting Scattered Water");
    unsigned int dotter = 0;
    for (size_t ctr = 0; ctr < Globals->SEA_LIMIT+1; ctr++) {
        for (unsigned int i = 0; i < pRegs->x; i++) {
            for (unsigned int j = 0; j < pRegs->y; j++) {
                ARegion::WeakHandle reg_w = pRegs->GetRegion(i, j);
                int remainocean = 0;
                if (reg_w.expired())
                {
                    continue;
                }
                const auto reg = reg_w.lock();
                if(reg->type != Regions::Types::R_OCEAN)
                {
                    continue;
                }
                for (const auto& d: ALL_DIRECTIONS) {
                    remainocean += reg->CheckSea(d, Globals->SEA_LIMIT, remainocean);
                }
                if (dotter++ % 2000 == 0) Adot();
                if (remainocean > 0) continue;
                reg->wages = 0;
                if (getrandom(100) < Globals->LAKES) {
                        reg->type = Regions::Types::R_LAKE;
                } else reg->type = Regions::Types::R_NUM;
            }
        }
    }
    Awrite("");
}

void ARegionList::RemoveCoastalLakes(const ARegionArray::Handle& pRegs)
{
    Awrite("Removing coastal 'lakes'");
    for (unsigned int c = 0; c < 2; c++) {
        for (unsigned int i = 0; i < pRegs->x; i++) {
            for (unsigned int j = 0; j < pRegs->y; j++) {
                ARegion::WeakHandle reg_w = pRegs->GetRegion(i, j);
                if(reg_w.expired())
                {
                    continue;
                }
                const auto reg = reg_w.lock();
                if (reg->type != Regions::Types::R_LAKE)
                {
                    continue;
                }
                if (reg->IsCoastal() > 0) {
                    reg->type = Regions::Types::R_OCEAN;
                    reg->wages = -1;
                    Adot();
                } else if (reg->wages <= 0) { // name the Lake
                    int wage1 = 0;
                    int count1 = 0;
                    int wage2 = 0;
                    int count2 = 0;
                    int temp = 0;
                    for (unsigned int d = 0; d < ALL_DIRECTIONS.size(); d++) {
                        ARegion::WeakHandle newregion_w = reg->neighbors[d];
                        if (newregion_w.expired())
                        {
                            continue;
                        }
                        const auto newregion = newregion_w.lock();
                        // name after neighboring lake regions preferrentially
                        if ((newregion->wages > 0) &&
                                (newregion->type == Regions::Types::R_LAKE)) {
                            count1 = 1;
                            wage1 = newregion->wages;
                            break;
                        }
                        if ((TerrainDefs[newregion->type].similar_type !=
                                    Regions::Types::R_OCEAN) && (newregion->wages > -1)) {
                            if (newregion->wages == wage1) count1++;
                            else if (newregion->wages == wage2) count2++;
                            else if (count2 == 0) {
                                wage2 = newregion->wages;
                                count2++;
                            }
                            if (count2 > count1) {
                                temp = wage1;
                                wage1 = wage2;
                                wage2 = temp;
                                int tmpin = count1;
                                count1 = count2;
                                count2 = tmpin;
                            }
                        }
                    }
                    if (count1 > 0) reg->wages = wage1;
                }
            }
        }
    }
    Awrite("");
}

void ARegionList::SeverLandBridges(const ARegionArray::Handle& pRegs)
{
    Awrite("Severing land bridges");
    // mark land hexes to delete
    for (unsigned int i = 0; i < pRegs->x; i++) {
        for (unsigned int j = 0; j < pRegs->y; j++) {
            ARegion::WeakHandle reg_w = pRegs->GetRegion(i, j);
            if(reg_w.expired())
            {
                continue;
            }
            const auto reg = reg_w.lock();
            if (TerrainDefs[reg->type].similar_type == Regions::Types::R_OCEAN)
            {
                continue;
            }
            if (reg->IsCoastal() != 4)
            {
                continue;
            }
            unsigned int tidych = Globals->SEVER_LAND_BRIDGES;
            for (unsigned int d = 0; d < ALL_DIRECTIONS.size(); d++) {
                ARegion::WeakHandle newregion_w = reg->neighbors[d];
                if(newregion_w.expired())
                {
                    continue;
                }
                const auto newregion = newregion_w.lock();
                if (TerrainDefs[newregion->type].similar_type == Regions::Types::R_OCEAN)
                {
                    continue;
                }
                if (newregion->IsCoastal() == 4)
                {
                    tidych = tidych * 2;
                }
            }
            if (getrandom(100U) < tidych)
            {
                reg->wages = -2;
            }
        }
    }
    // now change to ocean
    for (unsigned int i = 0; i < pRegs->x; i++) {
        for (unsigned int j = 0; j < pRegs->y; j++) {
            ARegion::WeakHandle reg_w = pRegs->GetRegion(i, j);
            if(reg_w.expired())
            {
                continue;
            }
            const auto reg = reg_w.lock();
            if (reg->wages > -2)
            {
                continue;
            }
            reg->type = Regions::Types::R_OCEAN;
            reg->wages = -1;
            Adot();
        }
    }
    Awrite("");
}

void ARegionList::SetRegTypes(const ARegionArray::Handle& pRegs, const Regions& newType)
{
    for (unsigned int i = 0; i < pRegs->x; i++) {
        for (unsigned int j = 0; j < pRegs->y; j++) {
            ARegion::WeakHandle reg_w = pRegs->GetRegion(i, j);
            if(reg_w.expired())
            {
                continue;
            }
            const auto reg = reg_w.lock();
            if (!reg->type.isValid())
            {
                reg->type = newType;
            }
        }
    }
}

void ARegionList::SetupAnchors(const ARegionArray::Handle& ta)
{
    // Now, setup the anchors
    Awrite("Setting up the anchors");
    int skip = 250;
    unsigned int f = 2;
    if (Globals->TERRAIN_GRANULARITY) {
        skip = Globals->TERRAIN_GRANULARITY;
        while (skip > 5) {
            f++;
            skip -= 5;
        }
        skip = 100 * ((skip+3) * static_cast<int>(f) + 2) / (skip + static_cast<int>(f) - 2);
    }
    unsigned int dotter = 0;
    for (unsigned int x=0; x<(ta->x)/f; x++) {
        for (unsigned int y=0; y<(ta->y)/(f*2); y++) {
            if (getrandom(1000) > skip) continue;
            for (unsigned int i=0; i<4; i++) {
                unsigned int tempx = x * f + getrandom(f);
                unsigned int tempy = y * f * 2 + getrandom(f)*2 + tempx%2;
                ARegion::WeakHandle reg_w = ta->GetRegion(tempx, tempy);
                if (reg_w.expired())
                {
                    continue;
                }
                const auto reg = reg_w.lock();
                if (reg->type == Regions::Types::R_NUM) {
                    reg->type = GetRegType(reg);
                    reg->population = 1;
                    if (TerrainDefs[reg->type].similar_type != Regions::Types::R_OCEAN)
                    {
                        reg->wages = AGetName(0, reg);
                    }
                    break;
                }
            }
            if (dotter++%30 == 0) Adot();
        }
    }

    Awrite("");
}

void ARegionList::GrowTerrain(const ARegionArray::Handle& pArr, bool growOcean)
{
    Awrite("Growing Terrain...");
    for (unsigned int j=0; j<30; j++) {
        for (unsigned int x = 0; x < pArr->x; x++) {
            for (unsigned int y = 0; y < pArr->y; y++) {
                ARegion::WeakHandle reg = pArr->GetRegion(x, y);
                if (reg.expired())
                {
                    continue;
                }
                reg.lock()->population = 1;
            }
        }
        for (unsigned int x = 0; x < pArr->x; x++) {
            for (unsigned int y = 0; y < pArr->y; y++) {
                ARegion::WeakHandle reg_w = pArr->GetRegion(x, y);
                if (reg_w.expired())
                {
                    continue;
                }
                const auto reg = reg_w.lock();
                if ((j > 0) && (j < 21) && (getrandom(3) < 2)) continue;
                if (reg->type == Regions::Types::R_NUM) {
                
                    // Check for Lakes
                    if (Globals->LAKES &&
                        (getrandom(100) < ((Globals->LAKES + 10)/10))) {
                            reg->type = Regions::Types::R_LAKE;
                            break;
                    }
                    // Check for Odd Terrain
                    if (getrandom(1000) < Globals->ODD_TERRAIN) {
                        reg->type = GetRegType(reg);
                        if (TerrainDefs[reg->type].similar_type != Regions::Types::R_OCEAN)
                            reg->wages = AGetName(0, reg);
                        break;
                    }
                    

                    size_t init = getrandom(6UL);

                    const size_t n_size = reg->neighbors.size();
                    for (size_t i = 0; i < n_size; i++) {
                        ARegion::WeakHandle t_w = reg->neighbors[(i+init) % n_size];
                        if (!t_w.expired()) {
                            const auto t = t_w.lock();
                            if (t->population < 1)
                            {
                                continue;
                            }
                            if (t->type != Regions::Types::R_NUM &&
                                (TerrainDefs[t->type].similar_type!=Regions::Types::R_OCEAN ||
                                 (growOcean && (t->type != Regions::Types::R_LAKE)))) {
                                if (j == 0)
                                {
                                    t->population = 0;
                                }
                                reg->population = 0;
                                reg->race = static_cast<Items>(t->type);
                                reg->wages = t->wages;
                                break;
                            }
                        }
                    }
                }
            }
        }

        for (unsigned int x = 0; x < pArr->x; x++) {
            for (unsigned int y = 0; y < pArr->y; y++) {
                ARegion::WeakHandle reg_w = pArr->GetRegion(x, y);
                if (reg_w.expired())
                {
                    continue;
                }
                const auto reg = reg_w.lock();
                if (reg->type == Regions::Types::R_NUM && reg->race.isValid())
                {
                    reg->type = static_cast<Regions>(reg->race);
                }
            }
        }
    }
}

void ARegionList::RandomTerrain(const ARegionArray::Handle& pArr)
{
    for (unsigned int x = 0; x < pArr->x; x++) {
        for (unsigned int y = 0; y < pArr->y; y++) {
            ARegion::WeakHandle reg_w = pArr->GetRegion(x, y);
            if (reg_w.expired())
            {
                continue;
            }

            const auto reg = reg_w.lock();
            if (reg->type == Regions::Types::R_NUM) {
                Regions adjtype;
                int adjname = -1;
                for (size_t d = 0; d < ALL_DIRECTIONS.size(); d++) {
                    ARegion::WeakHandle newregion_w = reg->neighbors[d];
                    if (newregion_w.expired())
                    {
                        continue;
                    }
                    const auto newregion = newregion_w.lock();
                    if ((TerrainDefs[newregion->type].similar_type !=
                                Regions::Types::R_OCEAN) && (newregion->type != Regions::Types::R_NUM) &&
                            (newregion->wages > 0)) {
                        adjtype = newregion->type;
                        adjname = newregion->wages;
                    }
                }
                if (adjtype.isValid() && !Globals->CONQUEST_GAME) {
                    reg->type = adjtype;
                    reg->wages = adjname;
                } else {
                    reg->type = GetRegType(reg);
                    reg->wages = AGetName(0, reg);
                }
            }
        }
    }
}

void ARegionList::MakeUWMaze(const ARegionArray::Handle& pArr)
{
    for (unsigned int x = 0; x < pArr->x; x++) {
        for (unsigned int y = 0; y < pArr->y; y++) {
            ARegion::WeakHandle reg_w = pArr->GetRegion(x, y);
            if (reg_w.expired())
            {
                continue;
            }

            const auto reg = reg_w.lock();
            for (const auto i: ALL_DIRECTIONS) {
                unsigned int count = 0;
                for (const auto& nn: reg->neighbors)
                {
                    if (!nn.expired())
                    {
                        count++;
                    }
                }
                if (count <= 1)
                {
                    break;
                }

                ARegion::WeakHandle n_w = reg->neighbors[static_cast<size_t>(i)];
                if (!n_w.expired()) {
                    const auto n = n_w.lock();
                    if (n->xloc < x || (n->xloc == x && n->yloc < y))
                    {
                        continue;
                    }
                    if (!CheckRegionExit(reg, n)) {
                        count = 0;
                        for (const auto& nn: n->neighbors) {
                            if (!nn.expired())
                            {
                                count++;
                            }
                        }
                        if (count <= 1)
                        {
                            break;
                        }
                        n->neighbors[static_cast<size_t>(reg->GetRealDirComp(i))].reset();
                        reg->neighbors[static_cast<size_t>(i)].reset();
                    }
                }
            }
        }
    }
}

void ARegionList::AssignTypes(const ARegionArray::Handle& pArr)
{
    // RandomTerrain() will set all of the un-set region types and names.
    RandomTerrain(pArr);
}

void ARegionList::UnsetRace(const ARegionArray::Handle& pArr)
{
    for (unsigned int x = 0; x < pArr->x; x++) {
        for (unsigned int y = 0; y < pArr->y; y++) {
            ARegion::WeakHandle reg = pArr->GetRegion(x, y);
            if (reg.expired())
            {
                continue;
            }
            reg.lock()->race.invalidate();
        }
    }
}

void ARegionList::RaceAnchors(const ARegionArray::Handle& pArr)
{
    Awrite("Setting Race Anchors");
    UnsetRace(pArr);
    unsigned int wigout = 0;
    for (unsigned int x = 0; x < pArr->x; x++) {
        for (unsigned int y = 0; y < pArr->y; y++) {
            // Anchor distribution: depends on GROW_RACES value
            unsigned int jiggle = 4 + 2 * Globals->GROW_RACES;
            if ((y + ((x % 2) * jiggle/2)) % jiggle > 1)
            {
                continue;
            }
            unsigned int xoff = x + 2 - getrandom(3U) - getrandom(3U);
            ARegion::WeakHandle reg_w = pArr->GetRegion(xoff, y);
            if (reg_w.expired())
            {
                continue;
            }

            const auto reg = reg_w.lock();
            if ((reg->type == Regions::Types::R_LAKE) && (!Globals->LAKESIDE_IS_COASTAL))
            {
                continue;
            }
            if (TerrainDefs[reg->type].flags & TerrainType::BARREN)
            {
                continue;
            }

            reg->race.invalidate();
            wigout = 0; // reset sanity
            
            if (TerrainDefs[reg->type].similar_type == Regions::Types::R_OCEAN) {
                // setup near coastal race here
                const size_t n_size = reg->neighbors.size();
                size_t d = getrandom(n_size);
                unsigned int ctr = 0;
                ARegion::WeakHandle nreg_w = reg->neighbors[d];
                if (nreg_w.expired())
                {
                    continue;
                }
                auto nreg = nreg_w.lock();
                while((ctr++ < 20) && (!reg->race.isValid())) {
                    if (TerrainDefs[nreg->type].similar_type != Regions::Types::R_OCEAN) {
                        const size_t rnum = TerrainDefs[nreg->type].coastal_races.size();

                        while ( !reg->race.isValid() || 
                                (ItemDefs[reg->race].flags & ItemType::DISABLED)) {
                            reg->race = 
                                TerrainDefs[nreg->type].coastal_races[getrandom(rnum)];
                            if (++wigout > 100) break;
                        }
                    } else {
                        size_t dir = getrandom(n_size);
                        if (static_cast<Directions>(d) == nreg->GetRealDirComp(static_cast<Directions>(dir)))
                        {
                            continue;
                        }
                        if(nreg->neighbors[dir].expired())
                        {
                            continue;
                        }
                        nreg = nreg->neighbors[dir].lock();
                    }
                }
            } else {
                // setup noncoastal race here
                const size_t rnum = TerrainDefs[reg->type].races.size();

                while ( !reg->race.isValid() || 
                        (ItemDefs[reg->race].flags & ItemType::DISABLED)) {
                    reg->race = TerrainDefs[reg->type].races[getrandom(rnum)];
                    if (++wigout > 100)
                    {
                        break;
                    }
                }
            }
            
            /* leave out this sort of check for the moment
            if (wigout > 100) {
                // do something!
                Awrite("There is a problem with the races in the ");
                Awrite(TerrainDefs[reg->type].name);
                Awrite(" region type");
            }
            */
            
            if (!reg->race.isValid()) {
                std::cout << "Hey! No race anchor got assigned to the " 
                          << TerrainDefs[reg->type].name 
                          << " at " << x << "," << y << std::endl;
            }
        }
    }
}

void ARegionList::GrowRaces(const ARegionArray::Handle& pArr)
{
    Awrite("Growing Races");
    RaceAnchors(pArr);
    for (unsigned int a = 0; a < 25; a++) {
        for (unsigned int x = 0; x < pArr->x; x++) {
            for (unsigned int y = 0; y < pArr->y; y++) {
                ARegion::WeakHandle reg_w = pArr->GetRegion(x, y);
                if(reg_w.expired())
                {
                    continue;
                }
                const auto reg = reg_w.lock();
                if (!reg->race.isValid())
                {
                    continue;
                }

                for (const auto& nreg_w: reg->neighbors) {
                    if (nreg_w.expired())
                    {
                        continue;
                    }
                    const auto nreg = nreg_w.lock();
                    if(nreg->race.isValid())
                    {
                        continue;
                    }
                    bool iscoastal = false;
                    for (const auto& crace: TerrainDefs[reg->type].coastal_races) {
                        if (reg->race.equals(crace))
                        {
                            iscoastal = true;
                            break;
                        }
                    }
                    // Only coastal races may pass from sea to land
                    if ((TerrainDefs[nreg->type].similar_type == Regions::Types::R_OCEAN) && !iscoastal)
                    {
                        continue;
                    }

                    size_t ch = getrandom(5UL);
                    if (iscoastal) {
                        if (TerrainDefs[nreg->type].similar_type == Regions::Types::R_OCEAN)
                            ch += 2;
                    } else {
                        ManType *mt = FindRace(ItemDefs[reg->race].abr);
                        if (mt->terrain.equals(TerrainDefs[nreg->type].similar_type))
                        {
                            ch += 2;
                        }
                        for (const auto& race: TerrainDefs[nreg->type].races) {
                            if (race.equals(reg->race))
                            {
                                ch++;
                            }
                        }
                    }
                    if (ch > 3)
                    {
                        nreg->race = reg->race;
                    }
                }
            }
        }
    }
}

void ARegionList::FinalSetup(const ARegionArray::Handle& pArr)
{
    for (unsigned int x = 0; x < pArr->x; x++) {
        for (unsigned int y = 0; y < pArr->y; y++) {
            ARegion::WeakHandle reg_w = pArr->GetRegion(x, y);
            if (reg_w.expired())
            {
                continue;
            }

            const auto reg = reg_w.lock();

            if ((TerrainDefs[reg->type].similar_type == Regions::Types::R_OCEAN) && (reg->type != Regions::Types::R_LAKE))
            {
                if (pArr->levelType == ARegionArray::LEVEL_UNDERWORLD)
                {
                    reg->SetName("The Undersea");
                }
                else if (pArr->levelType == ARegionArray::LEVEL_UNDERDEEP)
                {
                    reg->SetName("The Deep Undersea");
                }
                else
                {
                    AString ocean_name = Globals->WORLD_NAME;
                    ocean_name += " Ocean";
                    reg->SetName(ocean_name.Str());
                }
            }
            else
            {
                if (reg->wages == -1)
                {
                    reg->SetName("Unnamed");
                }
                else if (reg->wages != -2)
                {
                    reg->SetName(AGetNameString(reg->wages));
                }
                else
                {
                    reg->wages = -1;
                }
            }

            reg->Setup();
        }
    }
}

void ARegionList::MakeShaft(const ARegion::Handle& reg, const ARegionArray::Handle& pFrom, const ARegionArray::Handle& pTo)
{
    if (TerrainDefs[reg->type].similar_type == Regions::Types::R_OCEAN)
    {
        return;
    }

    unsigned int tempx = ((reg->xloc * pTo->x) / pFrom->x) + getrandom(pTo->x / pFrom->x);
    unsigned int tempy = ((reg->yloc * pTo->y) / pFrom->y) + getrandom(pTo->y / pFrom->y);
    //
    // Make sure we get a valid region.
    //
    tempy += (tempx + tempy) % 2;

    ARegion::WeakHandle temp_w = pTo->GetRegion(tempx, tempy);
    if (temp_w.expired())
    {
        return;
    }
    const auto temp = temp_w.lock();
    if (TerrainDefs[temp->type].similar_type == Regions::Types::R_OCEAN)
    {
        return;
    }

    {
        auto& o = reg->objects.emplace_back(std::make_shared<Object>(reg));
        o->num = reg->buildingseq++;
        o->name = new AString(AString("Shaft [") + o->num + "]");
        o->type = Objects::Types::O_SHAFT;
        o->incomplete = 0;
        o->inner = static_cast<ssize_t>(temp->num);
    }
    {
        auto& o = temp->objects.emplace_back(std::make_shared<Object>(reg));
        o->num = temp->buildingseq++;
        o->name = new AString(AString("Shaft [") + o->num + "]");
        o->type = Objects::Types::O_SHAFT;
        o->incomplete = 0;
        o->inner = static_cast<ssize_t>(reg->num);
    }
}

void ARegionList::MakeShaftLinks(unsigned int levelFrom, unsigned int levelTo, unsigned int odds)
{
    const auto& pFrom = pRegionArrays[levelFrom];
    const auto& pTo = pRegionArrays[levelTo];

    for (unsigned int x = 0; x < pFrom->x; x++) {
        for (unsigned int y = 0; y < pFrom->y; y++) {
            ARegion::WeakHandle reg = pFrom->GetRegion(x, y);
            if (reg.expired())
            {
                continue;
            }

            if (getrandom(odds) != 0)
            {
                continue;
            }

            MakeShaft(reg.lock(), pFrom, pTo);
        }
    }
}

void ARegionList::SetACNeighbors(unsigned int levelSrc, unsigned int levelTo, unsigned int maxX, unsigned int maxY)
{
    const auto& ar = GetRegionArray(levelSrc);

    for (unsigned int x = 0; x < ar->x; x++) {
        for (unsigned int y = 0; y < ar->y; y++) {
            ARegion::WeakHandle AC_w = ar->GetRegion(x, y);
            if (AC_w.expired())
            {
                continue;
            }
            const auto AC = AC_w.lock();
            if (Globals->START_CITIES_EXIST) {
                const size_t n_size = AC->neighbors.size();
                for (size_t i = 0; i < n_size; i++) {
                    if(!AC->neighbors[i].expired())
                    {
                        continue;
                    }
                    ARegion::WeakHandle pReg = GetStartingCity(*AC, i, levelTo, maxX, maxY);
                    if (pReg.expired())
                    {
                        continue;
                    }
                    AC->neighbors[i] = pReg;
                    pReg.lock()->MakeStartingCity();
                    if (Globals->GATES_EXIST) {
                        numberofgates++;
                    }
                }
            }
            else
            {
                // If we don't have starting cities, then put portals
                // from the nexus to a variety of terrain types.
                // These will transport the user to a randomly
                // selected region of the chosen terrain type.
                const auto& to = GetRegionArray(levelTo);
                for (size_t type = static_cast<size_t>(Regions::Types::R_PLAIN); type <= static_cast<size_t>(Regions::Types::R_TUNDRA); ++type) {
                    bool found = false;
                    for (unsigned int x2 = 0; !found && x2 < maxX; x2++)
                        for (unsigned int y2 = 0; !found && y2 < maxY; y2++) {
                            ARegion::WeakHandle reg_w = to->GetRegion(x2, y2);
                            if (reg_w.expired())
                            {
                                continue;
                            }
                            const auto reg = reg_w.lock();
                            if (reg->type == type) {
                                found = true;
                                auto& o = AC->objects.emplace_back(std::make_shared<Object>(AC));
                                o->num = AC->buildingseq++;
                                o->name = new AString(AString("Gateway to ") +
                                    TerrainDefs[type].name + " [" + o->num + "]");
                                o->type = Objects::Types::O_GATEWAY;
                                o->incomplete = 0;
                                o->inner = static_cast<ssize_t>(reg->num);
                            }
                        }
                }
            }
        }
    }
}

void ARegionList::InitSetupGates(unsigned int level)
{
    if (!Globals->GATES_EXIST) return;

    const auto& pArr = pRegionArrays[level];

    for (unsigned int i=0; i<pArr->x / 8; i++) {
        for (unsigned int j=0; j<pArr->y / 16; j++) {
            for (unsigned int k=0; k<5; k++) {
                unsigned int tempx = i*8 + getrandom(8U);
                unsigned int tempy = j*16 + getrandom(8U)*2 + tempx%2;
                ARegion::WeakHandle temp_w = pArr->GetRegion(tempx, tempy);
                if (temp_w.expired())
                {
                    continue;
                }
                const auto temp = temp_w.lock();
                if (TerrainDefs[temp->type].similar_type != Regions::Types::R_OCEAN && temp->gate != -1) {
                    numberofgates++;
                    temp->gate = -1;
                    break;
                }
            }
        }
    }
}

void ARegionList::FixUnconnectedRegions()
{
}

void ARegionList::FinalSetupGates()
{
    if (!Globals->GATES_EXIST) return;

    unsigned int ngates = numberofgates;

    if (Globals->DISPERSE_GATE_NUMBERS) {
        unsigned int log10 = 0;
        while (ngates > 0) {
            ngates /= 10;
            log10++;
        }
        ngates = 10;
        while (log10 > 0) {
            ngates *= 10;
            log10--;
        }
    }

    std::vector<bool> used(ngates, false);

    for(const auto& r: *this)
    {
        if (r->gate == -1) {
            unsigned int index = getrandom(ngates);
            while (used[index]) {
                if (Globals->DISPERSE_GATE_NUMBERS) {
                    index = getrandom(ngates);
                } else {
                    index++;
                    index = index % ngates;
                }
            }
            r->gate = static_cast<int>(index + 1);
            used[index] = true;
            // setting up gatemonth
            r->gatemonth = getrandom(12);;
        }
    }
}
