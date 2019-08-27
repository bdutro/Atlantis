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
class TurnOrder;
class ExchangeOrder;

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

extern const std::vector<std::string> OrderStrs;

Orders Parse1Order(AString *);

class Order {
    public:
        using Handle = std::shared_ptr<Order>;

        Order();
        virtual ~Order() = default;

        Orders type;
        bool quiet;
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

        int advancing;
        PtrList<MoveDir> dirs;
};

class WithdrawOrder : public Order {
    public:
        using Handle = std::shared_ptr<WithdrawOrder>;

        WithdrawOrder();

        Items item;
        int amount;
};

class GiveOrder : public Order {
    public:
        using Handle = std::shared_ptr<GiveOrder>;

        GiveOrder();

        ssize_t item;
        /* if amount == -1, transfer whole unit, -2 means all of item */
        int amount;
        int except;
        int unfinished;
        int merge;

        ValidValue<UnitId> target;
};

class StudyOrder : public Order {
    public:
        using Handle = std::shared_ptr<StudyOrder>;

        StudyOrder();

        Skills skill;
        size_t days;
        ValidValue<unsigned int> level;
};

class TeachOrder : public Order {
    public:
        using Handle = std::shared_ptr<TeachOrder>;

        TeachOrder();

        std::list<UnitId> targets;
};

class ProduceOrder : public Order {
    public:
        using Handle = std::shared_ptr<ProduceOrder>;

        ProduceOrder();

        Items item;
        Skills skill; /* -1 for none */
        int productivity;
        int target;
};

class BuyOrder : public Order {
    public:
        using Handle = std::shared_ptr<BuyOrder>;

        BuyOrder();

        Items item;
        int num;
};

class SellOrder : public Order {
    public:
        using Handle = std::shared_ptr<SellOrder>;

        SellOrder();

        Items item;
        int num;
};

class AttackOrder : public Order {
    public:
        using Handle = std::shared_ptr<AttackOrder>;

        AttackOrder();

        std::list<UnitId> targets;
};

class BuildOrder : public Order {
    public:
        using Handle = std::shared_ptr<BuildOrder>;

        BuildOrder();

        ValidValue<UnitId> target;
        int needtocomplete;
};

class SailOrder : public Order {
    public:
        using Handle = std::shared_ptr<SailOrder>;

        SailOrder();

        PtrList<MoveDir> dirs;
};

class FindOrder : public Order {
    public:
        using Handle = std::shared_ptr<FindOrder>;

        FindOrder();

        int find;
};

class StealOrder : public Order {
    public:
        using Handle = std::shared_ptr<StealOrder>;

        StealOrder();

        UnitId target;
        Items item;
};

class AssassinateOrder : public Order {
    public:
        using Handle = std::shared_ptr<AssassinateOrder>;

        AssassinateOrder();

        UnitId target;
};

class ForgetOrder : public Order {
    public:
        using Handle = std::shared_ptr<ForgetOrder>;

        ForgetOrder();

        Skills skill;
};

// Add class for exchange
class ExchangeOrder : public Order {
    public:
        using Handle = std::shared_ptr<ExchangeOrder>;

        ExchangeOrder();

        Items giveItem;
        int giveAmount;
        Items expectItem;
        int expectAmount;

        int exchangeStatus;

        UnitId target;
};

class TurnOrder : public Order {
    public:
        using Handle = std::shared_ptr<TurnOrder>;

        TurnOrder();
        int repeating;
        PtrList<AString> turnOrders;
};

class CastOrder : public Order {
    public:
        using Handle = std::shared_ptr<CastOrder>;

        CastOrder();

        int spell;
        int level;
};

class CastMindOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastMindOrder>;

        CastMindOrder();

        ValidValue<UnitId> id;
};

class CastRegionOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastRegionOrder>;

        CastRegionOrder() = default;

        int xloc, yloc, zloc;
};

class TeleportOrder : public CastRegionOrder {
    public:
        using Handle = std::shared_ptr<TeleportOrder>;

        TeleportOrder() = default;

        int gate;
        std::list<UnitId> units;
};

class CastIntOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastIntOrder>;

        CastIntOrder() = default;

        int target;
};

class CastUnitsOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastUnitsOrder>;

        CastUnitsOrder() = default;

        std::list<UnitId> units;
};

class CastTransmuteOrder : public CastOrder {
    public:
        using Handle = std::shared_ptr<CastTransmuteOrder>;

        CastTransmuteOrder() = default;

        int item;
        int number;
};

class EvictOrder : public Order {
    public:
        using Handle = std::shared_ptr<EvictOrder>;

        EvictOrder();

        std::list<UnitId> targets;
};

class IdleOrder : public Order {
    public:
        using Handle = std::shared_ptr<IdleOrder>;

        IdleOrder();
};

class TransportOrder : public Order {
    public:
        using Handle = std::shared_ptr<TransportOrder>;

        TransportOrder();

        Items item;
        // amount == -1 means all available at transport time
        int amount;
        int except;

        ValidValue<UnitId> target;
};

class JoinOrder : public Order {
    public:
        using Handle = std::shared_ptr<JoinOrder>;

        JoinOrder();

        UnitId target;
        int overload;
        int merge;
};

template<typename T>
typename std::enable_if<std::is_base_of<Order, T>::value>::type RemoveOrder(PtrList<T>& orders, const std::shared_ptr<T>& o)
{
    for(auto it = orders.begin(); it != orders.end(); ++it)
    {
        if(*it == o)
        {
            orders.erase(it);
            return;
        }
    }
}

#endif
