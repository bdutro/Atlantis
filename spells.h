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
#ifdef GAME_SPELLS
//
// If GAME_SPELLS is defined, this is being included from inside the Game
// class in game.h
//

//
// Spell parsing - generic
//
void ProcessGenericSpell(const Unit::Handle&, const Skills&, const OrdersCheck::Handle&pCheck);
void ProcessRegionSpell(const Unit::Handle&, AString *, const Skills&, const OrdersCheck::Handle&pCheck);

//
// Spell parsing - specific
//
void ProcessCastGateLore(const Unit::Handle&,AString *, const OrdersCheck::Handle&pCheck );
void ProcessCastPortalLore(const Unit::Handle&,AString *, const OrdersCheck::Handle&pCheck );
void ProcessPhanBeasts(const Unit::Handle&,AString *, const OrdersCheck::Handle&pCheck );
void ProcessPhanUndead(const Unit::Handle&,AString *, const OrdersCheck::Handle&pCheck );
void ProcessPhanDemons(const Unit::Handle&,AString *, const OrdersCheck::Handle&pCheck );
void ProcessInvisibility(const Unit::Handle&,AString *, const OrdersCheck::Handle&pCheck );
void ProcessBirdLore(const Unit::Handle&,AString *, const OrdersCheck::Handle&pCheck );
void ProcessMindReading(const Unit::Handle&,AString *, const OrdersCheck::Handle&pCheck );
void ProcessLacandonTeleport(const Unit::Handle&, AString *, const OrdersCheck::Handle&pCheck);
void ProcessTransmutation(const Unit::Handle&, AString *, const OrdersCheck::Handle&pCheck);

//
// Spell helpers
//
bool GetRegionInRange(const ARegion::Handle& r, const ARegion::Handle& tar, const Unit::Handle& u, const Skills& spell);

//
// Spell running
//
int RunDetectGates(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
int RunFarsight(const ARegion::Handle&,const Unit::Handle&);
int RunGateJump(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
int RunTeleport(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
int RunLacandonTeleport(const ARegion::Handle&, const Object::Handle&, const Unit::Handle&);
int RunPortalLore(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
int RunEarthLore(const ARegion::Handle&,const Unit::Handle&);
int RunWeatherLore(const ARegion::Handle&, const Unit::Handle&);
int RunClearSkies(const ARegion::Handle&,const Unit::Handle&);
int RunPhanBeasts(const ARegion::Handle&,const Unit::Handle&);
int RunPhanUndead(const ARegion::Handle&,const Unit::Handle&);
int RunPhanDemons(const ARegion::Handle&,const Unit::Handle&);
int RunInvisibility(const ARegion::Handle&,const Unit::Handle&);
int RunWolfLore(const ARegion::Handle&,const Unit::Handle&);
int RunBirdLore(const ARegion::Handle&,const Unit::Handle&);
int RunDragonLore(const ARegion::Handle&,const Unit::Handle&);
int RunSummonSkeletons(const ARegion::Handle&,const Unit::Handle&);
int RunRaiseUndead(const ARegion::Handle&,const Unit::Handle&);
int RunSummonLich(const ARegion::Handle&,const Unit::Handle&);
int RunSummonImps(const ARegion::Handle&,const Unit::Handle&);
int RunSummonDemon(const ARegion::Handle&,const Unit::Handle&);
bool RunSummonBalrog(const ARegion::Handle&,const Unit::Handle&);
int RunCreateArtifact(const ARegion::Handle&, const Unit::Handle&, const Skills&, const Items&);
bool RunEngraveRunes(const ARegion::Handle&,const Object::Handle&,const Unit::Handle&);
bool RunConstructGate(const ARegion::Handle&, const Unit::Handle&, const Skills&);
bool RunEnchant(const ARegion::Handle&,const Unit::Handle&, const Skills&, const Items&);
bool RunMindReading(const ARegion::Handle&,const Unit::Handle&);
int RunTransmutation(const ARegion::Handle&,const Unit::Handle&);
int RunBlasphemousRitual(const ARegion::Handle&,const Unit::Handle&);
#endif
