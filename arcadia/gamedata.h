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

//
// The items
//
enum {
    I_LEADERS,
    I_VIKING,
    I_BARBARIAN,
    I_ESKIMO,
    I_NOMAD,
    I_TRIBESMAN,
    I_DARKMAN,
    I_WOODELF,
    I_SEAELF,
    I_HIGHELF, 
    I_TRIBALELF,        //10
    I_PLAINSMAN,
    I_ICEDWARF,
    I_HILLDWARF,
    I_UNDERDWARF,
    I_DESERTDWARF,
    I_MERMEN,
    I_ORC,
    I_GNOME,
    I_SILVER,
    I_GRAIN,
    I_LIVESTOCK,      //21
    I_IRON,
    I_WOOD,
    I_STONE,
    I_FUR,
    I_FISH,
    I_HERBS,
    I_HORSE,
    I_DOLPHIN,
    I_SWORD,
    I_CROSSBOW,
    I_LONGBOW,        //31
    I_CHAINARMOR,
    I_PLATEARMOR,
    I_WAGON,
    I_MITHRIL,
    I_IRONWOOD,
    I_WHORSE,
    I_FDOLPHIN,
    I_FLOATER,
    I_ROOTSTONE,
    I_YEW,
    I_MSWORD,
    I_MPLATE,
    I_PPLATE,
    I_DOUBLEBOW,
    I_PEARL,
    I_WINE,
    I_WOOL,
    I_ROSES,
    I_CAVIAR,
    I_SILK,
    I_PERFUME,
    I_CHOCOLATE,
    I_CASHMERE,
    I_JEWELRY,
    I_FIGURINES,
    I_DYES,
    I_TRUFFLES,
    I_VELVET,
    I_MINK,
    I_SPICES,
    I_VODKA,
    I_COTTON,
    I_IVORY,
    I_TAROTCARDS,
    I_LION,
    I_WOLF,
    I_GBEAR,
    I_CROCODILE,
    I_ANACONDA,
    I_SCORPION,
    I_PBEAR,
    I_RAT,
    I_SPIDER,
    I_LIZARD,
    I_TRENT,
    I_ROC,
    I_BTHING,
    I_KONG,
    I_SPHINX,
    I_IWURM,
    I_BABYDRAGON,
    I_DRAGON,
    I_GRYFFIN,
    I_CENTAUR,
    I_KOBOLD,
    I_OGRE,
    I_LMEN,
    I_WMEN,
    I_SANDLING,
    I_YETI,
    I_GOBLIN,
    I_TROLL,
    I_ETTIN,
    I_SKELETON,
    I_UNDEAD,
    I_LICH,
    I_IMP,
    I_DEMON,
    I_BALROG,
    I_EAGLE,
    I_AMULETOFI,
    I_RINGOFI,
    I_CLOAKOFI,
    I_STAFFOFF,
    I_STAFFOFL,
    I_STAFFOFY,
    I_AMULETOFTS,
    I_AMULETOFP,
    I_RUNESWORD,
    I_SHIELDSTONE,
    I_MCARPET,
    I_IRAT,
    I_IWOLF,
    I_IEAGLE,
    I_IGRYFFIN,
    I_IDRAGON,
    I_ISKELETON,
    I_IUNDEAD,
    I_ILICH,
    I_IIMP,
    I_IDEMON,
    I_IBALROG,  //121
    I_PORTAL,
    I_PEASANT,
    // LLS
    I_PICK,
    I_SPEAR,
    I_AXE,
    I_HAMMER,
    I_MCROSSBOW,
    I_MWAGON,
    I_GLIDER,
    I_NET,  //131
    I_LASSO,
    I_BAG,
    I_SPINNING,
    I_LEATHERARMOR,
    I_CLOTHARMOR,
    I_BOOTS,
    I_PIRATES,
    I_KRAKEN,
    I_MERFOLK,
    I_ELEMENTAL,  //141
    I_MAN,
    /* Additional items for Ceran */
    I_FAIRY,
    I_LIZARDMAN,
    I_URUK,
    I_GOBLINMAN,
    I_HOBBIT,
    I_GNOLL,
    I_DROWMAN,
    I_MERC,
    I_TITAN,    //151
    I_AMAZON,
    I_OGREMAN,
    I_HIGHLANDER,
    I_MINOTAUR,
    I_LANCE,
    I_MUSHROOM,
    I_RRAT,
    I_NOOGLE,
    I_MUTANT,
    // Items from Tzarg
    I_BAXE,    //161
    I_MBAXE,
    I_ADMANTIUM,
    I_ADSWORD,
    I_ADBAXE,
    I_IMARM,
    I_ADRING,
    I_ADPLATE,
    I_SUPERBOW,
    I_CAMEL,
    I_GREYELF,
    I_DROW,
    I_HYDRA,
    I_STORMGIANT,
    I_CLOUDGIANT,
    I_ILLYRTHID,
    I_SORCERERS,
    I_MAGICIANS,
    I_DARKMAGE,
    I_WARRIORS,
    I_ICEDRAGON,
    I_HEALPOTION,
    I_ROUGHGEM,
    I_GEMS,
    I_JAVELIN,
    I_PIKE,
    I_MWOLF,
    I_MSPIDER,
    I_MOLE,
    I_BPLATE,
    I_MCHAIN,
    I_FSWORD,
    I_QSTAFF,
    I_SABRE,
    I_MACE,
    I_MSTAR,
    I_DAGGER,
    I_PDAGGER,
    I_BHAMMER,
    I_BOW,
    I_SHORTBOW,    //201
    I_HEAVYCROSSBOW,
    I_HARP,
    // Generic processed food
    I_FOOD,
    NITEMS
};

//
// Types of skills.
//
enum {
    S_LEADERSHIP,
    S_HEROSHIP,
    S_MINING,          //0
    S_LUMBERJACK,
    S_QUARRYING,
    S_HUNTING,
    S_FISHING,
    S_HERBLORE,
    S_HORSETRAINING,
    S_DOLPHINTRAINING,
    S_WEAPONSMITH,
    S_ARMORER,
    S_CARPENTER,
    S_BUILDING,       //10
    S_SHIPBUILDING,
    S_ENTERTAINMENT,
    S_TACTICS,
    S_COMBAT,
    S_RIDING,
    S_CROSSBOW,
    S_LONGBOW,
    S_STEALTH,
    S_OBSERVATION,
    S_HEALING,        //20
    S_SAILING,
    S_FARMING,
    S_RANCHING,
    S_FORCE,
    S_PATTERN,
    S_SPIRIT,
    S_FIRE,
    S_EARTHQUAKE,
    S_FORCE_SHIELD,
    S_ENERGY_SHIELD,     //30
    S_SPIRIT_SHIELD,
    S_MAGICAL_HEALING,
    S_GATE_LORE,
    S_FARSIGHT,
    S_TELEPORTATION,
    S_PORTAL_LORE,
    S_MIND_READING,
    S_WEATHER_LORE,
    S_SUMMON_WIND,
    S_SUMMON_STORM,      //40
    S_SUMMON_TORNADO,
    S_CALL_LIGHTNING,
    S_CLEAR_SKIES,
    S_EARTH_LORE,
    S_WOLF_LORE,
    S_BIRD_LORE,
    S_DRAGON_LORE,
    S_NECROMANCY,
    S_SUMMON_SKELETONS,
    S_RAISE_UNDEAD,     //50
    S_SUMMON_LICH,
    S_CREATE_AURA_OF_FEAR,
    S_SUMMON_BLACK_WIND,
    S_BANISH_UNDEAD,
    S_DEMON_LORE,
    S_SUMMON_IMPS,
    S_SUMMON_DEMON,
    S_SUMMON_BALROG,
    S_BANISH_DEMONS,
    S_ILLUSION,        //60
    S_PHANTASMAL_ENTERTAINMENT,
    S_CREATE_PHANTASMAL_BEASTS,
    S_CREATE_PHANTASMAL_UNDEAD,
    S_CREATE_PHANTASMAL_DEMONS,
    S_INVISIBILITY,
    S_TRUE_SEEING,
    S_DISPEL_ILLUSIONS,
    S_ARTIFACT_LORE,
    S_CREATE_RING_OF_INVISIBILITY,
    S_CREATE_CLOAK_OF_INVULNERABILITY,         //70
    S_CREATE_STAFF_OF_FIRE,
    S_CREATE_STAFF_OF_LIGHTNING,
    S_CREATE_AMULET_OF_TRUE_SEEING,
    S_CREATE_AMULET_OF_PROTECTION,
    S_CREATE_RUNESWORD,
    S_CREATE_SHIELDSTONE,
    S_CREATE_MAGIC_CARPET,
    S_ENGRAVE_RUNES_OF_WARDING,
    S_CONSTRUCT_GATE,
    S_ENCHANT_SWORDS,           //80
    S_ENCHANT_ARMOR,
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
    S_COOKING,                  //90
    S_CREATE_FOOD,
    S_BANKING,
    S_QUARTERMASTER,
    S_BLIZZARD,
    S_FOG,                     //95
    S_CONCEALMENT,
    S_ILLUSORY_CREATURES,
    S_ILLUSORY_WOUNDS,
    S_INSTILL_COURAGE,
    S_SUMMON_MEN,
    S_RESURRECTION,            //101
    S_SPIRIT_OF_DEAD,
    S_INNER_STRENGTH,
    S_TRANSMUTATION,
    S_MODIFICATION,
    S_REJUVENATION,
    S_SEAWARD,
    S_DIVERSION,
    S_GRYFFIN_LORE,
    S_HYPNOSIS,
    S_BINDING,
    S_CREATE_PORTAL,
    S_LIGHT,
    S_DARKNESS,
    S_DRAGON_TALK,
    S_TOUGHNESS,
    S_UNITY,
    S_FRENZY,
    S_SECSIGHT,
    S_SWIFTNESS,
    S_TRADING,
    S_MERCHANTRY,
    S_BASE_WINDKEY,
    S_BASE_ILLUSION,
    S_BASE_SUMMONING,
    S_BASE_PATTERNING,
    S_BASE_MYSTICISM,
    S_BASE_BATTLETRAINING,
    S_BASE_CHARISMA,
    S_BASE_ARTIFACTLORE,
    S_ARCADIA_QUARTERMASTERY,
    S_CONSTRUCTION,
    NSKILLS
};

//
// Types of objects.
//
enum {
    O_DUMMY,
    O_CORACLE,
    O_LONGBOAT,
    O_JUNK,
    O_MERCHANT,
    O_TRIREME,
    O_ATRIREME,
    O_TREASUREARK,
    O_BARGE,
    O_BALLOON,
    O_TOWER,
    O_FORT,  
    O_CASTLE,
    O_CITADEL, 
    O_MFORTRESS,
    O_SHAFT,
    O_LAIR,
    O_RUIN,
    O_CAVE,
    O_DEMONPIT,
    O_CRYPT,
    O_MINE,
    O_FARM,
    O_RANCH,
    O_TIMBERYARD,
    O_INN,
    O_QUARRY,  
    // LLS
    // New ocean lairs
    O_ISLE,
    O_DERELICT,
    O_OCAVE,
    O_WHIRL,
    // AS
    O_TEMPLE,
    O_MQUARRY,
    O_AMINE,
    O_PRESERVE,
    O_SACGROVE,
    // JT
    // Abyss Lair
    O_BKEEP,
    O_PALACE,
    // For Ceran
    O_DCLIFFS,
    O_MTOWER,
    O_WGALLEON,
    O_HUT,
    O_STOCKADE,
    O_CPALACE,
    O_NGUILD,
    O_AGUILD,
    O_ATEMPLE,
    O_HTOWER,
    // Tzargs monster lairs
    O_MAGETOWER,
    O_DARKTOWER,
    O_GIANTCASTLE,
    O_ILAIR,
    O_ICECAVE,
    O_BOG,
    O_TRAPPINGHUT,
    O_STABLE,
    O_FISHTRAP,
    O_MSTABLE,
    O_TRAPPINGLODGE,
    O_FAERIERING,
    O_ALCHEMISTLAB,
    O_OASIS,
    O_GEMAPPRAISER,
    O_HPTOWER,
    O_OBANK,
    O_CARAVANSERAI,
    O_ESEAPORTAL,
    O_MESSAGESTONE,
    O_ROADN,
    O_ROADNW,
    O_ROADNE,
    O_ROADSW,
    O_ROADSE,
    O_ROADS,
/*    O_ROAD,
    O_RIVER,
    O_RAVINE,
    O_CLIFF,
    O_BEACH,
    O_ROCKS,
    O_HARBOUR,
    O_BRIDGE,
    O_IWALL,
    O_OWALL,*/
    NOBJECTS
};

//
// Types of hexside terrain.
//
enum {
    H_DUMMY,
    H_ROCKS,
    H_BEACH,
    H_HARBOUR,
    H_RIVER,
    H_RAVINE,
    H_CLIFF,
    H_ROAD,
    H_BRIDGE,
    NHEXSIDES
};

//
// Types of terrain
//
/* ARegion Types */
enum {
    R_OCEAN,
    R_PLAIN,
    R_FOREST,
    R_MOUNTAIN,
    R_SWAMP,
    R_JUNGLE,
    R_DESERT,
    R_TUNDRA,
    R_CAVERN,
    R_UFOREST,
    R_TUNNELS,
    R_GROTTO,
    R_DFOREST,
    R_CHASM,    
    R_NEXUS,
    R_PARADISE,
    R_ISLAND_PLAIN,
    R_ISLAND_SWAMP,
    R_ISLAND_MOUNTAIN,
    R_CERAN_PLAIN1,
    R_CERAN_PLAIN2,
    R_CERAN_PLAIN3,
    R_CERAN_FOREST1,
    R_CERAN_FOREST2,
    R_CERAN_FOREST3,
    R_CERAN_MYSTFOREST,
    R_CERAN_MYSTFOREST1,
    R_CERAN_MYSTFOREST2,
    R_CERAN_MOUNTAIN1,
    R_CERAN_MOUNTAIN2,
    R_CERAN_MOUNTAIN3,
    R_CERAN_HILL,
    R_CERAN_HILL1,
    R_CERAN_HILL2,
    R_CERAN_SWAMP1,
    R_CERAN_SWAMP2,
    R_CERAN_SWAMP3,
    R_CERAN_JUNGLE1,
    R_CERAN_JUNGLE2,
    R_CERAN_JUNGLE3,
    R_CERAN_DESERT1,
    R_CERAN_DESERT2,
    R_CERAN_DESERT3,
    R_CERAN_WASTELAND,
    R_CERAN_WASTELAND1,
    R_CERAN_LAKE,
    R_CERAN_TUNDRA1,
    R_CERAN_TUNDRA2,
    R_CERAN_TUNDRA3,
    R_CERAN_CAVERN1,
    R_CERAN_CAVERN2,
    R_CERAN_CAVERN3,
    R_CERAN_UFOREST1,
    R_CERAN_UFOREST2,
    R_CERAN_UFOREST3,
    R_CERAN_TUNNELS1,
    R_CERAN_TUNNELS2,
    R_CERAN_GROTTO1,
    R_CERAN_DFOREST1,
    R_CERAN_CHASM1,
    R_VOLCANO,
    R_LAKE,
    R_FAKE,
    R_NUM
};

#endif
