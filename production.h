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
#ifndef PRODUCTION_CLASS
#define PRODUCTION_CLASS

#include <memory>
#include <list>

#include "ptrlist.h"
#include "gamedefs.h"
#include "fileio.h"
#include "itemtype.h"
#include "skilltype.h"

#define P_BIG 40
#define P_SMALL 20

class Production {
public:
    using Handle = std::shared_ptr<Production>;
    using WeakHandle = std::weak_ptr<Production>;

    Production(const Items&, int); /* item type, amt max */
    Production();
    
    void Writeout(Aoutfile&);
    void Readin(Ainfile&);
    AString WriteReport();
    
    Items itemtype;
    int baseamount;
    int amount;
    Skills skill;
    int productivity;
    int activity;
};

class ProductionList : public PtrList<Production>
{
public:
    Production::WeakHandle GetProd(const Items&, const Skills&); /* item type, skill */
    void AddProd(const Production::Handle&);

    void Writeout(Aoutfile&);
    void Readin(Ainfile&);

private:
    iterator GetProd_(const Items&, const Skills&); /* item type, skill */
};

#endif
