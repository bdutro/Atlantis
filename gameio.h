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
#ifndef GAME_IO
#define GAME_IO

#include <type_traits>
#include "astring.h"
#include "i_rand.h"

void initIO();
void doneIO();

extern randctx isaac_ctx;

/* Get a random number from 0 to (int-1) */
template<typename I>
I getrandom(I range)
{
    const bool neg = std::is_signed<I>::value && (range < 0);
    if (!range) return I(0);
    I ret = I(0);
    if (neg) range = -range;
    unsigned long i = isaac_rand( &isaac_ctx );
    i = i % static_cast<unsigned long>(range);
    if (neg) ret = static_cast<I>(-i);
    else ret = static_cast<I>(i);
    return ret;
}

template<>
float getrandom(float range)
{
    return static_cast<float>(getrandom(range));
}

/* Seed the random number generator */
void seedrandom(int);
void seedrandomrandom();

int Agetint();

void Awrite(const AString &);

void Adot();

void message(char *);

void morewait();

AString * getfilename(const AString &);
AString *AGetString();

#endif

