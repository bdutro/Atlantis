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

Location::WeakHandle GetUnit(const std::list<Location::Handle>& list, size_t n)
{
    for(const auto& l: list) {
        if (l->unit.lock()->num == n) return l;
    }
    return Location::WeakHandle();
}

ARegion::WeakHandle GetRegion(const std::list<ARegion::WeakHandle>& l, size_t n)
{
    for(const auto& p: l) {
        if (p.lock()->num == n) return p;
    }
    return ARegion::WeakHandle();
}

Farsight::Farsight()
{
    faction.reset();
    unit.reset();
    level = 0;
    observation = 0;
    std::fill(exits_used.begin(), exits_used.end(), false);
}

Farsight::WeakHandle GetFarsight(const std::list<Farsight::Handle>& l, const Faction& fac)
{
    for(const auto& f: l) {
        if (f->faction.lock().get() == &fac) return f;
    }
    return Farsight::WeakHandle();
}

AString TownString(TownTypeEnum i);

AString TownString(TownTypeEnum i)
{
    switch (i) {
        case TownTypeEnum::TOWN_VILLAGE:
            return "village";
        case TownTypeEnum::TOWN_TOWN:
            return "town";
        case TownTypeEnum::TOWN_CITY:
            return "city";
        default:
            return "huh?";
    }
}

TownInfo::TownInfo()
{
    name = 0;
    pop = 0;
    activity = 0;
    hab = 0;
}

TownInfo::~TownInfo()
{
    if (name) delete name;
}

void TownInfo::Readin(Ainfile *f, ATL_VER &)
{
    name = f->GetStr();
    pop = f->GetInt<int>();
    hab = f->GetInt<int>();
}

void TownInfo::Writeout(Aoutfile *f)
{
    f->PutStr(*name);
    f->PutInt(pop);
    f->PutInt(hab);
}

ARegion::ARegion()
{
    name = new AString("Region");
    xloc = 0;
    yloc = 0;
    buildingseq = 1;
    gate = 0;
    gatemonth = 0;
    gateopen = 1;
    town = 0;
    development = 0;
    habitat = 0;
    immigrants = 0;
    emigrants = 0;
    improvement = 0;
    clearskies = 0;
    earthlore = 0;
    ZeroNeighbors();
    visited = 0;
}

ARegion::~ARegion()
{
    if (name) delete name;
    if (town) delete town;
}

void ARegion::ZeroNeighbors()
{
    for(auto& n: neighbors) {
        n.reset();
    }
}

void ARegion::SetName(char const *c)
{
    if (name) delete name;
    name = new AString(c);
}


bool ARegion::IsNativeRace(const Items& item)
{
    const auto& typer = TerrainDefs[type];
    if (IsCoastal()) {
        for (const auto& r: typer.coastal_races) {
            if (item == r)
            {
                return true;
            }
        }
    }
    for (const auto& r: typer.races) {
        if (item == r)
        {
            return true;
        }
    }
    return false;
}

unsigned int ARegion::GetNearestProd(const Items& item)
{
    //AList regs, regs2;
    //AList *rptr = &regs;
    //AList *r2ptr = &regs2;
    //AList *temp;
    //ARegionPtr *p = new ARegionPtr;
    //p->ptr = this;
    //regs.Add(p);
    std::list<ARegion::WeakHandle> regs, regs2;
    regs.push_back(weak_from_this());

    for (unsigned int i=0; i<5; i++) {
        for(const auto& rp: regs) {
            ARegion::Handle r = rp.lock();
            AString skname = ItemDefs[item].pSkill;
            Skills sk = LookupSkill(skname);
            if (!r->products.GetProd(item, sk).expired()) {
                regs.clear();
                regs2.clear();
                return i;
            }
            for (const auto& n: neighbors) {
                if (!n.expired()) {
                    //p = new ARegionPtr;
                    //p->ptr = neighbors[j];
                    //r2ptr->Add(p);
                    regs2.push_back(n);
                }
            }
            //rptr->DeleteAll();
            //temp = rptr;
            //rptr = r2ptr;
            //r2ptr = temp;
            regs.clear();
            std::swap(regs, regs2);
        }
    }
    //regs.DeleteAll();
    //regs2.DeleteAll();
    return 5;
}


void ARegion::LairCheck()
{
    // No lair if town in region
    if (town) return;


    const auto& tt = TerrainDefs[type];

    if (!tt.lairChance) return;

    int check = getrandom(100);
    if (check >= tt.lairChance) return;

    int count = 0;
    for(const auto& l: tt.lairs)
    {
        if (l.isValid()) {
            if (!(ObjectDefs[l].flags & ObjectType::DISABLED)) {
                count++;
            }
        }
    }
    count = getrandom(count);

    Objects lair;
    for(const auto& l: tt.lairs)
    {
        if (l.isValid()) {
            if (!(ObjectDefs[l].flags & ObjectType::DISABLED)) {
                if (!count) {
                    lair = l;
                    break;
                }
                count--;
            }
        }
    }

    if (lair.isValid()) {
        MakeLair(lair);
        return;
    }
}

Object::Handle& ARegion::AddObject()
{
    return objects.emplace_back(std::make_shared<Object>(weak_from_this()));
}

void ARegion::MakeLair(const Objects& t)
{
    Object::Handle& o = AddObject();
    o->num = buildingseq++;
    o->name = new AString(AString(ObjectDefs[t].name) +
            " [" + o->num + "]");
    o->type = t;
    o->incomplete = 0;
    o->inner = -1;
}

unsigned int ARegion::GetPoleDistance(const Directions& dir)
{
    unsigned int ct = 1;
    ARegion::WeakHandle nreg = neighbors[dir];
    while (!nreg.expired()) {
        ct++;
        nreg = nreg.lock()->neighbors[dir];
    }
    return ct;
}

void ARegion::Setup()
{
    //
    // type and location have been setup, do everything else
    SetupProds();

    SetupPop();

    //
    // Make the dummy object
    //
    AddObject();

    if (Globals->LAIR_MONSTERS_EXIST)
        LairCheck();
}

int ARegion::TraceConnectedRoad(const Directions& dir, int sum, std::list<ARegion::WeakHandle>& con, int range, int dev)
{
    ARegion::WeakHandle rn = weak_from_this();
    bool isnew = true;
    for(const auto& reg: con)
    {
        if (!reg.expired() && (reg.lock().get() == this))
        {
            isnew = false;
        }
    }
    if (!isnew)
    {
        return sum;
    }
    con.push_back(rn);
    // Add bonus for connecting town
    if (town) sum++;
    // Add bonus if development is higher
    if (development > dev + 9) sum++;
    if (development * 2 > dev * 5) sum++;
    // Check further along road
    if (range > 0) {
        for (auto d = Directions::begin(); d != Directions::end(); ++d) {
            if (!HasExitRoad(*d))
            {
                continue;
            }
            const ARegion::WeakHandle& r = neighbors[*d];
            if (r.expired())
            {
                continue;
            }
            ARegion::Handle r_sp = r.lock();
            if (dir == r_sp->GetRealDirComp(*d))
            {
                continue;
            }
            if (r_sp->HasConnectingRoad(*d))
            {
                sum = r_sp->TraceConnectedRoad(*d, sum, con, range-1, dev+2);
            }
        }
    }
    return sum;
}

int ARegion::RoadDevelopmentBonus(int range, int dev)
{
    int bonus = 0;
    std::list<ARegion::WeakHandle> con;
    con.push_back(weak_from_this());
    for (auto d = Directions::begin(); d != Directions::end(); ++d) {
        if (!HasExitRoad(*d)) continue;
        const ARegion::WeakHandle& r = neighbors[*d];
        if (r.expired())
        {
            continue;
        }
        ARegion::Handle r_sp = r.lock();
        if (r_sp->HasConnectingRoad(*d))
        {
            bonus = r_sp->TraceConnectedRoad(*d, bonus, con, range-1, dev);
        }
    }
    return bonus;    
}

// AS
void ARegion::DoDecayCheck(const ARegionList& pRegs)
{
    for(const auto& o: objects)
    {
        if (!(ObjectDefs[o->type].flags & ObjectType::NEVERDECAY)) {
            DoDecayClicks(o, pRegs);
        }
    }
}

// AS
void ARegion::DoDecayClicks(const Object::Handle& o, const ARegionList& pRegs)
{
    if (ObjectDefs[o->type].flags & ObjectType::NEVERDECAY) return;

    int clicks = getrandom(GetMaxClicks());
    clicks += PillageCheck();

    if (clicks > ObjectDefs[o->type].maxMonthlyDecay)
        clicks = ObjectDefs[o->type].maxMonthlyDecay;

    o->incomplete += clicks;

    if (o->incomplete > 0) {
        // trigger decay event
        RunDecayEvent(o, pRegs);
    }
}

// AS
void ARegion::RunDecayEvent(const Object::Handle& o, const ARegionList& pRegs)
{
    std::list<Faction::WeakHandle> pFactions = PresentFactions();
    for(const auto& fp: pFactions) {
        const auto f = fp.lock();
        f->Event(GetDecayFlavor() + *o->name + " " +
                ObjectDefs[o->type].name + " in " +
                ShortPrint(pRegs));
    }
}

// AS
AString ARegion::GetDecayFlavor()
{
    AString flavor;
    int badWeather = 0;
    if (weather != Weather::Types::W_NORMAL && !clearskies) badWeather = 1;
    if (!Globals->WEATHER_EXISTS) badWeather = 0;
    switch (type.asEnum()) {
        case Regions::Types::R_PLAIN:
        case Regions::Types::R_ISLAND_PLAIN:
        case Regions::Types::R_CERAN_PLAIN1:
        case Regions::Types::R_CERAN_PLAIN2:
        case Regions::Types::R_CERAN_PLAIN3:
        case Regions::Types::R_CERAN_LAKE:
            flavor = AString("Floods have damaged ");
            break;
        case Regions::Types::R_DESERT:
        case Regions::Types::R_CERAN_DESERT1:
        case Regions::Types::R_CERAN_DESERT2:
        case Regions::Types::R_CERAN_DESERT3:
            flavor = AString("Flashfloods have damaged ");
            break;
        case Regions::Types::R_CERAN_WASTELAND:
        case Regions::Types::R_CERAN_WASTELAND1:
            flavor = AString("Magical radiation has damaged ");
            break;
        case Regions::Types::R_TUNDRA:
        case Regions::Types::R_CERAN_TUNDRA1:
        case Regions::Types::R_CERAN_TUNDRA2:
        case Regions::Types::R_CERAN_TUNDRA3:
            if (badWeather) {
                flavor = AString("Ground freezing has damaged ");
            } else {
                flavor = AString("Ground thaw has damaged ");
            }
            break;
        case Regions::Types::R_MOUNTAIN:
        case Regions::Types::R_ISLAND_MOUNTAIN:
        case Regions::Types::R_CERAN_MOUNTAIN1:
        case Regions::Types::R_CERAN_MOUNTAIN2:
        case Regions::Types::R_CERAN_MOUNTAIN3:
            if (badWeather) {
                flavor = AString("Avalanches have damaged ");
            } else {
                flavor = AString("Rockslides have damaged ");
            }
            break;
        case Regions::Types::R_CERAN_HILL:
        case Regions::Types::R_CERAN_HILL1:
        case Regions::Types::R_CERAN_HILL2:
            flavor = AString("Quakes have damaged ");
            break;
        case Regions::Types::R_FOREST:
        case Regions::Types::R_SWAMP:
        case Regions::Types::R_ISLAND_SWAMP:
        case Regions::Types::R_JUNGLE:
        case Regions::Types::R_CERAN_FOREST1:
        case Regions::Types::R_CERAN_FOREST2:
        case Regions::Types::R_CERAN_FOREST3:
        case Regions::Types::R_CERAN_MYSTFOREST:
        case Regions::Types::R_CERAN_MYSTFOREST1:
        case Regions::Types::R_CERAN_MYSTFOREST2:
        case Regions::Types::R_CERAN_SWAMP1:
        case Regions::Types::R_CERAN_SWAMP2:
        case Regions::Types::R_CERAN_SWAMP3:
        case Regions::Types::R_CERAN_JUNGLE1:
        case Regions::Types::R_CERAN_JUNGLE2:
        case Regions::Types::R_CERAN_JUNGLE3:
            flavor = AString("Encroaching vegetation has damaged ");
            break;
        case Regions::Types::R_CAVERN:
        case Regions::Types::R_UFOREST:
        case Regions::Types::R_TUNNELS:
        case Regions::Types::R_CERAN_CAVERN1:
        case Regions::Types::R_CERAN_CAVERN2:
        case Regions::Types::R_CERAN_CAVERN3:
        case Regions::Types::R_CERAN_UFOREST1:
        case Regions::Types::R_CERAN_UFOREST2:
        case Regions::Types::R_CERAN_UFOREST3:
        case Regions::Types::R_CERAN_TUNNELS1:
        case Regions::Types::R_CERAN_TUNNELS2:
        case Regions::Types::R_CHASM:
        case Regions::Types::R_CERAN_CHASM1:
        case Regions::Types::R_GROTTO:
        case Regions::Types::R_CERAN_GROTTO1:
        case Regions::Types::R_DFOREST:
        case Regions::Types::R_CERAN_DFOREST1:
            if (badWeather) {
                flavor = AString("Lava flows have damaged ");
            } else {
                flavor = AString("Quakes have damaged ");
            }
            break;
        default:
            flavor = AString("Unexplained phenomena have damaged ");
            break;
    }
    return flavor;
}

// AS
int ARegion::GetMaxClicks()
{
    int terrainAdd = 0;
    int terrainMult = 1;
    int weatherAdd = 0;
    int badWeather = 0;
    int maxClicks;
    if (weather != Weather::Types::W_NORMAL && !clearskies) badWeather = 1;
    if (!Globals->WEATHER_EXISTS) badWeather = 0;
    switch (type.asEnum()) {
        case Regions::Types::R_PLAIN:
        case Regions::Types::R_ISLAND_PLAIN:
        case Regions::Types::R_TUNDRA:
        case Regions::Types::R_CERAN_PLAIN1:
        case Regions::Types::R_CERAN_PLAIN2:
        case Regions::Types::R_CERAN_PLAIN3:
        case Regions::Types::R_CERAN_LAKE:
        case Regions::Types::R_CERAN_TUNDRA1:
        case Regions::Types::R_CERAN_TUNDRA2:
        case Regions::Types::R_CERAN_TUNDRA3:
            terrainAdd = -1;
            if (badWeather) weatherAdd = 4;
            break;
        case Regions::Types::R_MOUNTAIN:
        case Regions::Types::R_ISLAND_MOUNTAIN:
        case Regions::Types::R_CERAN_MOUNTAIN1:
        case Regions::Types::R_CERAN_MOUNTAIN2:
        case Regions::Types::R_CERAN_MOUNTAIN3:
        case Regions::Types::R_CERAN_HILL:
        case Regions::Types::R_CERAN_HILL1:
        case Regions::Types::R_CERAN_HILL2:
            terrainMult = 2;
            if (badWeather) weatherAdd = 4;
            break;
        case Regions::Types::R_FOREST:
        case Regions::Types::R_SWAMP:
        case Regions::Types::R_ISLAND_SWAMP:
        case Regions::Types::R_JUNGLE:
        case Regions::Types::R_CERAN_FOREST1:
        case Regions::Types::R_CERAN_FOREST2:
        case Regions::Types::R_CERAN_FOREST3:
        case Regions::Types::R_CERAN_MYSTFOREST:
        case Regions::Types::R_CERAN_MYSTFOREST1:
        case Regions::Types::R_CERAN_MYSTFOREST2:
        case Regions::Types::R_CERAN_SWAMP1:
        case Regions::Types::R_CERAN_SWAMP2:
        case Regions::Types::R_CERAN_SWAMP3:
        case Regions::Types::R_CERAN_JUNGLE1:
        case Regions::Types::R_CERAN_JUNGLE2:
        case Regions::Types::R_CERAN_JUNGLE3:
            terrainAdd = -1;
            terrainMult = 2;
            if (badWeather) weatherAdd = 1;
            break;
        case Regions::Types::R_DESERT:
        case Regions::Types::R_CERAN_DESERT1:
        case Regions::Types::R_CERAN_DESERT2:
        case Regions::Types::R_CERAN_DESERT3:
            terrainAdd = -1;
            if (badWeather) weatherAdd = 5;
            break;
        case Regions::Types::R_CAVERN:
        case Regions::Types::R_UFOREST:
        case Regions::Types::R_TUNNELS:
        case Regions::Types::R_CERAN_CAVERN1:
        case Regions::Types::R_CERAN_CAVERN2:
        case Regions::Types::R_CERAN_CAVERN3:
        case Regions::Types::R_CERAN_UFOREST1:
        case Regions::Types::R_CERAN_UFOREST2:
        case Regions::Types::R_CERAN_UFOREST3:
        case Regions::Types::R_CERAN_TUNNELS1:
        case Regions::Types::R_CERAN_TUNNELS2:
        case Regions::Types::R_CHASM:
        case Regions::Types::R_CERAN_CHASM1:
        case Regions::Types::R_GROTTO:
        case Regions::Types::R_CERAN_GROTTO1:
        case Regions::Types::R_DFOREST:
        case Regions::Types::R_CERAN_DFOREST1:
            terrainAdd = 1;
            terrainMult = 2;
            if (badWeather) weatherAdd = 6;
            break;
        default:
            if (badWeather) weatherAdd = 4;
            break;
    }
    maxClicks = terrainMult * (terrainAdd + 2) + (weatherAdd + 1);
    return maxClicks;
}

// AS
int ARegion::PillageCheck()
{
    int pillageAdd = maxwages - wages;
    if (pillageAdd > 0) return pillageAdd;
    return 0;
}

// AS
bool ARegion::HasRoad()
{
    for(const auto& o: objects)
    {
        if (o->IsRoad() && o->incomplete < 1) return true;
    }
    return false;
}

// AS
bool ARegion::HasExitRoad(const Directions& realDirection)
{
    for(const auto& o: objects)
    {
        if (o->IsRoad() && o->incomplete < 1) {
            if (o->type == GetRoadDirection(realDirection))
            {
                return true;
            }
        }
    }
    return false;
}

// AS
int ARegion::CountConnectingRoads()
{
    int connections = 0;
    for (auto i = Directions::begin(); i != Directions::end(); ++i) {
        if (HasExitRoad(*i) && !neighbors[*i].expired() && HasConnectingRoad(*i))
            connections ++;
    }
    return connections;
}

// AS
bool ARegion::HasConnectingRoad(const Directions& realDirection)
{
    Directions opposite = GetRealDirComp(realDirection);

    if (!neighbors[realDirection].expired() && neighbors[realDirection].lock()->HasExitRoad(opposite))
    {
        return true;
    }

    return false;
}

// AS
Objects ARegion::GetRoadDirection(const Directions& realDirection)
{
    Objects roadDirection;
    switch (realDirection.asEnum()) {
        case Directions::Types::D_NORTH:
            roadDirection = Objects::Types::O_ROADN;
            break;
        case Directions::Types::D_NORTHEAST:
            roadDirection = Objects::Types::O_ROADNE;
            break;
        case Directions::Types::D_NORTHWEST:
            roadDirection = Objects::Types::O_ROADNW;
            break;
        case Directions::Types::D_SOUTH:
            roadDirection = Objects::Types::O_ROADS;
            break;
        case Directions::Types::D_SOUTHEAST:
            roadDirection = Objects::Types::O_ROADSE;
            break;
        case Directions::Types::D_SOUTHWEST:
            roadDirection = Objects::Types::O_ROADSW;
            break;
        default:
            throw std::runtime_error("Invalid direction specified");
    }
    return roadDirection;
}

// AS
Directions ARegion::GetRealDirComp(const Directions& realDirection)
{
    Directions complementDirection;
    if (!neighbors[realDirection].expired()) {
        const ARegion::Handle n = neighbors[realDirection].lock();
        const ARegion::Handle this_shared = shared_from_this();
        for (auto i = Directions::begin(); i != Directions::end(); ++i)
        {
            if (n->neighbors[*i].lock() == this_shared)
            {
                return *i;
            }
        }
    }

    switch (realDirection.asEnum()) {
        case Directions::Types::D_NORTH:
            complementDirection = Directions::Types::D_SOUTH;
            break;
        case Directions::Types::D_NORTHEAST:
            complementDirection = Directions::Types::D_SOUTHWEST;
            break;
        case Directions::Types::D_NORTHWEST:
            complementDirection = Directions::Types::D_SOUTHEAST;
            break;
        case Directions::Types::D_SOUTH:
            complementDirection = Directions::Types::D_NORTH;
            break;
        case Directions::Types::D_SOUTHEAST:
            complementDirection = Directions::Types::D_NORTHWEST;
            break;
        case Directions::Types::D_SOUTHWEST:
            complementDirection = Directions::Types::D_NORTHEAST;
            break;
        default:
            throw std::runtime_error("Invalid direction specified");
    }
    return complementDirection;
}

AString ARegion::ShortPrint(const ARegionList& pRegs)
{
    AString temp = TerrainDefs[type].name;

    temp += AString(" (") + xloc + "," + yloc;

    const ARegionArray::Handle& pArr = pRegs.pRegionArrays[zloc];
    if (pArr->strName) {
        temp += ",";
        if (Globals->EASIER_UNDERWORLD &&
                (Globals->UNDERWORLD_LEVELS+Globals->UNDERDEEP_LEVELS > 1)) {
            temp += AString("") + zloc + " <";
        } else {
            // add less explicit multilevel information about the underworld
            if (zloc > 2 && zloc < Globals->UNDERWORLD_LEVELS+2) {
                for (unsigned int i = zloc; i > 3; i--) {
                    temp += "very ";
                }
                temp += "deep ";
            } else if ((zloc > Globals->UNDERWORLD_LEVELS+2) &&
                        (zloc < Globals->UNDERWORLD_LEVELS +
                        Globals->UNDERDEEP_LEVELS + 2)) {
                for (unsigned int i = zloc; i > Globals->UNDERWORLD_LEVELS + 3; i--) {
                    temp += "very ";
                }
                temp += "deep ";
            }
        }
        temp += *pArr->strName;
        if (Globals->EASIER_UNDERWORLD &&
                (Globals->UNDERWORLD_LEVELS+Globals->UNDERDEEP_LEVELS > 1)) {
            temp += ">";
        }
    }
    temp += ")";

    temp += AString(" in ") + *name;
    return temp;
}

AString ARegion::Print(const ARegionList& pRegs)
{
    AString temp = ShortPrint(pRegs);
    if (town) {
        temp += AString(", contains ") + *(town->name) + " [" +
            TownString(town->TownType()) + "]";
    }
    return temp;
}

void ARegion::SetLoc(unsigned int x, unsigned int y, unsigned int z)
{
    xloc = x;
    yloc = y;
    zloc = z;
}

void ARegion::SetGateStatus(size_t month)
{
    if ((type == Regions::Types::R_NEXUS) || (Globals->START_GATES_OPEN && IsStartingCity())) {
        gateopen = true;
        return;
    }
    gateopen = false;
    for (size_t i = 0; i < Globals->GATES_NOT_PERENNIAL; i++) {
        size_t dmon = gatemonth + i;
        if (dmon > 11)
        {
            dmon = dmon - 12;
        }
        if (dmon == month)
        {
            gateopen = true;
        }
    }
}

void ARegion::Kill(const Unit::Handle& u)
{
    Unit::Handle first = nullptr;
    for(const auto& obj: objects)
    {
        if (obj) {
            for(const auto& cur_unit: obj->units)
            {
                if((cur_unit->faction.lock()->num == u->faction.lock()->num) && (cur_unit != u)) {
                    first = cur_unit;
                    break;
                }
            }
        }
        if (first) break;
    }

    if (first) {
        // give u's stuff to first
        for(const auto& i: u->items) {
            if (ItemDefs[i->type].type & IT_SHIP &&
                    first->items.GetNum(i->type) > 0) {
                if (first->items.GetNum(i->type) > i->num)
                    first->items.SetNum(i->type, i->num);
                continue;
            }
            if (!IsSoldier(i->type)) {
                first->items.SetNum(i->type, first->items.GetNum(i->type) +
                                    i->num);
                // If we're in ocean and not in a structure, make sure that
                // the first unit can actually hold the stuff and not drown
                // If the item would cause them to drown then they won't
                // pick it up.
                if (TerrainDefs[type].similar_type == Regions::Types::R_OCEAN) {
                    if (first->object.lock()->type == Objects::Types::O_DUMMY) {
                        if (!first->CanReallySwim()) {
                            first->items.SetNum(i->type,
                                    first->items.GetNum(i->type) - i->num);
                        }
                    }
                }
            }
            u->items.SetNum(i->type, 0);
        }
    }

    u->Detach();
    hell.push_back(u);
}

void ARegion::ClearHell()
{
    hell.clear();
}

Object::WeakHandle ARegion::GetObject(int num)
{
    for(const auto& o: objects) {
        if (o->num == num) return o;
    }
    return Object::WeakHandle();
}

Object::WeakHandle ARegion::GetDummy()
{
    for(const auto& o: objects) {
        if (o->type == Objects::Types::O_DUMMY) return o;
    }
    return Object::WeakHandle();
}

/* Checks all fleets to see if they are empty.
 * Moves all units out of an empty fleet into the
 * dummy object.
 */
void ARegion::CheckFleets()
{
    auto o_it = objects.begin();
    while(o_it != objects.end())
    {
        const auto& o = *o_it;
        if (o->IsFleet()) {
            bool bail = false;
            if (o->FleetCapacity() < 1) bail = true;
            bool alive = false;
            for(const auto& unit: o->units) {
                alive = unit->IsAlive();
                if (bail)
                {
                    unit->MoveUnit(GetDummy());
                }
            }
            // don't remove fleets when no living units are
            // aboard when they're not at sea.
            if (TerrainDefs[type].similar_type != Regions::Types::R_OCEAN) alive = true;
            if (!alive || bail) {
                o_it = objects.erase(o_it);
                continue;
            }
        }
        ++o_it;
    }
}

Unit::WeakHandle ARegion::GetUnit(int num)
{
    for(const auto&obj: objects) {
        const auto& u = obj->GetUnit(num);
        if (!u.expired()) {
            return u;
        }
    }
    return Unit::WeakHandle();
}

Location::Handle ARegion::GetLocation(const UnitId& id, size_t faction) const
{
    Unit::WeakHandle retval;
    for(const auto& o: objects)
    {
        retval = o->GetUnitId(id, faction);
        if (!retval.expired()) {
            Location::Handle l = std::make_shared<Location>();
            l->region = std::const_pointer_cast<ARegion>(shared_from_this());
            l->obj = o;
            l->unit = retval;
            return l;
        }
    }
    return nullptr;
}

Unit::WeakHandle ARegion::GetUnitAlias(int alias, int faction)
{
    for(const auto& obj: objects) {
        Unit::WeakHandle u = obj->GetUnitAlias(alias, faction);
        if (!u.expired()) {
            return u;
        }
    }
    return Unit::WeakHandle();
}

Unit::WeakHandle ARegion::GetUnitId(const UnitId& id, size_t faction)
{
    Unit::WeakHandle retval;
    for(const auto&o: objects) {
        retval = o->GetUnitId(id, faction);
        if (!retval.expired())
        {
            return retval;
        }
    }
    return retval;
}

void ARegion::DeduplicateUnitList(std::list<UnitId>& list, size_t faction)
{
    std::set<UnitId> seen_ids;
    auto it = list.begin();
    while(it != list.end())
    {
        const auto& u = GetUnitId(*it, faction);
        if(u.expired())
        {
            continue;
        }
        if(!seen_ids.count(*it))
        {
            seen_ids.insert(*it);
            ++it;
        }
        else
        {
            it = list.erase(it);
        }
    }

    /*forlist(list) {
        id = dynamic_cast<UnitId *>(elem);
        outer = GetUnitId(id, faction);
        if (outer.expired())
            continue;
        j = 0;
        forlist(list) {
            id = dynamic_cast<UnitId *>(elem);
            inner = GetUnitId(id, faction);
            if (inner.expired())
                continue;
            if (inner.lock()->num == outer.lock()->num && j > i) {
                list->Remove(id);
                delete id;
            }
            j++;
        }
        i++;
    }*/
}

Location::Handle ARegionList::GetUnitId(const UnitId& id, size_t faction, const ARegion& cur)
{
    // Check current region first
    Location::Handle retval = cur.GetLocation(id, faction);
    if (retval)
    {
        return retval;
    }

    // No? We must be looking for an existing unit.
    if (!id.unitnum)
    {
        return nullptr;
    }

    return this->FindUnit(id.unitnum);
}

bool ARegion::Present(const Faction& f)
{
    for(const auto& obj: objects)
    {
        for(const auto& u: obj->units)
        {
            if (u->faction.lock().get() == &f) return true;
        }
    }
    return false;
}

std::list<Faction::WeakHandle> ARegion::PresentFactions()
{
    std::list<Faction::WeakHandle> facs;
    for(const auto& obj: objects)
    {
        for(const auto& u: obj->units)
        {
            if (GetFaction2(facs, u->faction.lock()->num).expired()) {
                facs.push_back(u->faction);
            }
        }
    }
    return facs;
}

void ARegion::Writeout(Aoutfile *f)
{
    f->PutStr(*name);
    f->PutInt(num);
    if (type.isValid())
    {
        f->PutStr(TerrainDefs[type].type);
    }
    else
    {
        f->PutStr("NO_TERRAIN");
    }
    f->PutInt(buildingseq);
    f->PutInt(gate);
    if (gate > 0)
    {
        f->PutInt(gatemonth);
    }
    if (race.isValid())
    {
        f->PutStr(ItemDefs[race].abr);
    }
    else
    {
        f->PutStr("NO_RACE");
    }
    f->PutInt(population);
    f->PutInt(basepopulation);
    f->PutInt(wages);
    f->PutInt(maxwages);
    f->PutInt(wealth);

    f->PutInt(elevation);
    f->PutInt(humidity);
    f->PutInt(temperature);
    f->PutInt(vegetation);
    f->PutInt(culture);

    f->PutInt(habitat);
    f->PutInt(development);

    if (town) {
        f->PutInt(1);
        town->Writeout(f);
    } else {
        f->PutInt(0);
    }

    f->PutInt(xloc);
    f->PutInt(yloc);
    f->PutInt(zloc);
    f->PutInt(visited);

    products.Writeout(f);
    markets.Writeout(f);

    f->PutInt(objects.size());
    for(const auto& o: objects)
    {
        o->Writeout(f);
    }
}

Regions LookupRegionType(AString *token)
{
    for (auto i = Regions::begin(); i != Regions::end(); ++i) {
        if (*token == TerrainDefs[*i].type)
        {
            return *i;
        }
    }
    return Regions();
}

void ARegion::Readin(Ainfile *f, const std::list<Faction::Handle>& facs, ATL_VER v)
{
    AString *temp;

    name = f->GetStr();

    num = f->GetInt<size_t>();
    temp = f->GetStr();
    type = LookupRegionType(temp);
    delete temp;
    buildingseq = f->GetInt<int>();
    gate = f->GetInt<int>();
    if (gate > 0) gatemonth = f->GetInt<size_t>();

    temp = f->GetStr();
    race = LookupItem(temp);
    delete temp;

    population = f->GetInt<int>();
    basepopulation = f->GetInt<int>();
    wages = f->GetInt<int>();
    maxwages = f->GetInt<int>();
    wealth = f->GetInt<int>();
    
    elevation = f->GetInt<int>();
    humidity = f->GetInt<int>();
    temperature = f->GetInt<int>();
    vegetation = f->GetInt<int>();
    culture = f->GetInt<int>();

    habitat = f->GetInt<int>();
    development = f->GetInt<int>();

    if (f->GetInt<int>()) {
        town = new TownInfo;
        town->Readin(f, v);
        town->dev = TownDevelopment();
    } else {
        town = 0;
    }

    xloc = f->GetInt<unsigned int>();
    yloc = f->GetInt<unsigned int>();
    zloc = f->GetInt<unsigned int>();
    visited = f->GetInt<int>();

    products.Readin(f);
    markets.Readin(f);

    int i = f->GetInt<int>();
    buildingseq = 1;
    for (int j=0; j<i; j++) {
        Object::Handle& temp = AddObject();
        temp->Readin(f, facs, v);
        if (temp->num >= buildingseq)
        {
            buildingseq = temp->num + 1;
        }
    }
    fleetalias = 1;
    newfleets.clear();
}

bool ARegion::CanMakeAdv(const Faction& fac, const Items& item)
{
    AString skname;
    Skills sk;

    if (Globals->IMPROVED_FARSIGHT) {
        for(const auto& f: farsees)
        {
            if (f && f->faction.lock().get() == &fac && !f->unit.expired()) {
                skname = ItemDefs[item].pSkill;
                sk = LookupSkill(skname);
                if (f->unit.lock()->GetSkill(sk) >= ItemDefs[item].pLevel)
                    return true;
            }
        }
    }

    if ((Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) &&
            (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_RESOURCES)) {
        for(const auto& f: passers)
        {
            if (f && f->faction.lock().get() == &fac && !f->unit.expired()) {
                skname = ItemDefs[item].pSkill;
                sk = LookupSkill(skname);
                if (f->unit.lock()->GetSkill(sk) >= ItemDefs[item].pLevel)
                    return true;
            }
        }
    }

    for(const auto& o: objects)
    {
        for(const auto& u: o->units)
        {
            if (u->faction.lock().get() == &fac) {
                skname = ItemDefs[item].pSkill;
                sk = LookupSkill(skname);
                if (u->GetSkill(sk) >= ItemDefs[item].pLevel)
                    return true;
            }
        }
    }
    return false;
}

void ARegion::WriteProducts(Areport *f, const Faction& fac, bool present)
{
    AString temp = "Products: ";
    bool has = false;
    for(const auto& p: products) {
        if (ItemDefs[p->itemtype].type & IT_ADVANCED) {
            if (CanMakeAdv(fac, p->itemtype) || (fac.IsNPC())) {
                if (has) {
                    temp += AString(", ") + p->WriteReport();
                } else {
                    has = true;
                    temp += p->WriteReport();
                }
            }
        } else {
            if (p->itemtype == Items::Types::I_SILVER) {
                if (p->skill == Skills::Types::S_ENTERTAINMENT) {
                    if ((Globals->TRANSIT_REPORT &
                            GameDefs::REPORT_SHOW_ENTERTAINMENT) || present) {
                        f->PutStr(AString("Entertainment available: $") +
                                p->amount + ".");
                    } else {
                        f->PutStr(AString("Entertainment available: $0."));
                    }
                }
            } else {
                if (!present &&
                        !(Globals->TRANSIT_REPORT &
                        GameDefs::REPORT_SHOW_RESOURCES))
                    continue;
                if (has) {
                    temp += AString(", ") + p->WriteReport();
                } else {
                    has = true;
                    temp += p->WriteReport();
                }
            }
        }
    }

    if (!has)
    {
        temp += "none";
    }
    temp += ".";
    f->PutStr(temp);
}

bool ARegion::HasItem(const Faction& fac, const Items& item)
{
    for(const auto& o: objects)
    {
        for(const auto& u: o->units)
        {
            if (u->faction.lock().get() == &fac)
            {
                if (u->items.GetNum(item))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void ARegion::WriteMarkets(Areport *f, const Faction& fac, bool present)
{
    AString temp = "Wanted: ";
    bool has = false;
    for(const auto& m: markets) {
        if (!m->amount) continue;
        if (!present &&
                !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_MARKETS))
            continue;
        if (m->type == M_SELL) {
            if (ItemDefs[m->item].type & IT_ADVANCED) {
                if (!Globals->MARKETS_SHOW_ADVANCED_ITEMS) {
                    if (!HasItem(fac, m->item)) {
                        continue;
                    }
                }
            }
            if (has) {
                temp += ", ";
            } else {
                has = true;
            }
            temp += m->Report();
        }
    }
    if (!has)
    {
        temp += "none";
    }
    temp += ".";
    f->PutStr(temp);

    temp = "For Sale: ";
    has = false;
    {
        for(const auto& m: markets) {
            if (!m->amount) continue;
            if (!present &&
                    !(Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_MARKETS))
                continue;
            if (m->type == M_BUY) {
                if (has) {
                    temp += ", ";
                } else {
                    has = true;
                }
                temp += m->Report();
            }
        }
    }
    if (!has)
    {
        temp += "none";
    }
    temp += ".";
    f->PutStr(temp);
}

void ARegion::WriteEconomy(Areport *f, const Faction& fac, bool present)
{
    f->AddTab();

    if ((Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_WAGES) || present) {
        f->PutStr(AString("Wages: ") + WagesForReport() + ".");
    } else {
        f->PutStr(AString("Wages: $0."));
    }

    WriteMarkets(f, fac, present);

    WriteProducts(f, fac, present);

    f->EndLine();
    f->DropTab();
}

void ARegion::WriteExits(Areport *f, const ARegionList& pRegs, const ExitArray& exits_seen)
{
    f->PutStr("Exits:");
    f->AddTab();
    int y = 0;
    for (auto d = Directions::begin(); d != Directions::end(); ++d) {
        const ARegion::WeakHandle& r = neighbors[*d];
        if (!r.expired() && exits_seen[*d]) {
            f->PutStr(AString(DirectionStrs[*d]) + " : " +
                    r.lock()->Print(pRegs) + ".");
            y = 1;
        }
    }
    if (!y) f->PutStr("none");
    f->DropTab();
    f->EndLine();
}

#define AC_STRING "%s Nexus is a magical place: the entryway " \
"to the world of %s. Enjoy your stay; the city guards should " \
"keep you safe as long as you should choose to stay. However, rumor " \
"has it that once you have left the Nexus, you can never return."

void ARegion::WriteReport(Areport *f, const Faction& fac, const ValidValue<size_t>& month, const ARegionList& pRegions)
{
    Farsight::WeakHandle farsight = GetFarsight(farsees, fac);
    const bool has_farsight = !farsight.expired();
    Farsight::WeakHandle passer = GetFarsight(passers, fac);
    const bool has_passer = !passer.expired();
    const bool present = Present(fac) || fac.IsNPC();

    if (has_farsight || has_passer || present) {
        AString temp = Print(pRegions);
        if (Population() &&
            (present || has_farsight ||
             (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_PEASANTS))) {
            temp += AString(", ") + Population() + " peasants";
            if (Globals->RACES_EXIST) {
                temp += AString(" (") + ItemDefs[race].names + ")";
            }
            if (present || has_farsight ||
                    Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_REGION_MONEY) {
                temp += AString(", $") + wealth;
            } else {
                temp += AString(", $0");
            }
        }
        temp += ".";
        f->PutStr(temp);
        f->PutStr("-------------------------------------------------"
                "-----------");

        f->AddTab();
        if (Globals->WEATHER_EXISTS) {
            temp = "It was ";
            if (clearskies) temp += "unnaturally clear ";
            else {
                if (weather == Weather::Types::W_BLIZZARD) temp = "There was an unnatural ";
                else if (weather == Weather::Types::W_NORMAL) temp = "The weather was ";
                temp += SeasonNames[weather];
            }
            temp += " last month; ";
            size_t next_month;
            if(month.isValid())
            {
                next_month = month + 1;
            }
            else
            {
                next_month = 0;
            }
            Weather nxtweather = pRegions.GetWeather(*this, next_month % 12);
            temp += "it will be ";
            temp += SeasonNames[nxtweather];
            temp += " next month.";
            f->PutStr(temp);
        }
        
#if 0
        f->PutStr("");
        temp = "Elevation is ";
        f->PutStr(temp + elevation);
        temp = "Humidity is ";
        f->PutStr(temp + humidity);
        temp = "Temperature is ";
        f->PutStr(temp + temperature);
#endif

        if (type == Regions::Types::R_NEXUS) {
            size_t len = strlen(AC_STRING) + 2*strlen(Globals->WORLD_NAME);
            char *nexus_desc = new char[len];
            sprintf(nexus_desc, AC_STRING, Globals->WORLD_NAME,
                    Globals->WORLD_NAME);
            f->PutStr("");
            f->PutStr(nexus_desc);
            f->PutStr("");
            delete [] nexus_desc;
        }

        f->DropTab();

        WriteEconomy(f, fac, present || has_farsight);

        ExitArray exits_seen;
        if (present || has_farsight || (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_ALL_EXITS)) {
            std::fill(exits_seen.begin(), exits_seen.end(), true);
        } else {
            // This is just a transit report and we're not showing all
            // exits.   See if we are showing used exits.

            // Show none by default.
            std::fill(exits_seen.begin(), exits_seen.end(), false);
            // Now, if we should, show the ones actually used.
            if (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_USED_EXITS) {
                for(const auto& p: passers) {
                    if (p->faction.lock().get() == &fac) {
                        for (size_t i = 0; i < exits_seen.size(); ++i) {
                            exits_seen[i] |= p->exits_used[i];
                        }
                    }
                }
            }
        }

        WriteExits(f, pRegions, exits_seen);

        if (Globals->GATES_EXIST && gate && gate != -1) {
            bool sawgate = false;
            if (fac.IsNPC())
                sawgate = true;
            if (Globals->IMPROVED_FARSIGHT && has_farsight) {
                for(const auto& watcher: farsees) {
                    if (watcher && watcher->faction.lock().get() == &fac && !watcher->unit.expired()) {
                        if (watcher->unit.lock()->GetSkill(Skills::Types::S_GATE_LORE)) {
                            sawgate = true;
                        }
                    }
                }
            }
            if (Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) {
                for(const auto& watcher: passers) {
                    if (watcher && watcher->faction.lock().get() == &fac && !watcher->unit.expired()) {
                        if (watcher->unit.lock()->GetSkill(Skills::Types::S_GATE_LORE)) {
                            sawgate = true;
                        }
                    }
                }
            }
            for(const auto& o: objects) {
                for(const auto& u: o->units) {
                    if (!sawgate &&
                            ((u->faction.lock().get() == &fac) &&
                             u->GetSkill(Skills::Types::S_GATE_LORE))) {
                        sawgate = true;
                    }
                }
            }
            if (sawgate) {
                if (gateopen) {
                    AString temp;
                    temp = "There is a Gate here (Gate ";
                    temp += gate;
                    if (!Globals->DISPERSE_GATE_NUMBERS) {
                        temp += " of ";
                        temp += pRegions.numberofgates;
                    }
                    temp += ").";
                    f->PutStr(temp);
                    f->PutStr("");
                } else if (Globals->SHOW_CLOSED_GATES) {
                    f->PutStr(AString("There is a closed Gate here."));
                    f->PutStr("");
                }
            }
        }

        int obs = GetObservation(fac, false);
        int truesight = GetTrueSight(fac, false);
        bool detfac = false;

        int passobs = GetObservation(fac, true);
        int passtrue = GetTrueSight(fac, true);
        bool passdetfac = detfac;

        if (fac.IsNPC()) {
            obs = 10;
            passobs = 10;
        }

        for(const auto& o: objects) {
            for(const auto& u: o->units) {
                if (u->faction.lock().get() == &fac && u->GetSkill(Skills::Types::S_MIND_READING) > 2) {
                    detfac = true;
                }
            }
        }
        if (Globals->IMPROVED_FARSIGHT && has_farsight) {
            for(const auto& watcher: farsees) {
                if (watcher && watcher->faction.lock().get() == &fac && !watcher->unit.expired()) {
                    if (watcher->unit.lock()->GetSkill(Skills::Types::S_MIND_READING) > 2) {
                        detfac = true;
                    }
                }
            }
        }

        if ((Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) &&
                (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_UNITS)) {
            for(const auto& watcher: passers) {
                if (watcher && watcher->faction.lock().get() == &fac && !watcher->unit.expired()) {
                    if (watcher->unit.lock()->GetSkill(Skills::Types::S_MIND_READING) > 2) {
                        passdetfac = true;
                    }
                }
            }
        }

        {
            for(const auto& o: objects) {
                o->Report(f, fac, obs, truesight, detfac,
                            passobs, passtrue, passdetfac,
                            present || has_farsight);
            }
            f->EndLine();
        }
    }
}

// DK
void ARegion::WriteTemplate(Areport *f, const Faction& fac,
        const ARegionList& pRegs, const ValidValue<size_t>& month)
{
    int header = 0;
    Orders order;
    AString temp, *token;

    for(const auto& o: objects) {
        for(const auto &u: o->units) {
            if (u->faction.lock().get() == &fac) {
                if (!header) {
                    // DK
                    if (fac.temformat == TEMPLATE_MAP) {
                        WriteTemplateHeader(f, fac, pRegs, month);
                    } else {
                        f->PutStr("");
                        f->PutStr(AString("*** ") + Print(pRegs) + " ***", 1);
                    }
                    header = 1;
                }

                f->PutStr("");
                f->PutStr(AString("unit ") + u->num);
                // DK
                if (fac.temformat == TEMPLATE_LONG ||
                        fac.temformat == TEMPLATE_MAP) {
                    f->PutStr(u->TemplateReport(), 1);
                }
                int gotMonthOrder = 0;
                for(const auto& s: u->oldorders) {
                    temp = *s;
                    temp.getat();
                    token = temp.gettoken();
                    if (token) {
                        order = Parse1Order(token);
                        delete token;
                    } else
                        order = Orders::Types::NORDERS;
                    switch (order.asEnum()) {
                        case Orders::Types::O_MOVE:
                        case Orders::Types::O_SAIL:
                        case Orders::Types::O_TEACH:
                        case Orders::Types::O_STUDY:
                        case Orders::Types::O_BUILD:
                        case Orders::Types::O_PRODUCE:
                        case Orders::Types::O_ENTERTAIN:
                        case Orders::Types::O_WORK:
                            gotMonthOrder = 1;
                            break;
                        case Orders::Types::O_TAX:
                        case Orders::Types::O_PILLAGE:
                            if (Globals->TAX_PILLAGE_MONTH_LONG)
                                gotMonthOrder = 1;
                            break;
                        default:
                            break;
                    }
                    if (order == Orders::Types::O_ENDTURN || order == Orders::Types::O_ENDFORM)
                        f->DropTab();
                    f->PutStr(*s);
                    if (order == Orders::Types::O_TURN || order == Orders::Types::O_FORM)
                        f->AddTab();
                }
                f->ClearTab();
                u->oldorders.clear();

                int firstMonthOrder = gotMonthOrder;
                if (!u->turnorders.empty()) {
                    for(const auto& tOrder: u->turnorders)
                    {
                        if (firstMonthOrder) {
                            if (tOrder->repeating)
                                f->PutStr(AString("@TURN"));
                            else
                                f->PutStr(AString("TURN"));
                            f->AddTab();
                        }
                        for(const auto& t: tOrder->turnOrders) {
                            temp = *t;
                            temp.getat();
                            token = temp.gettoken();
                            if (token) {
                                order = Parse1Order(token);
                                delete token;
                            } else
                                order = Orders::Types::NORDERS;
                            if (order == Orders::Types::O_ENDTURN || order == Orders::Types::O_ENDFORM)
                                f->DropTab();
                            f->PutStr(*t);
                            if (order == Orders::Types::O_TURN || order == Orders::Types::O_FORM)
                                f->AddTab();
                        }
                        if (firstMonthOrder) {
                            f->DropTab();
                            f->PutStr(AString("ENDTURN"));
                        }
                        firstMonthOrder = 1;
                        f->ClearTab();
                    }
                    const TurnOrder::Handle& tOrder = u->turnorders.front();
                    if (tOrder->repeating && !gotMonthOrder) {
                        f->PutStr(AString("@TURN"));
                        f->AddTab();
                        for(const auto& t: tOrder->turnOrders) {
                            temp = *t;
                            temp.getat();
                            token = temp.gettoken();
                            if (token) {
                                order = Parse1Order(token);
                                delete token;
                            } else
                                order = Orders::Types::NORDERS;
                            if (order == Orders::Types::O_ENDTURN || order == Orders::Types::O_ENDFORM)
                                f->DropTab();
                            f->PutStr(*t);
                            if (order == Orders::Types::O_TURN || order == Orders::Types::O_FORM)
                                f->AddTab();
                        }
                        f->ClearTab();
                        f->PutStr(AString("ENDTURN"));
                    }
                }
                u->turnorders.clear();
            }
        }
    }
}

int ARegion::GetTrueSight(const Faction& f, bool usepassers)
{
    int truesight = 0;

    if (Globals->IMPROVED_FARSIGHT) {
        for(const auto& farsight: farsees) {
            if (farsight && farsight->faction.lock().get() == &f && !farsight->unit.expired()) {
                int t = farsight->unit.lock()->GetSkill(Skills::Types::S_TRUE_SEEING);
                if (t > truesight) truesight = t;
            }
        }
    }

    if (usepassers &&
            (Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) &&
            (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_UNITS)) {
        for(const auto& farsight: passers) {
            if (farsight && farsight->faction.lock().get() == &f && !farsight->unit.expired()) {
                int t = farsight->unit.lock()->GetSkill(Skills::Types::S_TRUE_SEEING);
                if (t > truesight) truesight = t;
            }
        }
    }

    for(const auto& obj: objects) {
        for(const auto& u: obj->units) {
            if (u->faction.lock().get() == &f) {
                int temp = u->GetSkill(Skills::Types::S_TRUE_SEEING);
                if (temp>truesight) truesight = temp;
            }
        }
    }
    return truesight;
}

int ARegion::GetObservation(const Faction& f, bool usepassers)
{
    int obs = 0;

    if (Globals->IMPROVED_FARSIGHT) {
        for(const auto& farsight: farsees) {
            if (farsight && farsight->faction.lock().get() == &f && !farsight->unit.expired()) {
                int o = farsight->observation;
                if (o > obs) obs = o;
            }
        }
    }

    if (usepassers &&
            (Globals->TRANSIT_REPORT & GameDefs::REPORT_USE_UNIT_SKILLS) &&
            (Globals->TRANSIT_REPORT & GameDefs::REPORT_SHOW_UNITS)) {
        for(const auto& farsight: passers) {
            if (farsight && farsight->faction.lock().get() == &f && !farsight->unit.expired()) {
                int o = farsight->observation;
                if (o > obs) obs = o;
            }
        }
    }

    for(const auto& obj: objects) {
        for(const auto& u: obj->units) {
            if (u->faction.lock().get() == &f) {
                int temp = u->GetAttribute("observation");
                if (temp>obs) obs = temp;
            }
        }
    }
    return obs;
}

void ARegion::SetWeather(const Weather& newWeather)
{
    weather = newWeather;
}

unsigned int ARegion::IsCoastal()
{
    if (type == Regions::Types::R_LAKE) {
        if (Globals->LAKESIDE_IS_COASTAL)
            return 1;
    } else if (TerrainDefs[type].similar_type == Regions::Types::R_OCEAN)
        return 1;
    unsigned int seacount = 0;
    for (const auto& n: neighbors) {
        if (!n.expired())
        {
            const auto& n_type = n.lock()->type;
            if(TerrainDefs[n_type].similar_type == Regions::Types::R_OCEAN) {
                if (!Globals->LAKESIDE_IS_COASTAL && n_type == Regions::Types::R_LAKE) continue;
                seacount++;
            }
        }
    }
    return seacount;
}

unsigned int ARegion::IsCoastalOrLakeside()
{
    if (TerrainDefs[type].similar_type == Regions::Types::R_OCEAN) return true;
    unsigned int seacount = 0;
    for (const auto& n: neighbors) {
        if(!n.expired())
        {
            if (TerrainDefs[n.lock()->type].similar_type == Regions::Types::R_OCEAN) {
                seacount++;
            }
        }
    }
    return seacount;
}

int ARegion::MoveCost(int movetype, const ARegion::Handle& fromRegion, const Directions& dir, AString *road)
{
    int cost = 1;
    if (Globals->WEATHER_EXISTS) {
        cost = 2;
        if (weather == Weather::Types::W_BLIZZARD) return 10;
        if (weather == Weather::Types::W_NORMAL || clearskies) cost = 1;
    }
    if (movetype == M_SWIM) {
        cost = (TerrainDefs[type].movepoints * cost);
        // Roads don't help swimming, even if there are any in the ocean
    } else if (movetype == M_WALK || movetype == M_RIDE) {
        cost = (TerrainDefs[type].movepoints * cost);
        if (fromRegion->HasExitRoad(dir) && fromRegion->HasConnectingRoad(dir)) {
            cost -= cost/2;
            if (road)
                *road = "on a road ";
        }
    }
    if (cost < 1) cost = 1;
    return cost;
}

Unit::WeakHandle ARegion::Forbidden(const Unit::Handle& u)
{
    for(const auto& obj: objects) {
        for(const auto& u2: obj->units) {
            if (u2->Forbids(*this, u)) return u2;
        }
    }
    return Unit::WeakHandle();
}

Unit::WeakHandle ARegion::ForbiddenByAlly(const Unit::Handle& u)
{
    for(const auto& obj: objects) {
        for(const auto& u2: obj->units) {
            if (u->faction.lock()->GetAttitude(u2->faction.lock()->num) == A_ALLY &&
                u2->Forbids(*this, u)) return u2;
        }
    }
    return Unit::WeakHandle();
}

bool ARegion::HasCityGuard()
{
    for(const auto& obj: objects) {
        for(const auto& u: obj->units) {
            if (u->type == U_GUARD && u->GetSoldiers() &&
                u->guard == GUARD_GUARD) {
                return true;
            }
        }
    }
    return false;
}

bool ARegion::NotifySpell(const Unit::Handle& caster, char const *spell, const ARegionList& pRegs)
{
    std::list<Faction::Handle> flist;

    const auto& pS = FindSkill(spell);

    if (!(pS.flags & SkillType::NOTIFY)) {
        // Okay, we aren't notifyable, check our prerequisites
        for(const auto& d: pS.depends)
        {
            if (d.skill == nullptr)
            {
                break;
            }
            if (NotifySpell(caster, d.skill, pRegs))
            {
                return true;
            }
        }
        return false;
    }

    AString skname = spell;
    Skills sp = LookupSkill(skname);
    for(const auto& o: objects) {
        for(const auto& u: o->units) {
            if (u->faction.lock() == caster->faction.lock()) continue;
            if (u->GetSkill(sp)) {
                if (!GetFaction(flist, u->faction.lock()->num)) {
                    flist.emplace_back(u->faction);
                }
            }
        }
    }

    for(const auto& fp: flist) {
        fp->Event(AString(*(caster->name)) + " uses " + SkillStrs(sp) +
                " in " + Print(pRegs) + ".");
    }
    return true;
}

// ALT, 26-Jul-2000
// Procedure to notify all units in city about city name change
void ARegion::NotifyCity(const Unit::Handle& caster, AString& oldname, AString& newname)
{
    std::list<Faction::WeakHandle> flist;
    for(const auto& o: objects) {
        for(const auto& u: o->units) {
            if (u->faction.lock() == caster->faction.lock()) continue;
            if (GetFaction2(flist, u->faction.lock()->num).expired()) {
                flist.push_back(u->faction);
            }
        }
    }
    {
        for(const auto& fp: flist) {
            fp.lock()->Event(AString(*(caster->name)) + " renames " +
                             oldname + " to " + newname + ".");
        }
    }
}

bool ARegion::CanTax(const Unit::Handle& u)
{
    for(const auto& obj: objects) {
        for(const auto& u2: obj->units) {
            if (u2->guard == GUARD_GUARD && u2->IsAlive())
                if (u2->GetAttitude(*this, u) <= A_NEUTRAL)
                    return false;
        }
    }
    return true;
}

bool ARegion::CanPillage(const Unit::Handle& u)
{
    for(const auto& obj: objects) {
        for(const auto& u2: obj->units) {
            if (u2->guard == GUARD_GUARD && u2->IsAlive() &&
                    u2->faction.lock() != u->faction.lock())
                return false;
        }
    }
    return true;
}

bool ARegion::ForbiddenShip(const Object::Handle& ship)
{
    for(const auto& u: ship->units) {
        if (!Forbidden(u).expired()) return true;
    }
    return false;
}

void ARegion::DefaultOrders()
{
    for(const auto& obj: objects)
    {
        for(const auto& u: obj->units)
        {
            u->DefaultOrders(obj);
        }
    }
}

//
// This is just used for mapping; just check if there is an inner region.
//
bool ARegion::HasShaft()
{
    for(const auto& o: objects) {
        if (o->inner != -1) return true;
    }
    return false;
}

bool ARegion::IsGuarded()
{
    for(const auto& o: objects)
    {
        for(const auto& u: o->units)
        {
            if (u->guard == GUARD_GUARD)
            {
                return true;
            }
        }
    }
    return false;
}

int ARegion::CountWMons()
{
    int count = 0;
    for(const auto& o: objects) {
        for(const auto& u: o->units) {
            if (u->type == U_WMON) {
                count ++;
            }
        }
    }
    return count;
}

/* New Fleet objects are stored in the newfleets
 * map for resolving aliases in the Enter NEW phase.
 */
void ARegion::AddFleet(const Object::Handle& fleet)
{
    objects.push_back(fleet);
    //Awrite(AString("Setting up fleet alias #") + fleetalias + ": " + fleet->num);
    newfleets.insert(std::make_pair(fleetalias++, fleet->num));
}

int ARegion::ResolveFleetAlias(int alias)
{
    std::map<int, int>::iterator f;
    f = newfleets.find(alias);
    //Awrite(AString("Resolving Fleet Alias #") + alias + ": " + f->second);
    if (f == newfleets.end()) return -1;
    return f->second;
}

ARegionList::ARegionList()
{
    pRegionArrays.clear();
    numLevels = 0;
    numberofgates = 0;
}

void ARegionList::WriteRegions(Aoutfile *f)
{
    f->PutInt(regions_.size());

    f->PutInt(numLevels);
    for (const auto& pRegs: pRegionArrays) {
        f->PutInt(pRegs->x);
        f->PutInt(pRegs->y);
        if (pRegs->strName) {
            f->PutStr(*pRegs->strName);
        } else {
            f->PutStr("none");
        }
        f->PutInt(pRegs->levelType);
    }

    f->PutInt(numberofgates);
    for(const auto& reg: regions_)
    {
        reg->Writeout(f);
    }
    {
        f->PutStr("Neighbors");
        for(const auto& reg: regions_)
        {
            for (const auto& n: reg->neighbors) {
                if (!n.expired()) {
                    f->PutInt(n.lock()->num);
                } else {
                    f->PutInt(-1);
                }
            }
        }
    }
}

bool ARegionList::ReadRegions(Ainfile *f, const std::list<Faction::Handle>& factions, ATL_VER v)
{
    unsigned int num = f->GetInt<unsigned int>();

    numLevels = f->GetInt<unsigned int>();
    CreateLevels(numLevels);
    for (unsigned int i = 0; i < numLevels; i++) {
        unsigned int curX = f->GetInt<unsigned int>();
        unsigned int curY = f->GetInt<unsigned int>();
        AString *name = f->GetStr();
        ARegionArray::Handle pRegs = std::make_shared<ARegionArray>(curX, curY);
        if (*name == "none") {
            pRegs->strName = 0;
            delete name;
        } else {
            pRegs->strName = name;
        }
        pRegs->levelType = f->GetInt<int>();
        pRegionArrays[i] = pRegs;
    }

    numberofgates = f->GetInt<unsigned int>();

    ARegionFlatArray fa(num);

    Awrite("Reading the regions...");
    for (unsigned int i = 0; i < num; i++) {
        auto& temp = regions_.emplace_back(std::make_shared<ARegion>());
        temp->Readin(f, factions, v);
        fa.SetRegion(temp->num, temp);

        pRegionArrays[temp->zloc]->SetRegion(temp->xloc, temp->yloc, temp);
    }

    Awrite("Setting up the neighbors...");
    {
        delete f->GetStr();
        for(const auto& reg: regions_) {
            for (auto& n: reg->neighbors) {
                ssize_t j = f->GetInt<ssize_t>();
                if (j != -1) {
                    n = fa.GetRegion(static_cast<size_t>(j));
                } else {
                    n.reset();
                }
            }
        }
    }
    return true;
}

ARegion::WeakHandle ARegionList::GetRegion(size_t n)
{
    for(const auto& r: regions_) {
        if (r->num == n)
        {
            return r;
        }
    }
    return ARegion::WeakHandle();
}

ARegion::WeakHandle ARegionList::GetRegion(unsigned int x, unsigned int y, unsigned int z)
{

    if (z >= numLevels) return ARegion::WeakHandle();

    const auto& arr = pRegionArrays[z];

    x = (x + arr->x) % arr->x;
    y = (y + arr->y) % arr->y;

    return(arr->GetRegion(x, y));
}

Location::Handle ARegionList::FindUnit(size_t i)
{
    for(const auto& reg: regions_) {
        for(const auto& obj: reg->objects) {
            for(const auto& u: obj->units) {
                if (u->num == i) {
                    Location::Handle retval = std::make_shared<Location>();
                    retval->unit = u;
                    retval->region = reg;
                    retval->obj = obj;
                    return retval;
                }
            }
        }
    }
    return nullptr;
}

void ARegionList::NeighSetup(const ARegion::Handle& r, const ARegionArray::Handle& ar)
{
    r->ZeroNeighbors();

    if (r->yloc != 0 && r->yloc != 1) {
        r->neighbors[Directions::Types::D_NORTH] = ar->GetRegion(r->xloc, r->yloc - 2);
    }
    if (r->yloc != 0) {
        r->neighbors[Directions::Types::D_NORTHEAST] = ar->GetRegion(r->xloc + 1, r->yloc - 1);
        r->neighbors[Directions::Types::D_NORTHWEST] = ar->GetRegion(r->xloc - 1, r->yloc - 1);
    }
    if (r->yloc != ar->y - 1) {
        r->neighbors[Directions::Types::D_SOUTHEAST] = ar->GetRegion(r->xloc + 1, r->yloc + 1);
        r->neighbors[Directions::Types::D_SOUTHWEST] = ar->GetRegion(r->xloc - 1, r->yloc + 1);
    }
    if (r->yloc != ar->y - 1 && r->yloc != ar->y - 2) {
        r->neighbors[Directions::Types::D_SOUTH] = ar->GetRegion(r->xloc, r->yloc + 2);
    }
}

void ARegionList::IcosahedralNeighSetup(const ARegion::Handle& r, const ARegionArray::Handle& ar)
{
    unsigned int scale, x, y, x2, y2, x3, neighX, neighY;

    scale = ar->x / 10;

    r->ZeroNeighbors();

    y = r->yloc;
    x = r->xloc;
    // x2 is the x-coord of this hex inside its "wedge"
    if (y < 5 * scale)
        x2 = x % (2 * scale);
    else
        x2 = (x + 1) % (2 * scale);
    // x3 is the distance of this hex from the right side of its "wedge"
    x3 = (2 * scale - x2) % (2 * scale);
    // y2 is the distance from the SOUTH pole
    y2 = 10 * scale - 1 - y;
    // Always try to connect in the standard way...
    if (y > 1) {
        r->neighbors[Directions::Types::D_NORTH] = ar->GetRegion(x, y - 2);
    }
    // but if that fails, use the special icosahedral connections:
    if (r->neighbors[Directions::Types::D_NORTH].expired()) {
        if (y > 0 && y < 3 * scale)
        {
            if (y == 2) {
                neighX = 0;
                neighY = 0;
            }
            else if (y == 3 * x2) {
                neighX = x + 2 * (scale - x2) + 1;
                neighY = y - 1;
            }
            else {
                neighX = x + 2 * (scale - x2);
                neighY = y - 2;
            }
            neighX %= (scale * 10);
            r->neighbors[Directions::Types::D_NORTH] = ar->GetRegion(neighX, neighY);
        }
    }
    if (y > 0) {
        neighX = x + 1;
        neighY = y - 1;
        neighX %= (scale * 10);
        r->neighbors[Directions::Types::D_NORTHEAST] = ar->GetRegion(neighX, neighY);
    }
    if (r->neighbors[Directions::Types::D_NORTHEAST].expired()) {
        if (y == 0) {
            neighX = 4 * scale;
            neighY = 2;
        }
        else if (y < 3 * scale) {
            if (y == 3 * x2) {
                neighX = x + 2 * (scale - x2) + 1;
                neighY = y + 1;
            }
            else {
                neighX = x + 2 * (scale - x2);
                neighY = y;
            }
        }
        else if (y2 < 1) {
            neighX = x + 2 * scale;
            neighY = y - 2;
        }
        else if (y2 < 3 * scale) {
            neighX = x + 2 * (scale - x2);
            neighY = y - 2;
        }
        neighX %= (scale * 10);
        r->neighbors[Directions::Types::D_NORTHEAST] = ar->GetRegion(neighX, neighY);
    }
    if (y2 > 0) {
        neighX = x + 1;
        neighY = y + 1;
        neighX %= (scale * 10);
        r->neighbors[Directions::Types::D_SOUTHEAST] = ar->GetRegion(neighX, neighY);
    }
    if (r->neighbors[Directions::Types::D_SOUTHEAST].expired()) {
        if (y == 0) {
            neighX = 2 * scale;
            neighY = 2;
        }
        else if (y2 < 1) {
            neighX = x + 4 * scale;
            neighY = y - 2;
        }
        else if (y2 < 3 * scale) {
            if (y2 == 3 * x2) {
                neighX = x + 2 * (scale - x2) + 1;
                neighY = y - 1;
            }
            else {
                neighX = x + 2 * (scale - x2);
                neighY = y;
            }
        }
        else if (y < 3 * scale) {
            neighX = x + 2 * (scale - x2);
            neighY = y + 2;
        }
        neighX %= (scale * 10);
        r->neighbors[Directions::Types::D_SOUTHEAST] = ar->GetRegion(neighX, neighY);
    }
    if (y2 > 1) {
        r->neighbors[Directions::Types::D_SOUTH] = ar->GetRegion(x, y + 2);
    }
    if (r->neighbors[Directions::Types::D_SOUTH].expired()) {
        if (y2 > 0 && y2 < 3 * scale)
        {
            if (y2 == 2) {
                neighX = 10 * scale - 1;
                neighY = y + 2;
            }
            else if (y2 == 3 * x2) {
                neighX = x + 2 * (scale - x2) + 1;
                neighY = y + 1;
            }
            else {
                neighX = x + 2 * (scale - x2);
                neighY = y + 2;
            }
            neighX = (neighX + scale * 10) % (scale * 10);
            r->neighbors[Directions::Types::D_SOUTH] = ar->GetRegion(neighX, neighY);
        }
    }
    if (y2 > 0) {
        neighX = x - 1;
        neighY = y + 1;
        neighX = (neighX + scale * 10) % (scale * 10);
        r->neighbors[Directions::Types::D_SOUTHWEST] = ar->GetRegion(neighX, neighY);
    }
    if (r->neighbors[Directions::Types::D_SOUTHWEST].expired()) {
        if (y == 0) {
            neighX = 8 * scale;
            neighY = 2;
        }
        else if (y2 < 1) {
            neighX = x + 6 * scale;
            neighY = y - 2;
        }
        else if (y2 < 3 * scale) {
            if (y2 == 3 * x3 + 4) {
                neighX = x + 2 * (x3 - scale) + 1;
                neighY = y + 1;
            }
            else {
                neighX = x + 2 * (x3 - scale);
                neighY = y;
            }
        }
        else if (y < 3 * scale) {
            neighX = x - 2 * (scale - x3) + 1;
            neighY = y + 1;
        }
        neighX = (neighX + scale * 10) % (scale * 10);
        r->neighbors[Directions::Types::D_SOUTHWEST] = ar->GetRegion(neighX, neighY);
    }
    if (y > 0) {
        neighX = x - 1;
        neighY = y - 1;
        neighX = (neighX + scale * 10) % (scale * 10);
        r->neighbors[Directions::Types::D_NORTHWEST] = ar->GetRegion(neighX, neighY);
    }
    if (r->neighbors[Directions::Types::D_NORTHWEST].expired()) {
        if (y == 0) {
            neighX = 6 * scale;
            neighY = 2;
        }
        else if (y < 3 * scale) {
            if (y == 3 * x3 + 4) {
                neighX = x + 2 * (x3 - scale) + 1;
                neighY = y - 1;
            }
            else {
                neighX = x + 2 * (x3 - scale);
                neighY = y;
            }
        }
        else if (y2 < 1) {
            neighX = x + 8 * scale;
            neighY = y - 2;
        }
        else if (y2 < 3 * scale) {
            neighX = x - 2 * (scale - x3) + 1;
            neighY = y - 1;
        }
        neighX = (neighX + scale * 10) % (scale * 10);
        r->neighbors[Directions::Types::D_NORTHWEST] = ar->GetRegion(neighX, neighY);
    }
}

void ARegionList::CalcDensities()
{
    Awrite("Densities:");
    std::array<int, Regions::size()> arr = {0};

    for(const auto& reg: regions_)
    {
        arr[reg->type]++;
    }

    for (size_t i=0; i < arr.size(); ++i)
    {
        if (arr[i])
        {
            Awrite(AString(TerrainDefs[i].name) + " " + arr[i]);
        }
    }

    Awrite("");
}

void ARegionList::TownStatistics()
{
    int villages = 0;
    int towns = 0;
    int cities = 0;
    for(const auto& reg: regions_) {
        if (reg->town) {
            switch(reg->town->TownType()) {
                case TownTypeEnum::TOWN_VILLAGE:
                    villages++;
                    break;
                case TownTypeEnum::TOWN_TOWN:
                    towns++;
                    break;
                case TownTypeEnum::TOWN_CITY:
                    cities++;
                    break;
                default:
                    break;
            }    
        }
    }
    int tot = villages + towns + cities;
    int perv = villages * 100 / tot;
    int pert = towns * 100 / tot;
    int perc = cities * 100 / tot;
    Awrite(AString("Settlements: ") + tot);
    Awrite(AString("Villages: ") + villages + " (" + perv + "%)");
    Awrite(AString("Towns   : ") + towns + " (" + pert + "%)");
    Awrite(AString("Cities  : ") + cities + " (" + perc + "%)");
    Awrite("");
}

ARegion::WeakHandle ARegionList::FindGate(int x)
{
    if (x == -1)
    {
        int count = 0;

        for(const auto& r: regions_) {
            if (r->gate)
                count++;
        }
        count = getrandom(count);
        for(const auto& r: regions_) {
            if (r->gate) {
                if (!count)
                    return r;
                count--;
            }
        }
    }
    else
    {
        for(const auto& r: regions_) {
            if (r->gate == x) return r;
        }
    }
    return ARegion::WeakHandle();
}

ARegion::WeakHandle ARegionList::FindConnectedRegions(const ARegion::Handle& r, ARegion::Handle tail, bool shaft)
{
    for (const auto& n: r->neighbors)
    {
        if (!n.expired())
        {
            const auto r_sp = n.lock();
            if(!r_sp->distance.isValid())
            {
                tail->next = r_sp;
                tail = tail->next.lock();
                tail->distance = 0;
            }
        }
    }
    if (shaft) {
        for(const auto& o: r->objects)
        {
            if (o->inner != -1) {
                ARegion::WeakHandle inner = GetRegion(static_cast<size_t>(o->inner));
                if (!inner.expired())
                {
                    const auto inner_sp = inner.lock();
                    if(!inner_sp->distance.isValid())
                    {
                        tail->next = inner_sp;
                        tail = tail->next.lock();
                        tail->distance = r->distance + 1;
                    }
                }
            }
        }
    }

        return tail;
}

ARegion::WeakHandle ARegionList::FindNearestStartingCity(ARegion::WeakHandle start, Directions& dir)
{
    for(const auto& r: regions_) {
        r->distance.invalidate();
        r->next.reset();
    }

    start.lock()->distance = 0;
    ARegion::WeakHandle queue = start;
    while (!start.expired()) {
        queue = FindConnectedRegions(start.lock(), queue.lock(), true);
        bool valid = false;
        if (!start.expired()) {
            if (Globals->START_CITIES_EXIST) {
                if (start.lock()->IsStartingCity())
                {
                    valid = true;
                }
            }
            else
            {
                // No starting cities?
                // Then any explored settlement will do
                const auto start_sp = start.lock();
                if (start_sp->town && start_sp->visited)
                {
                    valid = true;
                }
            }
        }
        if (valid) {
            if (dir) {
                size_t offset = static_cast<unsigned int>(getrandom(Directions::size()));
                const auto start_sp = start.lock();
                for (auto i = Directions::begin(); i != Directions::end(); i++) {
                    const auto& r = start_sp->neighbors[(*i + offset) % Directions::size()];
                    if (r.expired())
                    {
                        continue;
                    }
                    if (r.lock()->distance + 1 == start_sp->distance) {
                        dir = (*i + offset) % Directions::size();
                        break;
                    }
                }
                for(const auto& o: start_sp->objects) {
                    if (o->inner != -1) {
                        const auto& inner = GetRegion(static_cast<size_t>(o->inner));
                        if (inner.lock()->distance + 1 == start_sp->distance) {
                            dir = Directions::MOVE_IN;
                            break;
                        }
                    }
                }
            }
            return start;
        }
        start = start.lock()->next;
    }

    // This should never happen!
    return ARegion::WeakHandle();
}

unsigned int ARegionList::GetPlanarDistance(const ARegion::Handle& one, const ARegion::Handle& two, int penalty, unsigned int maxdist)
{
    if (one->zloc == ARegionArray::LEVEL_NEXUS || two->zloc == ARegionArray::LEVEL_NEXUS)
    {
        return 10000000;
    }

    if (Globals->ABYSS_LEVEL) {
        // make sure you cannot teleport into or from the abyss
        unsigned int ablevel = Globals->UNDERWORLD_LEVELS + Globals->UNDERDEEP_LEVELS + 2;
        if (one->zloc == ablevel || two->zloc == ablevel)
        {
            return 10000000;
        }
    }

    unsigned int one_x, one_y, two_x, two_y;
    unsigned int maxy;
    const auto& pArr = pRegionArrays[ARegionArray::LEVEL_SURFACE];

    one_x = one->xloc * GetLevelXScale(one->zloc);
    one_y = one->yloc * GetLevelYScale(one->zloc);

    two_x = two->xloc * GetLevelXScale(two->zloc);
    two_y = two->yloc * GetLevelYScale(two->zloc);

    if (Globals->ICOSAHEDRAL_WORLD) {
        ARegion::WeakHandle start, target, queue;

        start = pArr->GetRegion(one_x, one_y);
        if (start.expired()) {
            one_x += GetLevelXScale(one->zloc) - 1;
            one_y += GetLevelYScale(one->zloc) - 1;
            start = pArr->GetRegion(one_x, one_y);
        }

        target = pArr->GetRegion(two_x, two_y);
        if (target.expired()) {
            two_x += GetLevelXScale(two->zloc) - 1;
            two_y += GetLevelYScale(two->zloc) - 1;
            target = pArr->GetRegion(two_x, two_y);
        }

        if (start.expired() || target.expired()) {
            // couldn't find equivalent locations on
            // the surface (this should never happen)
            Awrite(AString("Unable to find ends pathing from (") +
                one->xloc + "," +
                one->yloc + "," +
                one->zloc + ") to (" +
                two->xloc + "," +
                two->yloc + "," +
                two->zloc + ")!");
            return 10000000;
        }

        for(const auto& r: regions_) {
            r->distance.invalidate();
            r->next.reset();
        }
        
        unsigned int zdist = absdiff(one->zloc, two->zloc);

        auto start_sp  = start.lock();
        start_sp->distance = zdist * static_cast<unsigned int>(penalty);
        queue = start;
        while (maxdist == std::numeric_limits<decltype(maxdist)>::max() || static_cast<unsigned int>(start_sp->distance) <= maxdist) {
            if (start_sp->xloc == two_x && start_sp->yloc == two_y) {
                // found our target within range
                return start_sp->distance;
            }
            // add neighbours to the search list
            queue = FindConnectedRegions(start_sp, queue.lock(), 0);
            start = start_sp->next;
            if (start.expired())
            {
                // ran out of hexes to search
                // (this should never happen)
                Awrite(AString("Unable to find path from (") +
                    one->xloc + "," +
                    one->yloc + "," +
                    one->zloc + ") to (" +
                    two->xloc + "," +
                    two->yloc + "," +
                    two->zloc + ")!");
                return 10000000;
            }
            start_sp = start.lock();
        }
        // didn't find the target within range
        return start_sp->distance;
    } else {
        maxy = absdiff(one_y, two_y);

        unsigned int maxx = absdiff(one_x, two_x);

        unsigned int max2 = absdiff(one_x + pArr->x, two_x);
        if (max2 < maxx) maxx = max2;

        max2 = absdiff(one_x, two_x + pArr->x);
        if (max2 < maxx) maxx = max2;

        if (maxy > maxx) maxx = (maxx+maxy)/2;

        if (one->zloc != two->zloc) {
            unsigned int zdist = (one->zloc - two->zloc);
            if ((two->zloc - one->zloc) > zdist)
                zdist = two->zloc - one->zloc;
            maxx += (static_cast<unsigned int>(penalty) * zdist);
        }

        return maxx;
    }
}

const ARegionArray::Handle& ARegionList::GetRegionArray(size_t level)
{
    return pRegionArrays[level];
}

void ARegionList::CreateLevels(unsigned int n)
{
    numLevels = n;
    pRegionArrays.resize(n);
}

ARegionArray::ARegionArray(unsigned int xx, unsigned int yy)
{
    x = xx;
    y = yy;
    regions.resize(x * y / 2 + 1);
    strName = 0;

    for (unsigned int i = 0; i < x * y / 2; i++)
    {
        regions[i].reset();
    }
}

ARegionArray::~ARegionArray()
{
    if (strName) delete strName;
}

void ARegionArray::SetRegion(unsigned int xx, unsigned int yy, const ARegion::WeakHandle& r)
{
    regions[xx / 2 + yy * x / 2] = r;
}

ARegion::WeakHandle ARegionArray::GetRegion(unsigned int xx, unsigned int yy)
{
    xx = (xx + x) % x;
    yy = (yy + y) % y;
    if ((xx + yy) % 2)
    {
        return ARegion::WeakHandle();
    }
    return regions[xx / 2 + yy * x / 2];
}

int ARegion::calculateWagesWithRatio(float ratio, int multiplier)
{
    return Market::calculateWagesWithRatio(Wages(), ratio, multiplier);
}

void ARegionArray::SetName(char const *name)
{
    if (name) {
        strName = new AString(name);
    } else {
        delete strName;
        strName = 0;
    }
}

ARegionFlatArray::ARegionFlatArray(size_t s)
{
    size = s;
    regions.resize(s);
}

void ARegionFlatArray::SetRegion(size_t x, const ARegion::Handle& r) {
    regions[x] = r;
}

ARegion::WeakHandle ARegionFlatArray::GetRegion(size_t x) {
    return regions[x];
}

Regions ParseTerrain(AString *token)
{
    for (auto i = Regions::begin(); i != Regions::end(); ++i)
    {
        if (*token == TerrainDefs[*i].type)
        {
            return *i;
        }
    }

    for (auto i = Regions::begin(); i != Regions::end(); ++i)
    {
        if (*token == TerrainDefs[*i].name)
        {
            return *i;
        }
    }

    return Regions();
}

