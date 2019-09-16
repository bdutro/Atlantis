#ifndef ORDERS_CHECK_H
#define ORDERS_CHECK_H

#include <memory>

#include "gameio.h"
#include "unit.h"
#include "faction.h"
#include "orders.h"
#include "astring.h"

class OrdersCheck
{
public:
    using Handle = std::shared_ptr<OrdersCheck>;

    OrdersCheck();

    Aoutfile::Handle pCheckFile;
    Unit::Handle dummyUnit;
    Faction::Handle dummyFaction;
    Order dummyOrder;
    int numshows;
    int numerrors;

    void Error(const AString &error);
};

#endif
