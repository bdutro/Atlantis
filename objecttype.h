#ifndef OBJECT_TYPE_H
#define OBJECT_TYPE_H

#include "validenum.h"

//
// Types of objects.
//
enum class _ObjectTypes : size_t {
    O_DUMMY,
    O_FLEET,
    O_LONGBOAT,
    O_LONGSHIP,
    O_RAFT,
    O_COG,
    O_CLIPPER,
    O_GALLEON,
    O_AGALLEON,
    O_GALLEY,
    O_CORSAIR,
    O_BALLOON,
    O_AIRSHIP,
    O_CLOUDSHIP,
    O_TOWER,
    O_FORT,
    O_CASTLE,
    O_CITADEL,
    O_SHAFT,
    O_LAIR,
    O_RUIN,
    O_CAVE,
    O_DEMONPIT,
    O_CRYPT,
    O_MTOWER,
    O_MFORTRESS,
    O_MCASTLE,
    O_MCITADEL,
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
    O_ROADN,
    O_ROADNW,
    O_ROADNE,
    O_ROADSW,
    O_ROADSE,
    O_ROADS,
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
    O_MSTABLE,
    O_TRAPPINGLODGE,
    O_FAERIERING,
    O_ALCHEMISTLAB,
    O_OASIS,
    O_GEMAPPRAISER,
    O_HPTOWER,
    O_CARAVANSERAI,
    O_GATEWAY,
    NOBJECTS
};

using Objects = ValidEnum<_ObjectTypes, _ObjectTypes::NOBJECTS>;

#endif
