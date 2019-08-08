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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
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
// MODIFICATIONS
// Date                Person                 Comments
// ----                ------                 --------
// 2000/MAR/14 Davis Kulis        Added a new reporting Template.
// 2001/Feb/18 Joseph Traub     Added Apprentices from Lacandon Conquest
#ifndef FACTION_CLASS
#define FACTION_CLASS

class Faction;
class Game;

#include <memory>
#include <list>
#include <vector>

#include "itemtype.h"
#include "gameio.h"
#include "aregion.h"
#include "fileio.h"
#include "unit.h"
#include "battle.h"
#include "skills.h"
#include "items.h"
#include "alist.h"
#include "astring.h"

enum {
    A_HOSTILE,
    A_UNFRIENDLY,
    A_NEUTRAL,
    A_FRIENDLY,
    A_ALLY,
    NATTITUDES
};

enum {
    F_WAR,
    F_TRADE,
    F_MAGIC,
    NFACTYPES
};

// DK
// LLS - make templates cleaner for save/restore
enum {
    TEMPLATE_OFF,
    TEMPLATE_SHORT,
    TEMPLATE_LONG,
    TEMPLATE_MAP,
    NTEMPLATES
};

enum {
    QUIT_NONE,
    QUIT_BY_ORDER,
    QUIT_BY_GM,
    QUIT_AND_RESTART,
    QUIT_WON_GAME,
    QUIT_GAME_OVER,
};

extern char const ** AttitudeStrs;
extern char const ** FactionStrs;

// LLS - include strings for the template enum
extern char const **TemplateStrs;
int ParseTemplate(AString *);

int ParseAttitude(AString *);

int MagesByFacType(int);

class Attitude {
public:
    using Handle = std::shared_ptr<Attitude>;

    Attitude() = default;
    ~Attitude() = default;
    void Writeout(Aoutfile * );
    void Readin( Ainfile *, ATL_VER version );
    
    size_t factionnum;
    int attitude;
};

class Faction
{
public:
    using Handle = std::shared_ptr<Faction>;
    using WeakHandle = std::weak_ptr<Faction>;

    Faction();
    Faction(size_t);
    ~Faction();
    
    void Readin( Ainfile *, ATL_VER version );
    void Writeout( Aoutfile * );
    void View();
    
    void SetName(AString *);
    void SetNameNoChange( AString *str );
    void SetAddress( AString &strNewAddress );
    
    void CheckExist(const ARegionList&);
    void Error(const AString &);
    void Event(const AString &);
    
    AString FactionTypeStr();
    void WriteReport( Areport *f, Game *pGame );
    // LLS - write order template
    void WriteTemplate(Areport *f, Game *pGame);
    void WriteFacInfo(Aoutfile *);
    
    void SetAttitude(size_t,int); /* faction num, attitude */
    /* if attitude == -1, clear it */
    int GetAttitude(size_t);
    void RemoveAttitude(size_t);
    
    bool CanCatch(const std::shared_ptr<ARegion>&, const std::shared_ptr<Unit>&);
    /* Return 1 if can see, 2 if can see faction */
    int CanSee(const std::shared_ptr<ARegion>&, const std::shared_ptr<Unit>&, int practice = 0);
    
    void DefaultOrders();
    void TimesReward();
    
    void SetNPC();
    bool IsNPC() const;

    void DiscoverItem(const Items& item, int force, int full);

    size_t num;

    //
    // The type is only used if Globals->FACTION_LIMIT_TYPE ==
    // FACLIM_FACTION_TYPES
    //
    int type[NFACTYPES];

    int lastchange;
    int lastorders;
    int unclaimed;
    int bankaccount;
    int interest; // not written to game.out
    AString * name;
    AString * address;
    AString * password;
    int times;
    bool showunitattitudes;
    int temformat;
    bool exists;
    int quit;
    int numshows;
    
    int nummages;
    int numapprentices;
    int numqms;
    int numtacts;
    std::list<std::weak_ptr<ARegion>> war_regions;
    std::list<std::weak_ptr<ARegion>> trade_regions;

    /* Used when writing reports */
    std::list<std::weak_ptr<ARegion>> present_regions;
    
    int defaultattitude;
    std::list<Attitude::Handle> attitudes;
    SkillList skills;
    ItemList items;
    
    //
    // Both are lists of AStrings
    //
    AList extraPlayers;
    AList errors;
    AList events;
    std::list<std::weak_ptr<Battle>> battles;
    std::list<ShowSkill::Handle> shows;
    AList itemshows;
    AList objectshows;

    // These are used for 'granting' units to a faction via the players.in
    // file
    std::weak_ptr<ARegion> pReg;
    std::weak_ptr<ARegion> pStartLoc;
    int noStartLeader;
    int startturn;
};

class FactionVector {
public:
    using iterator = std::vector<Faction::WeakHandle>::iterator;
    using const_iterator = std::vector<Faction::WeakHandle>::const_iterator;

    FactionVector(size_t);
    ~FactionVector() = default;

    void ClearVector();
    void SetFaction(size_t, const Faction::WeakHandle&);
    Faction::WeakHandle GetFaction(size_t);
    iterator begin() { return vector.begin(); }
    iterator end() { return vector.end(); }
    const_iterator cbegin() const { return vector.cbegin(); }
    const_iterator cend() const { return vector.cend(); }
    const_iterator begin() const { return vector.begin(); }
    const_iterator end() const { return vector.end(); }

    std::vector<Faction::WeakHandle> vector;
    size_t vectorsize;
};
    
Faction::Handle GetFaction(const std::list<Faction::Handle>&, size_t);
Faction::WeakHandle GetFaction2(const std::list<Faction::WeakHandle>&, size_t); /*This AList is a list of FactionPtr*/

#endif
