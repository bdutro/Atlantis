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
#ifndef MARKET_CLASS
#define MARKET_CLASS

#include <memory>
#include <list>
#include "fileio.h"
#include "itemtype.h"

enum {
    M_BUY,
    M_SELL
};

class Market {
public:
    using Handle = std::shared_ptr<Market>;
    Market();

    /* type, item, price, amount, minpop, maxpop, minamt, maxamt */
    Market(int, const Items&, int, int, int, int, int, int);

    int type;
    Items item;
    int price;
    int amount;
    
    int minpop;
    int maxpop;
    int minamt;
    unsigned int maxamt;

    int baseprice;
    unsigned int activity;

    void PostTurn(unsigned int,int);
    void Writeout(Aoutfile * f);
    void Readin(Ainfile * f);

    static constexpr int DEFAULT_WAGE_MULTIPLIER = 4;
    static int calculateWagesWithRatio(int wages, float ratio, int multiplier = DEFAULT_WAGE_MULTIPLIER);

    AString Report();
};

class MarketList {
public:
    using iterator = std::list<Market::Handle>::iterator;
    using const_iterator = std::list<Market::Handle>::const_iterator;

    void PostTurn(unsigned int,int);
    void Writeout(Aoutfile * f);
    void Readin(Ainfile * f);
    void DeleteAll();
    void Add(int, const Items&, int, int, int, int, int, int);
    void Add(const Market::Handle&);
    iterator begin() { return markets_.begin(); }
    iterator end() { return markets_.end(); }
    const_iterator cbegin() const { return markets_.cbegin(); }
    const_iterator cend() const { return markets_.cend(); }
    const_iterator begin() const { return markets_.begin(); }
    const_iterator end() const { return markets_.end(); }
    iterator erase(iterator pos) { return markets_.erase(pos); }
    void clear() { markets_.clear(); }

private:
    std::list<Market::Handle> markets_;
};

#endif
