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

#ifndef GAME_CLASS
#define GAME_CLASS

class Game;

#include <vector>

#include "aregion.h"
#include "regiontype.h"
#include "itemtype.h"
#include "objecttype.h"
#include "ordertype.h"
#include "skilltype.h"
#include "alist.h"
#include "faction.h"
#include "production.h"
#include "object.h"

#define CURRENT_ATL_VER MAKE_ATL_VER(5, 1, 0)

class OrdersCheck
{
public:
    OrdersCheck();

    Aoutfile *pCheckFile;
    Unit dummyUnit;
    Faction dummyFaction;
    Order dummyOrder;
    int numshows;
    int numerrors;

    void Error(const AString &error);
};

/// The main game class
/** Currently this doc is here to switch on the class so that
I can see what the modify.cpp docs look like.
*/
class Game
{
    friend class Faction;
public:
    Game();
    ~Game();

    int NewGame();
    int OpenGame();
    void DummyGame();

    void DefaultWorkOrder();

    int RunGame();
    int EditGame(bool& pSaveGame);
    int SaveGame();
    int WritePlayers();
    int ReadPlayers();
    int ReadPlayersLine(AString *pToken, AString *pLine, const Faction::Handle& pFac,
                         int newPlayer);
    void WriteNewFac(const Faction::Handle& pFac);

    int ViewMap(const AString &, const AString &);
    // LLS
    void UnitFactionMap();
    int GenRules(const AString &, const AString &, const AString &);
    int DoOrdersCheck(const AString &strOrders, const AString &strCheck);

    Faction::Handle AddFaction(int noleader=0, const ARegion::Handle& pStart = nullptr);

    //
    // Give this particular game a chance to set up the faction. This is in
    // extra.cpp.
    //
    int SetupFaction(const Faction::Handle& pFac);

    void ViewFactions();

    //
    // Get a new unit, with its number assigned.
    //
    Unit::Handle GetNewUnit(const Faction::Handle& fac, int an = 0);

    //
    // Setup the array of units.
    //
    void SetupUnitSeq();
    void SetupUnitNums();

    //
    // Get a unit by its number.
    //
    Unit::WeakHandle GetUnit(int num);
    Unit::WeakHandle GetUnit(size_t num);

    // Handle special gm unit modification functions
    Unit::WeakHandle ParseGMUnit(AString *tag, const Faction::Handle& pFac);

    size_t TurnNumber();

    // JLT
    // Functions to allow enabling/disabling parts of the data tables
    void ModifyTablesPerRuleset(void);

private:
    //
    // Game editing functions.
    //
    ARegion::WeakHandle EditGameFindRegion();
    void EditGameFindUnit();
    void EditGameCreateUnit();
    void EditGameRegion(const ARegion::Handle& pReg);
    void EditGameRegionObjects(const ARegion::Handle& pReg);
    void EditGameRegionTerrain(const ARegion::Handle& pReg );
    void EditGameRegionMarkets(const ARegion::Handle& pReg );
    void EditGameUnit(const Unit::Handle& pUnit);
    void EditGameUnitItems(const Unit::Handle& pUnit);
    void EditGameUnitSkills(const Unit::Handle& pUnit);
    void EditGameUnitMove(const Unit::Handle& pUnit);
    void EditGameUnitDetails(const Unit::Handle& pUnit);
    
    void PreProcessTurn();
    void ReadOrders();
    void RunOrders();
    void ClearOrders(const Faction::Handle&);
    void MakeFactionReportLists();
    void CountAllSpecialists();
    //void CountAllMages();
    //void CountAllApprentices();
    //void CountAllQuarterMasters();
    //void CountAllTacticians();
    void WriteReport();
    // LLS - write order templates
    void WriteTemplates();

    void DeleteDeadFactions();

    //
    // Standard creation functions.
    //
    void CreateCityMons();
    void CreateWMons();
    void CreateLMons();
    void CreateVMons();
    Unit::Handle MakeManUnit(const Faction::Handle&, const Items&, size_t, int, int, int, int);

    //
    // Game-specific creation functions (see world.cpp).
    //
    void CreateWorld();
    void CreateNPCFactions();
    void CreateCityMon(const ARegion::Handle& pReg, size_t percent, int needmage);
    int MakeWMon(ARegion *pReg);
    void MakeLMon(Object *pObj);

    void WriteSurfaceMap(Aoutfile *f, const ARegionArray::Handle& pArr, int type);
    void WriteUnderworldMap(Aoutfile *f, const ARegionArray::Handle& pArr, int type);
    char GetRChar(const ARegion::Handle& r);
    AString GetXtraMap(const ARegion::Handle&, int);

    // LLS
    // Functions to do upgrades to the ruleset -- should be in extras.cpp
    int UpgradeMajorVersion(ATL_VER savedVersion);
    int UpgradeMinorVersion(ATL_VER savedVersion);
    int UpgradePatchLevel(ATL_VER savedVersion);

    // JLT
    // Functions to allow enabling/disabling parts of the data tables
    void EnableSkill(const Skills& sk); // Enabled a disabled skill
    void DisableSkill(const Skills& sk);  // Prevents skill being studied or used
    void ModifySkillDependancy(const Skills& sk, int i, char const *dep, int lev);
    void ModifySkillFlags(const Skills& sk, int flags);
    void ModifySkillCost(const Skills& sk, int cost);
    void ModifySkillSpecial(const Skills& sk, char const *special);
    void ModifySkillRange(const Skills& sk, char const *range);

    void EnableItem(const Items& it); // Enables a disabled item
    void DisableItem(const Items& it); // Prevents item being generated/produced
    void ModifyItemFlags(const Items& it, int flags);
    void ModifyItemType(const Items& it, int type);
    void ModifyItemWeight(const Items& it, int weight);
    void ModifyItemBasePrice(const Items& it, int price);
    void ModifyItemCapacities(const Items& it, int walk, int ride, int fly, int swim);
    void ModifyItemSpeed(const Items& it, int speed);
    void ModifyItemProductionBooster(const Items& it, const Items& item, int bonus);
    void ModifyItemHitch(const Items& it, const Items& item, int bonus);
    void ModifyItemProductionSkill(const Items& it, char *sk, int lev);
    void ModifyItemProductionOutput(const Items& it, int months, int count);
    void ModifyItemProductionInput(const Items& it, int i, const Items& input, int amount);
    void ModifyItemMagicSkill(const Items& it, char *sk, int lev);
    void ModifyItemMagicOutput(const Items& it, int count);
    void ModifyItemMagicInput(const Items& it, int i, const Items& input, int amount);
    void ModifyItemEscape(const Items& it, int escape, char const *skill, int val);

    void ModifyRaceSkillLevels(char const *race, int special, int def);
    void ModifyRaceSkills(char const *race, int i, char const *sk);

    void ModifyMonsterAttackLevel(char const *mon, int lev);
    void ModifyMonsterDefense(char const *mon, int defenseType, int level);
    void ModifyMonsterAttacksAndHits(char const *mon, int num, int hits, int regen);
    void ModifyMonsterSkills(char const *mon, int tact, int stealth, int obs);
    void ModifyMonsterSpecial(char const *mon, char const *special, int lev);
    void ModifyMonsterSpoils(char const *mon, int silver, int spoilType);
    void ModifyMonsterThreat(char const *mon, int num, int hostileChance);

    void ModifyWeaponSkills(char const *weap, char *baseSkill, char *orSkill);
    void ModifyWeaponFlags(char const *weap, int flags);
    void ModifyWeaponAttack(char const *weap, int wclass, int attackType, int numAtt);
    void ModifyWeaponBonuses(char const *weap, int attack, int defense, int vsMount);

    void ModifyArmorFlags(char const *armor, int flags);
    void ModifyArmorSaveFrom(char const *armor, int from);
    void ModifyArmorSaveValue(char const *armor, int wclass, int val);

    void ModifyMountSkill(char const *mount, char *skill);
    void ModifyMountBonuses(char const *mount, int min, int max, int hampered);
    void ModifyMountSpecial(char const *mount, char const *special, int level);

    void EnableObject(const Objects& ob); // Enables a disabled object
    void DisableObject(const Objects& ob); // Prevents object being built
    void ModifyObjectFlags(const Objects& ob, int flags);
    void ModifyObjectDecay(const Objects& ob, int maxMaint, int maxMonthDecay, int mFact);
    void ModifyObjectProduction(const Objects& ob, const Items& it);
    void ModifyObjectMonster(const Objects& ob, const Items& monster);
    void ModifyObjectConstruction(const Objects& ob, const ObjectTypeItems& it, int num, char const *sk, int lev);
    void ModifyObjectManpower(const Objects& ob, int prot, int cap, int sail, int mages);
    void ModifyObjectDefence(const Objects& ob, int co, int en, int sp, int we, int ri, int ra);
    void ModifyObjectName(const Objects& ob, char const *name);

    void ClearTerrainRaces(const Regions& t);
    void ModifyTerrainRace(const Regions& t, int i, Items r);
    void ModifyTerrainCoastRace(const Regions& t, int i, Items r);
    void ClearTerrainItems(const Regions& t);
    void ModifyTerrainItems(const Regions& t, int i, const Items& p, int c, int a);
    void ModifyTerrainWMons(const Regions& t, int freq, const Items& smon, const Items& bigmon, const Items& hum);
    void ModifyTerrainLairChance(const Regions& t, int chance);
    void ModifyTerrainLair(const Regions& t, int i, const Objects& lair);
    void ModifyTerrainEconomy(const Regions& t, int pop, int wages, int econ, int move);

    void ModifyBattleItemFlags(char const *item, int flags);
    void ModifyBattleItemSpecial(char const *item, char const *special, int level);

    void ModifySpecialTargetFlags(char const *special, int targetflags);
    void ModifySpecialTargetObjects(char const *special, int index, const Objects& obj);
    void ModifySpecialTargetItems(char const *special, int index, const Items& item);
    void ModifySpecialTargetEffects(char const *special, int index, char const *effect);
    void ModifySpecialEffectFlags(char const *special, int effectflags);
    void ModifySpecialShields(char const *special, int index, int type);
    void ModifySpecialDefenseMods(char const *special, int index, int type, int val);
    void ModifySpecialDamage(char const *special, int index, int type, int min,
            int val, int flags, int cls, char const *effect);

    void ModifyEffectFlags(char const *effect, int flags);
    void ModifyEffectAttackMod(char const *effect, int val);
    void ModifyEffectDefenseMod(char const *effect, int index, int type, int val);
    void ModifyEffectCancelEffect(char const *effect, char *uneffect);

    void ModifyRangeFlags(char const *range, int flags);
    void ModifyRangeClass(char const *range, int rclass);
    void ModifyRangeMultiplier(char const *range, int mult);
    void ModifyRangeLevelPenalty(char const *range, int pen);

    void ModifyAttribMod(char const *mod, int index, int flags, char const *ident,
            int type, int val);
    void ModifyHealing(int level, int patients, int success);

    std::list<Faction::Handle> factions;
    std::list<AString> newfactions; /* List of strings */
    std::list<Battle::Handle> battles;
    ARegionList regions;
    size_t factionseq;
    size_t unitseq;
    std::vector<Unit::Handle> ppUnits;
    size_t maxppunits;
    int shipseq;
    size_t year;
    ValidValue<size_t> month;

    enum {
        GAME_STATUS_UNINIT,
        GAME_STATUS_NEW,
        GAME_STATUS_RUNNING,
        GAME_STATUS_FINISHED,
    };
    int gameStatus;

    size_t guardfaction;
    size_t monfaction;
    int doExtraInit;
    
    //
    // Parsing functions
    //
    void ParseError(OrdersCheck *pCheck, const Unit::Handle&pUnit, Faction *pFac,
                     const AString &strError);
    UnitId *ParseUnit(AString *s);
    int ParseDir(AString *token);


    void ParseOrders(size_t faction, Aorders *ordersFile, OrdersCheck *pCheck);
    void ProcessOrder(Orders orderNum, const Unit::Handle& unit, AString *order,
                       OrdersCheck *pCheck);
    void ProcessMoveOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessAdvanceOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    Unit::WeakHandle ProcessFormOrder(const Unit::Handle& former, AString *order,
                       OrdersCheck *pCheck, int atsign);
    void ProcessAddressOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessAvoidOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessGuardOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessNameOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessDescribeOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessBehindOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessGiveOrder(int, const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessWithdrawOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessDeclareOrder(Faction *, AString *, OrdersCheck *pCheck);
    void ProcessStudyOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessTeachOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessWorkOrder(const Unit::Handle&, int quiet, OrdersCheck *pCheck);
    void ProcessProduceOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessBuyOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessSellOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessAttackOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessBuildOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessSailOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessEnterOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessLeaveOrder(const Unit::Handle&, OrdersCheck *pCheck);
    void ProcessPromoteOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessEvictOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessTaxOrder(const Unit::Handle&, OrdersCheck *pCheck);
    void ProcessPillageOrder(const Unit::Handle&, OrdersCheck *pCheck);
    void ProcessConsumeOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessRevealOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessFindOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessDestroyOrder(const Unit::Handle&, OrdersCheck *pCheck);
    void ProcessQuitOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessRestartOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessAssassinateOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessStealOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessFactionOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessClaimOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessCombatOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessPrepareOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessWeaponOrder(const Unit::Handle& u, AString *o, OrdersCheck *pCheck);
    void ProcessArmorOrder(const Unit::Handle& u, AString *o, OrdersCheck *pCheck);
    void ProcessCastOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessEntertainOrder(const Unit::Handle&, OrdersCheck *pCheck);
    void ProcessForgetOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessReshowOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessHoldOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessNoaidOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessNocrossOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessNospoilsOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessSpoilsOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessAutoTaxOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessOptionOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessPasswordOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessExchangeOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessIdleOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessTransportOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessDistributeOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    void ProcessShareOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);
    AString *ProcessTurnOrder(const Unit::Handle&, Aorders *, OrdersCheck *pCheck, int);
    void ProcessJoinOrder(const Unit::Handle&, AString *, OrdersCheck *pCheck);

    void RemoveInactiveFactions();

    //
    // Game running functions
    //

    //
    // This can be called by parse functions
    //
    size_t CountMages(const Faction::Handle&);
    size_t CountApprentices(const Faction::Handle&);
    size_t CountQuarterMasters(const Faction::Handle&);
    size_t CountTacticians(const Faction::Handle&);
    
    void FindDeadFactions();
    void DeleteEmptyUnits();
    void DeleteEmptyInRegion(const ARegion::Handle&);
    void EmptyHell();
    void DoGuard1Orders();
    void DoGiveOrders();
    void DoWithdrawOrders();

    void WriteTimesArticle(AString);

    void DoExchangeOrders();
    void DoExchangeOrder(const ARegion::Handle&, const Unit::Handle&, ExchangeOrder *);

    //
    // Faction limit functions.
    //
    // The first 4 are game specific and can be found in extra.cpp. They
    // may return -1 to indicate no limit.
    //
    static int getAllowedPoints(int p, const std::vector<int>& allowed);
    int AllowedMages(const Faction& pFac);
    int AllowedApprentices(const Faction& pFact);
    int AllowedQuarterMasters(const Faction& pFact);
    int AllowedTacticians(const Faction& pFact);
    int AllowedTaxes(const Faction& pFac);
    int AllowedTrades(const Faction& pFac);
    int TaxCheck(const ARegion::Handle& pReg, const Faction::Handle& pFac);
    int TradeCheck(const ARegion::Handle& pReg, const Faction::Handle& pFac);

    //
    // The DoGiveOrder returns 0 normally, or 1 if no more GIVE orders
    // should be allowed
    //
    int DoGiveOrder(const ARegion::Handle&, const Unit::Handle&, GiveOrder *);
    //
    // The DoWithdrawOrder returns 0 normally, or 1 if no more WITHDRAW
    // orders should be allowed
    //
    int DoWithdrawOrder(const ARegion::Handle&, const Unit::Handle&, WithdrawOrder *);

    //
    // These are game specific, and can be found in extra.cpp
    //
    void CheckUnitMaintenance(int consume);
    void CheckFactionMaintenance(int consume);
    void CheckAllyMaintenance();

    // Similar to the above, but for minimum food requirements
    void CheckUnitHunger();
    void CheckFactionHunger();
    void CheckAllyHunger();

    void CheckUnitMaintenanceItem(const Items& item, int value, int consume);
    void CheckFactionMaintenanceItem(const Items& item, int value, int consume);
    void CheckAllyMaintenanceItem(const Items& item, int value);

    // Hunger again
    void CheckUnitHungerItem(const Items& item, int value);
    void CheckFactionHungerItem(const Items& item, int value);
    void CheckAllyHungerItem(const Items& item, int value);

    void AssessMaintenance();

    void GrowWMons(int);
    void GrowLMons(int);
    void GrowVMons();
    void PostProcessUnit(ARegion *, Unit *);
    void MidProcessUnit(ARegion *, Unit *);

    //
    // Mid and PostProcessUnitExtra can be used to provide game-specific
    // unit processing at the approrpriate times.
    //
    void MidProcessUnitExtra(ARegion *, Unit *);
    void MidProcessTurn();
    void PostProcessUnitExtra(ARegion *, Unit *);
    void PostProcessTurn();
    
    // Migration effects for alternate player-driven economy
    void ProcessMigration();
    
    // Run setup and equilibration turns (econ-only) at start
    void DevelopTowns();
    void Equilibrate();

    // Handle escaped monster check
    void MonsterCheck(ARegion *r, Unit *u);

    //
    // CheckVictory is used to see if the game is over.
    //
    Faction::WeakHandle CheckVictory();

    void EndGame(Faction *pVictor);

    void RunBuyOrders();
    void DoBuy(ARegion *, Market *);
    int GetBuyAmount(ARegion *, Market *);
    void RunSellOrders();
    void DoSell(ARegion *, Market *);
    int GetSellAmount(ARegion *, Market *);
    void DoAttackOrders();
    void CheckWMonAttack(ARegion *, Unit *);
    Unit *GetWMonTar(ARegion *, int, Unit *);
    int CountWMonTars(ARegion *, Unit *);
    void AttemptAttack(ARegion *, Unit *, Unit *, int, int=0);
    void DoAutoAttacks();
    void DoAdvanceAttack(ARegion *, Unit *);
    void DoAutoAttack(ARegion *, Unit *);
    void DoMovementAttacks(const std::list<Location::Handle>&);
    void DoMovementAttack(ARegion *, Unit *);
    void DoAutoAttackOn(ARegion *, Unit *);
    void RemoveEmptyObjects();
    void RunEnterOrders(int);
    void Do1EnterOrder(ARegion *, Object *, Unit *);
    void Do1JoinOrder(ARegion *, Object *, Unit *);
    void RunPromoteOrders();
    void Do1PromoteOrder(Object *, Unit *);
    void Do1EvictOrder(Object *, Unit *);
    void RunPillageOrders();
    int CountPillagers(ARegion *);
    void ClearPillagers(ARegion *);
    void RunPillageRegion(ARegion *);
    void RunTaxOrders();
    void RunTaxRegion(ARegion *);
    int FortTaxBonus(Object *, Unit *);
    int CountTaxes(ARegion *);
    void RunFindOrders();
    void RunFindUnit(Unit *);
    void RunDestroyOrders();
    void Do1Destroy(ARegion *, Object *, Unit *);
    void RunQuitOrders();
    void RunForgetOrders();
    void Do1Quit(Faction *);
    void SinkUncrewedFleets();
    void DrownUnits();
    void RunStealOrders();
    void RunTransportOrders();
    void CheckTransportOrders();
    std::list<Faction::WeakHandle> CanSeeSteal(const ARegion::Handle&, const Unit::Handle&);
    void Do1Steal(const ARegion::Handle&, const Object::Handle&, const Unit::Handle&);
    void Do1Assassinate(const ARegion::Handle&, const Object::Handle&, const Unit::Handle&);
    void AdjustCityMons(const ARegion::Handle& pReg);
    void AdjustCityMon(const ARegion::Handle& pReg, const Unit::Handle& u);

    //
    // Month long orders
    //
    void RunMoveOrders();
    Location::Handle DoAMoveOrder(const Unit::Handle&, const ARegion::Handle&, const Object::Handle&);
    void DoMoveEnter(const Unit::Handle&, const ARegion::Handle&, Object::Handle&);
    void RunMonthOrders();
    void RunStudyOrders(const ARegion::Handle&);
    void Do1StudyOrder(const Unit::Handle&, const Object::Handle&);
    void RunTeachOrders();
    void Do1TeachOrder(const ARegion::Handle&, const Unit::Handle&);
    void RunProduceOrders(const ARegion::Handle&);
    void RunIdleOrders(const ARegion::Handle&);
    int ValidProd(const Unit::Handle&, const ARegion::Handle&, const Production::Handle&);
    int FindAttemptedProd(const ARegion::Handle&, const Production::Handle&);
    void RunAProduction(const ARegion::Handle&, const Production::Handle&);
    void RunUnitProduce(const ARegion::Handle&, const Unit::Handle&);
    void Run1BuildOrder(const ARegion::Handle&, const Object::Handle&, const Unit::Handle&);
    void RunBuildShipOrder(const ARegion::Handle&, const Object::Handle&, const Unit::Handle&);
    void RunBuildHelpers(const ARegion::Handle&);
    unsigned int ShipConstruction(const ARegion::Handle&,
                                  const Unit::Handle&,
                                  const Unit::Handle&,
                                  int,
                                  size_t,
                                  const Items&);
    void CreateShip(const ARegion::Handle&, const Unit::Handle&, const Items&);
    void RunSailOrders();
    void RunMovementOrders();
    Location::Handle Do1SailOrder(ARegion::Handle, const Object::Handle&, const Unit::Handle&);
    void ClearCastEffects();
    void RunCastOrders();
    void RunACastOrder(const ARegion::Handle&, const Object::Handle&, const Unit::Handle&);
    void RunTeleportOrders();

    //
    // include spells.h for spell function declarations
    //
#define GAME_SPELLS
#include "spells.h"
#undef GAME_SPELLS

    //
    // Battle function
    //
    size_t KillDead(const Location::Handle&, const Battle::Handle&);
    int RunBattle(const ARegion::Handle&, const Unit::Handle&, const Unit::Handle&, int = 0, bool = false);
    void GetSidesForRegion_(const ARegion::Handle&,
                            const ARegion::Handle&,
                            std::list<Faction::WeakHandle>&,
                            std::list<Faction::WeakHandle>&,
                            std::list<Location::Handle>&,
                            std::list<Location::Handle>&,
                            const Unit::Handle&,
                            const Unit::Handle&,
                            bool,
                            bool&,
                            bool&,
                            bool = false);
    void GetSides(const ARegion::Handle&,
                  std::list<Faction::WeakHandle>&,
                  std::list<Faction::WeakHandle>&,
                  std::list<Location::Handle>&,
                  std::list<Location::Handle>&,
                  const Unit::Handle&,
                  const Unit::Handle&,
                  int = 0,
                  bool = false);
    bool CanAttack(const ARegion::Handle&,
                   const std::list<Faction::WeakHandle>&,
                   const Unit::Handle&);
    void GetAFacs(const ARegion::Handle&,
                  const Unit::Handle&,
                  const Unit::Handle&,
                  std::list<Faction::WeakHandle>&,
                  std::list<Faction::WeakHandle>&,
                  std::list<Location::Handle>&);
    void GetDFacs(const ARegion::Handle&,
                  const Unit::Handle&,
                  std::list<Faction::WeakHandle>&);
};

#endif
