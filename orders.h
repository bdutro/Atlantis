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
#ifndef ORDERS_CLASS
#define ORDERS_CLASS

class Order;
class AttackOrder;
class MoveOrder;
class WithdrawOrder;
class GiveOrder;
class StudyOrder;
class TeachOrder;
class SellOrder;
class BuyOrder;
class ProduceOrder;
class BuildOrder;
class SailOrder;
class FindOrder;
class StealOrder;
class AssassinateOrder;
class CastOrder;
class CastMindOrder;
class CastRegionOrder;
class TeleportOrder;
class ForgetOrder;
class EvictOrder;
class BankOrder;
class IdleOrder;
class TransportOrder;

#include <memory>
#include <list>

#include "unitid.h"
#include "unit.h"
#include "gamedefs.h"
#include "astring.h"
#include "alist.h"
#include "ordertype.h"
#include "skilltype.h"

enum {
    M_NONE,
    M_WALK,
    M_RIDE,
    M_FLY,
    M_SWIM,
    M_SAIL
};

extern char const ** OrderStrs;

Orders Parse1Order(AString *);

class Order {
    public:
        using Handle = std::shared_ptr<Order>;

        Order();
        virtual ~Order();

        Orders type;
        int quiet;
};

class MoveDir {
    public:
        using Handle = std::shared_ptr<MoveDir>;
        Directions dir;
};

class MoveOrder : public Order {
    public:
        using Handle = std::shared_ptr<MoveOrder>;

        MoveOrder();
        ~MoveOrder();

        int advancing;
        std::list<MoveDir::Handle> dirs;
};

class WithdrawOrder : public Order {
    public:
        using Handle = std::shared_ptr<WithdrawOrder>;

        WithdrawOrder();
        ~WithdrawOrder();

        int item;
        int amount;
};

class GiveOrder : public Order {
    public:
        using Handle = std::shared_ptr<GiveOrder>;

        GiveOrder();
        ~GiveOrder();

        int item;
        /* if amount == -1, transfer whole unit, -2 means all of item */
        int amount;
        int except;
        int unfinished;
        int merge;

        UnitId target;
};

class StudyOrder : public Order {
    public:
        using Handle = std::shared_ptr<StudyOrder>;

        StudyOrder();
        ~StudyOrder();

        Skills skill;
        size_t days;
        unsigned int level;
};

class TeachOrder : public Order {
    public:
        using Handle = std::shared_ptr<TeachOrder>;

        TeachOrder();
        ~TeachOrder();

        std::list<UnitId> targets;
};

class ProduceOrder : public Order {
    public:
        using Handle = std::shared_ptr<ProduceOrder>;

        ProduceOrder();
        ~ProduceOrder();

        int item;
        int skill; /* -1 for none */
        int productivity;
        int target;
};

class BuyOrder : public Order {
    public:
        using Handle = std::shared_ptr<BuyOrder>;

        BuyOrder();
        ~BuyOrder();

        int item;
        int num;
};

class SellOrder : public Order {
    public:
        using Handle = std::shared_ptr<SellOrder>;

        SellOrder();
        ~SellOrder();

        int item;
        int num;
};

class AttackOrder : public Order {
    public:
        using Handle = std::shared_ptr<AttackOrder>;

        AttackOrder();
        ~AttackOrder();

        std::list<UnitId> targets;
};

class BuildOrder : public Order {
    public:
        using Handle = std::shared_ptr<BuildOrder>;

        BuildOrder();
        ~BuildOrder();

        UnitId target;
        int needtocomplete;
};

class SailOrder : public Order {
    public:
        using Handle = std::shared_ptr<SailOrder>;

        SailOrder();
        ~SailOrder();

        std::list<MoveDir::Handle> dirs;
};

class FindOrder : public Order {
    public:
        using Handle = std::shared_ptr<FindOrder>;

        FindOrder();
        ~FindOrder();

        int find;
};

class StealOrder : public Order {
    public:
        using Handle = std::shared_ptr<StealOrder>;

        StealOrder();
        ~StealOrder();

        UnitId target;
        int item;
};

class AssassinateOrder : public Order {
    public:
        using Handle = std::shared_ptr<AssassinateOrder>;

        AssassinateOrder();
        ~AssassinateOrder();

        UnitId target;
};

class ForgetOrder : public Order {
    public:
        using Handle = std::shared_ptr<ForgetOrder>;

        ForgetOrder();
        ~ForgetOrder();

        int skill;
};

// Add class for exchange
class ExchangeOrder : public Order {
    public:
        using Handle = std::shared_ptr<ExchangeOrder>;

        ExchangeOrder();
        ~ExchangeOrder();

        int giveItem;
        int giveAmount;
        int expectItem;
        int expectAmount;

        int exchangeStatus;

        UnitId target;
};

class TurnOrder : public Order {
    public:
        using Handle = std::shared_ptr<TurnOrder>;

        TurnOrder();
        ~TurnOrder();
        int repeating;
        std::list<AString::Handle> turnOrders;
};

class CastOrder : public Order {
    public:
        using Handle = std::shared_ptr<CastOrder>;

        CastOrder();
        ~CastOrder();

        int spell;
        int level;
};

class CastMindOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastMindOrder>;

        CastMindOrder();
        ~CastMindOrder();

        UnitId id;
};

class CastRegionOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastRegionOrder>;

        CastRegionOrder();
        ~CastRegionOrder();

        int xloc, yloc, zloc;
};

class TeleportOrder : public CastRegionOrder {
    public:
        using Handle = std::shared_ptr<TeleportOrder>;

        TeleportOrder();
        ~TeleportOrder();

        int gate;
        std::list<UnitId> units;
};

class CastIntOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastIntOrder>;

        CastIntOrder();
        ~CastIntOrder();

        int target;
};

class CastUnitsOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastUnitsOrder>;

        CastUnitsOrder();
        ~CastUnitsOrder();

        std::list<UnitId> units;
};

class CastTransmuteOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastTransmuteOrder>;

        CastTransmuteOrder();
        ~CastTransmuteOrder();

        int item;
        int number;
};

class EvictOrder : public Order {
    public:
        using Handle = std::shared_ptr<EvictOrder>;

        EvictOrder();
        ~EvictOrder();

        std::list<UnitId> targets;
};

class IdleOrder : public Order {
    public:
        using Handle = std::shared_ptr<IdleOrder>;

        IdleOrder();
        ~IdleOrder();
};

class TransportOrder : public Order {
    public:
        using Handle = std::shared_ptr<TransportOrder>;

        TransportOrder();
        ~TransportOrder();

        int item;
        // amount == -1 means all available at transport time
        int amount;
        int except;

        UnitId target;
};

class JoinOrder : public Order {
    public:
        using Handle = std::shared_ptr<JoinOrder>;

        JoinOrder();
        ~JoinOrder();

        UnitId target;
        int overload;
        int merge;
};

#endif
