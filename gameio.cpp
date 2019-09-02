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
#include <iostream>
#include <fstream>

#include "gameio.h"
#include "gamedefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

randctx isaac_ctx;

#define ENDLINE '\n'
static std::string buf;

void cleartoendl();

void cleartoendl()
{
    char ch = ' ';
    while (!(std::cin.eof()) && (ch != ENDLINE))
    {
        ch = static_cast<char>(std::cin.get());
    }
}

void initIO()
{
    seedrandom( 1783 );
}

void doneIO()
{
}

void seedrandom(int num)
{
    ub4 i;
    isaac_ctx.randa = isaac_ctx.randb = isaac_ctx.randc = static_cast<ub4>(0);
    for (i=0; i<256; ++i)
    {
        isaac_ctx.randrsl[i]=static_cast<ub4>(num)+i;
    }
    randinit(&isaac_ctx, true);
}

void seedrandomrandom()
{
    seedrandom( static_cast<int>(time( 0 ) ));
}

int Agetint()
{
    int x;
    std::cin >> x;
    cleartoendl();
    return x;
}

void Awrite(const AString & s)
{
    std::cout << s << std::endl;
}

void Adot()
{
    std::cout << ".";
}

void message(char * c)
{
    std::cout << c << std::endl;
    morewait();
}

void morewait()
{
    std::cout << std::endl;
    getline(std::cin, buf, '\n');
    std::cout << std::endl;
}


AString getfilename(const AString & s)
{
    std::cout << s;
    return AGetString();
}

AString AGetString()
{
    getline(std::cin, buf, '\n');
    return AString(buf);
}

template<>
float getrandom(float range)
{
    return static_cast<float>(getrandom(range));
}
