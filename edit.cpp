#ifdef WIN32
#include <memory.h>  // Needed for memcpy on windows
#endif

#include <string.h>

#include "game.h"
#include "unit.h"
#include "fileio.h"
#include "astring.h"
#include "gamedata.h"

int Game::EditGame(bool &pSaveGame)
{
    pSaveGame = false;

    Awrite("Editing an Atlantis Game: ");
    do {
        bool exit = false;

        Awrite("Main Menu");
        Awrite("  1) Find a region...");
        Awrite("  2) Find a unit...");
        Awrite("  3) Create a new unit...");
        Awrite("  qq) Quit without saving.");
        Awrite("  x) Exit and save.");
        Awrite("> ");

        AString *pStr = AGetString();
        Awrite("");

        if (*pStr == "qq") {
            exit = true;
            Awrite("Quitting without saving.");
        } else if (*pStr == "x") {
            exit = true;
            pSaveGame = true;
            Awrite("Exit and save.");
        } else if (*pStr == "1") {
            ARegion::WeakHandle pReg = EditGameFindRegion();
            if (!pReg.expired())
            {
                EditGameRegion(pReg.lock());
            }
        } else if (*pStr == "2") {
            EditGameFindUnit();
        } else if (*pStr == "3") {
            EditGameCreateUnit();
        } else {
            Awrite("Select from the menu.");
        }

        delete pStr;
        if (exit) {
            break;
        }
    } while(1);

    return(1);
}

ARegion::WeakHandle Game::EditGameFindRegion()
{
    ARegion::WeakHandle ret;
    int x, y, z;
    AString *pStr = 0, *pToken = 0;
    Awrite("Region coords (x y z):");
    do {
        pStr = AGetString();

        pToken = pStr->gettoken();
        if (!pToken) {
            Awrite("No such region.");
            break;
        }
        x = pToken->value();
        SAFE_DELETE(pToken);

        pToken = pStr->gettoken();
        if (!pToken) {
            Awrite("No such region.");
            break;
        }
        y = pToken->value();
        SAFE_DELETE(pToken);

        pToken = pStr->gettoken();
        if (pToken) {
            z = pToken->value();
            SAFE_DELETE(pToken);
        } else {
            z = 0;
        }

        ARegion::WeakHandle pReg = regions.GetRegion(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
        if (pReg.expired()) {
            Awrite("No such region.");
            break;
        }

        ret = pReg;
    } while(0);

    if (pStr) delete pStr;

    return ret;
}

void Game::EditGameFindUnit()
{
    AString *pStr;
    Awrite("Which unit number?");
    pStr = AGetString();
    int num = pStr->value();
    delete pStr;
    Unit::WeakHandle pUnit = GetUnit(num);
    if (pUnit.expired()) {
        Awrite("No such unit!");
        return;
    }
    EditGameUnit(pUnit.lock());
}

void Game::EditGameRegion(const ARegion::Handle& pReg)
//copied direct from AtlantisDev 030730 post
{
    do {
        Awrite( AString("Region ") + pReg->num + ": " + pReg->Print(regions) );
        Awrite( " 1) Edit objects..." );
        Awrite( " 2) Edit terrain..." );
        Awrite( " q) Return to previous menu." );

        bool exit = false;
        AString *pStr = AGetString();
        if ( *pStr == "1" ) {
            EditGameRegionObjects( pReg );
        }
        else if ( *pStr == "2" ) {
            EditGameRegionTerrain( pReg );
        }
        else if ( *pStr == "q" ) {
            exit = true;
        }
        else {
            Awrite( "Select from the menu." );
        }
        if (pStr) delete pStr;

        if ( exit ) {
        break;
        }
    }
    while( 1 );
}


/* RegionEdit Patch 030829 BS */
void Game::EditGameRegionObjects(const ARegion::Handle& pReg )
//template copied from AtlantisDev 030730 post. Modified option a, added option h, m.
{
    do {
        Awrite( AString( "Region: " ) + pReg->ShortPrint( regions ) );
        Awrite( "" );
        int i = 0;
        AString temp = AString("");
        for(const auto& obj: pReg->objects) {
            temp = AString ((AString(i) + ". " + *obj->name + " : " + ObjectDefs[obj->type].name));
//            if (Globals->HEXSIDE_TERRAIN && obj->hexside>-1) temp += AString( AString(" (side:") + DirectionAbrs[obj->hexside] + ").");
            Awrite(temp);
            i++;
        }
        Awrite( "" );

        Awrite( " [a] [object type] [dir] to add object" );
        Awrite( " [d] [index] to delete object" );
//        if (Globals->HEXSIDE_TERRAIN) Awrite( " [h] [index] [dir] to change the hexside of an object" );
        Awrite( " [n] [index] [name] to rename object" );
//        if (Globals->HEXSIDE_TERRAIN) Awrite( " [m] [index] to add/delete a mirrored object" );
        Awrite( " q) Return to previous menu." );

        bool exit = false;
        AString *pStr = AGetString();
        if ( *pStr == "q" ) {
            exit = true;
        } else {
            AString *pToken = 0;
            do {
                pToken = pStr->gettoken();
                if ( !pToken ) {
                    Awrite( "Try again." );
                    break;
                }

                // add object
                if (*pToken == "a") {
                    SAFE_DELETE( pToken );
                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    Objects objType = ParseObject(pToken, 0);
                    if ( !objType.isValid() || (ObjectDefs[objType].flags & ObjectType::DISABLED) ) {
                        Awrite( "No such object." );
                        break;
                    }
                    SAFE_DELETE( pToken );

                    /*
                    int dir=-1;
                    if (ObjectDefs[objType].hexside && Globals->HEXSIDE_TERRAIN ) {
                        if (!ObjectIsShip(objType) || !(TerrainDefs[pReg->type].similar_type == Regions::Types::R_OCEAN) ) {
                            pToken = pStr->gettoken();
                            if (!pToken) {
                                Awrite( "Specify direction" );
                                break;
                            }
                            dir = ParseHexside(pToken);
                            if (dir<0) {
                                Awrite("Incorrect direction. Use N,NE,SE,S,SW,NW");
                                break;
                            }
                        }
                    }
                    */

                    const auto& o = pReg->objects.emplace_back(std::make_shared<Object>(pReg));
                    o->type = objType;
                    o->incomplete = 0;
                    o->inner = -1;
                    // o->hexside = dir;
                    if (o->IsFleet()) {
                        o->num = shipseq++;
                        o->name = new AString(AString("Fleet") + " [" + o->num + "]");
                    }
                    else {
                        o->num = pReg->buildingseq++;
                        o->name = new AString(AString("Building") + " [" + o->num + "]");
                    }
                }
                // delete object
                else if (*pToken == "d") {
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    int index = pToken->value();
                    if ( (index < 0) || (static_cast<size_t>(index) >= pReg->objects.size()) ) { //modified minimum to <0 to allow deleting object 0. 030824 BS
                        Awrite( "Incorrect index." );
                        break;
                    }
                    SAFE_DELETE( pToken );

                    auto it = std::next(pReg->objects.begin(), index);
                    pReg->objects.erase(it);
                }
    //hexside change
    /*            else if (*pToken == "h") {
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    int index = pToken->value();
                    if ( (index < 1) || (index >= pReg->objects.Num()) ) {
                        Awrite( "Incorrect index." );
                        break;
                    }
                    SAFE_DELETE( pToken );

                    int i = 0;
                    Object *tmp = (Object *)pReg->objects.First();
                    for (i = 0; i < index; i++) tmp = (Object *)pReg->objects.Next(tmp);

                    if (!(ObjectDefs[tmp->type].hexside)) {
                        Awrite("Not a hexside object.");
                        break;
                    }

                    if (!Globals->HEXSIDE_TERRAIN) {
                        Awrite("Hexside terrain disabled under game rules.");
                        break;
                    }

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Specify Direction." );
                        break;
                    }

                    int dir=-1;
                    dir = ParseHexside(pToken);
                    if (dir==-1) {
                        Awrite("Incorrect direction. Use N,NE,SE,S,SW,NW");
                        break;
                    }

                    SAFE_DELETE(pToken);
                    if (dir) {
                        tmp->hexside = dir;
                        if (tmp->mirror) { // reset mirrors, else problems later
                            tmp->mirror->mirror = NULL;
                            tmp->mirror->mirrornum = -1;
                            Awrite("Object mirroring removed");
                        }
                        tmp->mirrornum = -1;
                        tmp->mirror = NULL;
                    }
                }
    //mirror change
                else if (*pToken == "m") {
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    int index = pToken->value();
                    if ( (index < 1) || (index >= pReg->objects.Num()) ) {
                        Awrite( "Incorrect index." );
                        break;
                    }
                    SAFE_DELETE( pToken );

                    int i = 0;
                    Object *tmp = (Object *)pReg->objects.First();
                    for (i = 0; i < index; i++) tmp = (Object *)pReg->objects.Next(tmp);

                    // if has a mirror, delete the mirror
                    if (tmp->mirror) {
    //                    Awrite(AString(AString("Mirror ") + tmp->mirror->name + " deleted."));
                        Awrite("Mirror deleted");
                        tmp->mirror->region->objects.Remove(tmp->mirror);
                        tmp->mirror == NULL;
                        tmp->mirrornum == -1;
                    }

                    else {
                        if (!(ObjectDefs[tmp->type].hexside)) {
                            Awrite("Not a hexside object.");
                            break;
                        }
                        if (tmp->hexside < 0) {
                            Awrite("Object not on a hexside.");
                            break;
                        }
                        if (tmp->IsFleet()) {
                            Awrite("Fleets cannot be mirrored.");
                            break;
                        }
                        if (!Globals->HEXSIDE_TERRAIN) {
                            Awrite("Hexside terrain disabled under game rules.");
                            break;
                        }

                        if (!pReg->neighbors[tmp->hexside]) {
                            Awrite("No neighbouring region.");
                            break;
                        }

                        Object *o = new Object(pReg->neighbors[tmp->hexside]);
                        o->num = pReg->neighbors[tmp->hexside]->buildingseq++;
                        o->type = ObjectDefs[tmp->type].mirror;
                        o->name = new AString(AString("Building [") + o->num + "]");
                        o->incomplete = 0;
                        o->hexside = pReg->GetRealDirComp(tmp->hexside);
                        o->inner = -1;
                        o->mirrornum = tmp->num;
                        o->mirror = tmp;
                        pReg->neighbors[tmp->hexside]->objects.Add(o);

                        tmp->mirrornum = o->num;
                        tmp->mirror = o;
                        Awrite("Mirror added");
                    }
                }
    */
    // rename object
                else if (*pToken == "n") {
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    int index = pToken->value();
                    if ( (index < 1) || (static_cast<size_t>(index) >= pReg->objects.size()) ) {
                        Awrite( "Incorrect index." );
                        break;
                    }
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "No name given." );
                        break;
                    }

                    auto it = std::next(pReg->objects.begin(), index);

                    AString * newname = pToken->getlegal();
                    SAFE_DELETE(pToken);
                    if (newname) {
                        const auto& tmp = *it;
                        delete tmp->name;
                        *newname += AString(" [") + tmp->num + "]";
                        tmp->name = newname;
                    }
                }

            } while( 0 );
        if (pToken) delete pToken;
        }
        if (pStr) delete pStr;

        if (exit) {
            break;
        }
    }
    while( 1 );
}

void Game::EditGameRegionTerrain(const ARegion::Handle& pReg)
{
    do {
        Awrite("");
        Awrite( AString( "Region: " ) + pReg->Print( regions ) );
        Awrite( "" );
// write pop stuff
        Awrite( AString("") + pReg->population + " " + ItemDefs[pReg->race].names + " basepop");
        if (pReg->town) Awrite( AString("") + pReg->town->pop + " " + ItemDefs[pReg->race].names + " townpop");
        Awrite( AString("") + pReg->Population() + " " + ItemDefs[pReg->race].names + " totalpop");

// write wages
        Awrite(AString("Wages: ") + pReg->WagesForReport() + ".");
        Awrite(AString("Maxwages: ") + pReg->maxwages + ".");

// write products
        AString temp = "Products: ";
        bool has = false;
        for(const auto& p: pReg->products) {
            if (ItemDefs[p->itemtype].type & IT_ADVANCED) {
                if (has) {
                    temp += AString(", ") + p->WriteReport();
                } else {
                    has = true;
                    temp += p->WriteReport();
                }
            } else {
                if (p->itemtype == Items::Types::I_SILVER) {
                    if (p->skill == Skills::Types::S_ENTERTAINMENT) {
                        Awrite (AString("Entertainment available: $") +
                                    p->amount + ".");
                    }
                } else {
                    if (has) {
                        temp += AString(", ") + p->WriteReport();
                    } else {
                        has = true;
                        temp += p->WriteReport();
                    }
                }
            }
        }
        if (!has) temp += "none";
        temp += ".";
        Awrite(temp);
        Awrite( "" );

        if (Globals->GATES_EXIST && pReg->gate && pReg->gate != -1) {
            Awrite(AString("There is a Gate here (Gate ") + pReg->gate +
                " of " + (regions.numberofgates) + ").");
            if (Globals->GATES_NOT_PERENNIAL) Awrite(AString("This gate opens "
                "in month ") + pReg->gatemonth);
            Awrite("");
        }


        Awrite( " [t] [terrain type] to modify terrain type" );
        Awrite( " [r] [race] to modify local race" );
        Awrite( "     (use none, None or 0 to unset)" );
        Awrite( " [w] [maxwages] to modify local wages" );
        Awrite( " [p] to regenerate products according to terrain type" );
        Awrite( " [g] to regenerate all according to terrain type" );
        if (pReg->gate > 0) Awrite( " [dg] to delete the gate in this region" );
        else Awrite( " [ag] to add a gate to this region" );
        Awrite( " [n] [name] to modify region name" );
        if (pReg->town) {
        Awrite( " [town] to regenerate a town" );
        Awrite( " [deltown] to remove a town" );
        Awrite( " [tn] [name] to rename a town" );
        Awrite( " [v] to view/modify town markets" );
        } else Awrite( " [town] to add a town" );
        Awrite( " q) Return to previous menu." );

        bool exit = false;
        AString *pStr = AGetString();
        if ( *pStr == "q" ) {
            exit = true;
        } else {
            AString *pToken = 0;
            do {
                pToken = pStr->gettoken();
                if ( !pToken ) {
                    Awrite( "Try again." );
                    break;
                }

                // modify terrain
                if (*pToken == "t") {
                    SAFE_DELETE( pToken );
                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    Regions terType = ParseTerrain(pToken);
                    if (!terType.isValid()) {
                        Awrite( "No such terrain." );
                        break;
                    }
                    SAFE_DELETE( pToken );

                    pReg->type = terType;
                }
                else if (*pToken == "r") {
                    SAFE_DELETE( pToken );
                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    Items prace = ParseAllItems(pToken);
                    if (!(ItemDefs[prace].type & IT_MAN) || (ItemDefs[prace].flags & ItemType::DISABLED) ) {
                        if (!(*pToken == "none" || *pToken == "None" || *pToken == "0")) {
                            Awrite( "No such race." );
                            break;
                        } else {
                            prace.invalidate();
                        }
                    }
                    if (prace != 0) pReg->race = prace;
                    pReg->UpdateEditRegion();
                    SAFE_DELETE( pToken );
                }
                else if (*pToken == "dg") {
                    SAFE_DELETE( pToken );
                    if (Globals->DISPERSE_GATE_NUMBERS) {
                        pReg->gate = 0;
                        regions.numberofgates--;
                    } else {
                        if (pReg->gate > 0) {
                            int numgates = static_cast<int>(regions.numberofgates);
                            for(const auto& reg: regions) {
                                if (reg->gate == numgates) {
                                    reg->gate = pReg->gate;
                                    pReg->gate = 0;
                                    regions.numberofgates--;
                                    break;
                                }
                            }
                            Awrite("Error: Could not find last gate");
                        }
                    }
                }
                else if (*pToken == "ag") {
                    SAFE_DELETE( pToken );
                    if (pReg->gate > 0) break;
                    regions.numberofgates++;
                    if (Globals->DISPERSE_GATE_NUMBERS) {
                        size_t ngates, log10;
                        log10 = 0;
                        ngates = regions.numberofgates;
                        while (ngates > 0) {
                            ngates /= 10;
                            log10++;
                        }
                        ngates = 10;
                        while (log10 > 0) {
                            ngates *= 10;
                            log10--;
                        }
                        std::vector<bool> used(ngates, false);
                        for(const auto& reg: regions)
                        {
                            if (reg->gate)
                            {
                                used[static_cast<size_t>(reg->gate - 1)] = true;
                            }
                        }
                        size_t cur_gate = getrandom(ngates);
                        while (used[cur_gate])
                            cur_gate = getrandom(ngates);
                        pReg->gate = static_cast<int>(cur_gate);
                        pReg->gate++;
                    } else {
                        unsigned int gatenum = getrandom(regions.numberofgates) + 1;
                        if (gatenum != regions.numberofgates) {
                            for(const auto& reg: regions) {
                                if (reg->gate == static_cast<int>(gatenum))
                                {
                                    reg->gate = static_cast<int>(regions.numberofgates);
                                }
                            }
                        }
                        pReg->gate = static_cast<int>(gatenum);
                    }
                    pReg->gatemonth = getrandom(12);
                }
                else if (*pToken == "w") {
                    SAFE_DELETE( pToken );
                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }
                    int val = pToken->value();
                    SAFE_DELETE( pToken );
                    if (val) {
                        int change = val - pReg->maxwages;
                        pReg->maxwages = val;
                        pReg->wages += change;
                    }
                    pReg->UpdateEditRegion();
                }
                else if (*pToken == "p") {
                    SAFE_DELETE(pToken);
                    auto it = pReg->products.begin();
                    while(it != pReg->products.end()) {
                        const auto& p = *it;
                        if (p->itemtype!=Items::Types::I_SILVER) {
                            it = pReg->products.erase(it);
                            continue;
                        }
                        ++it;
                    }
                    pReg->SetupProds();
                }
                else if (*pToken == "g") {
                    SAFE_DELETE(pToken);

                    if (pReg->town) delete pReg->town;
                    pReg->town = NULL;

                    pReg->products.clear();
                    pReg->SetupProds();

                    pReg->markets.clear();

                    pReg->SetupEditRegion();
                    pReg->UpdateEditRegion();
                }
                else if (*pToken == "n") {
                    SAFE_DELETE(pToken);
                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    *pReg->name = *pToken;
                    SAFE_DELETE(pToken);
                }
                else if (*pToken == "tn") {
                    SAFE_DELETE(pToken);
                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }

                    if (pReg->town) *pReg->town->name = *pToken;
                    SAFE_DELETE(pToken);
                }
                else if (*pToken == "town") {
                    SAFE_DELETE(pToken);

                    if (!pReg->race.isValid()) pReg->race = 9;

                    AString *townname = new AString("name");
                    if (pReg->town) {
                        *townname = *pReg->town->name;
                        delete pReg->town;
                        pReg->markets.DeleteAll();
                    }
                    pReg->AddTown(townname);

                    pReg->UpdateEditRegion(); // financial stuff! Does markets
                }
                else if (*pToken == "deltown") {
                    SAFE_DELETE(pToken);
                    if (pReg->town) {
                        delete pReg->town;
                        pReg->town = NULL;
                        pReg->markets.DeleteAll();
                        pReg->UpdateEditRegion();
                    }
                }
                else if (*pToken == "v") {
                    if (pReg->town) EditGameRegionMarkets(pReg);
                }
            }
            while( 0 );
            if (pToken) delete pToken;
        }
        if (pStr) delete pStr;

        if ( exit ) {
            break;
        }
    }
    while( 1 );
}

void Game::EditGameRegionMarkets(const ARegion::Handle& pReg)
{
/* This only gets called if pReg->town exists! */
    do {
        Awrite("");
        Awrite( AString( "Region: " ) + pReg->Print( regions ) );
        Awrite( "" );
// write pop stuff
        Awrite( AString("") + pReg->town->pop + " " + ItemDefs[pReg->race].names + " townpop");

//write markets
        Awrite(AString("Market Format: ... price(base). minpop/maxpop. minamt/maxamt."));

        Awrite("Wanted: ");
        for(const auto& m: pReg->markets) {
            if (m->type == M_SELL) {
                AString temp = ItemString(m->item, m->amount) + " at $" + m->price + "(" + m->baseprice + ").";
                temp += AString(" Pop: ") + m->minpop + "/" + m->maxpop + ".";
                temp += AString(" Amount: ") + m->minamt + "/" + m->maxamt + ".";
                Awrite(temp);
            }
        }
        Awrite("For Sale: ");
        for(const auto& m: pReg->markets) {
            if (m->type == M_BUY) {
                AString temp = ItemString(m->item, m->amount) + " at $" + m->price + "(" + m->baseprice + ").";
                temp += AString(" Pop: ") + m->minpop + "/" + m->maxpop + ".";
                temp += AString(" Amount: ") + m->minamt + "/" + m->maxamt + ".";
                Awrite(temp);
            }
        }


        Awrite( "" );

        Awrite( " [g] to regenerate all markets" );
        Awrite( " [p] [item] [minpop] [maxpop] to add/modify market population" );
        Awrite( " [a] [item] [minamt] [maxamt] to add/modify market amounts" );
        Awrite( " [c] [item] [price] [baseprice] to add/modify item prices" );
        Awrite( " [s] [item] to swop an item between wanted and sold" );
        Awrite( " [d] [item] to delete an item from the market" );
        Awrite( " q) Return to previous menu." );

        bool exit = false;
        AString *pStr = AGetString();
        if ( *pStr == "q" ) {
            exit = true;
        } else {
            AString *pToken = 0;
            do {
                pToken = pStr->gettoken();
                if ( !pToken ) {
                    Awrite( "Try again." );
                    break;
                }

                // regenerate markets
                if (*pToken == "g") {
                    SAFE_DELETE(pToken);

                    pReg->markets.DeleteAll();
                    pReg->SetupCityMarket();
                    pReg->UpdateEditRegion();
                }
                else if (*pToken == "p") {
                    SAFE_DELETE(pToken);

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }
                    Items mitem = ParseEnabledItem(pToken);
                    if (!mitem.isValid()) {
                        Awrite("No such item");
                        break;
                    }
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    int minimum = pToken->value();
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    int maximum = pToken->value();
                    SAFE_DELETE( pToken );

                    bool done = false;

                    if (minimum >= maximum) {
                        Awrite("Maximum must be more than minimum");
                        break;
                    }

                    int population = pReg->Population();

                    for(const auto& m: pReg->markets) {
                        if (m->item == mitem) {
                            m->minpop = minimum;
                            m->maxpop = maximum;

                            if (population <= m->minpop)
                                m->amount = m->minamt;
                            else {
                                if (population >= m->maxpop)
                                    m->amount = m->maxamt;
                                else {
                                    m->amount = m->minamt + ((m->maxamt - m->minamt) *
                                            (population - m->minpop)) /
                                        (m->maxpop - m->minpop);
                                }
                            }
                            done = true;
                        }
                    }

                    if (!done) {
                        int price = static_cast<int>((ItemDefs[mitem].baseprice * (100 + getrandom(50UL))) / 100);
    //                    m->PostTurn(pReg->Population(),pReg->Wages()); // updates amounts
                        pReg->markets.Add(M_SELL, mitem, price, 0, minimum, maximum, 0, 0);
                    }

                }
                else if (*pToken == "a") {
                    SAFE_DELETE(pToken);

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }
                    Items mitem = ParseEnabledItem(pToken);
                    if (!mitem.isValid()) {
                        Awrite("No such item");
                        break;
                    }
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    int minimum = pToken->value();
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    int maximum = pToken->value();
                    SAFE_DELETE( pToken );

                    bool done = false;

                    if (minimum >= maximum) {
                        Awrite("Maximum must be more than minimum");
                        break;
                    }

                    int population = pReg->Population();

                    for(const auto& m: pReg->markets) {
                        if (m->item == mitem) {
                            m->minamt = minimum;
                            m->maxamt = maximum;

                            if (population <= m->minpop)
                                m->amount = m->minamt;
                            else {
                                if (population >= m->maxpop)
                                    m->amount = m->maxamt;
                                else {
                                    m->amount = m->minamt + ((m->maxamt - m->minamt) *
                                            (population - m->minpop)) /
                                        (m->maxpop - m->minpop);
                                }
                            }
                            done = true;
                        }
                    }

                    if (!done) {
                        int price = static_cast<int>((ItemDefs[mitem].baseprice * (100 + getrandom(50UL))) / 100);
                        int mamount = minimum + ( maximum * population / 5000 );
                        pReg->markets.Add(M_SELL, mitem, price, mamount, 0, 5000, minimum, maximum);
                    }

                }
                else if (*pToken == "c") {
                    SAFE_DELETE(pToken);

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }
                    Items mitem = ParseEnabledItem(pToken);
                    if (!mitem.isValid()) {
                        Awrite("No such item");
                        break;
                    }
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    int price = pToken->value();
                    SAFE_DELETE( pToken );

                    pToken = pStr->gettoken();
                    int baseprice = pToken->value();
                    SAFE_DELETE( pToken );

                    if (price<1 || baseprice<1) {
                        Awrite("Price must be more than zero");
                        break;
                    }

                    bool done = false;


                    for(const auto& m: pReg->markets) {
                        if (m->item == mitem) {
                            m->price = price;
                            m->baseprice = baseprice;
                            done = true;
                        }
                    }

                    if (!done) {
                        Market::Handle m = std::make_shared<Market>(M_SELL, mitem, price, 0, 0, 5000, 0, 0);
                        m->baseprice = baseprice;
                        pReg->markets.Add(m);
                    }

                }
                else if (*pToken == "s") {
                    SAFE_DELETE(pToken);

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }
                    Items mitem = ParseEnabledItem(pToken);
                    if (!mitem.isValid()) {
                        Awrite("No such item");
                        break;
                    }
                    SAFE_DELETE( pToken );

                    bool done = false;
                    auto it = pReg->markets.begin();
                    while(it != pReg->markets.end()) {
                        const auto& m = *it;
                        if (m->item == mitem) {
                            if (!done) {
                                if (m->type == M_SELL) m->type = M_BUY;
                                else m->type = M_SELL;
                                done = true;
                            } else {
                                it = pReg->markets.erase(it);
                                continue;
                            }
                        }
                        ++it;
                    }
                    if (!done) Awrite("No such market");
                }
                else if (*pToken == "d") {
                    SAFE_DELETE(pToken);

                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }
                    Items mitem = ParseEnabledItem(pToken);
                    if (!mitem.isValid()) {
                        Awrite("No such item");
                        break;
                    }
                    SAFE_DELETE( pToken );

                    bool done = false;
                    auto it = pReg->markets.begin();
                    while(it != pReg->markets.end()) {
                        const auto& m = *it;
                        if (m->item == mitem) {
                            it = pReg->markets.erase(it);
                            done = true;
                            continue;
                        }
                        ++it;
                    }
                    if (!done) Awrite("No such market");
                }
            }
            while( 0 );
            if (pToken) delete pToken;
        }
        if (pStr) delete pStr;

        if ( exit ) {
            break;
        }
    }
    while( 1 );
}


void Game::EditGameUnit(const Unit::Handle& pUnit)
{
    do {
        Awrite(AString("Unit: ") + *(pUnit->name));
        Awrite(AString("Faction: ") + pUnit->faction.lock()->num);
        Awrite(AString("  in ") +
                pUnit->object.lock()->region.lock()->ShortPrint(regions));
        Awrite("  1) Edit items...");
        Awrite("  2) Edit skills...");
        Awrite("  3) Move unit...");
        Awrite("  4) Edit details...");

        Awrite("  q) Stop editing this unit.");

        bool exit = false;
        AString *pStr = AGetString();
        if (*pStr == "1") {
            EditGameUnitItems(pUnit);
        } else if (*pStr == "2") {
            EditGameUnitSkills(pUnit);
        } else if (*pStr == "3") {
            EditGameUnitMove(pUnit);
        } else if (*pStr == "4") {
            EditGameUnitDetails(pUnit);
        } else if (*pStr == "q") {
            exit = true;
        } else {
            Awrite("Select from the menu.");
        }
        delete pStr;

        if (exit) {
            break;
        }
    } while(1);
}

void Game::EditGameUnitItems(const Unit::Handle& pUnit)
{
    do {
        bool exit = false;
        Awrite(AString("Unit items: ") + pUnit->items.Report(2, 1, 1));
        Awrite("  [item] [number] to update an item.");
        Awrite("  q) Stop editing the items.");
        AString *pStr = AGetString();
        if (*pStr == "q") {
            exit = true;
        } else {
            AString *pToken = 0;
            do {
                pToken = pStr->gettoken();
                if (!pToken) {
                    Awrite("Try again.");
                    break;
                }

                Items itemNum = ParseAllItems(pToken);
                if (!itemNum.isValid()) {
                    Awrite("No such item.");
                    break;
                }
                if (ItemDefs[itemNum].flags & ItemType::DISABLED) {
                    Awrite("No such item.");
                    break;
                }
                SAFE_DELETE(pToken);

                int num;
                pToken = pStr->gettoken();
                if (!pToken) {
                    num = 0;
                } else {
                    num = pToken->value();
                }

                pUnit->items.SetNum(itemNum, static_cast<size_t>(num));
                /* Mark it as known about for 'shows' */
                pUnit->faction.lock()->items.SetNum(itemNum, 1);
            } while(0);
            if (pToken) delete pToken;
        }
        if (pStr) delete pStr;

        if (exit) {
            break;
        }
    } while(1);
}

void Game::EditGameUnitSkills(const Unit::Handle& pUnit)
{
    do {
        bool exit = false;
        Awrite(AString("Unit skills: ") +
                pUnit->skills.Report(static_cast<int>(pUnit->GetMen())));
        Awrite("  [skill] [days] to update a skill.");
        Awrite("  q) Stop editing the skills.");
        AString *pStr = AGetString();
        if (*pStr == "q") {
            exit = true;
        } else {
            AString *pToken = 0;
            do {
                pToken = pStr->gettoken();
                if (!pToken) {
                    Awrite("Try again.");
                    break;
                }

                Skills skillNum = ParseSkill(pToken);
                if (!skillNum.isValid()) {
                    Awrite("No such skill.");
                    break;
                }
                if (SkillDefs[skillNum].flags & SkillType::DISABLED) {
                    Awrite("No such skill.");
                    break;
                }
                SAFE_DELETE(pToken);

                size_t days;
                pToken = pStr->gettoken();
                if (!pToken) {
                    days = 0;
                } else {
                    days = static_cast<size_t>(pToken->value());
                }

                if ((SkillDefs[skillNum].flags & SkillType::MAGIC) &&
                        (pUnit->type != U_MAGE)) {
                    pUnit->type = U_MAGE;
                }
                if ((SkillDefs[skillNum].flags & SkillType::APPRENTICE) &&
                        (pUnit->type == U_NORMAL)) {
                    pUnit->type = U_APPRENTICE;
                }
                pUnit->skills.SetDays(skillNum, days * pUnit->GetMen());
                size_t lvl = pUnit->GetRealSkill(skillNum);
                if (lvl > pUnit->faction.lock()->skills.GetDays(skillNum)) {
                    pUnit->faction.lock()->skills.SetDays(skillNum, lvl);
                }
            } while(0);
            delete pToken;
        }
        delete pStr;

        if (exit) {
            break;
        }
    } while(1);
}

void Game::EditGameUnitMove(const Unit::Handle& pUnit)
{
    ARegion::WeakHandle pReg = EditGameFindRegion();
    if (pReg.expired())
    {
        return;
    }

    pUnit->MoveUnit(pReg.lock()->GetDummy());
}

void Game::EditGameUnitDetails(const Unit::Handle& pUnit)
{
    do {
        bool exit = false;
        Awrite(AString("Unit: ") + *(pUnit->name));
        Awrite(AString("Unit faction: ") +
                *(pUnit->faction.lock()->name));
        AString temp = " (";
        switch(pUnit->type) {
            case U_NORMAL:
                temp += "normal";
                break;
            case U_MAGE:
                temp += "mage";
                break;
            case U_GUARD:
                temp += "guard";
                break;
            case U_WMON:
                temp += "monster";
                break;
            case U_GUARDMAGE:
                temp += "guardmage";
                break;
            case U_APPRENTICE:
                temp += Globals->APPRENTICE_NAME;
                break;
        }
        temp += ")";
        Awrite(AString("Unit type: ") + pUnit->type + temp);

        Awrite("");
        Awrite("  [f] [num] to change the unit's faction.");
        Awrite("  [t] [num] to change the unit's type.");
        Awrite("  [q] Go back one screen.");

        AString *pStr = AGetString();
        if (*pStr == "q") {
            exit = true;
        }
        else {
            AString *pToken = 0;
            do {
                pToken = pStr->gettoken();
                if (!pToken) {
                    Awrite("Try again.");
                    break;
                }
                // change faction
                if (*pToken == "f") {
                    SAFE_DELETE( pToken );
                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }
                    size_t fnum = static_cast<size_t>(pToken->value());
                    SAFE_DELETE( pToken );
                    if (fnum<1) {
                        Awrite("Invalid Faction Number");
                        break;
                    }

                    Faction::Handle fac = GetFaction(factions, fnum);
                    if (fac) pUnit->faction = fac;
                    else Awrite("Cannot Find Faction");
                }
                else if (*pToken == "t") {
                    SAFE_DELETE( pToken );
                    pToken = pStr->gettoken();
                    if ( !pToken ) {
                        Awrite( "Try again." );
                        break;
                    }
                    int newtype = pToken->value();
                    SAFE_DELETE( pToken );
                    if (newtype<0 || newtype>NUNITTYPES-1) {
                        Awrite("Invalid Type");
                        break;
                    }
                    pUnit->type = newtype;
                }

            } while(0);
            delete pToken;
        }
        delete pStr;

        if (exit) {
            break;
        }
    } while(1);
}

void Game::EditGameCreateUnit()
{
    Faction::Handle fac = GetFaction(factions, 1);
    Unit::Handle newunit = GetNewUnit(fac);
    newunit->SetMen(Items::Types::I_LEADERS, 1);
    newunit->reveal = REVEAL_FACTION;
    newunit->MoveUnit(regions.front()->GetDummy());

    EditGameUnit(newunit);
}
