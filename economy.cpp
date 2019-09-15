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

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "game.h"
#include "gamedata.h"

/* TownType */
// Depends on population and town development
TownTypeEnum TownInfo::TownType()
{
    int prestige = pop * (dev + 220) / 270;
    if (prestige < Globals->CITY_POP / 4) return TownTypeEnum::TOWN_VILLAGE;
    if (prestige < Globals->CITY_POP * 4 / 5) return TownTypeEnum::TOWN_TOWN;
    return TownTypeEnum::TOWN_CITY;
}

int ARegion::Population()
{
    if (town) {
        return population + town->pop;
    } else {
        return population;
    }
}

// IMPORTANT: wages now represent fractional wages, that
// means the value of the wages variable is 10 times the silver value
int ARegion::Wages()
{
    // Calculate new wages
    wages = 0;
    if (Population() == 0) return 0;
    int level = 1;
    int last = 0;
    int dv = development + RoadDevelopment() + static_cast<int>(earthlore + clearskies);
    // Note: earthlore and clearskies represent LEVEL of the spell
    // Adjust for TownType
    if (town) {
        int tsize = static_cast<int>(town->TownType());
        dv += (tsize * tsize + 1);
    }
    while (dv >= level) {
        wages++;
        last = level;
        level += wages+1;
    }
    wages *= 10;
    if (dv > last)
        wages += 10 * (dv - last) / (level - last);    
    return wages;
}

// Removed the function LakeEffect()!

AString ARegion::WagesForReport()
{
    Production::WeakHandle p_w = products.GetProd(Items::Types::I_SILVER, -1);
    if (!p_w.expired())
    {
        const auto p = p_w.lock();
        return AString("$") + (p->productivity / 10) +
            "." + (p->productivity % 10) + " (Max: $" + p->amount + ")";
    }
    else
    {
        return AString("$") + 0;
    }
}

void ARegion::SetupPop()
{
    const auto& typer = TerrainDefs[type];
    habitat = typer.pop+1;
    if (habitat > 1) habitat *= 5;
    if ((habitat < 100) && (typer.similar_type != Regions::Types::R_OCEAN)) habitat = 100;

    int pop = typer.pop;
    int mw = typer.wages;
    
    // fix economy when MAINTENANCE_COST has been adjusted
    mw += static_cast<int>(Globals->MAINTENANCE_COST) - 10;
    if (mw < 0) mw = 0;

    if (pop == 0) {
        population = 0;
        basepopulation = 0;
        wages = 0;
        maxwages = 0;
        wealth = 0;
        return;
    }

    // Only select race here if it hasn't been set during Race Growth
    // in the World Creation process.
    if (!race.isValid() || (!Globals->GROW_RACES)) {
        std::vector<Items> allowed_races(typer.races.begin(), typer.races.end());
        if(IsCoastal())
        {
            allowed_races.insert(allowed_races.end(), typer.coastal_races.begin(), typer.coastal_races.end());
        }

        race.invalidate();
        while (!race.isValid() || (ItemDefs[race].flags & ItemType::DISABLED)) {
            size_t n = getrandom(allowed_races.size());
            race = allowed_races[n];
        }
    }

    habitat = habitat * 2/3 + getrandom(habitat/3);
    const auto& mt = FindRace(ItemDefs[race].abr);
    if (mt.terrain == typer.similar_type) {
        habitat = (habitat * 9)/8;
    }
    if (!IsNativeRace(race)) {
        habitat = (habitat * 4)/5;
    }
    basepopulation = habitat / 3;
    // hmm... somewhere not too far off equilibrium pop
    population = habitat * (60 + getrandom(6) + getrandom(6)) / 100;
    
    // Setup development
    int level = 1;
    development = 1;
    int prev = 0;
    while (level < mw) {
        development++;
        prev++;
        if (prev > level) {
            level++;
            prev = 0;
        }
    }
    development += getrandom(25);

    if (Globals->TOWNS_EXIST) {
        unsigned int adjacent = 0;
        unsigned int prob = Globals->TOWN_PROBABILITY;
        if (prob < 1) prob = 100;
        unsigned int townch = 80000 / prob;
        if (Globals->TOWNS_NOT_ADJACENT)
        {
            for (const auto& newregion: neighbors) {
                if (!newregion.expired() && (newregion.lock()->town))
                {
                    adjacent++;
                }
            }
        }
        if (Globals->LESS_ARCTIC_TOWNS) {
            unsigned int dnorth = GetPoleDistance(Directions::Types::D_NORTH);
            unsigned int dsouth = GetPoleDistance(Directions::Types::D_SOUTH);
            if (dnorth < 9)
                townch = townch + 25 * (9 - dnorth) *
                    (9 - dnorth) * Globals->LESS_ARCTIC_TOWNS;
            if (dsouth < 9)
                townch = townch + 25 * (9 - dsouth) *
                    (9 - dsouth) * Globals->LESS_ARCTIC_TOWNS;
        }
        unsigned int spread = Globals->TOWN_SPREAD;
        if (spread > 100) spread = 100;
        unsigned int townprob = (TerrainDefs[type].economy * 4 * (100 - spread) + (100 * spread)) / 100;
        if (adjacent > 0) townprob = townprob * (100 - Globals->TOWNS_NOT_ADJACENT) / 100;
        if (getrandom(townch) < townprob) {
            AddTown();
        }
    }

    /* Setup basic economy */
    maxwages = Wages();

    /* taxable region wealth */
    wealth = static_cast<int>(static_cast<float>(Population() * (Wages() - static_cast<int>(10 * Globals->MAINTENANCE_COST)) / 50));
    if (wealth < 0) wealth = 0;

    // wage-relevant population (/10 wages /5 popfactor)
    int pp = Population();
    // adjustment for rural areas
    if (pp < 3000) {
        float wpfactor = 120 / (61 - static_cast<float>(pp) / 50);
        pp += static_cast<int>(static_cast<float>((wpfactor * static_cast<float>(pp) + 3000)/(wpfactor + 1)));
    }
    int wagelimit = static_cast<int>(static_cast<float>(pp * (Wages() - static_cast<int>(10 * Globals->MAINTENANCE_COST))) /50);
    if (wagelimit < 0) wagelimit = 0;
    Production::Handle w = std::make_shared<Production>();
    w->itemtype = Items::Types::I_SILVER;
    w->amount = wagelimit / Globals->WORK_FRACTION;
    w->baseamount = wagelimit / Globals->WORK_FRACTION;
    w->skill.invalidate();
    w->productivity = wages;

    /* Entertainment - setup or adjust */
    int ep = Population();
    // adjustment for rural areas
    if (ep < 3000) {
        int epf = (ep / 10 + 300) / 6;
        ep = ep * epf / 100;
    }
    int maxent = static_cast<int>(static_cast<float>(ep * ((Wages() - static_cast<int>(10 * Globals->MAINTENANCE_COST)) + 1)) / 50);
    if (maxent < 0) maxent = 0;
    Production::Handle e = std::make_shared<Production>();
    e->itemtype = Items::Types::I_SILVER;
    e->skill = Skills::Types::S_ENTERTAINMENT;
    e->amount = maxent / Globals->ENTERTAIN_FRACTION;
    e->baseamount = maxent / Globals->ENTERTAIN_FRACTION;
    // raise entertainment income by productivity factor 10
    e->productivity = Globals->ENTERTAIN_INCOME * 10;
    
    // note: wage factor 10, population factor 5 - included as "/ 50"
    /* More wealth in safe Starting Cities */
    if ((Globals->SAFE_START_CITIES) && (IsStartingCity())) {
        int wbonus = (Population() / 5) * static_cast<int>(Globals->MAINTENANCE_COST);
        wealth += wbonus;
        w->amount += wbonus / Globals->WORK_FRACTION;
        w->baseamount += wbonus / Globals->WORK_FRACTION;
        e->amount += wbonus / Globals->ENTERTAIN_FRACTION;
        e->baseamount += wbonus / Globals->ENTERTAIN_FRACTION;
    }
    products.push_back(w);
    products.push_back(e);

    float ratio = static_cast<float>(ItemDefs[race].baseprice) / static_cast<float>(Globals->BASE_MAN_COST * 10);
    // hack: include wage factor of 10 in float assignment above
    // Setup Recruiting
    markets.emplace_back(M_BUY,
                         race,
                         calculateWagesWithRatio(ratio),
                         Population() / 25,
                         0,
                         10000,
                         0,
                         2000);

    if (Globals->LEADERS_EXIST) {
        ratio = static_cast<float>(ItemDefs[Items::Types::I_LEADERS].baseprice) / static_cast<float>(Globals->BASE_MAN_COST * 10);
        // hack: include wage factor of 10 in float assignment above
        markets.emplace_back(M_BUY,
                             Items::Types::I_LEADERS,
                             calculateWagesWithRatio(ratio),
                             Population() / 125,
                             0,
                             10000,
                             0,
                             400);
    }
}

/* ONE function where the wage and entertainment income
 * is set (previously this has been all over the place!) */
void ARegion::SetIncome()
{
    /* do nothing in unpopulated regions */
    if (basepopulation == 0)
    {
        return;
    }

    maxwages = Wages();
    
    /* taxable region wealth */
    wealth = static_cast<int>(static_cast<float>(Population() * (Wages() - static_cast<int>(10 * Globals->MAINTENANCE_COST))) / 50);
    if (wealth < 0) wealth = 0;
    
    /* Wages */
    // wage-relevant population (/10 wages /5 popfactor)
    int pp = Population();
    // adjustment for rural areas
    if (pp < 3000) {
        float wpfactor = (120 / (61 - static_cast<float>(pp) / 50));
        pp += static_cast<int>((wpfactor * static_cast<float>(pp) + 3000)/(wpfactor + 1));
    }
    int prod_maxwages = static_cast<int>(static_cast<float>(pp * (Wages() - static_cast<int>(10 * Globals->MAINTENANCE_COST))) / 50);
    if (prod_maxwages < 0) prod_maxwages = 0;
    Production::WeakHandle w_weak = products.GetProd(Items::Types::I_SILVER,-1);
    Production::Handle w;
    // In some cases (ie. after products.DeleteAll() in EditGameRegionTerrain)
    // Items::Types::I_SILVER is not in ProductionList
    if( w_weak.expired() )
    {
        w = products.emplace_back();
    }
    else
    {
        w = w_weak.lock();
    }
    w->itemtype = Items::Types::I_SILVER;
    w->amount = prod_maxwages / Globals->WORK_FRACTION;
    w->baseamount = prod_maxwages / Globals->WORK_FRACTION;
    w->skill.invalidate();
    w->productivity = wages;

    /* Entertainment - setup or adjust */
    int ep = Population();
    // adjustment for rural areas
    if (ep < 3000) {
        int epf = (ep / 10 + 300) / 6;
        ep = ep * epf / 100;
    }
    int maxent = static_cast<int>(static_cast<float>(ep * ((Wages() - static_cast<int>(10 * Globals->MAINTENANCE_COST)) + 10)) / 50);
    if (maxent < 0) maxent = 0;
    Production::Handle e;
    Production::WeakHandle e_w = products.GetProd(Items::Types::I_SILVER,
                                                  Skills::Types::S_ENTERTAINMENT);
    // In some cases (ie. after products.DeleteAll() in EditGameRegionTerrain)
    // Items::Types::I_SILVER is not in ProductionList
    if( e_w.expired() )
    {
        e = products.emplace_back();
    }
    else
    {
        e = e_w.lock();
    }
    e->itemtype = Items::Types::I_SILVER;
    e->amount = maxent / Globals->ENTERTAIN_FRACTION;
    e->baseamount = maxent / Globals->ENTERTAIN_FRACTION;
    e->skill = Skills::Types::S_ENTERTAINMENT;
    // raise entertainment income by productivity factor 10
    e->productivity = Globals->ENTERTAIN_INCOME * 10;
    
    // note: wage factor 10, population factor 5 - included as "/ 50"
    /* More wealth in safe Starting Cities */
    if ((Globals->SAFE_START_CITIES) && (IsStartingCity())) {
        int wbonus = (Population() / 5) * static_cast<int>(Globals->MAINTENANCE_COST);
        wealth += wbonus;
        w->amount += wbonus / Globals->WORK_FRACTION;
        w->baseamount += wbonus / Globals->WORK_FRACTION;
        e->amount += wbonus / Globals->ENTERTAIN_FRACTION;
        e->baseamount += wbonus / Globals->ENTERTAIN_FRACTION;
    }
}

void ARegion::DisbandInRegion(const Items& item, int amt)
{
    if (!Globals->DYNAMIC_POPULATION) return;
    if (amt > 0) {
        if (amt > Population()) {
            // exchange region race!
            race = item;
            population = 0;
            if (town) town->pop = 0;
            AdjustPop(amt);
        } else {
            if (race != item) amt = amt * 2 / 3;
            AdjustPop(amt);
        }
    }
}

void ARegion::Recruit(int amt)
{
    if (!Globals->DYNAMIC_POPULATION) return;
    AdjustPop(-amt);
}

void ARegion::AdjustPop(int adjustment)
{
    if (!town) {
        population += adjustment;
        return;
    }
    // split between town and rural pop
    int tspace = town->hab - town->pop;
    int rspace = habitat - population;
    town->pop += adjustment * tspace / (tspace + rspace);
    if (town->pop < 0) town->pop = 0;
    population += adjustment * rspace / (tspace + rspace);
    if (population < 0) population = 0;
}

void ARegion::SetupCityMarket()
{
    int numtrade = 0;
    int cap;
    int offset = 0;
    int citymax = Globals->CITY_POP;
    const auto& locals = [&]() -> const ManType& {
        try
        {
            return FindRace(ItemDefs[race].abr);
        }
        catch(const NoSuchItemException&)
        {
            return FindRace("SELF");
        }
    } ();
    /* compose array of possible supply & demand items */
    int supply[Items::size()] = {0};
    int demand[Items::size()] = {0};
    /* possible advanced and magic items */
    int rare[Items::size()] = {0};
    int antiques[Items::size()] = {0};
    for (auto it = Items::begin(); it != Items::end(); ++it) {
        size_t i = *it;
        if (ItemDefs[i].flags & ItemType::DISABLED) continue;
        if (ItemDefs[i].flags & ItemType::NOMARKET) continue;
        if (ItemDefs[i].type & IT_SHIP) continue;
        if (ItemDefs[i].type & IT_TRADE) numtrade++;
        if (it == Items::Types::I_SILVER) continue;
        if ((ItemDefs[i].type & IT_MAN)
            || (ItemDefs[i].type & IT_LEADER)) continue;

        bool canProduceHere = false;
        // Check if the product can be produced in the region
        // Raw goods
        if (!ItemDefs[i].pInput[0].item.isValid()) {
            for(const auto& p: TerrainDefs[type].prods)
            {
                const auto& resource = p.product;
                if(!resource.isValid())
                {
                    continue;
                }
                if (i == resource) {
                    canProduceHere = true;
                    break;
                }
            }
        } 
        // Non-raw goods
        else {
            canProduceHere = true;
            for(const auto& pinput: ItemDefs[i].pInput)
            {
                bool match = false;
                const auto& need = pinput.item;
                for(const auto& p: TerrainDefs[type].prods)
                {
                    if (p.product == need)
                    {
                        match = true;
                        break;
                    }
                }
                if (!match) {
                    canProduceHere = false;
                    break;
                }
            }
        }
        int canProduce = 0;
        // Check if the locals can produce this item
        if (canProduceHere) canProduce = locals.CanProduce(i);
        int isUseful = 0;
        // Check if the item is useful to the locals
        isUseful = locals.CanUse(i);
        //Normal Items
        if (ItemDefs[ i ].type & IT_NORMAL) {
            
            if (it == Items::Types::I_GRAIN || it == Items::Types::I_LIVESTOCK || it == Items::Types::I_FISH) {
                // Add foodstuffs directly to market    
                int amt = Globals->CITY_MARKET_NORMAL_AMT;
                size_t price;

                if (Globals->RANDOM_ECONOMY) {
                    amt += getrandom(amt);
                    price = (ItemDefs[i].baseprice * (100 + getrandom(50UL))) / 100;
                } else {
                    price = ItemDefs[ i ].baseprice;
                }

                cap = (citymax * 3/4) - 5000;
                if (cap < 0) cap = citymax/2;
                markets.emplace_back(M_SELL,
                                     i,
                                     static_cast<int>(price),
                                     amt,
                                     population,
                                     population + cap,
                                     amt,
                                     2 * amt);
            } else if (it == Items::Types::I_FOOD) {
                // Add foodstuffs directly to market
                int amt = Globals->CITY_MARKET_NORMAL_AMT;
                int price;
                if (Globals->RANDOM_ECONOMY) {
                    amt += getrandom(amt);
                    price = static_cast<int>((ItemDefs[i].baseprice * (120 + getrandom(80UL))) / 100);
                } else {
                    price = static_cast<int>(ItemDefs[i].baseprice);
                }

                cap = (citymax * 3/4) - 5000;
                if (cap < 0) cap = citymax/2;
                markets.emplace_back(M_BUY,
                                     i,
                                     price,
                                     amt,
                                     population,
                                     population + 2 * cap,
                                     amt,
                                     5 * amt);
            } else if (!ItemDefs[i].pInput[0].item.isValid()) {
                // Basic resource
                // Add to supply?
                if (canProduce) supply[i] = 4;
                // Add to demand?
                if (!canProduceHere) {
                    // Is it a mount?
                    if (ItemDefs[i].type & IT_MOUNT) {
                        if (locals.CanProduce(i)) demand[i] = 4;
                    } else if (isUseful) demand[i] = 4;
                }
            } else {
            
                // Tool, weapon or armor
                if (isUseful) {
                    // Add to supply?
                    if (canProduce) supply[i] = 2;
                    // Add to demand?
                    if (!canProduceHere) demand[i] = 2;
                }
            }
        } // end Normal Items
        // Advanced Items
        else if ((Globals->CITY_MARKET_ADVANCED_AMT)
            && (ItemDefs[i].type & IT_ADVANCED)) {
            if (isUseful) rare[i] = 4;
            if (ItemDefs[i].hitchItem.isValid() && ItemDefs[i].hitchItem > *Items::begin()) rare[i] = 2;
        }
        // Magic Items
        else if ((Globals->CITY_MARKET_MAGIC_AMT)
            && (ItemDefs[i].type & IT_MAGIC)) {
            if (isUseful) antiques[i] = 4;
                else antiques[i] = 1;
            if (ItemDefs[i].hitchItem.isValid() && ItemDefs[i].hitchItem > *Items::begin()) antiques[i] = 2;
        }
    }
    /* Check for advanced item */
    if ((Globals->CITY_MARKET_ADVANCED_AMT) && (getrandom(4) == 1)) {
        int ad = 0;
        for (size_t i = 0; i < Items::size(); i++)
        {
            ad += rare[i];
        }
        ad = getrandom(ad);
        int sum = 0;
        size_t i;
        for (i = 0; i < Items::size(); i++) {
            sum += rare[i];
            if (ad < sum) break;
        }
        if (ad < sum) {
            int amt = Globals->CITY_MARKET_ADVANCED_AMT;
            int price;
            if (Globals->RANDOM_ECONOMY) {
                amt += getrandom(amt);
                price = static_cast<int>((ItemDefs[i].baseprice * (100 + getrandom(50UL))) / 100);
            } else {
                price = static_cast<int>(ItemDefs[i].baseprice);
            }

            cap = (citymax *3/4) - 5000;
            if (cap < citymax/2) cap = citymax / 2;
            offset = citymax / 8;
            if (cap+offset < citymax) {
                markets.emplace_back(M_SELL,
                                     i,
                                     price,
                                     amt / 6,
                                     population + cap + offset,
                                     population + citymax,
                                     0,
                                     amt);
            }
        }
    }
    /* Check for magic item */
    if ((Globals->CITY_MARKET_MAGIC_AMT) && (getrandom(8) == 1)) {
        int mg = 0;
        for (size_t i = 0; i < Items::size(); i++)
        {
            mg += antiques[i];
        }
        mg = getrandom(mg);
        size_t i;
        int sum = 0;
        for (i = 0; i < Items::size(); i++) {
            sum += antiques[i];
            if (mg < sum) break;
        }
        if (mg < sum) {
            int amt = Globals->CITY_MARKET_MAGIC_AMT;
            int price;

            if (Globals->RANDOM_ECONOMY) {
                amt += getrandom(amt);
                price = static_cast<int>((ItemDefs[i].baseprice * (100 + getrandom(50UL))) / 100);
            } else {
                price = static_cast<int>(ItemDefs[i].baseprice);
            }
    
            cap = (citymax *3/4) - 5000;
            if (cap < citymax/2) cap = citymax / 2;
            offset = (citymax/20) + ((citymax/5) * 2);
            markets.emplace_back(M_SELL,
                                 i,
                                 price,
                                 amt / 6,
                                 population + cap,
                                 population + citymax,
                                 0,
                                 amt);
        }
    }
    
    /* Add demand (normal) items */
    int num_items = 4;
    int sum = 1;
    while((num_items > 0) && (sum > 0)) {
        int dm = 0;
        for (size_t i = 0; i < Items::size(); i++)
        {
            dm += demand[i];
        }
        dm = getrandom(dm);
        size_t i;
        sum = 0;
        for (i = 0; i < Items::size(); i++) {
            sum += demand[i];
            if (dm < sum) break;
        }
        if (dm >= sum) continue;
        
        int amt = Globals->CITY_MARKET_NORMAL_AMT;
        amt = demand[i] * amt / 4;
        int price;
    
        if (Globals->RANDOM_ECONOMY) {
            amt += getrandom(amt);
            price = static_cast<int>((ItemDefs[i].baseprice *(100 + getrandom(50UL))) / 100);
        } else {
            price = static_cast<int>(ItemDefs[i].baseprice);
        }
                            
        cap = (citymax/4);
        offset = - (citymax/20) + ((5-num_items) * citymax * 3/40);
        markets.emplace_back(M_SELL,
                             i,
                             price,
                             amt / 6,
                             population + cap + offset,
                             population + citymax,
                             0,
                             amt);
        demand[i] = 0;
        num_items--;    
    }
    
    /* Add supply (normal) items */
    num_items = 2;
    sum = 1;
    while((num_items > 0) && (sum > 0)) {
        int su = 0;
        for (size_t i = 0; i < Items::size(); i++)
        {
            su += supply[i];
        }
        su = getrandom(su);
        size_t i;
        sum = 0;
        for (i = 0; i < Items::size(); i++) {
            sum += supply[i];
            if (su < sum) break;
        }
        if (su >= sum) continue;

        int amt = Globals->CITY_MARKET_NORMAL_AMT;
        amt = supply[i] * amt / 4;
        int price;

        if (Globals->RANDOM_ECONOMY) {
            amt += getrandom(amt);
            price = static_cast<int>((ItemDefs[i].baseprice *(150 + getrandom(50UL))) / 100);
        } else {
            price = static_cast<int>(ItemDefs[i].baseprice);
        }
                            
        cap = (citymax/4);
        offset = ((3-num_items) * citymax * 3 / 40);
        if (supply[i] < 4) offset += citymax / 20;
        markets.emplace_back(M_BUY,
                             i,
                             price,
                             0,
                             population + cap + offset,
                             population + citymax,
                             0,
                             amt);
        supply[i] = 0;
        num_items--;
    }
    /* Set up the trade items */
    int buy1 = getrandom(numtrade);
    int buy2 = getrandom(numtrade);
    int sell1 = getrandom(numtrade);
    int sell2 = getrandom(numtrade);
    int tradebuy = 0;
    int tradesell = 0;
    offset = 0;
    cap = 0;

    buy1 = getrandom(numtrade);
    while (buy1 == buy2) buy2 = getrandom(numtrade);
    while (sell1 == buy1 || sell1 == buy2) sell1 = getrandom(numtrade);
    while (sell2 == sell1 || sell2 == buy2 || sell2 == buy1)
        sell2 = getrandom(numtrade);

    for (size_t i = 0; i < Items::size(); i++) {
        if (ItemDefs[i].flags & ItemType::DISABLED) continue;
        if (ItemDefs[i].flags & ItemType::NOMARKET) continue;

        if (ItemDefs[ i ].type & IT_TRADE) {
            int addbuy = 0;
            int addsell = 0;

            if (buy1 == 0 || buy2 == 0) {
                addbuy = 1;
            }
            buy1--;
            buy2--;

            if (sell1 == 0 || sell2 == 0) {
                addsell = 1;
            }
            sell1--;
            sell2--;

            if (addbuy) {
                int amt = Globals->CITY_MARKET_TRADE_AMT;
                int price;

                if (Globals->RANDOM_ECONOMY) {
                    amt += getrandom(amt);
                    if (Globals->MORE_PROFITABLE_TRADE_GOODS) {
                        price = static_cast<int>((ItemDefs[i].baseprice * (250 + getrandom(100UL))) / 100);
                    } else {
                        price = static_cast<int>((ItemDefs[i].baseprice * (150 + getrandom(50UL))) / 100);
                    }
                } else {
                    price = static_cast<int>(ItemDefs[i].baseprice);
                }
                
                cap = (citymax/2);
                tradesell++;
                offset = - (citymax/20) + tradesell * (tradesell * tradesell * citymax/40);
                if (cap + offset < citymax) {
                    markets.emplace_back(M_SELL,
                                         i,
                                         price,
                                         amt / 5,
                                         cap + population + offset,
                                         citymax + population,
                                         0,
                                         amt);
                }
            }

            if (addsell) {
                int amt = Globals->CITY_MARKET_TRADE_AMT;
                int price;

                if (Globals->RANDOM_ECONOMY) {
                    amt += getrandom(amt);
                    if (Globals->MORE_PROFITABLE_TRADE_GOODS) {
                        price = static_cast<int>((ItemDefs[i].baseprice * (100 + getrandom(90UL))) / 100);
                    } else {
                        price = static_cast<int>((ItemDefs[i].baseprice * (100 + getrandom(50UL))) / 100);
                    }
                } else {
                    price = static_cast<int>(ItemDefs[i].baseprice);
                }

                cap = (citymax/2);
                offset = tradebuy++ * (citymax/6);
                if (cap+offset < citymax) {
                    markets.emplace_back(M_BUY,
                                         i,
                                         price,
                                         amt / 6,
                                         cap + population + offset,
                                         citymax + population,
                                         0,
                                         amt);
                }
            }
        }
    }
}

void ARegion::SetupProds()
{
    const auto& typer = TerrainDefs[type];

    if (Globals->FOOD_ITEMS_EXIST) {
        if (typer.economy) {
            // Foodchoice = 0 or 1 if inland, 0, 1, or 2 if coastal
            int foodchoice = getrandom(2 +
                    (Globals->COASTAL_FISH && IsCoastal()));
            switch (foodchoice) {
                case 0:
                    if (!(ItemDefs[Items::Types::I_GRAIN].flags & ItemType::DISABLED))
                    {
                        products.emplace_back(Items::Types::I_GRAIN, static_cast<int>(typer.economy));
                    }
                    break;
                case 1:
                    if (!(ItemDefs[Items::Types::I_LIVESTOCK].flags & ItemType::DISABLED))
                    {
                        products.emplace_back(Items::Types::I_LIVESTOCK, static_cast<int>(typer.economy));
                    }
                    break;
                case 2:
                    if (!(ItemDefs[Items::Types::I_FISH].flags & ItemType::DISABLED))
                    {
                        products.emplace_back(Items::Types::I_FISH, static_cast<int>(typer.economy));
                    }
                    break;
            }
        }
    }

    for(const auto& prod: typer.prods)
    {
        const auto& item = prod.product;
        int chance = prod.chance;
        int amt = prod.amount;
        if (item.isValid()) {
            if (!(ItemDefs[item].flags & ItemType::DISABLED) && (getrandom(100) < chance))
            {
                products.emplace_back(item, amt);
            }
        }
    }
}

/* Create a town randomly */
void ARegion::AddTown()
{
    AString tname(AGetNameString(AGetName(1, *this)));
    const auto size = DetermineTownSize();
    AddTown(size, tname);
}

/* Create a town of any type with given name */
void ARegion::AddTown(const AString& tname)
{
    const auto size = DetermineTownSize();
    AddTown(size, tname);    
}

/* Create a town of given Town Type */
void ARegion::AddTown(TownTypeEnum size)
{
    AString tname(AGetNameString(AGetName(1, *this)));
    AddTown(size, tname);    
}

/* Create a town of specific type with name
 * All other town creation functions call this one
 * in the last instance. */
void ARegion::AddTown(TownTypeEnum size, const AString& town_name)
{
    town = std::make_shared<TownInfo>();
    town->name = town_name;
    SetTownType(size);
    SetupCityMarket();
    /* remove all lairs */

    objects.remove_if([](auto& obj){
        if (obj->type != Objects::Types::O_DUMMY)
        {
            if ((ObjectDefs[obj->type].monster.isValid()) &&
                (!(ObjectDefs[obj->type].flags & ObjectType::CANENTER)))
            {
                    obj->units.clear();
                    return true;
            }
        }
        return false;
    });
}

// Used at start to set initial town's size
TownTypeEnum ARegion::DetermineTownSize()
{
    // is it a city?
    if (getrandom(300U) < Globals->TOWN_DEVELOPMENT) {
        return TownTypeEnum::TOWN_CITY;
    }
    // is it a town?
    if (getrandom(220U) < Globals->TOWN_DEVELOPMENT + 10) {
        return TownTypeEnum::TOWN_TOWN;
    }
    // ... then it's a village!
    return TownTypeEnum::TOWN_VILLAGE;
}

// Set an existing town to a specific town type
void ARegion::SetTownType(TownTypeEnum town_type)
{
    if (!town) return;
    // set some basics
    town->hab = TownHabitat();
    town->pop = town->hab * 2 / 3;
    town->dev = TownDevelopment();
    
    // Sanity check
    if ((town_type < TownTypeEnum::TOWN_VILLAGE) || (town_type > TownTypeEnum::TOWN_CITY)) return;

    int level = static_cast<int>(town_type);
    // increment values
    int poptown = getrandom((level -1) * (level -1) * Globals->CITY_POP/12) + level * level * Globals->CITY_POP/12;
    town->hab += poptown;
    town->pop = town->hab * 2 / 3;
    development += level * 6 + 2;
    town->dev = TownDevelopment();

    // now increment until we reach the right size
    while(town->TownType() != town_type) {
        // Increase?
        if (town_type > town->TownType()) {
            development += static_cast<int>(getrandom(Globals->TOWN_DEVELOPMENT / 10 + 5));
            int poplus = getrandom(Globals->CITY_POP/3) + getrandom(Globals->CITY_POP/3);
            // don't overgrow!
            while (town->pop + poplus > Globals->CITY_POP) {
                poplus = poplus / 2;
            }
            town->hab += poplus;
            town->pop = town->hab * 2 / 3;
            town->dev = TownDevelopment();
        }
            // or decrease...
        else {
            development -= getrandom(20 - static_cast<int>(Globals->TOWN_DEVELOPMENT / 10));
            int popdecr = getrandom(Globals->CITY_POP/3) + getrandom(Globals->CITY_POP/3);
            // don't depopulate
            while ((town->pop < popdecr) || (town->hab < popdecr)) {
                popdecr = popdecr / 2;    
            }
            town->hab -= popdecr;
            town->pop = town->hab * 2 / 3;
            town->dev = TownDevelopment();
        }
    }
}

void ARegion::UpdateEditRegion()
{
    // redo markets and entertainment/tax income for extra people.
    SetIncome();
    markets.PostTurn(Population(), Wages());
    
    //Replace man selling
    markets.remove_if([](const auto& m){
        if (ItemDefs[m->item].type & IT_MAN)
        {
            return true;
        }

        return false;
    });
    /*for(auto it = markets.begin(); it != markets.end(); ++it)
    {
        const auto& m = *it;
        if (ItemDefs[m->item].type & IT_MAN)
        {
            markets.erase(it);
        }
    }*/

    float ratio = static_cast<float>(ItemDefs[race].baseprice) / static_cast<float>(Globals->BASE_MAN_COST * 10);
    // hack: include wage factor of 10 in float calculation above
    markets.emplace_back(M_BUY,
                         race,
                         calculateWagesWithRatio(ratio),
                         Population() / 25,
                         0,
                         10000,
                         0,
                         2000);

    if (Globals->LEADERS_EXIST) {
        ratio = static_cast<float>(ItemDefs[Items::Types::I_LEADERS].baseprice) / static_cast<float>(Globals->BASE_MAN_COST * 10);
        // hack: include wage factor of 10 in float calculation above
        markets.emplace_back(M_BUY,
                             Items::Types::I_LEADERS,
                             calculateWagesWithRatio(ratio),
                             Population() / 125,
                             0,
                             10000,
                             0,
                             400);
    }    
}

void ARegion::SetupEditRegion()
{
    // Direct copy of SetupPop() except that it calls AddTown(AString*)
    const auto& typer = TerrainDefs[type];
    habitat = typer.pop+1;
    // Population factor: 5 times
    if (habitat > 1) habitat *= 5;
    if ((habitat < 100) && (typer.similar_type != Regions::Types::R_OCEAN)) habitat = 100;

    int pop = typer.pop;
    int mw = typer.wages;
    
    // fix economy when MAINTENANCE_COST has been adjusted
    mw += static_cast<int>(Globals->MAINTENANCE_COST) - 10;
    if (mw < 0) mw = 0;

    if (pop == 0) {
        population = 0;
        basepopulation = 0;
        wages = 0;
        maxwages = 0;
        wealth = 0;
        return;
    }

    // Only select race here if it hasn't been set during Race Growth
    // in the World Creation process.
    if (!race.isValid() || (!Globals->GROW_RACES)) {
        std::vector<Items> allowed_races(typer.races.begin(), typer.races.end());
        if(IsCoastal())
        {
            allowed_races.insert(allowed_races.end(), typer.coastal_races.begin(), typer.coastal_races.end());
        }

        race.invalidate();
        while (!race.isValid() || (ItemDefs[race].flags & ItemType::DISABLED)) {
            size_t n = getrandom(allowed_races.size());
            race = allowed_races[n];
        }
    }

    habitat = habitat * 2/3 + getrandom(habitat/3);
    const auto& mt = FindRace(ItemDefs[race].abr);
    if (mt.terrain == typer.similar_type) {
        habitat = (habitat * 9)/8;
    }
    if (!IsNativeRace(race)) {
        habitat = (habitat * 4)/5;
    }
    basepopulation = habitat / 3;
    // hmm... somewhere not too far off equilibrium pop
    population = habitat * (60 + getrandom(6) + getrandom(6)) / 100;
    
    // Setup development
    int level = 1;
    development = 1;
    int prev = 0;
    while (level < mw) {
        development++;
        prev++;
        if (prev > level) {
            level++;
            prev = 0;
        }
    }
    development += getrandom(25);

    if (Globals->TOWNS_EXIST) {
        int adjacent = 0;
        unsigned int prob = Globals->TOWN_PROBABILITY;
        if (prob < 1) prob = 100;
        unsigned int townch = 80000 / prob;
        if (Globals->TOWNS_NOT_ADJACENT) {
            for (const auto& n_w: neighbors) {
                if(!n_w.expired())
                {
                    const auto& newregion = n_w.lock();
                    if(newregion->town)
                    {
                        adjacent++;
                    }
                }
            }
        }
        if (Globals->LESS_ARCTIC_TOWNS) {
            unsigned int dnorth = GetPoleDistance(Directions::Types::D_NORTH);
            unsigned int dsouth = GetPoleDistance(Directions::Types::D_SOUTH);
            if (dnorth < 9)
            {
                townch = townch + 25 * (9 - dnorth) * (9 - dnorth) * Globals->LESS_ARCTIC_TOWNS;
            }
            if (dsouth < 9)
            {
                townch = townch + 25 * (9 - dsouth) * (9 - dsouth) * Globals->LESS_ARCTIC_TOWNS;
            }
        }
        unsigned int spread = Globals->TOWN_SPREAD;
        if (spread > 100) spread = 100;
        unsigned int townprob = (TerrainDefs[type].economy * 4 * (100 - spread) + 100 * spread) / 100;
        if (adjacent > 0) townprob = townprob * (100 - Globals->TOWNS_NOT_ADJACENT) / 100;
        if (getrandom(townch) < townprob) {
            AddTown("Newtown");
        }
    }

    // set up work and entertainment income
    SetIncome();

    float ratio = static_cast<float>(ItemDefs[race].baseprice) / static_cast<float>(Globals->BASE_MAN_COST * 10);
    // hack: include wage factor of 10 in float assignment above
    // Setup Recruiting
    markets.emplace_back(M_BUY,
                         race,
                         calculateWagesWithRatio(ratio),
                         Population() / 25,
                         0,
                         10000,
                         0,
                         2000);

    if (Globals->LEADERS_EXIST) {
        ratio = static_cast<float>(ItemDefs[Items::Types::I_LEADERS].baseprice) / static_cast<float>(Globals->BASE_MAN_COST * 10);
        // hack: include wage factor of 10 in float assignment above
        markets.emplace_back(M_BUY,
                             Items::Types::I_LEADERS,
                             calculateWagesWithRatio(ratio),
                             Population() / 125,
                             0,
                             10000,
                             0,
                             400);
    }
}

void ARegion::UpdateProducts()
{
    for(const auto& prod: products) {
        int lastbonus = prod->baseamount / 2;
        int bonus = 0;

        if (prod->itemtype == Items::Types::I_SILVER && !prod->skill.isValid()) continue;

        for(const auto& o: objects) {
            if (o->incomplete < 1 &&
                    ObjectDefs[o->type].productionAided == prod->itemtype) {
                lastbonus /= 2;
                bonus += lastbonus;
            }
        }
        prod->amount = prod->baseamount + bonus;

        if (prod->itemtype == Items::Types::I_GRAIN || prod->itemtype == Items::Types::I_LIVESTOCK) {
            prod->amount += static_cast<int>((earthlore + clearskies) * 40) / prod->baseamount;
        }
    }
}

/* BaseDev is the development floor at which poor
 * regions stabilise without player activity */
int ARegion::BaseDev()
{
    int level = 1;
    int basedev = 1;
    int prev = 0;
    while (level <= TerrainDefs[type].wages) {
        prev++;
        basedev++;
        if (prev > level) {
            level++;
            prev = 0;
        }
    }
    
    basedev = (static_cast<int>(Globals->MAINTENANCE_COST) + basedev) / 2;
    return basedev;
}

/* ProdDev is the development floor for regions
 * factoring in player production in the region */
int ARegion::ProdDev()
{
    int basedev = BaseDev();
    for(const auto& p: products) {
        if (ItemDefs[p->itemtype].type & IT_NORMAL &&
            p->itemtype != Items::Types::I_SILVER) {
            basedev += p->activity;
        }
    }
    return basedev;
}

int ARegion::TownHabitat()
{
    // Effect of existing buildings
    int farm = 0;
    int inn = 0;
    int temple = 0;
    int caravan = 0;
    int fort = 0;
    for(const auto& obj: objects) {
        if (ObjectDefs[obj->type].protect > fort) fort = ObjectDefs[obj->type].protect;
        const auto& production_aided = ObjectDefs[obj->type].productionAided;
        if(!production_aided.isValid())
        {
            continue;
        }
        if (ItemDefs[production_aided].flags & IT_FOOD) farm++;
        if (production_aided == Items::Types::I_SILVER) inn++;
        if (production_aided == Items::Types::I_HERBS) temple++;
        if ((ObjectDefs[obj->type].flags & ObjectType::TRANSPORT)
            && (ItemDefs[production_aided].flags & IT_MOUNT)) caravan++;
    }
    int hab = 2;
    int step = 0;
    for (int i=0; i<5; i++) {
        if (fort > step) hab++;
        step *= 5;
        if (step == 0) step = 10;
    }
    int build = 0;
    if (farm) build++;
    if (inn) build++;
    if (temple) build++;
    if (caravan) build++;
    if (build > 2) build = 2;
    
    build++;
    hab = (build * build + 1) * hab * hab + habitat / 4 + 50;
    
    // Effect of town development on habitat:
    int totalhab = hab + (TownDevelopment() * (habitat + 800 + hab
        + Globals->CITY_POP / 2)) / 100;
    
    return totalhab;
}

// Use this to determine development advantages
// due to connecting roads
int ARegion::RoadDevelopment()
{
    // Road bonus
    int roads = 0;
    for (auto i = Directions::begin(); i != Directions::end(); ++i)
    {
        if (HasExitRoad(*i))
        {
            roads++;
        }
    }
    int dbonus = 0;
    if (roads > 0) {
        dbonus = RoadDevelopmentBonus(8, development);
        if (!town) dbonus = dbonus / 2;
    }
    // Maximum bonus of 45 to development for roads
    int bonus = 5;
    int leveloff = 1;
    int plateau = 4;
    int totalb = 0;
    while((dbonus > 0) && (totalb < 46)) {
        dbonus--;
        // reduce development adjustment gradually for large road bonuses
        if (leveloff >= plateau) if (bonus > 1) {
            bonus--;
            leveloff = 1;
            plateau--;
        }
        leveloff++;
        totalb += bonus;
    }
    return totalb;
}

// Measure of the economic development of a town
// on a scale of 0-100. Used for town growth/hab/limits
// and also for raising development through markets.
// Do NOT take roads into account as they are a bonus and
// considered outside of these limits.
int ARegion::TownDevelopment()
{
    int basedev = BaseDev();
    int df = development - basedev;
    if (df < 0) df = 0;
    if (df > 100) df = 100;
    
    return df;
} 

// Checks the growth potential of towns
// and cancels unlimited markets for Starting Cities
// that have been taken over
int ARegion::TownGrowth()
{
    int tarpop = town->pop;

    // Don't update population in Starting Cities
    if (!IsStartingCity()) {
        // Calculate target population from market activity
        int amt = 0;
        int tot = 0;
        for(const auto& m: markets) {
            if (Population() > m->minpop) {
                if (ItemDefs[m->item].type & IT_TRADE) {
                    if (m->type == M_BUY) {
                        amt += 5 * m->activity;
                        tot += 5 * m->maxamt;
                        /* regional economic improvement */
                        improvement += 3 * amt;
                    }
                } else {
                    if (m->type == M_SELL) {
                        // Only food items except fish are mandatory
                        // for town growth - other items can
                        // be used in replacement
                        if (ItemDefs[m->item].type & IT_FOOD) {
                            amt += 2 * m->activity;
                        } else amt += m->activity;
                        if ((ItemDefs[m->item].type & IT_FOOD)
                            && (m->item != Items::Types::I_FISH))    tot += 2 * m->maxamt;
                        if (ItemDefs[m->item].type & IT_TRADE) {
                            /* regional economic improvement */
                            improvement += 3 * amt;
                        }
                    }
                }
            }
        }
        
        if (amt > tot) amt = tot;

        if (tot) {
            tarpop += (Globals->CITY_POP * amt) / tot;
        } 
        // Let's bump tarpop up
        // tarpop = (tarpop * 5) / 4;
        if (tarpop > Globals->CITY_POP) tarpop = Globals->CITY_POP;
    }
    return tarpop;
}

/* Damage region because of pillaging */
void ARegion::Pillage()
{
    wealth = 0;
    int damage = development / 3;
    development -= damage;
    int popdensity = Globals->CITY_POP / 2000;
    AdjustPop(- damage * getrandom(popdensity) - getrandom(5 * popdensity));
    /* Stabilise at minimal development levels */
    while (Wages() < static_cast<int>(Globals->MAINTENANCE_COST / 20))
    {
        development += getrandom(5);
    }
}


/* Grow region and town population and set basic
 * migration parameters */
void ARegion::Grow()
{
    // We don't need to grow 0 pop regions
    if (basepopulation == 0) return;
    
    // growpop is the overall population growth    
    int growpop = 0;
    
    // Init migration parameters
    // immigrants = entering this region, 
    // emigrants = leaving
    immigrants = habitat - basepopulation;
    emigrants = population - basepopulation;
    
    // First, check regional population growth
    // Check resource production activity
    int activity = 0;
    int amount = 0;
    for(const auto& p: products) {
        if (ItemDefs[p->itemtype].type & IT_NORMAL &&
            p->itemtype != Items::Types::I_SILVER) {
            activity += p->activity;
            // bonuses for productivity are calculated from
            // the _baseamount_ of all resources.
            // because trade structures increase the produceable
            // amount and not the baseamount this creates an 
            // effective advantage for trade structures
            amount += p->baseamount;
        }
    }
    
    // Now set the target population for the hex
    // Ant: I'm not sure why population's being subtracted here..
    //         Shouldn't it be something like :
    //             tarpop = habitat + basepopulation?
    int tarpop = habitat - population + basepopulation;
    
    // Ant: Increase tarpop for any trading that's going on?
    //         Not sure why (habitat - basepopulation) is included
    if (amount) tarpop += ((habitat - basepopulation) * 2 * activity) /
                (3 * amount);
    
    // diff is the amount we can grow?
    int diff = tarpop - population;
    int adiff = abs(diff);
    //if (adiff < 0) adiff = adiff * (- 1);
    
    // Adjust basepop? 
    // raise basepop depending on production
    // absolute basepop increase
    if (diff > (basepopulation / 20)) {
        int gpop = getrandom(Globals->CITY_POP / 600);
        if (gpop > diff / 20) gpop = diff / 20;
        // relative basepop increase
        int relativeg = basepopulation * gpop / 1000;
        if (diff > habitat / 20) basepopulation += gpop + relativeg;
    }
    // lower basepop for extremely low levels of population
    if (population < basepopulation) {
        int depop = (basepopulation - population) / 4;
        basepopulation -= depop + getrandom(depop);            
    }
    
    // debug strings
    //Awrite(AString("immigrants = ") + immigrants);
    //Awrite(AString("emigrants = ") + emigrants);
    //Awrite(AString("adiff = ") + adiff);
    //Awrite(AString("diff = ") + diff);
    //Awrite(AString("habitat = ") + habitat);
    
    // Limit excessive growth at low pop / hab ratios
    // and avoid overflowing
    // What are grow2 and grow representing? Maybe these formulae
    // could be broken up a bit more and commented?
    long int grow2 = 5 * (static_cast<long int>(habitat) + (3 * static_cast<long int>(adiff)));
    //Awrite(AString("grow2 = ") + (unsigned int) grow2); // debug string
    
    // Ant: In the following formula, dgrow is almost always 0!
    long int dgrow = (static_cast<long int>(adiff) * static_cast<long int>(habitat)) / grow2;
    //if (diff < 0) dgrow = dgrow * (- 1);
    //Awrite(AString("dgrow = ") + (unsigned int) dgrow); // debug string
    
    // long int dgrow = ((long int) diff) * ((long int) habitat)
    //    / (5 * (long int) ((long int) habitat + 3 * (long int) abs(diff)));
    if (diff < 0) growpop -= static_cast<int>(dgrow);
    if (diff > 0) growpop += static_cast<int>(dgrow);
    /*
        Awrite(AString("growpop = ") + growpop);
        Awrite(AString("grow2 = ") + (unsigned int) grow2);
        Awrite(AString("adiff = ") + adiff);
        Awrite(AString("diff = ") + diff);
    */
    // update emigrants - only if region has a decent population level
    if (emigrants > 0) emigrants += diff;

    
    // Now check town population growth
    if (town) {
        int maxpop = TownGrowth();
        int tgrowth = maxpop - town->pop; // available room to grow
        immigrants += tgrowth;
        // less growth of towns in DYNAMIC_POPULATION
        // to balance town creation and population dynamics
        // through migration
        if (Globals->DYNAMIC_POPULATION) tgrowth = tgrowth / 4;
        // Dampen growth curve at high population levels
        // Ant: maybe this formula could be broken up a bit?
        //        also, is (2 * town->hab - town->pop) correct?
        //        it's not meant to be 2 * (town->hab - town->pop)?
        //        ((2 * town->hab) - town->pop) seems clearer
        float increase = static_cast<float>(tgrowth * (2 * town->hab - town->pop));
        float limitingfactor = static_cast<float>(10 * town->hab);
        
        // Ant: Not sure whether we still need the typecasts here
        growpop += static_cast<int>(increase / limitingfactor);
    }
    
    // Update population
    AdjustPop(growpop);
    
    /* Initialise the migration variables */
    migdev = 0;
    migfrom.clear();
}


/* Performs a search for each round of Migration for
 * the most attractive valid target region within
 * 2 hexes distance. */
void ARegion::FindMigrationDestination(int round)
{
    // is emigration possible?
    if (emigrants < 0) return;
    
    int maxattract = 0;
    ARegion::Handle target = shared_from_this();
    // Check all hexes within 2 hexes
    // range one neighbours
    for (const auto& nb_w: neighbors) {
        if (nb_w.expired())
        {
            continue;
        }
        const auto& nb = nb_w.lock();
        if (TerrainDefs[nb->type].similar_type == Regions::Types::R_OCEAN)
        {
            continue;
        }
        int ma = nb->MigrationAttractiveness(development, 1, round);
        // check that we didn't migrate there in previous round
        if ((ma > maxattract) &&
            (!((nb->xloc == target->xloc) && (nb->yloc == target->yloc)))) {
            // set migration target
            target = nb;
            maxattract = ma;
        }
        // range two neighbours
        for (const auto& nb2_w: nb->neighbors) {
            if (nb2_w.expired())
            {
                continue;
            }
            const auto& nb2 = nb2_w.lock();
            if (TerrainDefs[nb2->type].similar_type == Regions::Types::R_OCEAN)
            {
                continue;
            }
            ma = nb2->MigrationAttractiveness(development, 2, round);
            // check that we didn't migrate there the previous round
            if ((ma > maxattract) &&
                (!((nb2->xloc == target->xloc) && (nb2->yloc == target->yloc)))) {
                // set migration target
                target = nb2;
                maxattract = ma;
            }
        }    
    }
    // do we have a target?
    if (target.get() == this)
    {
        return;
    }

    // then add this region to the target's migfrom list
    target->migfrom.push_back(weak_from_this());
}

/* Attractiveness of the region as a destination for migrants */
int ARegion::MigrationAttractiveness(int homedev, int range, int round)
{
    int attractiveness = 0;
    int mdev = development;
    /* Is there enough immigration capacity? */
    if (immigrants < 100) return 0;    
    /* on the second round, consider as a mid-way target */
    if (round > 1) mdev = migdev;
    /* minimum development difference 8 x range */
    mdev -= 8 * range;
    if (mdev <= homedev) return 0;
    /* available entertainment */
    Production::WeakHandle p_w = products.GetProd(Items::Types::I_SILVER, Skills::Types::S_ENTERTAINMENT);
    const auto& p = p_w.lock();
    int entertain = p->activity / 20;
    /* available space */
    float space = 0.5;
    int offset = Globals->CITY_POP / 100;
    if (town) {
        space += static_cast<float>((habitat - population) + (town->hab - town->pop) + offset)
            / static_cast<float>(habitat + town->hab + offset);
    } else {
        space += static_cast<float>(habitat - population + offset) / static_cast<float>(habitat + offset);
    }
    /* attractiveness due to development */
    attractiveness += static_cast<int>(space * (static_cast<float>(100 * (mdev - homedev)) / static_cast<float>(homedev) + static_cast<float>(entertain)));
    
    return attractiveness;    
}

/* Performs migration for each region with a migration
 * route pointing to the region (i.e. element of migfrom AList),
 * adjusting population for hex of origin and itself */
void ARegion::Migrate()
{
    // calculate total potential migrants
    int totalmig = 0;
    if (!migfrom.empty()) {
        for(const auto& r_w: migfrom) {
            if (r_w.expired())
            {
                continue;
            }
            const auto& r = r_w.lock();
            totalmig += r->emigrants;
        }
    }
    
    // is there any migration to perform?
    if (totalmig == 0) return;
    
    // do each migration
    int totalimm = 0;
    for(const auto& r_w: migfrom) {
        const auto& r = r_w.lock();
        
        // figure range
        unsigned int xdist = absdiff(r->xloc, xloc);
        unsigned int ydist = absdiff(r->yloc, yloc);
        ydist = (ydist - xdist) / 2;
        unsigned int range = xdist + ydist;

        // sanity check - huh?
        if (range < 1) continue;
        //Original line: int migrants = (int) (immigrants * ((float) (r->emigrants / totalmig)));
        int migrants = static_cast<int>(static_cast<float>(immigrants * r->emigrants) / static_cast<float>(totalmig));
        int mdiff = development - 7 - r->development;
        mdiff -= static_cast<int>(8 * (range - 1));
        if (mdiff < 0) continue;
        int mmult = 1;
        for (int x=1; x*x < mdiff; x++)
        {
            mmult = x;
        }
        // adjust migrants according to development difference
        migrants = static_cast<int>(static_cast<float>(migrants) * (static_cast<float>((mdiff + 100) * mmult) / 500));
        AdjustPop(migrants);
        r->AdjustPop(-migrants);
        r->emigrants -= migrants;
        totalimm += migrants;
        AString wout = AString("Migrating from ") /* + (r->name) + " in " */ + r->xloc
            + "," + (r->yloc) + " to " /* + name + " in " */ + (xloc) + "," + yloc 
            + ": " + migrants + " migrants.";
        Awrite(wout);
        // set the region's mid-way migration development
        r->migdev = (development - 8 * static_cast<int>(range-1) + r->development) / 2;
        if (r->development > migdev) r->migdev = r->development;
    }
    // reduce possible immigrants
    immigrants -= totalimm;
    // clear migfrom
    migfrom.clear();
}

void ARegion::PostTurn(const ARegionList& pRegs)
{

    /* Check decay */
    if (Globals->DECAY) {
        DoDecayCheck(pRegs);
    }
    
    /* Development increase due to player activity */
    // scale improvement
    float imp1 = static_cast<float>(improvement) / 25;
    float imp2 = static_cast<float>(improvement * 2 + 15) / 3;
    improvement = static_cast<int>(imp1 * imp2);
    // development increase possible?
    if (improvement > development) {
        int diff = improvement - development;
        /* Let road development increase chance of improvement */
        int progress = development - RoadDevelopment();
        /* Three chances to improve */
        for (int a=0; a<3; a++) if (getrandom(progress) < diff) development++;
    }
    
    /* Development increase for very poor regions */
    int basedev = ProdDev();
    
    if (basedev > development) {
        if (getrandom(basedev) > (basedev - development)) development++;    
    }

    /* Check if we were a starting city and got taken over */
    if (IsStartingCity() && !HasCityGuard() && !Globals->SAFE_START_CITIES) {
        // Make sure we haven't already been modified.
        int done = 1;
        for(const auto& m: markets) {
            if (m->minamt == -1) {
                done = 0;
                break;
            }
        }
        if (!done) {
            markets.clear();
            SetupCityMarket();
            float ratio = static_cast<float>(ItemDefs[race].baseprice) /
                          static_cast<float>(Globals->BASE_MAN_COST * 10);
            // Setup Recruiting
            markets.emplace_back(M_BUY,
                                 race,
                                 calculateWagesWithRatio(ratio),
                                 Population() / 25,
                                 0,
                                 10000,
                                 0,
                                 2000);
            if (Globals->LEADERS_EXIST) {
                ratio = static_cast<float>(ItemDefs[Items::Types::I_LEADERS].baseprice) /
                        static_cast<float>(Globals->BASE_MAN_COST);
                markets.emplace_back(M_BUY,
                                     Items::Types::I_LEADERS,
                                     calculateWagesWithRatio(ratio),
                                     Population() / 125,
                                     0,
                                     10000,
                                     0,
                                     400);
            }
        }
    }

    /* Set wage income and entertainment */
    if (type != Regions::Types::R_NEXUS) {
        SetIncome();
    }
    
    /* update markets */
    markets.PostTurn(Population(),Wages());

    /* update resources */
    UpdateProducts();

    // Set these guys to 0.
    earthlore = 0;
    clearskies = 0;

    for(const auto& o: objects) {
        for(const auto& u: o->units) {
            u->PostTurn(*this);
        }
    }
}
