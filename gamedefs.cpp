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
#include "gamedefs.h"

const DirectionStringArray DirectionStrs {
    "North",
    "Northeast",
    "Southeast",
    "South",
    "Southwest",
    "Northwest"
};

const DirectionStringArray DirectionAbrs {
    "N",
    "NE",
    "SE",
    "S",
    "SW",
    "NW"
};

const MonthStringArray MonthNames {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

const SeasonStringArray SeasonNames {
    "clear",
    "winter",
    "monsoon season",
    "blizzard"
};

const Directions::MoveDirection Directions::MoveDirection::MOVE_PAUSE(MoveDirection::_MOVE_PAUSE);
const Directions::MoveDirection Directions::MoveDirection::MOVE_IN(MoveDirection::_MOVE_IN);
const Directions::MoveDirection Directions::MoveDirection::MOVE_OUT(MoveDirection::_MOVE_OUT);
const Directions::MoveDirection Directions::MoveDirection::MOVE_ENTER(MoveDirection::_MOVE_ENTER);

