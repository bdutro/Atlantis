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

enum {
    M_NONE,
    M_WALK,
    M_RIDE,
    M_FLY,
    M_SWIM,
    M_SAIL
};

#define MOVE_PAUSE 97
#define MOVE_IN 98
#define MOVE_OUT 99
/* Enter is MOVE_ENTER + num of object */
#define MOVE_ENTER 100

extern char const ** OrderStrs;

Orders Parse1Order(AString *);

class Order {
    public:
        Order();
        virtual ~Order();

        Orders type;
        int quiet;
};

class MoveDir {
    public:
        using Handle = std::shared_ptr<MoveDir>;
        int dir;
};

class MoveOrder : public Order {
    public:
        MoveOrder();
        ~MoveOrder();

        int advancing;
        std::list<MoveDir::Handle> dirs;
};

class WithdrawOrder : public Order {
    public:
        WithdrawOrder();
        ~WithdrawOrder();

        int item;
        int amount;
};

class GiveOrder : public Order {
    public:
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
        StudyOrder();
        ~StudyOrder();

        int skill;
        int days;
        int level;
};

class TeachOrder : public Order {
    public:
        TeachOrder();
        ~TeachOrder();

        std::list<UnitId> targets;
};

class ProduceOrder : public Order {
    public:
        ProduceOrder();
        ~ProduceOrder();

        int item;
        int skill; /* -1 for none */
        int productivity;
        int target;
};

class BuyOrder : public Order {
    public:
        BuyOrder();
        ~BuyOrder();

        int item;
        int num;
};

class SellOrder : public Order {
    public:
        SellOrder();
        ~SellOrder();

        int item;
        int num;
};

class AttackOrder : public Order {
    public:
        AttackOrder();
        ~AttackOrder();

        std::list<UnitId> targets;
};

class BuildOrder : public Order {
    public:
        BuildOrder();
        ~BuildOrder();

        UnitId target;
        int needtocomplete;
};

class SailOrder : public Order {
    public:
        SailOrder();
        ~SailOrder();

        std::list<MoveDir::Handle> dirs;
};

class FindOrder : public Order {
    public:
        FindOrder();
        ~FindOrder();

        int find;
};

class StealOrder : public Order {
    public:
        StealOrder();
        ~StealOrder();

        UnitId target;
        int item;
};

class AssassinateOrder : public Order {
    public:
        AssassinateOrder();
        ~AssassinateOrder();

        UnitId target;
};

class ForgetOrder : public Order {
    public:
        ForgetOrder();
        ~ForgetOrder();

        int skill;
};

// Add class for exchange
class ExchangeOrder : public Order {
    public:
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
        CastOrder();
        ~CastOrder();

        int spell;
        int level;
};

class CastMindOrder : public CastOrder {
    public:
        CastMindOrder();
        ~CastMindOrder();

        UnitId id;
};

class CastRegionOrder : public CastOrder {
    public:
        CastRegionOrder();
        ~CastRegionOrder();

        int xloc, yloc, zloc;
};

class TeleportOrder : public CastRegionOrder {
    public:
        TeleportOrder();
        ~TeleportOrder();

        int gate;
        std::list<UnitId> units;
};

class CastIntOrder : public CastOrder {
    public:
        CastIntOrder();
        ~CastIntOrder();

        int target;
};

class CastUnitsOrder : public CastOrder {
    public:
        CastUnitsOrder();
        ~CastUnitsOrder();

        std::list<UnitId> units;
};

class CastTransmuteOrder : public CastOrder {
    public:
        CastTransmuteOrder();
        ~CastTransmuteOrder();

        int item;
        int number;
};

class EvictOrder : public Order {
    public:
        EvictOrder();
        ~EvictOrder();

        std::list<UnitId> targets;
};

class IdleOrder : public Order {
    public:
        IdleOrder();
        ~IdleOrder();
};

class TransportOrder : public Order {
    public:
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
        JoinOrder();
        ~JoinOrder();

        UnitId target;
        int overload;
        int merge;
};

#endif
