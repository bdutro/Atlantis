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

#ifdef WIN32
#include <memory.h>    // Needed for memcpy on windows
#include "io.h"        // Needed for access() on windows
#define F_OK    0
#endif

#include <string.h>
#include <unistd.h>

#include "game.h"
#include "unit.h"
#include "fileio.h"
#include "astring.h"
#include "gamedata.h"
#include "quests.h"

Game::Game()
{
    gameStatus = GAME_STATUS_UNINIT;
    maxppunits = 0;
}

Game::~Game()
{
    maxppunits = 0;
}

int Game::TurnNumber()
{
    return (year-1)*12 + month + 1;
}

// ALT, 25-Jul-2000
// Default work order procedure
void Game::DefaultWorkOrder()
{
    for(const auto& r: regions) {
        if (r->type == Regions::Types::R_NEXUS) continue;
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->monthorders || u->faction.lock()->IsNPC() ||
                        (Globals->TAX_PILLAGE_MONTH_LONG &&
                         u->taxing != TAX_NONE))
                    continue;
                if (u->GetFlag(FLAG_AUTOTAX) &&
                        (Globals->TAX_PILLAGE_MONTH_LONG && u->Taxers(1))) {
                    u->taxing = TAX_AUTO;
                } else if (Globals->DEFAULT_WORK_ORDER) {
                    ProcessWorkOrder(u, 1, 0);
                }
            }
        }
    }
}

AString Game::GetXtraMap(const ARegion::Handle& reg,int type)
{
    int i;

    if (!reg)
        return(" ");

    switch (type) {
        case 0:
            return (reg->IsStartingCity() ? "!" :
                    (reg->HasShaft() ? "*" : " "));
        case 1:
            i = reg->CountWMons();
            return (i ? AString(i) : AString(" "));
        case 2:
            for(const auto& o: reg->objects) {
                if (!(ObjectDefs[o->type].flags & ObjectType::CANENTER)) {
                    if (!o->units.empty()) {
                        return "*";
                    } else {
                        return ".";
                    }
                }
            }
            return " ";
        case 3:
            if (reg->gate) return "*";
            return " ";
    }
    return(" ");
}

void Game::WriteSurfaceMap(Aoutfile *f, const ARegionArray::Handle& pArr, int type)
{
    unsigned int yy = 0;
    unsigned int xx = 0;

    f->PutStr(AString("Map (") + xx*32 + "," + yy*16 + ")");
    for (unsigned int y=0; y < pArr->y; y+=2) {
        AString temp;
        for (unsigned int x=0; x< pArr->x; x+=2) {
            const auto reg = pArr->GetRegion(x+xx*32,y+yy*16).lock();
            temp += AString(GetRChar(reg));
            temp += GetXtraMap(reg, type);
            temp += "  ";
        }
        f->PutStr(temp);
        temp = "  ";
        for (unsigned int x=1; x< pArr->x; x+=2) {
            const auto reg = pArr->GetRegion(x+xx*32,y+yy*16+1).lock();
            temp += AString(GetRChar(reg));
            temp += GetXtraMap(reg,type);
            temp += "  ";
        }
        f->PutStr(temp);
    }
    f->PutStr("");
}

void Game::WriteUnderworldMap(Aoutfile *f, const ARegionArray::Handle& pArr, int type)
{
    unsigned int xx = 0;
    unsigned int yy = 0;
    f->PutStr(AString("Map (") + xx*32 + "," + yy*16 + ")");
    for (unsigned int y=0; y< pArr->y; y+=2) {
        AString temp = " ";
        AString temp2;
        for (unsigned int x=0; x< pArr->x; x+=2) {
            const auto reg = pArr->GetRegion(x+xx*32,y+yy*16).lock();
            const auto reg2 = pArr->GetRegion(x+xx*32+1,y+yy*16+1).lock();
            temp += AString(GetRChar(reg));
            temp += GetXtraMap(reg,type);
            if (reg2 && !reg2->neighbors[static_cast<size_t>(Directions::D_NORTH)].expired()) temp += "|";
            else temp += " ";

            temp += " ";
            if (reg && !reg->neighbors[static_cast<size_t>(Directions::D_SOUTHWEST)].expired()) temp2 += "/";
            else temp2 += " ";

            temp2 += " ";
            if (reg && !reg->neighbors[static_cast<size_t>(Directions::D_SOUTHEAST)].expired()) temp2 += "\\";
            else temp2 += " ";

            temp2 += " ";
        }
        f->PutStr(temp);
        f->PutStr(temp2);

        temp = " ";
        temp2 = "  ";
        for (unsigned int x=1; x< pArr->x; x+=2) {
            const auto reg = pArr->GetRegion(x+xx*32,y+yy*16+1).lock();
            const auto reg2 = pArr->GetRegion(x+xx*32-1,y+yy*16).lock();

            if (reg2 && !reg2->neighbors[static_cast<size_t>(Directions::D_SOUTH)].expired()) temp += "|";
            else temp += " ";

            temp += AString(" ");
            temp += AString(GetRChar(reg));
            temp += GetXtraMap(reg,type);

            if (reg && !reg->neighbors[static_cast<size_t>(Directions::D_SOUTHWEST)].expired()) temp2 += "/";
            else temp2 += " ";

            temp2 += " ";
            if (reg && !reg->neighbors[static_cast<size_t>(Directions::D_SOUTHEAST)].expired()) temp2 += "\\";
            else temp2 += " ";

            temp2 += " ";
        }
        f->PutStr(temp);
        f->PutStr(temp2);
    }
    f->PutStr("");
}

int Game::ViewMap(const AString & typestr,const AString & mapfile)
{
    int type = 0;
    if (AString(typestr) == "wmon") type = 1;
    if (AString(typestr) == "lair") type = 2;
    if (AString(typestr) == "gate") type = 3;

    Aoutfile f;
    if (f.OpenByName(mapfile) == -1) return(0);

    switch (type) {
        case 0:
            f.PutStr("Geographical Map");
            break;
        case 1:
            f.PutStr("Wandering Monster Map");
            break;
        case 2:
            f.PutStr("Lair Map");
            break;
        case 3:
            f.PutStr("Gate Map");
            break;
    }

    for (unsigned int i = 0; i < regions.numLevels; i++) {
        f.PutStr("");
        const ARegionArray::Handle& pArr = regions.pRegionArrays[i];
        switch(pArr->levelType) {
            case ARegionArray::LEVEL_NEXUS:
                f.PutStr(AString("Level ") + i + ": Nexus");
                break;
            case ARegionArray::LEVEL_SURFACE:
                f.PutStr(AString("Level ") + i + ": Surface");
                WriteSurfaceMap(&f, pArr, type);
                break;
            case ARegionArray::LEVEL_UNDERWORLD:
                f.PutStr(AString("Level ") + i + ": Underworld");
                WriteUnderworldMap(&f, pArr, type);
                break;
            case ARegionArray::LEVEL_UNDERDEEP:
                f.PutStr(AString("Level ") + i + ": Underdeep");
                WriteUnderworldMap(&f, pArr, type);
                break;
        }
    }

    f.Close();

    return(1);
}

int Game::NewGame()
{
    factionseq = 1;
    guardfaction = 0;
    monfaction = 0;
    unitseq = 1;
    SetupUnitNums();
    shipseq = 100;
    year = 1;
    month = -1;
    gameStatus = GAME_STATUS_NEW;

    //
    // Seed the random number generator with a different value each time.
    //
    seedrandomrandom();

    CreateWorld();
    CreateNPCFactions();

    if (Globals->CITY_MONSTERS_EXIST)
        CreateCityMons();
    if (Globals->WANDERING_MONSTERS_EXIST)
        CreateWMons();
    if (Globals->LAIR_MONSTERS_EXIST)
        CreateLMons();

    if (Globals->LAIR_MONSTERS_EXIST)
        CreateVMons();
    
    /*    
    if (Globals->PLAYER_ECONOMY) {
        Equilibrate();
    }
    */
    
    

    return(1);
}

int Game::OpenGame()
{
    //
    // The order here must match the order in SaveGame
    //
    Ainfile f;
    if (f.OpenByName("game.in") == -1) return(0);

    //
    // Read in Globals
    //
    AString *s1 = f.GetStr();
    if (!s1) return(0);

    AString *s2 = s1->gettoken();
    delete s1;
    if (!s2) return(0);

    if (!(*s2 == "atlantis_game")) {
        delete s2;
        f.Close();
        return(0);
    }
    delete s2;

    ATL_VER eVersion = f.GetInt<ATL_VER>();
    Awrite(AString("Saved Game Engine Version: ") + ATL_VER_STRING(eVersion));
    if (ATL_VER_MAJOR(eVersion) != ATL_VER_MAJOR(CURRENT_ATL_VER) ||
            ATL_VER_MINOR(eVersion) != ATL_VER_MINOR(CURRENT_ATL_VER)) {
        Awrite("Incompatible Engine versions!");
        return(0);
    }
    if (ATL_VER_PATCH(eVersion) > ATL_VER_PATCH(CURRENT_ATL_VER)) {
        Awrite("This game was created with a more recent Atlantis Engine!");
        return(0);
    }

    AString *gameName = f.GetStr();
    if (!gameName) return(0);

    if (!(*gameName == Globals->RULESET_NAME)) {
        Awrite("Incompatible rule-set!");
        return(0);
    }

    ATL_VER gVersion = f.GetInt<ATL_VER>();
    Awrite(AString("Saved Rule-Set Version: ") + ATL_VER_STRING(gVersion));

    if (ATL_VER_MAJOR(gVersion) < ATL_VER_MAJOR(Globals->RULESET_VERSION)) {
        Awrite(AString("Upgrading to ") +
                ATL_VER_STRING(MAKE_ATL_VER(
                        ATL_VER_MAJOR(Globals->RULESET_VERSION), 0, 0)));
        if (!UpgradeMajorVersion(gVersion)) {
            Awrite("Unable to upgrade!  Aborting!");
            return(0);
        }
        gVersion = MAKE_ATL_VER(ATL_VER_MAJOR(Globals->RULESET_VERSION), 0, 0);
    }
    if (ATL_VER_MINOR(gVersion) < ATL_VER_MINOR(Globals->RULESET_VERSION)) {
        Awrite(AString("Upgrading to ") +
                ATL_VER_STRING(MAKE_ATL_VER(
                        ATL_VER_MAJOR(Globals->RULESET_VERSION),
                        ATL_VER_MINOR(Globals->RULESET_VERSION), 0)));
        if (! UpgradeMinorVersion(gVersion)) {
            Awrite("Unable to upgrade!  Aborting!");
            return(0);
        }
        gVersion = MAKE_ATL_VER(ATL_VER_MAJOR(gVersion),
                ATL_VER_MINOR(Globals->RULESET_VERSION), 0);
    }
    if (ATL_VER_PATCH(gVersion) < ATL_VER_PATCH(Globals->RULESET_VERSION)) {
        Awrite(AString("Upgrading to ") +
                ATL_VER_STRING(Globals->RULESET_VERSION));
        if (! UpgradePatchLevel(gVersion)) {
            Awrite("Unable to upgrade!  Aborting!");
            return(0);
        }
        gVersion = MAKE_ATL_VER(ATL_VER_MAJOR(gVersion),
                ATL_VER_MINOR(gVersion),
                ATL_VER_PATCH(Globals->RULESET_VERSION));
    }

    year = f.GetInt<int>();
    month = f.GetInt<int>();
    seedrandom(f.GetInt<int>());
    factionseq = f.GetInt<size_t>();
    unitseq = f.GetInt<unsigned int>();
    shipseq = f.GetInt<int>();
    guardfaction = f.GetInt<size_t>();
    monfaction = f.GetInt<size_t>();

    //
    // Read in the Factions
    //
    int i = f.GetInt<int>();

    for (int j=0; j<i; j++) {
        Faction::Handle& temp = factions.emplace_back(std::make_shared<Faction>());
        temp->Readin(&f, eVersion);
    }

    //
    // Read in the ARegions
    //
    i = regions.ReadRegions(&f, factions, eVersion);
    if (!i) return 0;

    // read in quests
    if (!quests.ReadQuests(&f))
        return 0;

    SetupUnitNums();

    f.Close();
    return(1);
}

int Game::SaveGame()
{
    Aoutfile f;
    if (f.OpenByName("game.out") == -1) return(0);

    //
    // Write out Globals
    //
    f.PutStr("atlantis_game");
    f.PutInt(CURRENT_ATL_VER);
    f.PutStr(Globals->RULESET_NAME);
    f.PutInt(Globals->RULESET_VERSION);

    f.PutInt(year);
    f.PutInt(month);
    f.PutInt(getrandom(10000));
    f.PutInt(factionseq);
    f.PutInt(unitseq);
    f.PutInt(shipseq);
    f.PutInt(guardfaction);
    f.PutInt(monfaction);

    //
    // Write out the Factions
    //
    f.PutInt(factions.size());

    for(const auto& fac: factions) {
        fac->Writeout(&f);
    }

    //
    // Write out the ARegions
    //
    regions.WriteRegions(&f);

    // Write out quests
    quests.WriteQuests(&f);

    f.Close();
    return(1);
}

void Game::DummyGame()
{
    //
    // No need to set anything up; we're just syntax checking some orders.
    //
}

#define PLAYERS_FIRST_LINE "AtlantisPlayerStatus"

int Game::WritePlayers()
{
    Aoutfile f;
    if (f.OpenByName("players.out") == -1) return(0);

    f.PutStr(PLAYERS_FIRST_LINE);
    f.PutStr(AString("Version: ") + CURRENT_ATL_VER);
    f.PutStr(AString("TurnNumber: ") + TurnNumber());

    if (gameStatus == GAME_STATUS_UNINIT)
        return(0);
    else if (gameStatus == GAME_STATUS_NEW)
        f.PutStr(AString("GameStatus: New"));
    else if (gameStatus == GAME_STATUS_RUNNING)
        f.PutStr(AString("GameStatus: Running"));
    else if (gameStatus == GAME_STATUS_FINISHED)
        f.PutStr(AString("GameStatus: Finished"));

    f.PutStr("");

    for(const auto& fac: factions) {
        fac->WriteFacInfo(&f);
    }

    f.Close();
    return(1);
}

int Game::ReadPlayers()
{
    Aorders f;
    if (f.OpenByName("players.in") == -1) return(0);

    AString *pLine = 0;
    AString *pToken = 0;

    //
    // Default: failure.
    //
    int rc = 0;

    do {
        //
        // The first line of the file should match.
        //
        pLine = f.GetLine();
        if (!(*pLine == PLAYERS_FIRST_LINE)) break;
        SAFE_DELETE(pLine);

        //
        // Get the file version number.
        //
        pLine = f.GetLine();
        pToken = pLine->gettoken();
        if (!pToken || !(*pToken == "Version:")) break;
        SAFE_DELETE(pToken);

        pToken = pLine->gettoken();
        if (!pToken) break;

        int nVer = pToken->value();
        if (ATL_VER_MAJOR(nVer) != ATL_VER_MAJOR(CURRENT_ATL_VER) ||
                ATL_VER_MINOR(nVer) != ATL_VER_MINOR(CURRENT_ATL_VER) ||
                ATL_VER_PATCH(nVer) > ATL_VER_PATCH(CURRENT_ATL_VER)) {
            Awrite("The players.in file is not compatible with this "
                    "version of Atlantis.");
            break;
        }
        SAFE_DELETE(pToken);
        SAFE_DELETE(pLine);

        //
        // Ignore the turn number line.
        //
        pLine = f.GetLine();
        SAFE_DELETE(pLine);

        //
        // Next, the game status.
        //
        pLine = f.GetLine();
        pToken = pLine->gettoken();
        if (!pToken || !(*pToken == "GameStatus:")) break;
        SAFE_DELETE(pToken);

        pToken = pLine->gettoken();
        if (!pToken) break;

        if (*pToken == "New")
            gameStatus = GAME_STATUS_NEW;
        else if (*pToken == "Running")
            gameStatus = GAME_STATUS_RUNNING;
        else if (*pToken == "Finished")
            gameStatus = GAME_STATUS_FINISHED;
        else {
            //
            // The status doesn't seem to be valid.
            //
            break;
        }
        SAFE_DELETE(pToken);

        //
        // Now, we should have a list of factions.
        //
        pLine = f.GetLine();
        Faction::Handle pFac = nullptr;

        int lastWasNew = 0;

        //
        // OK, set our return code to success; we'll set it to fail below
        // if necessary.
        //
        rc = 1;

        while(pLine) {
            pToken = pLine->gettoken();
            if (!pToken) {
                SAFE_DELETE(pLine);
                pLine = f.GetLine();
                continue;
            }

            if (*pToken == "Faction:") {
                //
                // Get the new faction
                //
                SAFE_DELETE(pToken);
                pToken = pLine->gettoken();
                if (!pToken) {
                    rc = 0;
                    break;
                }

                if (*pToken == "new") {
                    AString save = *pLine;
                    int noleader = 0;
                    int x, y, z;
                    ARegion::WeakHandle pReg;

                    /* Check for the noleader flag */
                    SAFE_DELETE(pToken);
                    pToken = pLine->gettoken();
                    if (pToken && *pToken == "noleader") {
                        noleader = 1;
                        SAFE_DELETE(pToken);
                        pToken = pLine->gettoken();
                        /* Initialize pReg to something useful */
                        pReg = regions.GetRegion(0, 0, 0);
                    }
                    if (pToken) {
                        x = pToken->value();
                        y = -1;
                        z = -1;
                        SAFE_DELETE(pToken);
                        pToken = pLine->gettoken();
                        if (pToken) {
                            y = pToken->value();
                            SAFE_DELETE(pToken);
                            pToken = pLine->gettoken();
                            if (pToken) {
                                z = pToken->value();
                                pReg = regions.GetRegion(static_cast<unsigned int>(x),
                                                         static_cast<unsigned int>(y),
                                                         static_cast<unsigned int>(z));
                            }
                        }
                        if (pReg.expired())
                            Awrite(AString("Bad faction line: ")+save);
                    }

                    pFac = AddFaction(noleader, pReg.lock());
                    if (!pFac) {
                        Awrite("Failed to add a new faction!");
                        rc = 0;
                        break;
                    }

                    lastWasNew = 1;
                } else {
                    if (pFac && lastWasNew) {
                        WriteNewFac(pFac);
                    }
                    size_t nFacNum = static_cast<size_t>(pToken->value());
                    pFac = GetFaction(factions, nFacNum);
                    if (pFac)
                        pFac->startturn = TurnNumber();
                    lastWasNew = 0;
                }
            } else if (pFac) {
                if (!ReadPlayersLine(pToken, pLine, pFac, lastWasNew)) {
                    rc = 0;
                    break;
                }
            }

            SAFE_DELETE(pToken);
            SAFE_DELETE(pLine);
            pLine = f.GetLine();
        }
        if (pFac && lastWasNew) {
            WriteNewFac(pFac);
        }
    } while(0);

    SAFE_DELETE(pLine);
    SAFE_DELETE(pToken);
    f.Close();

    return(rc);
}

Unit::WeakHandle Game::ParseGMUnit(AString *tag, const Faction::Handle& pFac)
{
    char *str = tag->Str();
    if (*str == 'g' && *(str+1) == 'm') {
        AString p = AString(str+2);
        int gma = p.value();
        for(const auto& reg: regions) {
            for(const auto& obj: reg->objects) {
                for(const auto& u: obj->units) {
                    if (u->faction.lock()->num == pFac->num && u->gm_alias == gma) {
                        return u;
                    }
                }
            }
        }
    } else {
        int v = tag->value();
        if (static_cast<unsigned int>(v) >= maxppunits) return Unit::WeakHandle();
        return GetUnit(v);
    }
    return Unit::WeakHandle();
}

int Game::ReadPlayersLine(AString *pToken, AString *pLine, const Faction::Handle& pFac,
        int newPlayer)
{
    AString *pTemp = 0;

    if (*pToken == "Name:") {
        pTemp = pLine->StripWhite();
        if (pTemp) {
            if (newPlayer) {
                *pTemp += AString(" (") + (pFac->num) + ")";
            }
            pFac->SetNameNoChange(pTemp);
        }
    } else if (*pToken == "RewardTimes") {
        pFac->TimesReward();
    } else if (*pToken == "Email:") {
        pTemp = pLine->gettoken();
        if (pTemp) {
            delete pFac->address;
            pFac->address = pTemp;
            pTemp = 0;
        }
    } else if (*pToken == "Password:") {
        pTemp = pLine->StripWhite();
        delete pFac->password;
        if (pTemp) {
            pFac->password = pTemp;
            pTemp = 0;
        } else {
            AString * pDefault = new AString("none");
            pFac->password = pDefault;
        }
        
    } else if (*pToken == "Template:") {
        // LLS - looked like a good place to stick the Template test
        pTemp = pLine->gettoken();
        int nTemp = ParseTemplate(pTemp);
        pFac->temformat = TEMPLATE_LONG;
        if (nTemp != -1) pFac->temformat = nTemp;
    } else if (*pToken == "Reward:") {
        pTemp = pLine->gettoken();
        int nAmt = pTemp->value();
        pFac->Event(AString("Reward of ") + nAmt + " silver.");
        pFac->unclaimed += nAmt;
    } else if (*pToken == "SendTimes:") {
        // get the token, but otherwise ignore it
        pTemp = pLine->gettoken();
        pFac->times = pTemp->value();
    } else if (*pToken == "LastOrders:") {
        // Read this line and correctly set the lastorders for this
        // faction if the game itself isn't maintaining them.
        pTemp = pLine->gettoken();
        if (Globals->LASTORDERS_MAINTAINED_BY_SCRIPTS)
            pFac->lastorders = pTemp->value();
    } else if (*pToken == "FirstTurn:") {
        pTemp = pLine->gettoken();
        pFac->startturn = pTemp->value();
    } else if (*pToken == "Loc:") {
        int x, y, z;
        pTemp = pLine->gettoken();
        if (pTemp) {
            x = pTemp->value();
            delete pTemp;
            pTemp = pLine->gettoken();
            if (pTemp) {
                y = pTemp->value();
                delete pTemp;
                pTemp = pLine->gettoken();
                if (pTemp) {
                    z = pTemp->value();
                    ARegion::WeakHandle pReg = regions.GetRegion(static_cast<unsigned int>(x),
                                                                 static_cast<unsigned int>(y),
                                                                 static_cast<unsigned int>(z));
                    if (!pReg.expired()) {
                        pFac->pReg = pReg;
                    } else {
                        Awrite(AString("Invalid Loc:")+x+","+y+","+z+
                                " in faction " + pFac->num);
                        pFac->pReg.reset();
                    }
                }
            }
        }
    } else if (*pToken == "NewUnit:") {
        // Creates a new unit in the location specified by a Loc: line
        // with a gm_alias of whatever is after the NewUnit: tag.
        if (pFac->pReg.expired()) {
            Awrite(AString("NewUnit is not valid without a Loc: ") +
                    "for faction "+ pFac->num);
        } else {
            pTemp = pLine->gettoken();
            if (!pTemp) {
                Awrite(AString("NewUnit: must be followed by an alias ") +
                        "in faction "+pFac->num);
            } else {
                int val = pTemp->value();
                if (!val) {
                    Awrite(AString("NewUnit: must be followed by an alias ") +
                            "in faction "+pFac->num);
                } else {
                    Unit::Handle u = GetNewUnit(pFac);
                    u->gm_alias = val;
                    u->MoveUnit(pFac->pReg.lock()->GetDummy());
                    u->Event("Is given to your faction.");
                }
            }
        }
    } else if (*pToken == "Item:") {
        pTemp = pLine->gettoken();
        if (!pTemp) {
            Awrite(AString("Item: needs to specify a unit in faction ") +
                    pFac->num);
        } else {
            Unit::Handle u = ParseGMUnit(pTemp, pFac).lock();
            if (!u) {
                Awrite(AString("Item: needs to specify a unit in faction ") +
                        pFac->num);
            } else {
                if (u->faction.lock()->num != pFac->num) {
                    Awrite(AString("Item: unit ")+ u->num +
                            " doesn't belong to " + "faction " + pFac->num);
                } else {
                    delete pTemp;
                    pTemp = pLine->gettoken();
                    if (!pTemp) {
                        Awrite(AString("Must specify a number of items to ") +
                                "give for Item: in faction " + pFac->num);
                    } else {
                        int v = pTemp->value();
                        if (!v) {
                            Awrite(AString("Must specify a number of ") +
                                        "items to give for Item: in " +
                                        "faction " + pFac->num);
                        } else {
                            delete pTemp;
                            pTemp = pLine->gettoken();
                            if (!pTemp) {
                                Awrite(AString("Must specify a valid item ") +
                                        "to give for Item: in faction " +
                                        pFac->num);
                            } else {
                                Items it = ParseAllItems(pTemp);
                                if (!it.isValid()) {
                                    Awrite(AString("Must specify a valid ") +
                                            "item to give for Item: in " +
                                            "faction " + pFac->num);
                                } else {
                                    size_t has = u->items.GetNum(it);
                                    u->items.SetNum(it, has + static_cast<size_t>(v));
                                    if (!u->gm_alias) {
                                        u->Event(AString("Is given ") +
                                                ItemString(it, v) +
                                                " by the gods.");
                                    }
                                    u->faction.lock()->DiscoverItem(it, 0, 1);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (*pToken == "Skill:") {
        pTemp = pLine->gettoken();
        if (!pTemp) {
            Awrite(AString("Skill: needs to specify a unit in faction ") +
                    pFac->num);
        } else {
            Unit::Handle u = ParseGMUnit(pTemp, pFac).lock();
            if (!u) {
                Awrite(AString("Skill: needs to specify a unit in faction ") +
                        pFac->num);
            } else {
                if (u->faction.lock()->num != pFac->num) {
                    Awrite(AString("Skill: unit ")+ u->num +
                            " doesn't belong to " + "faction " + pFac->num);
                } else {
                    delete pTemp;
                    pTemp = pLine->gettoken();
                    if (!pTemp) {
                        Awrite(AString("Must specify a valid skill for ") +
                                "Skill: in faction " + pFac->num);
                    } else {
                        Skills sk = ParseSkill(pTemp);
                        if (!sk.isValid()) {
                            Awrite(AString("Must specify a valid skill for ")+
                                    "Skill: in faction " + pFac->num);
                        } else {
                            delete pTemp;
                            pTemp = pLine->gettoken();
                            if (!pTemp) {
                                Awrite(AString("Must specify a days for ") +
                                        "Skill: in faction " + pFac->num);
                            } else {
                                size_t days = static_cast<size_t>(pTemp->value()) * u->GetMen();
                                if (!days) {
                                    Awrite(AString("Must specify a days for ")+
                                            "Skill: in faction " + pFac->num);
                                } else {
                                    size_t odays = u->skills.GetDays(sk);
                                    u->skills.SetDays(sk, odays + days);
                                    u->AdjustSkills();
                                    size_t lvl = u->GetRealSkill(sk);
                                    if (lvl > pFac->skills.GetDays(sk)) {
                                        pFac->skills.SetDays(sk, lvl);
                                        pFac->shows.emplace_back(std::make_shared<ShowSkill>(sk,lvl));
                                    }
                                    if (!u->gm_alias) {
                                        u->Event(AString("Is taught ") +
                                                days + " days of " +
                                                SkillStrs(sk) +
                                                " by the gods.");
                                    }
                                    /*
                                     * This is NOT quite the same, but the gods
                                     * are more powerful than mere mortals
                                     */
                                    int mage = (SkillDefs[sk].flags &
                                            SkillType::MAGIC);
                                    int app = (SkillDefs[sk].flags &
                                            SkillType::APPRENTICE);
                                    if (mage) {
                                        u->type = U_MAGE;
                                    }
                                    if (app && u->type == U_NORMAL) {
                                        u->type = U_APPRENTICE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (*pToken == "Order:") {
        pTemp = pLine->StripWhite();
        if (*pTemp == "quit") {
            pFac->quit = QUIT_BY_GM;
        } else {
            // handle this as a unit order
            delete pTemp;
            pTemp = pLine->gettoken();
            if (!pTemp) {
                Awrite(AString("Order: needs to specify a unit in faction ") +
                        pFac->num);
            } else {
                Unit::Handle u = ParseGMUnit(pTemp, pFac).lock();
                if (!u) {
                    Awrite(AString("Order: needs to specify a unit in ")+
                            "faction " + pFac->num);
                } else {
                    if (u->faction.lock()->num != pFac->num) {
                        Awrite(AString("Order: unit ")+ u->num +
                                " doesn't belong to " + "faction " +
                                pFac->num);
                    } else {
                        delete pTemp;
                        AString saveorder = *pLine;
                        int getatsign = pLine->getat();
                        pTemp = pLine->gettoken();
                        if (!pTemp) {
                            Awrite(AString("Order: must provide unit order ")+
                                    "for faction "+pFac->num);
                        } else {
                            Orders o = Parse1Order(pTemp);
                            if (!o.isValid() || o == Orders::Types::O_ATLANTIS || o == Orders::Types::O_END ||
                                    o == Orders::Types::O_UNIT || o == Orders::Types::O_FORM ||
                                    o == Orders::Types::O_ENDFORM) {
                                Awrite(AString("Order: invalid order given ")+
                                        "for faction "+pFac->num);
                            } else {
                                if (getatsign) {
                                    u->oldorders.emplace_back(std::make_shared<AString>(saveorder));
                                }
                                ProcessOrder(o, u, pLine, NULL);
                            }
                        }
                    }
                }
            }
        }
    } else {
        pTemp = new AString(*pToken + *pLine);
        pFac->extraPlayers.Add(pTemp);
        pTemp = 0;
    }

    if (pTemp) delete pTemp;
    return(1);
}

void Game::WriteNewFac(const Faction::Handle& pFac)
{
    newfactions.emplace_back(AString("Adding ") + *(pFac->address) + ".");
}

int Game::DoOrdersCheck(const AString &strOrders, const AString &strCheck)
{
    Aorders ordersFile;
    if (ordersFile.OpenByName(strOrders) == -1) {
        Awrite("No such orders file!");
        return(0);
    }

    Aoutfile checkFile;
    if (checkFile.OpenByName(strCheck) == -1) {
        Awrite("Couldn't open the orders check file!");
        return(0);
    }

    OrdersCheck check;
    check.pCheckFile = &checkFile;

    ParseOrders(0, &ordersFile, &check);

    ordersFile.Close();
    checkFile.Close();

    return(1);
}

int Game::RunGame()
{
    Awrite("Setting Up Turn...");
    PreProcessTurn();

    Awrite("Reading the Gamemaster File...");
    if (!ReadPlayers()) return(0);

    if (gameStatus == GAME_STATUS_FINISHED) {
        Awrite("This game is finished!");
        return(0);
    }
    gameStatus = GAME_STATUS_RUNNING;

    Awrite("Reading the Orders File...");
    ReadOrders();

    if (Globals->MAX_INACTIVE_TURNS != -1) {
        Awrite("QUITting Inactive Factions...");
        RemoveInactiveFactions();
    }

    Awrite("Running the Turn...");
    RunOrders();

    Awrite("Writing the Report File...");
    WriteReport();
    Awrite("");
    // LLS - write order templates
    Awrite("Writing order templates...");
    WriteTemplates();
    Awrite("");
    battles.clear();

    EmptyHell();

    Awrite("Writing Playerinfo File...");
    WritePlayers();

    Awrite("Removing Dead Factions...");
    DeleteDeadFactions();

    Awrite("done");

    return(1);
}

void Game::PreProcessTurn()
{
    month++;
    if (month>11) {
        month = 0;
        year++;
    }
    SetupUnitNums();
    for(const auto& fac: factions) {
        fac->DefaultOrders();
    }
    for(const auto& pReg: regions) {
        if (Globals->WEATHER_EXISTS)
            pReg->SetWeather(regions.GetWeather(*pReg, month));
        if (Globals->GATES_NOT_PERENNIAL)
            pReg->SetGateStatus(month);
        pReg->DefaultOrders();
    }
}

void Game::ClearOrders(const Faction::Handle& f)
{
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->faction.lock() == f) {
                    u->ClearOrders();
                }
            }
        }
    }
}

void Game::ReadOrders()
{
    for(const auto& fac: factions) {
        if (!fac->IsNPC()) {
            AString str = "orders.";
            str += fac->num;

            Aorders file;
            if (file.OpenByName(str) != -1) {
                ParseOrders(fac->num, &file, 0);
                file.Close();
            }
            DefaultWorkOrder();
        }
    }
}

void Game::MakeFactionReportLists()
{
    FactionVector vector(factionseq);

    for(const auto& reg: regions) {
        vector.ClearVector();

        for(const auto& fs: reg->farsees) {
            const auto fac = fs->faction.lock();
            vector.SetFaction(fac->num, fac);
        }
        for(const auto& fs: reg->passers) {
            const auto fac = fs->faction.lock();
            vector.SetFaction(fac->num, fac);
        }
        for(const auto& obj: reg->objects) {
            for(const auto& unit: obj->units) {
                vector.SetFaction(unit->faction.lock()->num, unit->faction);
            }
        }

        for (const auto& fac: vector) {
            if (!fac.expired()) {
                fac.lock()->present_regions.push_back(reg);
            }
        }
    }
}

void Game::WriteReport()
{
    Areport f;

    MakeFactionReportLists();
    CountAllSpecialists();
    for(const auto& fac: factions) {
        AString str = "report.";
        str = str + fac->num;

        if (!fac->IsNPC() ||
                ((((month == 0) && (year == 1)) || Globals->GM_REPORT) &&
            (fac->num == 1))) {
            int i = f.OpenByName(str);
            if (i != -1) {
                fac->WriteReport(&f, this);
                f.Close();
            }
        }
        Adot();
    }
}

// LLS - write order templates for factions
void Game::WriteTemplates()
{
    Areport f;

    for(const auto& fac: factions) {
        AString str = "template.";
        str = str + fac->num;

        if (!fac->IsNPC()) {
            int i = f.OpenByName(str);
            if (i != -1) {
                fac->WriteTemplate(&f, this);
                f.Close();
            }
            fac->present_regions.clear();
        }
        Adot();
    }
}


void Game::DeleteDeadFactions()
{
    auto it = factions.begin();
    while(it != factions.end()) {
        const auto& fac = *it;
        if (!fac->IsNPC() && !fac->exists)
        {
            for(const auto& fac2: factions)
            {
                fac2->RemoveAttitude(fac->num);
            }
            it = factions.erase(it);
            continue;
        }
        ++it;
    }
}

Faction::Handle Game::AddFaction(int noleader, const ARegion::Handle& pStart)
{
    //
    // set up faction
    //
    Faction::Handle temp = std::make_shared<Faction>(factionseq);
    AString x("NoAddress");
    temp->SetAddress(x);
    temp->lastorders = TurnNumber();
    temp->startturn = TurnNumber();
    temp->pStartLoc = pStart;
    temp->pReg = pStart;
    temp->noStartLeader = noleader;

    if (!SetupFaction(temp)) {
        return nullptr;
    }
    factions.push_back(temp);
    factionseq++;
    return temp;
}

void Game::ViewFactions()
{
    for(const auto& fac: factions)
    {
        fac->View();
    }
}

void Game::SetupUnitSeq()
{
    size_t max = 0;
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u && u->num > max) max = u->num;
            }
        }
    }
    unitseq = max + 1;
}

void Game::SetupUnitNums()
{
    ppUnits.clear();

    SetupUnitSeq();

    maxppunits = unitseq+10000;

    ppUnits.resize(maxppunits);

    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                size_t i = u->num;
                if ((i > 0) && (i < maxppunits)) {
                    if (!ppUnits[i])
                        ppUnits[u->num] = u;
                    else {
                        Awrite(AString("Error: Unit number ") + i +
                                " multiply defined.");
                        if ((unitseq > 0) && (unitseq < maxppunits)) {
                            u->num = unitseq;
                            ppUnits[unitseq++] = u;
                        }
                    }
                } else {
                    Awrite(AString("Error: Unit number ")+i+
                            " out of range.");
                    if ((unitseq > 0) && (unitseq < maxppunits)) {
                        u->num = unitseq;
                        ppUnits[unitseq++] = u;
                    }
                }
            }
        }
    }
}

Unit::Handle Game::GetNewUnit(const Faction::Handle& fac, int an)
{
    unsigned int i;
    for (i = 1; i < unitseq; i++) {
        if (!ppUnits[i]) {
            Unit::Handle pUnit = std::make_shared<Unit>(i, fac, an);
            ppUnits[i] = pUnit;
            return pUnit;
        }
    }

    Unit::Handle pUnit = std::make_shared<Unit>(unitseq, fac, an);
    ppUnits[unitseq] = pUnit;
    unitseq++;
    if (unitseq >= maxppunits) {
        maxppunits += 10000;
        ppUnits.resize(maxppunits);
    }

    return pUnit;
}

Unit::WeakHandle Game::GetUnit(int num)
{
    if(num < 0)
    {
        return Unit::WeakHandle();
    }

    return GetUnit(static_cast<size_t>(num));
}

Unit::WeakHandle Game::GetUnit(size_t num)
{
    if (num >= maxppunits)
    {
        return Unit::WeakHandle();
    }

    return ppUnits[num];
}


void Game::CountAllSpecialists()
{
    for(const auto& fac: factions) {
        fac->nummages = 0;
        fac->numqms = 0;
        fac->numtacts = 0;
        fac->numapprentices = 0;
    }

    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                const auto fac = u->faction.lock();
                if (u->type == U_MAGE)
                {
                    fac->nummages++;
                }
                if (u->GetSkill(Skills::Types::S_QUARTERMASTER))
                {
                    fac->numqms++;
                }
                if (u->GetSkill(Skills::Types::S_TACTICS) == 5)
                {
                    fac->numtacts++;
                }
                if (u->type == U_APPRENTICE)
                {
                    fac->numapprentices++;
                }
            }
        }
    }
}

// LLS
void Game::UnitFactionMap()
{
    Aoutfile f;

    Awrite("Opening units.txt");
    if (f.OpenByName("units.txt") == -1) {
        Awrite("Couldn't open file!");
    } else {
        Awrite(AString("Writing ") + unitseq + " units");
        for (size_t i = 1; i < unitseq; i++) {
            const auto u = GetUnit(i);
            if (u.expired()) {
                Awrite("doesn't exist");
            } else {
                const auto fac_num = u.lock()->faction.lock()->num;
                Awrite(AString(i) + ":" + fac_num);
                f.PutStr(AString(i) + ":" + fac_num);
            }
        }
    }
    f.Close();
}


//The following function added by Creative PBM February 2000
void Game::RemoveInactiveFactions()
{
    if (Globals->MAX_INACTIVE_TURNS == -1) return;

    int cturn;
    cturn = TurnNumber();
    for(const auto& fac: factions) {
        if ((cturn - fac->lastorders) >= Globals->MAX_INACTIVE_TURNS &&
                !fac->IsNPC()) {
            fac->quit = QUIT_BY_GM;
        }
    }
}

/*
void Game::CountAllApprentices()
{
    if (!Globals->APPRENTICES_EXIST) return;

    forlist(&factions) {
        ((Faction *)elem)->numapprentices = 0;
    }
    {
        forlist(&regions) {
            ARegion *r = (ARegion *)elem;
            forlist(&r->objects) {
                Object *o = (Object *)elem;
                forlist(&o->units) {
                    Unit *u = (Unit *)elem;
                    if (u->type == U_APPRENTICE)
                        u->faction->numapprentices++;
                }
            }
        }
    }
}
*/

size_t Game::CountMages(const Faction::Handle& pFac)
{
    size_t i = 0;
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->faction.lock() == pFac && u->type == U_MAGE) i++;
            }
        }
    }
    return(i);
}

size_t Game::CountQuarterMasters(const Faction::Handle& pFac)
{
    size_t i = 0;
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->faction.lock() == pFac && u->GetSkill(Skills::Types::S_QUARTERMASTER)) i++;
            }
        }
    }
    return i;
}

size_t Game::CountTacticians(const Faction::Handle& pFac)
{
    size_t i = 0;
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->faction.lock() == pFac && u->GetSkill(Skills::Types::S_TACTICS) == 5) i++;
            }
        }
    }
    return i;
}

size_t Game::CountApprentices(const Faction::Handle& pFac)
{
    size_t i = 0;
    for(const auto& r: regions) {
        for(const auto& o: r->objects) {
            for(const auto& u: o->units) {
                if (u->faction.lock() == pFac && u->type == U_APPRENTICE)
                {
                    i++;
                }
            }
        }
    }
    return i;
}

int Game::getAllowedPoints(int p, const std::vector<int>& allowed)
{
    if(p < 0)
    {
        return allowed.front();
    }

    size_t points = static_cast<size_t>(p);
    if(points >= allowed.size())
    {
        return allowed.back();
    }

    return allowed[points];
}

int Game::AllowedMages(const Faction& pFac)
{
    return getAllowedPoints(pFac.type[F_MAGIC], allowedMages);
}

int Game::AllowedQuarterMasters(const Faction& pFac)
{
    return getAllowedPoints(pFac.type[F_TRADE], allowedQuartermasters);
}

int Game::AllowedTacticians(const Faction& pFac)
{
    return getAllowedPoints(pFac.type[F_WAR], allowedTacticians);
}

int Game::AllowedApprentices(const Faction& pFac)
{
    return getAllowedPoints(pFac.type[F_MAGIC], allowedApprentices);
}

int Game::AllowedTaxes(const Faction& pFac)
{
    return getAllowedPoints(pFac.type[F_WAR], allowedTaxes);
}

int Game::AllowedTrades(const Faction& pFac)
{
    return getAllowedPoints(pFac.type[F_TRADE], allowedTrades);
}

int Game::UpgradeMajorVersion(ATL_VER)
{
    return 0;
}

int Game::UpgradeMinorVersion(ATL_VER)
{
    return 1;
}

int Game::UpgradePatchLevel(ATL_VER)
{
    return 1;
}

void Game::MidProcessUnitExtra(ARegion *r, Unit *u)
{
    if (Globals->CHECK_MONSTER_CONTROL_MID_TURN) MonsterCheck(r, u);
}

void Game::PostProcessUnitExtra(ARegion *r, Unit *u)
{
    if (!Globals->CHECK_MONSTER_CONTROL_MID_TURN) MonsterCheck(r, u);
}

void Game::MonsterCheck(ARegion *r, Unit *u)
{
    AString tmp;
    int linked = 0;
    using MapType = std::map<int, int>;
    MapType chances;

    if (u->type != U_WMON) {

        for (const auto& i: u->items) {
            if (!i->num) continue;
            if (!ItemDefs[i->type].escape) continue;

            Skills skill;
            // Okay, check flat loss.
            if (ItemDefs[i->type].escape & ItemType::LOSS_CHANCE) {
                size_t losses = (i->num +
                        getrandom(ItemDefs[i->type].esc_val)) /
                    ItemDefs[i->type].esc_val;
                // LOSS_CHANCE and HAS_SKILL together mean the
                // decay rate only applies if you don't have
                // the required skill (this might get used if
                // you made illusions GIVEable, for example).
                if (ItemDefs[i->type].escape & ItemType::HAS_SKILL) {
                    tmp = ItemDefs[i->type].esc_skill;
                    skill = LookupSkill(&tmp);
                    if (u->GetSkill(skill) >= static_cast<int>(ItemDefs[i->type].esc_val))
                        losses = 0;
                }
                if (losses) {
                    tmp = ItemString(i->type, static_cast<int>(losses));
                    tmp += " decay";
                    if (losses == 1)
                        tmp += "s";
                    tmp += " into nothingness.";
                    u->Event(tmp);
                    u->items.SetNum(i->type,i->num - losses);
                }
            } else if (ItemDefs[i->type].escape & ItemType::HAS_SKILL) {
                tmp = ItemDefs[i->type].esc_skill;
                skill = LookupSkill(&tmp);
                if (u->GetSkill(skill) < static_cast<int>(ItemDefs[i->type].esc_val)) {
                    if (Globals->WANDERING_MONSTERS_EXIST) {
                        const auto mfac = GetFaction(factions, monfaction);
                        const auto mon = GetNewUnit(mfac, 0);
                        MonType *mp = FindMonster(ItemDefs[i->type].abr,
                                (ItemDefs[i->type].type & IT_ILLUSION));
                        mon->MakeWMon(mp->name, i->type, i->num);
                        mon->MoveUnit(r->GetDummy());
                        // This will be zero unless these are set. (0 means
                        // full spoils)
                        mon->free = Globals->MONSTER_NO_SPOILS +
                            Globals->MONSTER_SPOILS_RECOVERY;
                    }
                    u->Event(AString("Loses control of ") +
                            ItemString(i->type, static_cast<int>(i->num)) + ".");
                    u->items.SetNum(i->type, 0);
                }
            } else {
                // ESC_LEV_*
                tmp = ItemDefs[i->type].esc_skill;
                skill = LookupSkill(&tmp);
                int level = u->GetSkill(skill);
                int chance;

                if (!level)
                    chance = 10000;
                else {
                    int top;
                    int i_num = static_cast<int>(i->num);
                    if (ItemDefs[i->type].escape & ItemType::ESC_NUM_SQUARE)
                        top = i_num * i_num;
                    else
                        top = i_num;
                    int bottom = 0;
                    if (ItemDefs[i->type].escape & ItemType::ESC_LEV_LINEAR)
                        bottom = level;
                    else if (ItemDefs[i->type].escape & ItemType::ESC_LEV_SQUARE)
                        bottom = level * level;
                    else if (ItemDefs[i->type].escape & ItemType::ESC_LEV_CUBE)
                        bottom = level * level * level;
                    else if (ItemDefs[i->type].escape & ItemType::ESC_LEV_QUAD)
                        bottom = level * level * level * level;
                    else
                        bottom = 1;
                    bottom = bottom * static_cast<int>(ItemDefs[i->type].esc_val);
                    chance = (top * 10000)/bottom;
                }

                if (ItemDefs[i->type].escape & ItemType::LOSE_LINKED) {
                    if (chance > chances[ItemDefs[i->type].type])
                        chances[ItemDefs[i->type].type] = chance;
                    linked = 1;
                } else if (chance > getrandom(10000)) {
                    if (Globals->WANDERING_MONSTERS_EXIST) {
                        const auto mfac = GetFaction(factions, monfaction);
                        const auto mon = GetNewUnit(mfac, 0);
                        MonType *mp = FindMonster(ItemDefs[i->type].abr,
                                (ItemDefs[i->type].type & IT_ILLUSION));
                        mon->MakeWMon(mp->name, i->type, i->num);
                        mon->MoveUnit(r->GetDummy());
                        // This will be zero unless these are set. (0 means
                        // full spoils)
                        mon->free = Globals->MONSTER_NO_SPOILS +
                            Globals->MONSTER_SPOILS_RECOVERY;
                    }
                    u->Event(AString("Loses control of ") +
                            ItemString(i->type, static_cast<int>(i->num)) + ".");
                    u->items.SetNum(i->type, 0);
                }
            }
        }

        if (linked) {
            MapType::iterator i;
            for (i = chances.begin(); i != chances.end(); i++) {
                // walk the chances list and for each chance, see if
                // escape happens and if escape happens then walk all items
                // and everything that is that type, get rid of it.
                if ((*i).second < getrandom(10000)) continue;
                for(const auto& it: u->items) {
                    if (ItemDefs[it->type].type == (*i).first) {
                        if (Globals->WANDERING_MONSTERS_EXIST) {
                            const auto mfac = GetFaction(factions, monfaction);
                            const auto mon = GetNewUnit(mfac, 0);
                            MonType *mp = FindMonster(ItemDefs[it->type].abr,
                                    (ItemDefs[it->type].type & IT_ILLUSION));
                            mon->MakeWMon(mp->name, it->type, it->num);
                            mon->MoveUnit(r->GetDummy());
                            // This will be zero unless these are set. (0 means
                            // full spoils)
                            mon->free = Globals->MONSTER_NO_SPOILS +
                                Globals->MONSTER_SPOILS_RECOVERY;
                        }
                        u->Event(AString("Loses control of ") +
                                ItemString(it->type, static_cast<int>(it->num)) + ".");
                        u->items.SetNum(it->type, 0);
                    }
                }
            }
        }

    }
}

void Game::CheckUnitMaintenance(int consume)
{
    CheckUnitMaintenanceItem(Items::Types::I_FOOD, Globals->UPKEEP_FOOD_VALUE, consume);
    CheckUnitMaintenanceItem(Items::Types::I_GRAIN, Globals->UPKEEP_FOOD_VALUE, consume);
    CheckUnitMaintenanceItem(Items::Types::I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE, consume);
    CheckUnitMaintenanceItem(Items::Types::I_FISH, Globals->UPKEEP_FOOD_VALUE, consume);
}

void Game::CheckFactionMaintenance(int con)
{
    CheckFactionMaintenanceItem(Items::Types::I_FOOD, Globals->UPKEEP_FOOD_VALUE, con);
    CheckFactionMaintenanceItem(Items::Types::I_GRAIN, Globals->UPKEEP_FOOD_VALUE, con);
    CheckFactionMaintenanceItem(Items::Types::I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE, con);
    CheckFactionMaintenanceItem(Items::Types::I_FISH, Globals->UPKEEP_FOOD_VALUE, con);
}

void Game::CheckAllyMaintenance()
{
    CheckAllyMaintenanceItem(Items::Types::I_FOOD, Globals->UPKEEP_FOOD_VALUE);
    CheckAllyMaintenanceItem(Items::Types::I_GRAIN, Globals->UPKEEP_FOOD_VALUE);
    CheckAllyMaintenanceItem(Items::Types::I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE);
    CheckAllyMaintenanceItem(Items::Types::I_FISH, Globals->UPKEEP_FOOD_VALUE);
}

void Game::CheckUnitHunger()
{
    CheckUnitHungerItem(Items::Types::I_FOOD, Globals->UPKEEP_FOOD_VALUE);
    CheckUnitHungerItem(Items::Types::I_GRAIN, Globals->UPKEEP_FOOD_VALUE);
    CheckUnitHungerItem(Items::Types::I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE);
    CheckUnitHungerItem(Items::Types::I_FISH, Globals->UPKEEP_FOOD_VALUE);
}

void Game::CheckFactionHunger()
{
    CheckFactionHungerItem(Items::Types::I_FOOD, Globals->UPKEEP_FOOD_VALUE);
    CheckFactionHungerItem(Items::Types::I_GRAIN, Globals->UPKEEP_FOOD_VALUE);
    CheckFactionHungerItem(Items::Types::I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE);
    CheckFactionHungerItem(Items::Types::I_FISH, Globals->UPKEEP_FOOD_VALUE);
}

void Game::CheckAllyHunger()
{
    CheckAllyHungerItem(Items::Types::I_FOOD, Globals->UPKEEP_FOOD_VALUE);
    CheckAllyHungerItem(Items::Types::I_GRAIN, Globals->UPKEEP_FOOD_VALUE);
    CheckAllyHungerItem(Items::Types::I_LIVESTOCK, Globals->UPKEEP_FOOD_VALUE);
    CheckAllyHungerItem(Items::Types::I_FISH, Globals->UPKEEP_FOOD_VALUE);
}

char Game::GetRChar(const ARegion::Handle& r)
{
    if (!r)
    {
        return ' ';
    }
    const auto& t = r->type;
    if (!t.isValid() || t > Regions::Types::R_NUM) return '?';
    char c = TerrainDefs[r->type].marker;
    if (r->town) {
        c = static_cast<char>((c - 'a') + 'A');
    }
    return c;
}

void Game::CreateNPCFactions()
{
    AString *temp;
    if (Globals->CITY_MONSTERS_EXIST) {
        auto& f = factions.emplace_back(std::make_shared<Faction>(factionseq++));
        guardfaction = f->num;
        temp = new AString("The Guardsmen");
        f->SetName(temp);
        f->SetNPC();
        f->lastorders = 0;
    } else
        guardfaction = 0;
    // Only create the monster faction if wandering monsters or lair
    // monsters exist.
    if (Globals->LAIR_MONSTERS_EXIST || Globals->WANDERING_MONSTERS_EXIST) {
        auto& f = factions.emplace_back(std::make_shared<Faction>(factionseq++));
        monfaction = f->num;
        temp = new AString("Creatures");
        f->SetName(temp);
        f->SetNPC();
        f->lastorders = 0;
    } else
        monfaction = 0;
}

void Game::CreateCityMon(const ARegion::Handle& pReg, size_t percent, int needmage)
{
    int skilllevel;
    int AC = 0;
    int IV = 0;
    size_t num;
    if (pReg->type == Regions::Types::R_NEXUS || pReg->IsStartingCity()) {
        skilllevel = static_cast<int>(TownTypeEnum::TOWN_CITY) + 1;
        if (Globals->SAFE_START_CITIES || (pReg->type == Regions::Types::R_NEXUS))
            IV = 1;
        AC = 1;
        num = Globals->AMT_START_CITY_GUARDS;
    } else {
        skilllevel = static_cast<int>(pReg->town->TownType()) + 1;
        num = Globals->CITY_GUARD * static_cast<size_t>(skilllevel);
    }
    num = num * percent / 100;
    const auto pFac = GetFaction(factions, guardfaction);
    Unit::Handle u = GetNewUnit(pFac);
    Unit::Handle u2;
    AString *s = new AString("City Guard");
    
    /*
    Awrite(AString("Begin setting up city guard in..."));
        
    AString temp = TerrainDefs[pReg->type].name;
    temp += AString(" (") + pReg->xloc + "," + pReg->yloc;
    temp += ")";
    temp += AString(" in ") + *pReg->name;
    Awrite(temp);
    */
    
    if ((Globals->LEADERS_EXIST) || (pReg->type == Regions::Types::R_NEXUS)) {
        /* standard Leader-type guards */
        u->SetMen(Items::Types::I_LEADERS,num);
        u->items.SetNum(Items::Types::I_SWORD,num);
        if (IV) u->items.SetNum(Items::Types::I_AMULETOFI,num);
        u->SetMoney(static_cast<int>(num * Globals->GUARD_MONEY));
        u->SetSkill(Skills::Types::S_COMBAT, skilllevel);
        u->SetName(s);
        u->type = U_GUARD;
        u->guard = GUARD_GUARD;
        u->reveal = REVEAL_FACTION;
    } else {
        /* non-leader guards */
        size_t n = 3 * num / 4;
        int plate = 0;
        if ((AC) && (Globals->START_CITY_GUARDS_PLATE)) plate = 1;
        u = MakeManUnit(pFac, pReg->race, n, skilllevel, 1, plate, 0);
        if (IV) u->items.SetNum(Items::Types::I_AMULETOFI,num);
        u->SetMoney(num * Globals->GUARD_MONEY / 2);
        u->SetName(s);
        u->type = U_GUARD;
        u->guard = GUARD_GUARD;
        u->reveal = REVEAL_FACTION;
        u2 = MakeManUnit(pFac, pReg->race, n, skilllevel, 1, plate, 1);
        if (IV) u2->items.SetNum(Items::Types::I_AMULETOFI,num);
        u2->SetMoney(num * Globals->GUARD_MONEY / 2);
        AString *un = new AString("City Guard");
        u2->SetName(un);
        u2->type = U_GUARD;
        u2->guard = GUARD_GUARD;
        u2->reveal = REVEAL_FACTION;
    }            
    
    if (AC) {
        if (Globals->START_CITY_GUARDS_PLATE) {
            if (Globals->LEADERS_EXIST) u->items.SetNum(Items::Types::I_PLATEARMOR, num);
        }
        u->SetSkill(Skills::Types::S_OBSERVATION,10);
        if (Globals->START_CITY_TACTICS)
            u->SetSkill(Skills::Types::S_TACTICS, Globals->START_CITY_TACTICS);
    } else {
        u->SetSkill(Skills::Types::S_OBSERVATION, skilllevel);
    }
    u->SetFlag(FLAG_HOLDING,1);
    u->MoveUnit(pReg->GetDummy());
    /*
    Awrite(AString(*u->BattleReport(3)));
    */
    if ((!Globals->LEADERS_EXIST) && (pReg->type != Regions::Types::R_NEXUS)) {
        u2->SetFlag(FLAG_HOLDING,1);
        u2->MoveUnit(pReg->GetDummy());
        /*
        Awrite(AString(*u2->BattleReport(3)));
        */
    }

    if (AC && Globals->START_CITY_MAGES && needmage) {
        u = GetNewUnit(pFac);
        s = new AString("City Mage");
        u->SetName(s);
        u->type = U_GUARDMAGE;
        u->reveal = REVEAL_FACTION;
        u->SetMen(Items::Types::I_LEADERS,1);
        if (IV) u->items.SetNum(Items::Types::I_AMULETOFI,1);
        u->SetMoney(Globals->GUARD_MONEY);
        u->SetSkill(Skills::Types::S_FORCE,Globals->START_CITY_MAGES);
        u->SetSkill(Skills::Types::S_FIRE,Globals->START_CITY_MAGES);
        if (Globals->START_CITY_TACTICS)
            u->SetSkill(Skills::Types::S_TACTICS, Globals->START_CITY_TACTICS);
        u->combat = Skills::Types::S_FIRE;
        u->SetFlag(FLAG_BEHIND, 1);
        u->SetFlag(FLAG_HOLDING, 1);
        u->MoveUnit(pReg->GetDummy());
    }
}

void Game::AdjustCityMons(const ARegion::Handle& r)
{
    int needguard = 1;
    int needmage = 1;
    for(const auto& o: r->objects) {
        for(const auto& u: o->units) {
            if (u->type == U_GUARD || u->type == U_GUARDMAGE) {
                AdjustCityMon(r, u);
                /* Don't create new city guards if we have some */
                needguard = 0;
                if (u->type == U_GUARDMAGE)
                    needmage = 0;
            }
            if (u->guard == GUARD_GUARD) needguard = 0;
        }
    }

    if (needguard && (getrandom(100) < Globals->GUARD_REGEN)) {
        CreateCityMon(r, 10, needmage);
    }
}

void Game::AdjustCityMon(const ARegion::Handle& r, const Unit::Handle& u)
{
    TownTypeEnum towntype;
    int AC = 0;
    size_t men;
    int IV = 0;
    Items mantype;
    size_t maxmen;
    Items weapon;
    size_t maxweapon = 0;
    Items armor;
    size_t maxarmor = 0;
    for (size_t i = 0; i < Items::size(); i++) {
        size_t num = u->items.GetNum(i);
        if (num == 0) continue;
        if (ItemDefs[i].type & IT_MAN) mantype = i;
        if ((ItemDefs[i].type & IT_WEAPON)
            && (num > maxweapon)) {
            weapon = i;
            maxweapon = num;
        }
        if ((ItemDefs[i].type & IT_ARMOR)
            && (num > maxarmor)) {
            armor = i;    
            maxarmor = num;
        }
    }
    Skills skill = Skills::Types::S_COMBAT;
    
    if (weapon.isValid()) {
        WeaponType *wp = FindWeapon(ItemDefs[weapon].abr);
        if (FindSkill(wp->baseSkill) == FindSkill("XBOW")) skill = Skills::Types::S_CROSSBOW;
        if (FindSkill(wp->baseSkill) == FindSkill("LBOW")) skill = Skills::Types::S_LONGBOW;
    }
    
    size_t sl = u->GetRealSkill(skill);
        
    if (r->type == Regions::Types::R_NEXUS || r->IsStartingCity()) {
        towntype = TownTypeEnum::TOWN_CITY;
        AC = 1;
        if (Globals->SAFE_START_CITIES || (r->type == Regions::Types::R_NEXUS))
            IV = 1;
        if (u->type == U_GUARDMAGE) {
            men = 1;
        } else {
            maxmen = Globals->AMT_START_CITY_GUARDS;
            if ((!Globals->LEADERS_EXIST) && (r->type != Regions::Types::R_NEXUS))
                maxmen = 3 * maxmen / 4;
            men = u->GetMen() + (Globals->AMT_START_CITY_GUARDS/10);
            if (men > maxmen)
                men = maxmen;
        }
    } else {
        towntype = r->town->TownType();
        maxmen = Globals->CITY_GUARD * (static_cast<size_t>(towntype)+1);
        if (!Globals->LEADERS_EXIST) maxmen = 3 * maxmen / 4;
        men = u->GetMen() + (maxmen/10);
        if (men > maxmen)
            men = maxmen;
    }

    u->SetMen(mantype,men);
    if (IV) u->items.SetNum(Items::Types::I_AMULETOFI,men);

    if (u->type == U_GUARDMAGE) {
        if (Globals->START_CITY_TACTICS)
            u->SetSkill(Skills::Types::S_TACTICS, Globals->START_CITY_TACTICS);
        u->SetSkill(Skills::Types::S_FORCE, Globals->START_CITY_MAGES);
        u->SetSkill(Skills::Types::S_FIRE, Globals->START_CITY_MAGES);
        u->combat = Skills::Types::S_FIRE;
        u->SetFlag(FLAG_BEHIND, 1);
        u->SetMoney(Globals->GUARD_MONEY);
    } else {
        size_t money = men * (Globals->GUARD_MONEY * men / maxmen);
        u->SetMoney(money);
        u->SetSkill(skill, static_cast<int>(sl));
        if (AC) {
            u->SetSkill(Skills::Types::S_OBSERVATION,10);
            if (Globals->START_CITY_TACTICS)
                u->SetSkill(Skills::Types::S_TACTICS, Globals->START_CITY_TACTICS);
            if (Globals->START_CITY_GUARDS_PLATE)
                u->items.SetNum(armor,men);
        } else {
            u->SetSkill(Skills::Types::S_OBSERVATION, static_cast<int>(towntype) + 1);
        }
        if (weapon.isValid()) {
            u->items.SetNum(weapon,men);
        }
    }
}

void Game::Equilibrate()
{
    Awrite("Initialising the economy");
    for (int a=0; a<25; a++) {
        Adot();
        ProcessMigration();
        for(const auto& r: regions) {
            r->PostTurn(regions);
        }
    }
    Awrite("");
}

void Game::WriteTimesArticle(AString article)
{
        AString filename;
        int result;
        Arules f;

        do {
                filename = "times.";
                filename += getrandom(10000);
                result = access(filename.Str(), F_OK);
        } while (result == 0);

        if (f.OpenByName(filename) != -1) {
                f.PutStr(article);
                f.Close();
        }
}

