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
// MODIFICATIONS
// Date        Person            Comments
// ----        ------            --------
// 2000/MAR/16 Davis Kulis       Added a new reporting Template.
// 2000/MAR/21 Azthar Septragen  Added roads.
#ifndef REGION_CLASS
#define REGION_CLASS

class ARegion;
class ARegionList;
class ARegionArray;

#include <limits>
#include <map>
#include <list>
#include <vector>
#include <array>
#include <memory>

#include "gamedataarray.h"
#include "gamedefs.h"
#include "gameio.h"
#include "faction.h"
#include "alist.h"
#include "unit.h"
#include "fileio.h"
#include "production.h"
#include "market.h"
#include "object.h"
#include "regiontype.h"
#include "validvalue.h"

/* Weather Types */
enum {
    W_NORMAL,
    W_WINTER,
    W_MONSOON,
    W_BLIZZARD
};

struct Product
{
    Items product;
    int chance;
    int amount;
};

class TerrainType
{
    public:
        char const *name;
        char const *type;
        char marker;
        Regions similar_type;

        enum {
            RIDINGMOUNTS = 0x1,
            FLYINGMOUNTS = 0x2,
            BARREN       = 0x4,
        };
        int flags;

        int pop;
        int wages;
        unsigned int economy;        
        int movepoints;
        std::array<Product, 7> prods;
        // Race information
        // A hex near water will have either one of the normal races or one
        // of the coastal races in it.   Non-coastal hexes will only have one
        // of the normal races.
        std::array<Items, 4> races;
        std::array<Items, 3> coastal_races;
        int wmonfreq;
        Items smallmon;
        Items bigmon;
        Items humanoid;
        int lairChance;
        std::array<Objects, 6> lairs;
};

extern const GameDataArray<TerrainType> TerrainDefs;

class Location
{
    public:
        using Handle = std::shared_ptr<Location>;
        using WeakHandle = std::weak_ptr<Location>;

        std::weak_ptr<Unit> unit;
        std::weak_ptr<Object> obj;
        std::weak_ptr<ARegion> region;
};

Location::WeakHandle GetUnit(const std::list<Location::Handle>&, size_t);

char const *AGetNameString(int name);

class Farsight
{
    public:
        using Handle = std::shared_ptr<Farsight>;
        using WeakHandle = std::weak_ptr<Farsight>;

        Farsight();

        std::weak_ptr<Faction> faction;
        std::weak_ptr<Unit> unit;
        int level;
        int observation;
        std::array<bool, static_cast<size_t>(Directions::NDIRS)> exits_used;
};

Farsight::WeakHandle GetFarsight(const std::list<Farsight::Handle>&, const Faction&);

enum class TownTypeEnum {
    TOWN_VILLAGE = 0,
    TOWN_TOWN = 1,
    TOWN_CITY = 2,
    NTOWNS = 3
};

class TownInfo
{
    public:
        TownInfo();
        ~TownInfo();

        void Readin(Ainfile *, ATL_VER &);
        void Writeout(Aoutfile *);
        TownTypeEnum TownType();

        AString *name;
        int pop;
        int activity;
        // achieved settled habitat
        int hab;
        // town's development
        int dev;
};

class ARegion : std::enable_shared_from_this<ARegion>
{
    friend class Game;
    friend class ARegionArray;

    public:
        using Handle = std::shared_ptr<ARegion>;
        using WeakHandle = std::weak_ptr<ARegion>;

        ARegion();
        ARegion(int, int);
        ~ARegion();
        void Setup();

        void ZeroNeighbors();
        void SetName(char const *);

        void Writeout(Aoutfile *);
        void Readin(Ainfile *, const std::list<std::shared_ptr<Faction>>&, ATL_VER v);

        bool CanMakeAdv(const Faction&, const Items&);
        bool HasItem(const Faction&, const Items&);
        void WriteProducts(Areport *, const Faction&, bool);
        void WriteMarkets(Areport *, const Faction&, bool);
        void WriteEconomy(Areport *, const Faction&, bool);
        void WriteExits(Areport *, const ARegionList& pRegs, const std::array<bool, ALL_DIRECTIONS.size()>& exits_seen);
        void WriteReport(Areport *f, const Faction& fac, int month,
                const ARegionList& pRegions);
        // DK
        void WriteTemplate(Areport *, const Faction&, const ARegionList& , int);
        void WriteTemplateHeader(Areport *, const Faction&, const ARegionList& , int);
        void GetMapLine(char *, int, const ARegionList& );

        AString ShortPrint(const ARegionList& pRegs);
        AString Print(const ARegionList& pRegs);

        void Kill(const std::shared_ptr<Unit>&);
        void ClearHell();

        std::weak_ptr<Unit> GetUnit(int);
        std::weak_ptr<Unit> GetUnitAlias(int, int); /* alias, faction number */
        std::weak_ptr<Unit> GetUnitId(const UnitId&, size_t);
        void DeduplicateUnitList(std::list<UnitId>&, size_t);
        Location::Handle GetLocation(const UnitId&, size_t) const;

        void SetLoc(unsigned int, unsigned int, unsigned int);
        bool Present(const Faction&);
        std::list<std::weak_ptr<Faction>> PresentFactions();
        int GetObservation(const Faction&, bool);
        int GetTrueSight(const Faction&, bool);

        std::weak_ptr<Object> GetObject(int);
        std::weak_ptr<Object> GetDummy();
        void CheckFleets();

        int MoveCost(int, ARegion *, Directions, AString *road);
        std::weak_ptr<Unit> Forbidden(const std::shared_ptr<Unit>&); /* Returns unit that is forbidding */
        std::weak_ptr<Unit> ForbiddenByAlly(const std::shared_ptr<Unit>&); /* Returns unit that is forbidding */
        bool CanTax(const std::shared_ptr<Unit>&);
        bool CanPillage(const std::shared_ptr<Unit>&);
        void Pillage();
        bool ForbiddenShip(const std::shared_ptr<Object>&);
        bool HasCityGuard();
        bool NotifySpell(const std::shared_ptr<Unit>&, char const *, const ARegionList& pRegs);
        void NotifyCity(const std::shared_ptr<Unit>&, AString& oldname, AString& newname);

        void DefaultOrders();
        int TownGrowth();
        void PostTurn(const ARegionList& pRegs);
        void UpdateProducts();
        void SetWeather(int newWeather);
        unsigned int IsCoastal();
        unsigned int IsCoastalOrLakeside();
        void MakeStartingCity();
        bool IsStartingCity();
        bool IsSafeRegion();
        bool CanBeStartingCity(const ARegionArray& pRA);
        bool HasShaft();

        // AS
        bool HasRoad();
        bool HasExitRoad(Directions realDirection);
        int CountConnectingRoads();
        bool HasConnectingRoad(Directions realDirection);
        Objects GetRoadDirection(Directions realDirection);
        Directions GetRealDirComp(Directions realDirection);
        void DoDecayCheck(const ARegionList& pRegs);
        void DoDecayClicks(const std::shared_ptr<Object>& o, const ARegionList& pRegs);
        void RunDecayEvent(const std::shared_ptr<Object>& o, const ARegionList& pRegs);
        AString GetDecayFlavor();
        int GetMaxClicks();
        int PillageCheck();

        // JR
        unsigned int GetPoleDistance(Directions dir);
        void SetGateStatus(int month);
        void DisbandInRegion(const Items&, int);
        void Recruit(int);
        bool IsNativeRace(const Items&);
        void AdjustPop(int);
        void FindMigrationDestination(int round);
        int MigrationAttractiveness(int, int, int);
        void Migrate();
        void SetTownType(TownTypeEnum);
        TownTypeEnum DetermineTownSize();
        int TraceConnectedRoad(Directions, int, std::list<std::weak_ptr<ARegion>>&, int, int);
        int RoadDevelopmentBonus(int, int);
        int BaseDev();
        int ProdDev();
        int TownHabitat();
        int RoadDevelopment();
        int TownDevelopment();
        int CheckSea(Directions, unsigned int, int);
        int Slope();
        int SurfaceWater();
        int Soil();
        int Winds();
        int TerrainFactor(int, int);
        int TerrainProbability(int);
        void AddFleet(const std::shared_ptr<Object>&);
        int ResolveFleetAlias(int);

        int CountWMons();
        bool IsGuarded();

        int Wages();
        int calculateWagesWithRatio(float ratio, int multiplier = 4);
        AString WagesForReport();
        int Population();


        AString *name;
        size_t num;
        Regions type;
        int buildingseq;
        int weather;
        int gate;
        int gatemonth;
        int gateopen;

        TownInfo *town;
        Items race;
        int population;
        int basepopulation;
        int wages;
        int maxwages;
        int wealth;
        
        /* Economy */
        int habitat;
        int development;
        int elevation;
        int humidity;
        int temperature;
        int vegetation;
        int culture;
        // migration origins
        std::list<ARegion::WeakHandle> migfrom;
        // mid-way migration development
        int migdev;
        int immigrants;
        int emigrants;
        // economic improvement
        int improvement;
        
        /* Potential bonuses to economy */
        int clearskies;
        int earthlore;

        std::array<ARegion::WeakHandle, static_cast<size_t>(Directions::NDIRS)> neighbors;
        std::list<std::shared_ptr<Object>> objects;
        std::map<int,int> newfleets;
        int fleetalias;
        std::list<std::shared_ptr<Unit>> hell; /* Where dead units go */
        std::list<Farsight::Handle> farsees;
        // List of units which passed through the region
        std::list<Farsight::Handle> passers;
        ProductionList products;
        MarketList markets;
        unsigned int xloc, yloc, zloc;
        int visited;

        // Used for calculating distances using an A* search
        ValidValue<unsigned int> distance;
        ARegion::WeakHandle next;

        // Editing functions
        void UpdateEditRegion();
        void SetupEditRegion();

    private:
        /* Private Setup Functions */
        void SetupPop();
        void SetupProds();
        void SetIncome();
        void Grow();
        unsigned int GetNearestProd(const Items&);
        void SetupCityMarket();
        void AddTown();
        void AddTown(TownTypeEnum);
        void AddTown(AString *);
        void AddTown(TownTypeEnum, AString *);
        std::shared_ptr<Object>& AddObject();
        void MakeLair(const Objects&);
        void LairCheck();

};

int AGetName(int town, const ARegion::Handle& r);

ARegion::WeakHandle GetRegion(const std::list<ARegion::WeakHandle>&, size_t);

class ARegionArray
{
    public:
        using Handle = std::shared_ptr<ARegionArray>;

        ARegionArray(unsigned int, unsigned int);
        ~ARegionArray();

        void SetRegion(unsigned int, unsigned int, const ARegion::WeakHandle&);
        ARegion::WeakHandle GetRegion(unsigned int, unsigned int);
        void SetName(char const *name);

        unsigned int x;
        unsigned int y;
        std::vector<ARegion::WeakHandle> regions;
        AString *strName;

        enum {
            LEVEL_NEXUS,
            LEVEL_SURFACE,
            LEVEL_UNDERWORLD,
            LEVEL_UNDERDEEP,
        };
        int levelType;
};

class ARegionFlatArray
{
    public:
        ARegionFlatArray(size_t);
        ~ARegionFlatArray() = default;

        void SetRegion(size_t, const ARegion::Handle&);
        ARegion::WeakHandle GetRegion(size_t);

        size_t size;
        std::vector<ARegion::Handle> regions;
};

struct Geography
{
    int elevation;
    int humidity;
    int temperature;
    int vegetation;
    int culture;
};

class GeoMap
{
    public:
        GeoMap(int, int);
        void Generate(int spread, int smoothness);
        int GetElevation(int, int);
        int GetHumidity(int, int);
        int GetTemperature(int, int);
        int GetVegetation(int, int);
        int GetCulture(int, int);
        void ApplyGeography(ARegionArray *pArr);
        
        int size, xscale, yscale, xoff, yoff;
        std::map<long int,Geography> geomap;
        
};

class ARegionList
{
    public:
        ARegionList();
        ~ARegionList() = default;

        ARegion::WeakHandle GetRegion(size_t);
        ARegion::WeakHandle GetRegion(unsigned int, unsigned int, unsigned int);
        bool ReadRegions(Ainfile *f, const std::list<std::shared_ptr<Faction>>&, ATL_VER v);
        void WriteRegions(Aoutfile *f);
        Location::Handle FindUnit(size_t);
        Location::Handle GetUnitId(const UnitId& id, size_t faction, const ARegion& cur);

        void ChangeStartingCity(ARegion *, int);
        ARegion::WeakHandle GetStartingCity(const ARegion& AC, size_t num, unsigned int level, unsigned int maxX, unsigned int maxY);

        ARegion::WeakHandle FindGate(int);
        unsigned int GetPlanarDistance(const ARegion::Handle&, const ARegion::Handle&, int penalty, unsigned int maxdist = std::numeric_limits<unsigned int>::max());
        int GetWeather(const ARegion& pReg, int month) const;

        const ARegionArray::Handle& GetRegionArray(size_t level);

        using iterator = std::list<ARegion::Handle>::iterator;
        using const_iterator = std::list<ARegion::Handle>::const_iterator;
        const ARegion::Handle& front() const { return regions_.front(); }
        iterator begin() { return regions_.begin(); }
        iterator end() { return regions_.end(); }
        const_iterator begin() const { return regions_.begin(); }
        const_iterator end() const { return regions_.end(); }
        const_iterator cbegin() const { return regions_.cbegin(); }
        const_iterator cend() const { return regions_.cend(); }
        size_t size() const { return regions_.size(); }
        bool empty() const { return regions_.empty(); }


        unsigned int numberofgates;
        unsigned int numLevels;
        std::vector<ARegionArray::Handle> pRegionArrays;

    public:
        //
        // Public world creation stuff
        //
        void CreateLevels(unsigned int numLevels);
        void CreateAbyssLevel(unsigned int level, char const *name);
        void CreateNexusLevel(unsigned int level, unsigned int xSize, unsigned int ySize, char const *name);
        void CreateSurfaceLevel(unsigned int level, unsigned int xSize, unsigned int ySize, char const *name);
        void CreateIslandLevel(unsigned int level, unsigned int nPlayers, char const *name);
        void CreateUnderworldLevel(unsigned int level, unsigned int xSize, unsigned int ySize, char const *name);
        void CreateUnderdeepLevel(unsigned int level, unsigned int xSize, unsigned int ySize, char const *name);

        void MakeShaftLinks(unsigned int levelFrom, unsigned int levelTo, unsigned int odds);
        void SetACNeighbors(unsigned int levelSrc, unsigned int levelTo, unsigned int maxX, unsigned int maxY);
        ARegion::WeakHandle FindConnectedRegions(const ARegion::Handle& r, ARegion::Handle tail, bool shaft);
        ARegion::WeakHandle FindNearestStartingCity(ARegion::WeakHandle r, unsigned int *dir);
        void FixUnconnectedRegions();
        void InitSetupGates(unsigned int level);
        void FinalSetupGates();

        // JR
        void InitGeographicMap(const ARegionArray::Handle& pRegs);
        void CleanUpWater(const ARegionArray::Handle& pRegs);
        void RemoveCoastalLakes(const ARegionArray::Handle& pRegs);
        void SeverLandBridges(const ARegionArray::Handle& pRegs);
        void RescaleFractalParameters(const ARegionArray::Handle& pArr);
        void SetFractalTerrain(const ARegionArray::Handle& pArr);
        void NameRegions(const ARegionArray::Handle& pArr);
        void UnsetRace(const ARegionArray::Handle& pRegs);
        void RaceAnchors(const ARegionArray::Handle& pRegs);
        void GrowRaces(const ARegionArray::Handle& pRegs);
        
        void TownStatistics();

        void CalcDensities();
        unsigned int GetLevelXScale(unsigned int level);
        unsigned int GetLevelYScale(unsigned int level);

    private:
        //
        // Private world creation stuff
        //
        void MakeRegions(unsigned int level, unsigned int xSize, unsigned int ySize);
        void SetupNeighbors(const ARegionArray::Handle& pRegs);
        void NeighSetup(const ARegion::Handle& r, const ARegionArray::Handle& ar);
        void MakeIcosahedralRegions(unsigned int level, unsigned int xSize, unsigned int ySize);
        void SetupIcosahedralNeighbors(const ARegionArray::Handle& pRegs);
        void IcosahedralNeighSetup(const ARegion::Handle& r, const ARegionArray::Handle& ar);

        void SetRegTypes(const ARegionArray::Handle& pRegs, const Regions& newType);
        void MakeLand(const ARegionArray::Handle& pRegs, unsigned int percentOcean, unsigned int continentSize);
        void MakeCentralLand(const ARegionArray::Handle& pRegs);

        void SetupAnchors(const ARegionArray::Handle& pArr);
        void GrowTerrain(const ARegionArray::Handle& pArr, bool growOcean);
        void RandomTerrain(const ARegionArray::Handle& pArr);
        void MakeUWMaze(const ARegionArray::Handle& pArr);
        void MakeIslands(const ARegionArray::Handle& pArr, unsigned int nPlayers);
        void MakeOneIsland(const ARegionArray::Handle& pRegs, unsigned int xx, unsigned int yy);

        void AssignTypes(const ARegionArray::Handle& pArr);
        void FinalSetup(const ARegionArray::Handle&);

        void MakeShaft(const ARegion::Handle& reg, const ARegionArray::Handle& pFrom, const ARegionArray::Handle& pTo);

        //
        // Game-specific world stuff (see world.cpp)
        //
        Regions GetRegType(const ARegion::Handle& pReg);
        int CheckRegionExit(const ARegion::Handle& pFrom, const ARegion::Handle& pTo);
        std::list<ARegion::Handle> regions_;
};

template<typename T>
T absdiff(const T& a, const T& b)
{
    return (a > b) ? (a - b) : (b - a);
}

Regions LookupRegionType(AString *);
Regions ParseTerrain(AString *);

#endif
