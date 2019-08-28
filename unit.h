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
// Date        Person          Comments
// ----        ------          --------
// 2000/MAR/14 Larry Stanbery  Replaced specfic skill bonus functions with
//                             generic function.
//                             Added function to compute production bonus.
// 2001/FEB/07 Joseph Traub    Changes to allow mage support for city guards.
// 2001/Feb/18 Joseph Traub    Added support for Apprentices.
// 2001/Feb/25 Joseph Traub    Added a flag preventing units from crossing
//                             water.

#ifndef UNIT_CLASS
#define UNIT_CLASS

class Unit;
class UnitId;
class Object;

#include <memory>
#include <set>
#include <string>

#include "ptrlist.h"
#include "unitid.h"
#include "faction.h"
#include "gameio.h"
#include "orders.h"
#include "fileio.h"
#include "skills.h"
#include "items.h"
#include "itemtype.h"
#include "skilltype.h"

enum {
    GUARD_NONE,
    GUARD_GUARD,
    GUARD_AVOID,
    GUARD_SET,
    GUARD_ADVANCE
};

enum {
    TAX_NONE,
    TAX_TAX,
    TAX_PILLAGE,
    TAX_AUTO,
};

enum {
    REVEAL_NONE,
    REVEAL_UNIT,
    REVEAL_FACTION
};

enum {
    U_NORMAL,
    U_MAGE,
    U_GUARD,
    U_WMON,
    U_GUARDMAGE,
    U_APPRENTICE,
    NUNITTYPES
};

#define MAX_READY 4 // maximum number of ready weapons or armors

#define FLAG_BEHIND            0x0001
#define FLAG_NOCROSS_WATER        0x0002
#define FLAG_AUTOTAX            0x0004
#define FLAG_HOLDING            0x0008
#define FLAG_NOAID            0x0010
#define FLAG_INVIS            0x0020
#define FLAG_CONSUMING_UNIT        0x0040
#define FLAG_CONSUMING_FACTION        0x0080
#define FLAG_NOSPOILS            0x0100
#define FLAG_FLYSPOILS            0x0200
#define FLAG_WALKSPOILS            0x0400
#define FLAG_RIDESPOILS            0x0800
#define FLAG_SHARING            0x1000
#define FLAG_SWIMSPOILS            0x2000
#define FLAG_SAILSPOILS            0x4000

class Unit : std::enable_shared_from_this<Unit>
{
    public:
        using Handle = std::shared_ptr<Unit>;
        using WeakHandle = std::weak_ptr<Unit>;

        Unit();
        Unit(int, const std::shared_ptr<Faction>&, int = 0);
        ~Unit();

        void SetMonFlags();
        void MakeWMon(char const *, const Items&, size_t);

        void Writeout( Aoutfile *f );
        void Readin( Ainfile *f, const PtrList<Faction>&, ATL_VER v );

        AString SpoilsReport(void);
        bool CanGetSpoil(const Item::Handle& i);
        void WriteReport(Areport *,int,size_t,bool, bool, int, bool);
        AString GetName(int);
        AString MageReport();
        AString ReadyItem();
        AString StudyableSkills();
        AString * BattleReport(int);
        AString TemplateReport();

        void ClearOrders();
        void ClearCastOrders();
        void DefaultOrders(const std::shared_ptr<Object>&);
        void SetName(AString *);
        void SetDescribe(AString *);
        void PostTurn(const ARegion& reg);

        bool IsLeader();
        bool IsNormal();
        size_t GetMons();
        size_t GetMen();
        size_t GetLeaders();
        size_t GetSoldiers();
        size_t GetMen(const Items&);
        void SetMen(const Items&, size_t);
        unsigned int GetMoney();
        void SetMoney(size_t);
        size_t GetSharedNum(const Items&);
        void ConsumeShared(const Items&, size_t);
        int GetSharedMoney();
        void ConsumeSharedMoney(size_t);
        bool IsAlive();

        int MaintCost();
        void Short(int, int);
        int SkillLevels();
        void SkillStarvation();
        Skill *GetSkillObject(int);

        int GetAttackRiding();
        int GetDefenseRiding();

        //
        // These are rule-set specific, in extra.cpp.
        //
        // LLS
        int GetAttribute(char const *ident);
        int PracticeAttribute(char const *ident);
        int GetProductionBonus(const Items&);

        unsigned int GetSkill(const Skills&);
        void SetSkill(const Skills&, int);
        int GetSkillMax(const Skills&);
        unsigned int GetAvailSkill(const Skills&);
        unsigned int GetRealSkill(const Skills&);
        void ForgetSkill(const Skills&);
        bool CheckDepend(int,SkillDepend &s);
        bool CanStudy(const Skills&);
        int Study(const Skills&, int); /* Returns 1 if it succeeds */
        int Practice(const Skills&);
        void AdjustSkills();

        /* Return 1 if can see, 2 if can see faction */
        bool CanSee(const ARegion&, const Unit::Handle&, int practice = 0);
        bool CanCatch(const ARegion&, const Unit::Handle&);
        int AmtsPreventCrime(const Unit::Handle&);
        int GetAttitude(const ARegion&, const Unit::Handle&); /* Get this unit's attitude toward
                                              the Unit parameter */
        int Hostile();
        bool Forbids(const ARegion&,const Unit::Handle&);
        size_t Weight();
        size_t FlyingCapacity();
        size_t RidingCapacity();
        size_t SwimmingCapacity();
        size_t WalkingCapacity();
        bool CanFly(size_t);
        bool CanRide(size_t);
        bool CanWalk(size_t);
        bool CanFly();
        bool CanSwim();
        bool CanReallySwim();
        int MoveType(const std::shared_ptr<ARegion>& r = nullptr);
        unsigned int CalcMovePoints(const std::shared_ptr<ARegion>& r = nullptr);
        bool CanMoveTo(const ARegion&, const ARegion&);
        int GetFlag(int);
        void SetFlag(int,int);
        void CopyFlags(const Unit::Handle&);
        Items GetBattleItem(AString &itm);
        Items GetArmor(AString &itm, int ass);
        Items GetMount(AString &itm, int canFly, int canRide, int &bonus);
        Items GetWeapon(AString &itm, const Items& riding, int ridingBonus,
                int &attackBonus, int &defenseBonus, int &attacks);
        int CanUseWeapon(WeaponType *pWep, int riding);
        int CanUseWeapon(WeaponType *pWep);
        int Taxers(int);

        void MoveUnit(const std::weak_ptr<Object>& newobj);
        void Detach();
        void DiscardUnfinishedShips();

        void Event(const AString &);
        void Error(const AString &);

        std::weak_ptr<Faction> faction;
        std::weak_ptr<Faction> formfaction;
        std::weak_ptr<Object> object;
        AString *name;
        AString *describe;
        size_t num;
        int type;
        int alias;
        int gm_alias; /* used for gm manual creation of new units */
        int guard; /* Also, avoid- see enum above */
        int reveal;
        int flags;
        int taxing;
        unsigned int movepoints;
        int canattack;
        int nomove;
        int routed;
        SkillList skills;
        ItemList items;
        Skills combat;
        Items readyItem;
        Items readyWeapon[MAX_READY];
        Items readyArmor[MAX_READY];
        PtrList<AString> oldorders;
        int needed; /* For assessing maintenance */
        int hunger;
        int stomach_space;
        size_t losses;
        size_t free;
        int practiced; // Has this unit practiced a skill this turn
        unsigned int moved;
        ValidValue<size_t> phase;
        int savedmovement;
        Directions savedmovedir;

        /* Orders */
        int destroy;
        int enter;
        int build;
        UnitId::Handle promote;
        PtrList<FindOrder> findorders;
        PtrList<GiveOrder> giveorders;
        PtrList<WithdrawOrder> withdraworders;
        PtrList<BankOrder> bankorders;
        PtrList<BuyOrder> buyorders;
        PtrList<SellOrder> sellorders;
        PtrList<ForgetOrder> forgetorders;
        std::shared_ptr<CastOrder> castorders;
        std::shared_ptr<TeleportOrder> teleportorders;
        std::shared_ptr<Order> stealorders;
        std::shared_ptr<Order> monthorders;
        std::shared_ptr<AttackOrder> attackorders;
        std::shared_ptr<EvictOrder> evictorders;
        std::weak_ptr<ARegion> advancefrom;

        PtrList<ExchangeOrder> exchangeorders;
        PtrList<TurnOrder> turnorders;
        int inTurnBlock;
        std::shared_ptr<Order> presentMonthOrders;
        int presentTaxing;
        PtrList<TransportOrder> transportorders;
        std::shared_ptr<Order> joinorders;
        Unit::Handle former;
        int format;

        // Used for tracking VISIT quests
        std::set<std::string> visited;
        size_t raised;
};

Unit::WeakHandle GetUnitList(const WeakPtrList<Unit>&, const Unit::Handle&);

#endif
