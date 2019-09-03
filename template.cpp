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

#include <cstdio>
#ifdef WIN32
#include <memory.h>
#endif
#include <cstring>

#include "game.h"
#include "gamedata.h"

#define LINE_WIDTH 70
#define MAP_WIDTH 23
#define TMPL_MAP_WIDTH 20
#define TMPL_MAP_OFS 1
#define FILL_SIZE 6
#define TEMPLATE_MAX_LINES 13

static void TrimWrite(Areport *f, char *buffer);

static char const *TemplateMap[] = {
 //12345678901234567890
  "        ____        ",   // 1
  "nw     /    \\     ne",  // 2
  "  ____/      \\____  ",  // 3
  " /    \\      /    \\ ", // 4
  "/      \\____/      \\", // 5
  "\\      /    \\      /", // 6
  " \\____/      \\____/ ", // 7
  " /    \\      /    \\ ", // 8
  "/      \\____/      \\", // 9
  "\\      /    \\      /", // 10
  " \\____/      \\____/ ", // 11
  "      \\      /      ",  // 12
  "sw     \\____/     se"   // 13
};

static unsigned int dircrd[] = {
 // X    Y
   8-1,  7-1, // center
   8-1,  3-1, // N
  14-1,  5-1, // NE
  14-1,  9-1, // SE
   8-1, 11-1, // S
   2-1,  9-1, // SW
   2-1,  5-1  // NW
};


static char const *ter_fill[] = {
 // block
 " #### ",
 " #### ",

 // ocean
 "  ~ ~ ",
 " ~ ~  ",

 // plain
 "      ",
 "      ",

 // forest
 "  ^ ^ ",
 " ^ ^  ",

 // mountain
 " /\\/\\ ",
 "/  \\ \\",

 // swamp
 "  v v ",
 " v v  ",

 // jungle
 "  @ @ ",
 " @ @  ",

 // desert
 "  . . ",
 " . .  ",

 // tundra
 "  ' ' ",
 " ' '  ",

 // cavern
 "  . . ",
 " . .  ",

 // underforest
 "  ^ ^ ",
 " ^ ^  ",

 // tunnels
 "      ",
 "      ",

 // nexus
 " !!!! ",
 " !!!! ",

 // For conquest
 // Island Plain
 "      ",
 "      ",

 // Island swamp
 "  v v ",
 " v v  ",

 // Island mountain
 " /\\/\\ ",
 "/  \\ \\",

 // For Ceran terrains

 // plain1
 "      ",
 "      ",
 // plain2
 "      ",
 "      ",
 // plain3
 "      ",
 "      ",

 // forest1
 "  ^ ^ ",
 " ^ ^  ",
 // forest2
 "  ^ ^ ",
 " ^ ^  ",
 // forest3
 "  ^ ^ ",
 " ^ ^  ",

 // mystforest
 "  ` ` ",
 " ` `  ",
 // mystforest1
 "  ` ` ",
 " ` `  ",
 // mystforest2
 "  ` ` ",
 " ` `  ",

 // mountain1
 " /\\/\\ ",
 "/  \\ \\",
 // mountain2
 " /\\/\\ ",
 "/  \\ \\",
 // mountain3
 " /\\/\\ ",
 "/  \\ \\",

 // hill
 "  * * ",
 " * *  ",
 // hill1
 "  * * ",
 " * *  ",
 // hill2
 "  * * ",
 " * *  ",

 // swamp1
 "  v v ",
 " v v  ",
 // swamp2
 "  v v ",
 " v v  ",
 // swamp3
 "  v v ",
 " v v  ",

 // jungle1
 "  @ @ ",
 " @ @  ",
 // jungle2
 "  @ @ ",
 " @ @  ",
 // jungle3
 "  @ @ ",
 " @ @  ",

 // desert1
 "  . . ",
 " . .  ",
 // desert2
 "  . . ",
 " . .  ",
 // desert3
 "  . . ",
 " . .  ",

 // wasteland
 "  ; ; ",
 " ; ;  ",
 // wasteland1
 "  ; ; ",
 " ; ;  ",

 // lake
 "  ~ ~ ",
 " ~ ~  ",

 // tundra1
 "  ' ' ",
 " ' '  ",
 // tundra2
 "  ' ' ",
 " ' '  ",
 // tundra2
 "  ' ' ",
 " ' '  ",

 // cavern1
 "  . . ",
 " . .  ",
 // cavern2
 "  . . ",
 " . .  ",
 // cavern3
 "  . . ",
 " . .  ",

 // underforest1
 "  ^ ^ ",
 " ^ ^  ",
 // underforest2
 "  ^ ^ ",
 " ^ ^  ",
 // underforest3
 "  ^ ^ ",
 " ^ ^  ",

 // tunnels1
 "      ",
 "      ",
 // tunnels2
 "      ",
 "      ",

 // grotto
 "  . . ",
 " . .  ",
 // deepforest
 "  ^ ^ ",
 " ^ ^  ",
 // chasm
 "      ",
 "      ",
 // grotto1
 "  . . ",
 " . .  ",
 // deepforest1
 "  ^ ^ ",
 " ^ ^  ",
 // chasm1
 "      ",
 "      ",

 // volcano
 " /\\/\\ ",
 "/  \\ \\",

 // lake
 "  ~ ~ ",
 " ~ ~  ",

};

// NEW FUNCTION DK 2000.03.07,
// converted WriteReport
//
void ARegion::WriteTemplateHeader(Areport *f,
                                  const Faction& fac,
                                  const ARegionList& pRegs,
                                  const ValidValue<size_t>& month)
{

    f->PutStr("");

    f->PutStr("-------------------------------------------------"
            "----------", 1);

    // plain (X,Y) in Blah, contains Blah
    f->PutStr(Print(pRegs), 1);

    char buffer[LINE_WIDTH+1];
    char *data = buffer + MAP_WIDTH;
    unsigned int line = 0;

    // ----------------------------------------------------------------
    GetMapLine(buffer, line, pRegs);
    TrimWrite(f, buffer);
    line++;

    // ----------------------------------------------------------------

    if (Globals->WEATHER_EXISTS) {
        GetMapLine(buffer, line, pRegs);

        char const * nextWeather = "";
        size_t next_month;
        if(month.isValid())
        {
            next_month = month + 1;
        }
        else
        {
            next_month = 0;
        }

        Weather nxtweather = pRegs.GetWeather(*this, next_month % 12);
        if (nxtweather == Weather::Types::W_WINTER)
        {
            nextWeather = "winter";
        }
        else if (nxtweather == Weather::Types::W_MONSOON)
        {
            nextWeather = "monsoon";
        }
        else if (nxtweather == Weather::Types::W_NORMAL)
        {
            nextWeather = "clear";
        }
        sprintf(data, "Next %s", nextWeather);

        TrimWrite(f, buffer);
        line++;
    }

    // ----------------------------------------------------------------
    GetMapLine(buffer, line, pRegs);
    sprintf(data, "Tax  %5i", wealth);
    TrimWrite(f, buffer);
    line++;

    // ----------------------------------------------------------------
    auto prod_w = products.GetProd(Items::Types::I_SILVER, Skills::Types::S_ENTERTAINMENT);
    if (!prod_w.expired()) {
        const auto prod = prod_w.lock();
        GetMapLine(buffer, line, pRegs);
        sprintf(data, "Ente %5i", prod->amount);
        TrimWrite(f, buffer);
        line++;
    }

    // ----------------------------------------------------------------
    prod_w = products.GetProd(Items::Types::I_SILVER, -1);
    if (!prod_w.expired()) {
        const auto prod = prod_w.lock();
        GetMapLine(buffer, line, pRegs);
        sprintf(data, "Wage %5i.%1i (max %i)", (prod->productivity/10), (prod->productivity%10), prod->amount);
        TrimWrite(f, buffer);
        line++;
    }

    // ----------------------------------------------------------------
    bool any = false;
    for(const auto& m: markets) {
        if (!m->amount)
        {
            continue;
        }
        if (m->type == M_SELL)
        {
            if (ItemDefs[m->item].type & IT_ADVANCED) {
                if (!HasItem(fac, m->item)) {
                    continue;
                }
            }

            if (!any) {
                GetMapLine(buffer, line, pRegs);
                TrimWrite(f, buffer);
                line++;
            }

            GetMapLine(buffer, line, pRegs);

            if (m->amount == -1) {
                sprintf(data, "%s unlim %4s @ %3i",
                    (any ? "    " : "Want"),
                    ItemDefs[m->item].abr,
                    m->price);
            } else {
                sprintf(data, "%s %5i %4s @ %3i",
                    (any ? "    " : "Want"),
                    m->amount,
                    ItemDefs[m->item].abr,
                    m->price);
            }
            TrimWrite(f, buffer);
            line++;

            any = true;
        }
    }

    // ----------------------------------------------------------------
    any = false;
    for(const auto& m: markets) {
        if (!m->amount)
        {
            continue;
        }
        if (m->type == M_BUY)
        {
            if (!any) {
                GetMapLine(buffer, line, pRegs);
                TrimWrite(f, buffer);
                line++;
            }

            GetMapLine(buffer, line, pRegs);

            if (m->amount == -1) {
                sprintf(data, "%s unlim %4s @ %3i",
                    (any ? "    " : "Sell"),
                    ItemDefs[m->item].abr,
                    m->price);
            } else {
                sprintf(data, "%s %5i %4s @ %3i",
                    (any ? "    " : "Sell"),
                    m->amount,
                    ItemDefs[m->item].abr,
                    m->price);
            }

            TrimWrite(f, buffer);
            line++;
            any = true;
        }
    }

    // ----------------------------------------------------------------
    any = false;
    for(const auto& p: products) {
        if (ItemDefs[p->itemtype].type & IT_ADVANCED) {
            if (!CanMakeAdv(fac, p->itemtype)) {
                continue;
            }
        } else {
            if (p->itemtype == Items::Types::I_SILVER) {
                continue;
            }
        }

        if (!any) {
            GetMapLine(buffer, line, pRegs);
            TrimWrite(f, buffer);
            line++;
        }

        GetMapLine(buffer, line, pRegs);

        if (p->amount == -1) {
            sprintf(data, "%s unlim %4s",
                (any ? "    " : "Prod"),
                ItemDefs[p->itemtype].abr);
        } else {
            sprintf(data, "%s %5i %4s",
                (any ? "    " : "Prod"),
                p->amount,
                ItemDefs[p->itemtype].abr);
        }

        TrimWrite(f, buffer);
        line++;
        any = true;

    }

    // ----------------------------------------------------------------

    if (Globals->GATES_EXIST && gate) {
        for(const auto& o: objects) {
            for(const auto& u: o->units) {
                if (u->faction.lock().get() == &fac && u->GetSkill(Skills::Types::S_GATE_LORE)) {
                    GetMapLine(buffer, line, pRegs);
                    TrimWrite(f, buffer);
                    line++;

                    GetMapLine(buffer, line, pRegs);
                    sprintf(data, "Gate %4i", gate);
                    TrimWrite(f, buffer);
                    line++;

                    break;
                }
            }
        }
    }

    // ----------------------------------------------------------------
    while (line < TEMPLATE_MAX_LINES) {
        GetMapLine(buffer, line, pRegs);
        TrimWrite(f, buffer);
        line++;
    }
}


// NEW FUNCTION DK 2000.03.07,
// converted WriteExits
//
void ARegion::GetMapLine(std::string& buffer, unsigned int line, const ARegionList&)
{

    for (int m=0; m<MAP_WIDTH; m++) {
        buffer[m] = ' ';
    }
    buffer[MAP_WIDTH] = 0;

    if (line >= TEMPLATE_MAX_LINES)
    {
        return;
    }

    char *dest = buffer+TMPL_MAP_OFS;
    memcpy(dest, TemplateMap[line], TMPL_MAP_WIDTH);

    size_t t = (static_cast<size_t>(type) + 1) * 2;
    const char* name = (town ? town->name.Str() : nullptr);

    auto i = Directions::begin();
    for (;;) {

        unsigned int x = dircrd[*i*2];
        unsigned int y = dircrd[*i*2+1];

        if (y == line || y+1 == line) {
            if (y == line) {
                if (name) {
                    size_t len = strlen(name);
                    if (len > FILL_SIZE) len = FILL_SIZE;
                    memcpy(dest + x, name, len);
                } else {
                    memcpy(dest + x, ter_fill[t], FILL_SIZE);
                }
            } else {
                t++;
                memcpy(dest + x, ter_fill[t], FILL_SIZE);
            }
        }

        if (i == Directions::end())
        {
            break;
        }

        const auto& r_w = neighbors[*i];
        if (!r_w.expired()) {
            const auto r = r_w.lock();
            t = (r->type + 1) * 2;
            name = (r->town ? r->town->name.Str() : nullptr);
        } else {
            t = 0;
            name = NULL;
        }

        ++i;
    }
}

static void TrimWrite(Areport *f, char *buffer) {

    char *p = buffer + strlen(buffer) - 1;
    while (p > buffer) {
        if (*p == ' ') {
            p--;
        } else {
            break;
        }
    }
    p[1] = 0;

    f->PutStr(buffer, 1);
}
