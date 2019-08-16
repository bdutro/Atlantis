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

#include "market.h"
#include "items.h"
#include "gameio.h"
#include "gamedata.h"

Market::Market()
{
    activity = 0;
}

Market::Market(int a, const Items& b, int c, int d, int e, int f, int g, int h)
{
    type = a;
    item = b;
    price = c;
    amount = d;
    minpop = e;
    maxpop = f;
    minamt = g;
    maxamt = h;
    baseprice = price;
    activity = 0;
}

int Market::calculateWagesWithRatio(int wages, float ratio, int multiplier)
{
    return static_cast<int>(static_cast<float>(wages * multiplier) * ratio);
}

void Market::PostTurn(int population, int wages)
{
    // Nothing to do to the markets.
    if (!(Globals->VARIABLE_ECONOMY)) return;

    //
    // Unlimited, unchanging market.
    //
    if (amount == -1) return;

    if (ItemDefs[item].type & IT_MAN) {
        float ratio = static_cast<float>(ItemDefs[item].baseprice) /
                      static_cast<float>(10 * Globals->BASE_MAN_COST);
        // hack: included new wage factor of ten in float assignment above
        price = calculateWagesWithRatio(wages, ratio);
        if (ItemDefs[item].type & IT_LEADER)
            amount = population / 125;
        else
            amount = population / 25;
        return;
    }

    int tarprice = price;
    if (amount) {
        int fluctuation = (baseprice * activity)/amount;
        if (type == M_BUY)
            tarprice = (2 * baseprice + fluctuation) / 2;
        else
            tarprice = (3 * baseprice - fluctuation) / 2;
    }
    price = price + (tarprice - price) / 5;

    if (population <= minpop)
        amount = minamt;
    else {
        if (population >= maxpop)
            amount = maxamt;
        else {
            amount = minamt + ((maxamt - minamt) *
                    (population - minpop)) /
                (maxpop - minpop);
        }
    }
}

void Market::Writeout(Aoutfile *f)
{
    f->PutInt(type);
    if (item.isValid())
    {
        f->PutStr(ItemDefs[item].abr);
    }
    else
    {
        f->PutStr("NO_ITEM");
    }
    f->PutInt(price);
    f->PutInt(amount);
    f->PutInt(minpop);
    f->PutInt(maxpop);
    f->PutInt(minamt);
    f->PutInt(maxamt);
    f->PutInt(baseprice);
}

void Market::Readin(Ainfile *f)
{
    AString *temp;
    type = f->GetInt<int>();

    temp = f->GetStr();
    item = LookupItem(*temp);
    delete temp;

    price = f->GetInt<int>();
    amount = f->GetInt<int>();
    minpop = f->GetInt<int>();
    maxpop = f->GetInt<int>();
    minamt = f->GetInt<int>();
    maxamt = f->GetInt<int>();
    baseprice = f->GetInt<int>();
}

AString Market::Report()
{
    AString temp;
    temp += ItemString(item, amount) + " at $" + price;
    return temp;
}

void MarketList::PostTurn(int population, int wages)
{
    for(const auto& elem: markets_) {
        elem->PostTurn(population, wages);
    }
}

void MarketList::Writeout(Aoutfile *f)
{
    f->PutInt(markets_.size());
    for(const auto& elem: markets_)
    {
        elem->Writeout(f);
    }
}

void MarketList::Readin(Ainfile *f)
{
    size_t n = f->GetInt<size_t>();
    for (size_t i = 0; i < n; ++i) {
        auto& m = markets_.emplace_back(std::make_shared<Market>());
        m->Readin(f);
    }
}

void MarketList::DeleteAll()
{
    markets_.clear();
}

void MarketList::Add(const Market::Handle& m)
{
    markets_.push_back(m);
}

void MarketList::Add(int type, const Items& item, int price, int amount, int minpop, int maxpop, int minamt, int maxamt)
{
    markets_.emplace_back(std::make_shared<Market>(type,
                                                   item,
                                                   price,
                                                   amount,
                                                   minpop,
                                                   maxpop,
                                                   minamt,
                                                   maxamt));
}
