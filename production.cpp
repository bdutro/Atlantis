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

#include "production.h"
#include "gameio.h"
#include "items.h"
#include "skills.h"
#include "gamedata.h"

Production::Production()
{
    itemtype = -1;
    amount = 0;
    baseamount = 0;
    productivity = 0;
    skill = -1;
}

Production::Production(int it, int maxamt)
{
    itemtype = it;
    amount = maxamt;
    if (Globals->RANDOM_ECONOMY)
        amount += getrandom(maxamt);
    baseamount = amount;
    productivity = 10;
    AString skname = ItemDefs[it].pSkill;
    skill = LookupSkill(&skname);
}

void Production::Writeout(Aoutfile *f)
{
    if (itemtype != -1) f->PutStr(ItemDefs[itemtype].abr);
    else f->PutStr("NO_ITEM");
    f->PutInt(amount);
    f->PutInt(baseamount);
    if (itemtype == I_SILVER) {
        if (skill != -1) f->PutStr(SkillDefs[skill].abbr);
        else f->PutStr("NO_SKILL");
    }
    f->PutInt(productivity);
}

void Production::Readin(Ainfile *f)
{
    AString *temp;

    temp = f->GetStr();
    itemtype = LookupItem(temp);
    delete temp;

    amount = f->GetInt();
    baseamount = f->GetInt();

    if (itemtype == I_SILVER) temp = f->GetStr();
    else temp = new AString(ItemDefs[itemtype].pSkill);
    skill = LookupSkill(temp);
    delete temp;

    productivity = f->GetInt();
}

AString Production::WriteReport()
{
    AString temp = ItemString(itemtype, amount);
    return temp;
}

void ProductionList::Writeout(Aoutfile *f)
{
    f->PutInt(products_.size());
    for(const auto& p: products_)
    {
        p->Writeout(f);
    }
}

void ProductionList::Readin(Ainfile *f)
{
    int n = f->GetInt();
    for (int i = 0; i<n; i++) {
        p = products.emplace_back(std::make_shared<Production());
        p->Readin(f);
    }
}
    
Production::WeakHandle ProductionList::GetProd(int t, int s)
{
    Production::WeakHandle p;
    auto it = GetProd_(t, s);
    if(it != products_.end())
    {
        p = *it;
    }

    return p;
}

ProductionList::iterator ProductionList::GetProd_(int t, int s)
{
    ProductionList::iterator found_it = products_.end();

    for(auto it = products_.begin(); it != products_.end(); ++it)
    {
        const auto& p = *it;
        if(p->itemtype == t && p->skill == s)
        {
            found_it = it;
            break;
        }
    }

    return found_it;
}

void ProductionList::AddProd(const Production::Handle& p)
{
    iterator it = GetProd(p->itemtype, p->skill);
    if (it != end()) {
        products_.erase(it);
    }
    
    products_.push_back(p);
}

void ProductionList::Add(int it, int maxamt)
{
    products_.emplace_back(std::make_shared<Production>(it, maxamt));
}

void ProductionList::Add(const Production::Handle& p)
{
    products_.push_back(p);
}
