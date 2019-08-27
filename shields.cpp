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
#include "shields.h"

ShieldList::const_iterator ShieldList::GetHighShield(int type) const
{
    const_iterator hi = shields_.end();
    for(const_iterator it = shields_.begin(); it != shields_.end(); ++it) {
        if (it->shieldtype == type) {
            if (hi == shields_.end()) {
                hi = it;
            } else {
                if (it->shieldskill > hi->shieldskill)
                {
                    hi = it;
                }
            }
        }
    }
    return hi;
}
