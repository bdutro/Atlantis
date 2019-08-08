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

#ifndef RULES_H
#define RULES_H

#include "regiontype.h"
#include "itemtype.h"
#include "objecttype.h"

//
// Types of skills.
//
enum {
    S_MINING,
    S_LUMBERJACK,
    S_QUARRYING,
    S_HUNTING,
    S_FISHING,
    S_HERBLORE,
    S_HORSETRAINING,
    S_WEAPONSMITH,
    S_ARMORER,
    S_CARPENTER,
    S_BUILDING,
    S_SHIPBUILDING,
    S_ENTERTAINMENT,
    S_TACTICS,
    S_COMBAT,
    S_RIDING,
    S_CROSSBOW,
    S_LONGBOW,
    S_STEALTH,
    S_OBSERVATION,
    S_HEALING,
    S_SAILING,
    S_FARMING,
    S_RANCHING,
    S_FORCE,
    S_PATTERN,
    S_SPIRIT,
    S_FIRE,
    S_EARTHQUAKE,
    S_FORCE_SHIELD,
    S_ENERGY_SHIELD,
    S_SPIRIT_SHIELD,
    S_MAGICAL_HEALING,
    S_GATE_LORE,
    S_FARSIGHT,
    S_TELEPORTATION,
    S_PORTAL_LORE,
    S_MIND_READING,
    S_WEATHER_LORE,
    S_SUMMON_WIND,
    S_SUMMON_STORM,
    S_SUMMON_TORNADO,
    S_CALL_LIGHTNING,
    S_CLEAR_SKIES,
    S_EARTH_LORE,
    S_WOLF_LORE,
    S_BIRD_LORE,
    S_DRAGON_LORE,
    S_NECROMANCY,
    S_SUMMON_SKELETONS,
    S_RAISE_UNDEAD,
    S_SUMMON_LICH,
    S_CREATE_AURA_OF_FEAR,
    S_SUMMON_BLACK_WIND,
    S_BANISH_UNDEAD,
    S_DEMON_LORE,
    S_SUMMON_IMPS,
    S_SUMMON_DEMON,
    S_SUMMON_BALROG,
    S_BANISH_DEMONS,
    S_ILLUSION,
    S_PHANTASMAL_ENTERTAINMENT,
    S_CREATE_PHANTASMAL_BEASTS,
    S_CREATE_PHANTASMAL_UNDEAD,
    S_CREATE_PHANTASMAL_DEMONS,
    S_INVISIBILITY,
    S_TRUE_SEEING,
    S_DISPEL_ILLUSIONS,
    S_ARTIFACT_LORE,
    S_CREATE_RING_OF_INVISIBILITY,
    S_CREATE_CLOAK_OF_INVULNERABILITY,
    S_CREATE_STAFF_OF_FIRE,
    S_CREATE_STAFF_OF_LIGHTNING,
    S_CREATE_AMULET_OF_TRUE_SEEING,
    S_CREATE_AMULET_OF_PROTECTION,
    S_CREATE_RUNESWORD,
    S_CREATE_SHIELDSTONE,
    S_CREATE_MAGIC_CARPET,
    S_ENGRAVE_RUNES_OF_WARDING,
    S_CONSTRUCT_GATE,
    S_ENCHANT_SWORDS,
    S_ENCHANT_ARMOR,
    S_ENCHANT_SHIELDS,
    S_CONSTRUCT_PORTAL,
    S_MANIPULATE,
    // Skills for Ceran
    S_WEAPONCRAFT,
    S_ARMORCRAFT,
    S_CAMELTRAINING,
    S_GEMCUTTING,
    S_MONSTERTRAINING,
    S_CREATE_FLAMING_SWORD,
    // Food related skills
    S_COOKING,
    S_CREATE_FOOD,
    S_QUARTERMASTER,
    // new item creation skills to give apprentices something to do
    S_CREATE_AEGIS,
    S_CREATE_WINDCHIME,
    S_CREATE_GATE_CRYSTAL,
    S_CREATE_STAFF_OF_HEALING,
    S_CREATE_SCRYING_ORB,
    S_CREATE_CORNUCOPIA,
    S_CREATE_BOOK_OF_EXORCISM,
    S_CREATE_HOLY_SYMBOL,
    S_CREATE_CENSER,
    S_TRANSMUTATION,
    S_BLASPHEMOUS_RITUAL,
    S_ENDURANCE,
    NSKILLS
};

#endif
