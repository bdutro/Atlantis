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
#include <array>

#include "itemtype.h"
#include "gameio.h"
#include "aregion.h"
#include "fileio.h"
#include "unit.h"
#include "battle.h"
#include "skills.h"
#include "items.h"
#include "astring.h"
#include "attitudetype.h"
#include "enumarray.h"

enum class _Factions : size_t {
    F_WAR,
    F_TRADE,
    F_MAGIC,
    NFACTYPES
};

using Factions = ValidEnum<_Factions, _Factions::NFACTYPES>;

// DK
// LLS - make templates cleaner for save/restore
enum class _Templates : size_t {
    TEMPLATE_OFF,
    TEMPLATE_SHORT,
    TEMPLATE_LONG,
    TEMPLATE_MAP,
    NTEMPLATES
};

using Templates = ValidEnum<_Templates, _Templates::NTEMPLATES>;

enum class QuitReason {
    QUIT_NONE,
    QUIT_BY_ORDER,
    QUIT_BY_GM,
    QUIT_AND_RESTART,
    QUIT_WON_GAME,
    QUIT_GAME_OVER,
};

extern const EnumArray<Attitudes, const char*, Attitudes::size()> AttitudeStrs;
extern const EnumArray<Factions, const char*, Factions::size()> FactionStrs;

// LLS - include strings for the template enum
extern const EnumArray<Templates, const char*, Templates::size()> TemplateStrs;
Templates ParseTemplate(const AString&);

Attitudes ParseAttitude(const AString&);

int MagesByFacType(int);

class Attitude {
public:
    using Handle = std::shared_ptr<Attitude>;

    Attitude() = default;
    ~Attitude() = default;
    void Writeout(Aoutfile& );
    void Readin( Ainfile&, ATL_VER version );
    
    size_t factionnum;
    Attitudes attitude;
};

class Faction
{
public:
    using Handle = std::shared_ptr<Faction>;
    using WeakHandle = std::weak_ptr<Faction>;

    Faction();
    Faction(size_t);
    ~Faction() = default;
    
    void Readin( Ainfile&, ATL_VER version );
    void Writeout( Aoutfile& );
    void View();
    
    void SetName(const AString&);
    void SetNameNoChange(const AString& str);
    void SetAddress(const AString &strNewAddress);
    
    void CheckExist(const ARegionList&);
    void Error(const AString &);
    void Event(const AString &);
    
    AString FactionTypeStr();
    void WriteReport(Areport& f, const Game& pGame );
    // LLS - write order template
    void WriteTemplate(Areport& f, const Game& pGame);
    void WriteFacInfo(Aoutfile&);
    
    void SetAttitude(size_t, const Attitudes&); /* faction num, attitude */
    /* if attitude == -1, clear it */
    Attitudes GetAttitude(size_t) const;
    void RemoveAttitude(size_t);
    
    bool CanCatch(const ARegion&, const std::shared_ptr<Unit>&);
    /* Return 1 if can see, 2 if can see faction */
    unsigned int CanSee(const ARegion&, const std::shared_ptr<Unit>&, int practice = 0);
    
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
    EnumArray<Factions, int, Factions::size()> type;

    int lastchange;
    size_t lastorders;
    size_t unclaimed;
    int bankaccount;
    int interest; // not written to game.out
    AString name;
    AString address;
    AString password;
    int times;
    bool showunitattitudes;
    Templates temformat;
    bool exists;
    QuitReason quit;
    int numshows;
    
    int nummages;
    int numapprentices;
    int numqms;
    int numtacts;
    WeakPtrList<ARegion> war_regions;
    WeakPtrList<ARegion> trade_regions;

    /* Used when writing reports */
    WeakPtrList<ARegion> present_regions;
    
    Attitudes defaultattitude;
    PtrList<Attitude> attitudes;
    SkillList skills;
    ItemList items;
    
    //
    // Both are lists of AStrings
    //
    std::list<AString> extraPlayers;
    std::list<AString> errors;
    std::list<AString> events;
    WeakPtrList<Battle> battles;
    PtrList<ShowSkill> shows;
    std::list<AString::Handle> itemshows;
    std::list<AString::Handle> objectshows;

    // These are used for 'granting' units to a faction via the players.in
    // file
    std::weak_ptr<ARegion> pReg;
    std::weak_ptr<ARegion> pStartLoc;
    int noStartLeader;
    size_t startturn;
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
    
Faction::Handle GetFaction(const PtrList<Faction>&, size_t);
Faction::WeakHandle GetFaction2(const WeakPtrList<Faction>&, size_t); /*This AList is a list of FactionPtr*/

#endif
