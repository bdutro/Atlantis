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
#include "game.h"
#include "gamedata.h"

static int RandomiseSummonAmount(int num)
{
    int retval, i;

    retval = 0;

    for (i = 0; i < 2 * num; i++)
    {
        if (getrandom(2))
            retval++;
    }
    if (retval < 1 && num > 0)
        retval = 1;

    return retval;
}

void Game::ProcessCastOrder(const Unit::Handle& u,
                            AString * o,
                            const OrdersCheck::Handle& pCheck )
{
    AString * token = o->gettoken();
    if (!token) {
        ParseError( pCheck, u, 0, "CAST: No skill given.");
        return;
    }

    Skills sk = ParseSkill(token);
    delete token;
    if (!sk.isValid()) {
        ParseError( pCheck, u, 0, "CAST: Invalid skill.");
        return;
    }

    if ( !( SkillDefs[sk].flags & SkillType::MAGIC )) {
        ParseError( pCheck, u, 0, "CAST: That is not a magic skill.");
        return;
    }
    if ( !( SkillDefs[sk].flags & SkillType::CAST )) {
        ParseError( pCheck, u, 0, "CAST: That skill cannot be CAST.");
        return;
    }

    if ( !pCheck ) {
        //
        // XXX -- should be error checking spells
        //
        switch(sk.asEnum()) {
            case Skills::Types::S_MIND_READING:
                ProcessMindReading(u,o, pCheck );
                break;
            case Skills::Types::S_CONSTRUCT_PORTAL:
            case Skills::Types::S_ENCHANT_SWORDS:
            case Skills::Types::S_ENCHANT_ARMOR:
            case Skills::Types::S_ENCHANT_SHIELDS:
            case Skills::Types::S_CONSTRUCT_GATE:
            case Skills::Types::S_ENGRAVE_RUNES_OF_WARDING:
            case Skills::Types::S_SUMMON_IMPS:
            case Skills::Types::S_SUMMON_DEMON:
            case Skills::Types::S_SUMMON_BALROG:
            case Skills::Types::S_SUMMON_SKELETONS:
            case Skills::Types::S_RAISE_UNDEAD:
            case Skills::Types::S_SUMMON_LICH:
            case Skills::Types::S_DRAGON_LORE:
            case Skills::Types::S_WOLF_LORE:
            case Skills::Types::S_EARTH_LORE:
            case Skills::Types::S_SUMMON_WIND:
            case Skills::Types::S_CREATE_RING_OF_INVISIBILITY:
            case Skills::Types::S_CREATE_CLOAK_OF_INVULNERABILITY:
            case Skills::Types::S_CREATE_STAFF_OF_FIRE:
            case Skills::Types::S_CREATE_STAFF_OF_LIGHTNING:
            case Skills::Types::S_CREATE_AMULET_OF_TRUE_SEEING:
            case Skills::Types::S_CREATE_AMULET_OF_PROTECTION:
            case Skills::Types::S_CREATE_RUNESWORD:
            case Skills::Types::S_CREATE_SHIELDSTONE:
            case Skills::Types::S_CREATE_MAGIC_CARPET:
            case Skills::Types::S_CREATE_FLAMING_SWORD:
            case Skills::Types::S_CREATE_FOOD:
            case Skills::Types::S_CREATE_AEGIS:
            case Skills::Types::S_CREATE_WINDCHIME:
            case Skills::Types::S_CREATE_GATE_CRYSTAL:
            case Skills::Types::S_CREATE_STAFF_OF_HEALING:
            case Skills::Types::S_CREATE_SCRYING_ORB:
            case Skills::Types::S_CREATE_CORNUCOPIA:
            case Skills::Types::S_CREATE_BOOK_OF_EXORCISM:
            case Skills::Types::S_CREATE_HOLY_SYMBOL:
            case Skills::Types::S_CREATE_CENSER:
            case Skills::Types::S_BLASPHEMOUS_RITUAL:
                ProcessGenericSpell(u,sk, pCheck );
                break;
            case Skills::Types::S_CLEAR_SKIES:
                try
                {
                    FindRange(SkillDefs[sk].range);
                    ProcessRegionSpell(u, o, sk, pCheck);
                }
                catch(const NoSuchItemException&)
                {
                    ProcessGenericSpell(u, sk, pCheck);
                }
                break;
            case Skills::Types::S_FARSIGHT:
            case Skills::Types::S_TELEPORTATION:
            case Skills::Types::S_WEATHER_LORE:
                ProcessRegionSpell(u, o, sk, pCheck);
                break;
            case Skills::Types::S_BIRD_LORE:
                ProcessBirdLore(u,o, pCheck );
                break;
            case Skills::Types::S_INVISIBILITY:
                ProcessInvisibility(u,o, pCheck );
                break;
            case Skills::Types::S_GATE_LORE:
                ProcessCastGateLore(u,o, pCheck );
                break;
            case Skills::Types::S_PORTAL_LORE:
                ProcessCastPortalLore(u,o, pCheck );
                break;
            case Skills::Types::S_CREATE_PHANTASMAL_BEASTS:
                ProcessPhanBeasts(u,o, pCheck );
                break;
            case Skills::Types::S_CREATE_PHANTASMAL_UNDEAD:
                ProcessPhanUndead(u,o, pCheck );
                break;
            case Skills::Types::S_CREATE_PHANTASMAL_DEMONS:
                ProcessPhanDemons(u,o, pCheck );
                break;
            case Skills::Types::S_TRANSMUTATION:
                ProcessTransmutation(u, o, pCheck);
                break;
            default:
                break;
        }
    }
}

void Game::ProcessMindReading(const Unit::Handle& u, AString *o, const OrdersCheck::Handle&)
{
    const auto id = ParseUnit(o);

    if (!id) {
        u->Error("CAST: No unit specified.");
        return;
    }

    const CastMindOrder::Handle order = std::make_shared<CastMindOrder>();
    order->id = *id;
    order->spell = Skills::Types::S_MIND_READING;
    order->level = 1;

    u->ClearCastOrders();
    u->castorders = order;
}

void Game::ProcessBirdLore(const Unit::Handle& u, AString *o, const OrdersCheck::Handle&)
{
    AString *token = o->gettoken();

    if (!token) {
        u->Error("CAST: Missing arguments.");
        return;
    }

    if (*token == "eagle") {
        const CastIntOrder::Handle order = std::make_shared<CastIntOrder>();
        order->spell = Skills::Types::S_BIRD_LORE;
        order->level = 3;
        u->ClearCastOrders();
        u->castorders = order;
        return;
    }

    if (*token == "direction") {
        delete token;
        token = o->gettoken();

        if (!token) {
            u->Error("CAST: Missing arguments.");
            return;
        }

        const Directions dir = ParseDir(token);
        delete token;
        if (!dir.isRegularDirection()) {
            u->Error("CAST: Invalid direction.");
            return;
        }

        CastIntOrder::Handle order = std::make_shared<CastIntOrder>();
        order->spell = Skills::Types::S_BIRD_LORE;
        order->level = 1;
        order->target = dir;
        u->ClearCastOrders();
        u->castorders = order;

        return;
    }

    u->Error("CAST: Invalid arguments.");
    delete token;
}

void Game::ProcessInvisibility(const Unit::Handle& u, AString *o, const OrdersCheck::Handle&)
{
    AString *token = o->gettoken();

    if (!token || !(*token == "units")) {
        u->Error("CAST: Must specify units to render invisible.");
        return;
    }
    delete token;

    CastUnitsOrder::Handle order;
    if (u->castorders && u->castorders->type == Orders::Types::O_CAST &&
        u->castorders->spell == Skills::Types::S_INVISIBILITY &&
        u->castorders->level == 1) {
        order = std::dynamic_pointer_cast<CastUnitsOrder>(u->castorders);
    } else {
        order = std::make_shared<CastUnitsOrder>();
        order->spell = Skills::Types::S_INVISIBILITY;
        order->level = 1;
        u->ClearCastOrders();
        u->castorders = order;
    }

    auto id = ParseUnit(o);
    while (id) {
        order->units.push_back(*id);
        id = ParseUnit(o);
    }
}

void Game::ProcessPhanDemons(const Unit::Handle& u, AString *o, const OrdersCheck::Handle&)
{
    const CastIntOrder::Handle order = std::make_shared<CastIntOrder>();
    order->spell = Skills::Types::S_CREATE_PHANTASMAL_DEMONS;
    order->level = 0;
    order->target = 1;

    AString *token = o->gettoken();

    if (!token) {
        u->Error("CAST: Illusion to summon must be given.");
        return;
    }

    if (*token == "imp" || *token == "imps") {
        order->level = 1;
    }

    if (*token == "demon" || *token == "demons") {
        order->level = 3;
    }

    if (*token == "balrog" || *token == "balrogs") {
        order->level = 5;
    }

    delete token;

    if (!order->level) {
        u->Error("CAST: Can't summon that illusion.");
        return;
    }

    token = o->gettoken();

    if (!token) {
        order->target = 1;
    } else {
        order->target = token->value();
        delete token;
    }

    u->ClearCastOrders();
    u->castorders = order;
}

void Game::ProcessPhanUndead(const Unit::Handle& u, AString *o, const OrdersCheck::Handle&)
{
    const CastIntOrder::Handle order = std::make_shared<CastIntOrder>();
    order->spell = Skills::Types::S_CREATE_PHANTASMAL_UNDEAD;
    order->level = 0;
    order->target = 1;

    AString *token = o->gettoken();

    if (!token) {
        u->Error("CAST: Must specify which illusion to summon.");
        return;
    }

    if (*token == "skeleton" || *token == "skeletons") {
        order->level = 1;
    }

    if (*token == "undead") {
        order->level = 3;
    }

    if (*token == "lich" || *token == "liches") {
        order->level = 5;
    }

    delete token;

    if (!order->level) {
        u->Error("CAST: Must specify which illusion to summon.");
        return;
    }

    token = o->gettoken();

    if (token) {
        order->target = token->value();
        delete token;
    } else {
        order->target = 1;
    }

    u->ClearCastOrders();
    u->castorders = order;
}

void Game::ProcessPhanBeasts(const Unit::Handle& u, AString *o, const OrdersCheck::Handle&)
{
    const CastIntOrder::Handle order = std::make_shared<CastIntOrder>();
    order->spell = Skills::Types::S_CREATE_PHANTASMAL_BEASTS;
    order->level = 0;
    order->target = 1;

    AString *token = o->gettoken();

    if (!token) {
        u->Error("CAST: Must specify which illusion to summon.");
        return;
    }

    if (*token == "wolf" || *token == "wolves") {
        order->level = 1;
    }
    if (*token == "eagle" || *token == "eagles") {
        order->level = 3;
    }
    if (*token == "dragon" || *token == "dragon") {
        order->level = 5;
    }

    delete token;
    if (!order->level) {
        u->Error("CAST: Must specify which illusion to summon.");
        return;
    }

    token = o->gettoken();
    if (token) {
        order->target = token->value();
        delete token;
    }

    u->ClearCastOrders();
    u->castorders = order;
}

void Game::ProcessGenericSpell(const Unit::Handle& u,  const Skills& spell, const OrdersCheck::Handle&)
{
    const CastOrder::Handle orders = std::make_shared<CastOrder>();
    orders->spell = spell;
    orders->level = 1;
    u->ClearCastOrders();
    u->castorders = orders;
}

void Game::ProcessRegionSpell(const Unit::Handle& u,  AString *o, const Skills& spell, const OrdersCheck::Handle&)
{
    AString *token = o->gettoken();
    ValidValue<unsigned int> x;
    ValidValue<unsigned int> y;
    ValidValue<unsigned int> z;

    if (token) {
        if (*token == "region") {
            delete token;
            token = o->gettoken();
            if (!token) {
                u->Error("CAST: Region X coordinate not specified.");
                return;
            }
            const int x_temp = token->value();
            if(x_temp != -1)
            {
                x = static_cast<unsigned int>(x_temp);
            }
            delete token;

            token = o->gettoken();
            if (!token) {
                u->Error("CAST: Region Y coordinate not specified.");
                return;
            }
            const int y_temp = token->value();
            if(y_temp != -1)
            {
                y = static_cast<unsigned int>(y_temp);
            }
            delete token;

            try
            {
                const RangeType& range = FindRange(SkillDefs[spell].range);
                if (range.flags & RangeType::RNG_CROSS_LEVELS) {
                    token = o->gettoken();
                    if (token) {
                        const int z_temp = token->value();
                        if(z_temp != -1)
                        {
                            z = static_cast<unsigned int>(z_temp);
                        }
                        delete token;
                        if (!z.isValid() || (z >= Globals->UNDERWORLD_LEVELS +
                                    Globals->UNDERDEEP_LEVELS +
                                    Globals->ABYSS_LEVEL + 2)) {
                            u->Error("CAST: Invalid Z coordinate specified.");
                            return;
                        }
                    }
                }
            }
            catch(const NoSuchItemException&)
            {
            }
        } else {
            delete token;
        }
    }

    if(!x.isValid() || !y.isValid() || !z.isValid())
    {
        const auto u_reg = u->object.lock()->region.lock();
        if (!x.isValid())
        {
            x = u_reg->xloc;
        }
        if (!y.isValid())
        {
            y = u_reg->yloc;
        }
        if (!z.isValid())
        {
            z = u_reg->zloc;
        }
    }

    CastRegionOrder::Handle order;
    if (spell == Skills::Types::S_TELEPORTATION)
    {
        order = std::make_shared<TeleportOrder>();
    }
    else
    {
        order = std::make_shared<CastRegionOrder>();
    }
    order->spell = spell;
    order->level = 1;
    order->xloc = x;
    order->yloc = y;
    order->zloc = z;

    u->ClearCastOrders();
    /* Teleports happen late in the turn! */
    if (spell == Skills::Types::S_TELEPORTATION)
    {
        u->teleportorders = std::dynamic_pointer_cast<TeleportOrder>(order);
    }
    else
    {
        u->castorders = order;
    }
}

void Game::ProcessCastPortalLore(const Unit::Handle& u, AString *o, const OrdersCheck::Handle&)
{
    AString *token = o->gettoken();
    if (!token) {
        u->Error("CAST: Requires a target mage.");
        return;
    }
    int gate = token->value();
    delete token;
    token = o->gettoken();

    if (!token) {
        u->Error("CAST: No units to teleport.");
        return;
    }

    if (!(*token == "units")) {
        u->Error("CAST: No units to teleport.");
        delete token;
        return;
    }

    TeleportOrder::Handle order;

    if (u->teleportorders && u->teleportorders->spell == Skills::Types::S_PORTAL_LORE) {
        order = u->teleportorders;
    } else {
        order = std::make_shared<TeleportOrder>();
        u->ClearCastOrders();
        u->teleportorders = order;
    }

    order->gate = gate;
    order->spell = Skills::Types::S_PORTAL_LORE;
    order->level = 1;

    UnitId::Handle id = ParseUnit(o);
    while(id) {
        order->units.push_back(*id);
        id = ParseUnit(o);
    }
}

void Game::ProcessCastGateLore(const Unit::Handle& u, AString *o, const OrdersCheck::Handle&)
{
    AString *token = o->gettoken();

    if (!token) {
        u->Error("CAST: Missing argument.");
        return;
    }

    if ((*token) == "gate") {
        delete token;
        token = o->gettoken();

        if (!token || token->value() < 1) {
            u->Error("CAST: Requires a target gate.");
            return;
        }

        TeleportOrder::Handle order;

        if (u->teleportorders && u->teleportorders->spell == Skills::Types::S_GATE_LORE &&
                u->teleportorders->gate == token->value()) {
            order = u->teleportorders;
        } else {
            order = std::make_shared<TeleportOrder>();
            u->ClearCastOrders();
            u->teleportorders = order;
        }

        order->gate = token->value();
        order->spell = Skills::Types::S_GATE_LORE;
        order->level = 3;

        delete token;

        token = o->gettoken();

        if (!token) return;
        if (!(*token == "units")) {
            delete token;
            return;
        }

        UnitId::Handle id = ParseUnit(o);
        while(id) {
            order->units.push_back(*id);
            id = ParseUnit(o);
        }
        return;
    }

    if ((*token) == "random") {
        TeleportOrder::Handle order;

        if (u->teleportorders &&
                u->teleportorders->spell == Skills::Types::S_GATE_LORE &&
                u->teleportorders->gate == -1 ) {
            order = u->teleportorders;
        } else {
            order = std::make_shared<TeleportOrder>();
            u->ClearCastOrders();
            u->teleportorders = order;
        }

        order->gate = -1;
        order->spell = Skills::Types::S_GATE_LORE;
        order->level = 1;

        delete token;

        token = o->gettoken();

        if (!token) return;
        if (*token == "level") {
            order->gate = -2;
            order->level = 2;
            delete token;
            token = o->gettoken();
        }
        if (!token) return;
        if (!(*token == "units")) {
            delete token;
            return;
        }

        UnitId::Handle id = ParseUnit(o);
        while(id) {
            order->units.push_back(*id);
            id = ParseUnit(o);
        }
        return;
    }

    if ((*token) == "detect") {
        delete token;
        u->ClearCastOrders();
        CastOrder::Handle to = std::make_shared<CastOrder>();
        to->spell = Skills::Types::S_GATE_LORE;
        to->level = 2;
        u->castorders = to;
        return;
    }

    delete token;
    u->Error("CAST: Invalid argument.");
}

void Game::ProcessTransmutation(const Unit::Handle& u,  AString *o, const OrdersCheck::Handle&)
{
    AString *token;

    CastTransmuteOrder::Handle order = std::make_shared<CastTransmuteOrder>();
    order->spell = Skills::Types::S_TRANSMUTATION;
    order->level = 0;
    order->item.invalidate();
    order->number = -1;

    token = o->gettoken();
    if (!token) {
        u->Error("CAST: You must specify what you wish to create.");
        return;
    }
    if (token->value() > 0) {
        order->number = token->value();
        delete token;
        token = o->gettoken();
    }

    order->item = ParseEnabledItem(token);
    delete token;
    if (!order->item.isValid()) {
        u->Error("CAST: You must specify what you wish to create.");
        return;
    }

    switch(order->item.asEnum()) {
        case Items::Types::I_MITHRIL:
        case Items::Types::I_ROOTSTONE:
            order->level = 1;
            break;
        case Items::Types::I_IRONWOOD:
            order->level = 2;
            break;
        case Items::Types::I_FLOATER:
            order->level = 3;
            break;
        case Items::Types::I_YEW:
            order->level = 4;
            break;
        case Items::Types::I_WHORSE:
            order->level = 5;
            break;
        default:
            u->Error("CAST: Can't create that by transmutation.");
            return;
    }

    u->ClearCastOrders();
    u->castorders = order;

    return;
}

void Game::RunACastOrder(const ARegion::Handle& r, const Object::Handle& o,const Unit::Handle& u)
{
    int val = 0;
    if (u->type != U_MAGE && u->type != U_APPRENTICE) {
        u->Error("CAST: Unit is not a mage.");
        return;
    }

    if (u->castorders->level == 0) {
        u->castorders->level = u->GetSkill(u->castorders->spell);
    }

    if (u->GetSkill(u->castorders->spell) < u->castorders->level ||
            u->castorders->level == 0) {
        u->Error("CAST: Skill level isn't that high.");
        return;
    }

    const Skills sk = u->castorders->spell;
    switch (sk.asEnum()) {
        case Skills::Types::S_MIND_READING:
            val = RunMindReading(r,u);
            break;
        case Skills::Types::S_ENCHANT_ARMOR:
            val = RunEnchant(r, u, sk, Items::Types::I_MPLATE);
            break;
        case Skills::Types::S_ENCHANT_SWORDS:
            val = RunEnchant(r, u, sk, Items::Types::I_MSWORD);
            break;
        case Skills::Types::S_ENCHANT_SHIELDS:
            val = RunEnchant(r, u, sk, Items::Types::I_MSHIELD);
            break;
        case Skills::Types::S_CONSTRUCT_GATE:
            val = RunConstructGate(r,u,sk);
            break;
        case Skills::Types::S_ENGRAVE_RUNES_OF_WARDING:
            val = RunEngraveRunes(r,o,u);
            break;
        case Skills::Types::S_CONSTRUCT_PORTAL:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_PORTAL);
            break;
        case Skills::Types::S_CREATE_RING_OF_INVISIBILITY:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_RINGOFI);
            break;
        case Skills::Types::S_CREATE_CLOAK_OF_INVULNERABILITY:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_CLOAKOFI);
            break;
        case Skills::Types::S_CREATE_STAFF_OF_FIRE:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_STAFFOFF);
            break;
        case Skills::Types::S_CREATE_STAFF_OF_LIGHTNING:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_STAFFOFL);
            break;
        case Skills::Types::S_CREATE_AMULET_OF_TRUE_SEEING:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_AMULETOFTS);
            break;
        case Skills::Types::S_CREATE_AMULET_OF_PROTECTION:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_AMULETOFP);
            break;
        case Skills::Types::S_CREATE_RUNESWORD:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_RUNESWORD);
            break;
        case Skills::Types::S_CREATE_SHIELDSTONE:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_SHIELDSTONE);
            break;
        case Skills::Types::S_CREATE_MAGIC_CARPET:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_MCARPET);
            break;
        case Skills::Types::S_CREATE_FLAMING_SWORD:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_FSWORD);
            break;
        case Skills::Types::S_SUMMON_IMPS:
            val = RunSummonImps(r,u);
            break;
        case Skills::Types::S_SUMMON_DEMON:
            val = RunSummonDemon(r,u);
            break;
        case Skills::Types::S_SUMMON_BALROG:
            val = RunSummonBalrog(r,u);
            break;
        case Skills::Types::S_SUMMON_LICH:
            val = RunSummonLich(r,u);
            break;
        case Skills::Types::S_RAISE_UNDEAD:
            val = RunRaiseUndead(r,u);
            break;
        case Skills::Types::S_SUMMON_SKELETONS:
            val = RunSummonSkeletons(r,u);
            break;
        case Skills::Types::S_DRAGON_LORE:
            val = RunDragonLore(r,u);
            break;
        case Skills::Types::S_BIRD_LORE:
            val = RunBirdLore(r,u);
            break;
        case Skills::Types::S_WOLF_LORE:
            val = RunWolfLore(r,u);
            break;
        case Skills::Types::S_INVISIBILITY:
            val = RunInvisibility(r,u);
            break;
        case Skills::Types::S_CREATE_PHANTASMAL_DEMONS:
            val = RunPhanDemons(r,u);
            break;
        case Skills::Types::S_CREATE_PHANTASMAL_UNDEAD:
            val = RunPhanUndead(r,u);
            break;
        case Skills::Types::S_CREATE_PHANTASMAL_BEASTS:
            val = RunPhanBeasts(r,u);
            break;
        case Skills::Types::S_GATE_LORE:
            val = RunDetectGates(r,o,u);
            break;
        case Skills::Types::S_FARSIGHT:
            val = RunFarsight(r,u);
            break;
        case Skills::Types::S_EARTH_LORE:
            val = RunEarthLore(r,u);
            break;
        case Skills::Types::S_WEATHER_LORE:
            val = RunWeatherLore(r, u);
            break;
        case Skills::Types::S_CLEAR_SKIES:
            val = RunClearSkies(r,u);
            break;
        case Skills::Types::S_SUMMON_WIND:
            val = RunCreateArtifact(r, u, sk, Items::Types::I_CLOUDSHIP);
            break;
        case Skills::Types::S_CREATE_FOOD:
            val = RunCreateArtifact(r, u, sk, Items::Types::I_FOOD);
            break;
        case Skills::Types::S_CREATE_AEGIS:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_AEGIS);
            break;
        case Skills::Types::S_CREATE_WINDCHIME:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_WINDCHIME);
            break;
        case Skills::Types::S_CREATE_GATE_CRYSTAL:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_GATE_CRYSTAL);
            break;
        case Skills::Types::S_CREATE_STAFF_OF_HEALING:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_STAFFOFH);
            break;
        case Skills::Types::S_CREATE_SCRYING_ORB:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_SCRYINGORB);
            break;
        case Skills::Types::S_CREATE_CORNUCOPIA:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_CORNUCOPIA);
            break;
        case Skills::Types::S_CREATE_BOOK_OF_EXORCISM:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_BOOKOFEXORCISM);
            break;
        case Skills::Types::S_CREATE_HOLY_SYMBOL:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_HOLYSYMBOL);
            break;
        case Skills::Types::S_CREATE_CENSER:
            val = RunCreateArtifact(r,u,sk,Items::Types::I_CENSER);
            break;
        case Skills::Types::S_TRANSMUTATION:
            val = RunTransmutation(r, u);
            break;
        case Skills::Types::S_BLASPHEMOUS_RITUAL:
            val = RunBlasphemousRitual(r, u);
            break;
        default:
            val = 0;
            break;
    }
    if (val) {
        u->Practice(sk);
        r->NotifySpell(u, SkillDefs[sk].abbr, regions);
    }
}

bool Game::GetRegionInRange(const ARegion::Handle& r, const ARegion::Handle& tar, const Unit::Handle& u,  const Skills& spell)
{
    const unsigned int level = u->GetSkill(spell);
    if (!level) {
        u->Error("CAST: You don't know that spell.");
        return false;
    }

    try
    {
        const RangeType& range = FindRange(SkillDefs[spell].range);

        int rtype = regions.GetRegionArray(r->zloc)->levelType;
        if ((rtype == ARegionArray::LEVEL_NEXUS) &&
                !(range.flags & RangeType::RNG_NEXUS_SOURCE)) {
            u->Error("CAST: Spell does not work from the Nexus.");
            return false;
        }

        if (!tar) {
            u->Error("CAST: No such region.");
            return false;
        }

        rtype = regions.GetRegionArray(tar->zloc)->levelType;
        if ((rtype == ARegionArray::LEVEL_NEXUS) &&
                !(range.flags & RangeType::RNG_NEXUS_TARGET)) {
            u->Error("CAST: Spell does not work to the Nexus.");
            return false;
        }

        if ((rtype != ARegionArray::LEVEL_SURFACE) &&
                (range.flags & RangeType::RNG_SURFACE_ONLY)) {
            u->Error("CAST: Spell can only target regions on the surface.");
            return false;
        }
        if (!(range.flags&RangeType::RNG_CROSS_LEVELS) && (r->zloc != tar->zloc)) {
            u->Error("CAST: Spell is not able to work across levels.");
            return false;
        }

        unsigned int maxdist;
        switch(range.rangeClass) {
            default:
            case RangeType::RNG_ABSOLUTE:
                maxdist = 1;
                break;
            case RangeType::RNG_LEVEL:
                maxdist = level;
                break;
            case RangeType::RNG_LEVEL2:
                maxdist = level * level;
                break;
            case RangeType::RNG_LEVEL3:
                maxdist = level * level * level;
                break;
        }
        maxdist *= range.rangeMult;

        unsigned int dist = regions.GetPlanarDistance(tar, r, range.crossLevelPenalty, maxdist);
        if (dist > maxdist) {
            u->Error("CAST: Target region out of range.");
            return false;
        }
        return true;
    }
    catch(const NoSuchItemException&)
    {
        u->Error("CAST: Spell is not castable at range.");
        return false;
    }
}

bool Game::RunMindReading(const ARegion::Handle& r, const Unit::Handle& u)
{
    const CastMindOrder::Handle order = std::dynamic_pointer_cast<CastMindOrder>(u->castorders);
    const unsigned int level = u->GetSkill(Skills::Types::S_MIND_READING);

    const Unit::WeakHandle tar_w = r->GetUnitId(order->id, u->faction.lock()->num);
    if (tar_w.expired()) {
        u->Error("No such unit.");
        return false;
    }

    const auto tar = tar_w.lock();
    AString temp = AString("Casts Mind Reading: ") + *(tar->name) + ", " +
        *(tar->faction.lock()->name);

    if (level < 4) {
        u->Event(temp + ".");
        return true;
    }

    temp += tar->items.Report(2,5,0) + ". Skills: ";
    temp += tar->skills.Report(tar->GetMen()) + ".";

    u->Event(temp);
    return true;
}

bool Game::RunEnchant(const ARegion::Handle&, const Unit::Handle& u, const Skills& skill, const Items& item)
{
    const unsigned int level = u->GetSkill(skill);
    const unsigned int max = static_cast<unsigned int>(ItemDefs[item].mOut) * level / 100;
    size_t num = max;

    // Figure out how many we can make based on available resources
    for (const auto& c: ItemDefs[item].mInput) {
        if (!c.item.isValid())
        {
            continue;
        }
        const auto& i = c.item;
        const unsigned int a = static_cast<unsigned int>(c.amt);
        if (u->GetSharedNum(i) < num * a) {
            num = u->GetSharedNum(i) / a;
        }
    }

    // collect all the materials
    for (const auto& c: ItemDefs[item].mInput) {
        if (!c.item.isValid())
        {
            continue;
        }
        const auto& i = c.item;
        const unsigned int a = static_cast<unsigned int>(c.amt);
        u->ConsumeShared(i, num * a);
    }

    // Add the created items
    u->items.SetNum(item, u->items.GetNum(item) + num);
    u->Event(AString("Enchants ") + num + " " + ItemDefs[item].names + ".");
    if (num == 0)
    {
        return false;
    }
    return true;
}

bool Game::RunConstructGate(const ARegion::Handle& r, const Unit::Handle& u, const Skills& spell)
{
    if (TerrainDefs[r->type].similar_type == Regions::Types::R_OCEAN) {
        u->Error("Gates may not be constructed at sea.");
        return false;
    }

    if (r->gate) {
        u->Error("There is already a gate in that region.");
        return false;
    }

    if (u->GetSharedMoney() < 1000) {
        u->Error("Can't afford to construct a Gate.");
        return false;
    }

    u->ConsumeSharedMoney(1000);

    const unsigned int level = u->GetSkill(spell);
    const unsigned int chance = level * 20;
    if (getrandom(100U) >= chance) {
        u->Event("Attempts to construct a gate, but fails.");
        return false;
    }

    u->Event(AString("Constructs a Gate in ") + r->ShortPrint(regions) + ".");
    regions.numberofgates++;
    if (Globals->DISPERSE_GATE_NUMBERS) {
        int log10 = 0;
        int ngates = static_cast<int>(regions.numberofgates);
        while (ngates > 0) {
            ngates /= 10;
            log10++;
        }
        ngates = 10;
        while (log10 > 0) {
            ngates *= 10;
            log10--;
        }
        std::vector<bool> used(static_cast<size_t>(ngates), false);
        for(const auto& reg: regions) {
            if (reg->gate)
            {
                used[static_cast<size_t>(reg->gate - 1)] = true;
            }
        }
        r->gate = getrandom(ngates);
        while (used[static_cast<size_t>(r->gate)])
        {
            r->gate = getrandom(ngates);
        }
        r->gate++;
    } else {
        r->gate = static_cast<int>(regions.numberofgates);
    }
    if (Globals->GATES_NOT_PERENNIAL) {
        int dm = static_cast<int>(Globals->GATES_NOT_PERENNIAL / 2);
        int gm = static_cast<int>(month) + 1 - getrandom(dm) - getrandom(dm) - getrandom(static_cast<int>(Globals->GATES_NOT_PERENNIAL % 2));
        while(gm < 0)
        {
            gm += 12;
        }
        r->gatemonth = static_cast<size_t>(gm);
    }
    return true;
}

bool Game::RunEngraveRunes(const ARegion::Handle&, const Object::Handle& o, const Unit::Handle& u)
{
    if (o->IsFleet() || !o->IsBuilding()) {
        u->Error("Runes of Warding may only be engraved on a building.");
        return false;
    }

    if (o->incomplete > 0) {
        u->Error( "Runes of Warding may only be engraved on a completed "
                "building.");
        return false;
    }

    const unsigned int level = u->GetSkill(Skills::Types::S_ENGRAVE_RUNES_OF_WARDING);

    switch (level) {
        case 5:
            if (o->type == Objects::Types::O_MCASTLE) break;
            if (o->type == Objects::Types::O_MCITADEL) break;
            /* FALLTHRU */
        case 4:
            if (o->type == Objects::Types::O_CITADEL) break;
            if (o->type == Objects::Types::O_MFORTRESS) break;
            /* FALLTHRU */
        case 3:
            if (o->type == Objects::Types::O_CASTLE) break;
            if (o->type == Objects::Types::O_MTOWER) break;
            /* FALLTHRU */
        case 2:
            if (o->type == Objects::Types::O_FORT) break;
            /* FALLTHRU */
        case 1:
            if (o->type == Objects::Types::O_TOWER) break;
            /* FALLTHRU */
        default:
            u->Error("Not high enough level to engrave Runes of Warding on "
                    "that building.");
            return false;
    }

    if (u->GetSharedMoney() < 600) {
        u->Error("Can't afford to engrave Runes of Warding.");
        return false;
    }

    u->ConsumeSharedMoney(600);
    if (o->type == Objects::Types::O_MCITADEL ) {
        o->runes = 5;
    } else if (o->type == Objects::Types::O_MCASTLE) {
        o->runes = 5;
    } else if (o->type == Objects::Types::O_MFORTRESS) {
        o->runes = 5;
    } else if (o->type == Objects::Types::O_MTOWER) {
        o->runes = 5;
    } else {
        o->runes = 3;
    }
    u->Event(AString("Engraves Runes of Warding on ") + *(o->name) + ".");
    return true;
}

bool Game::RunSummonBalrog(const ARegion::Handle& r, const Unit::Handle& u)
{
    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return false;
    }

    if (static_cast<int>(u->items.GetNum(Items::Types::I_BALROG)) >= ItemDefs[Items::Types::I_BALROG].max_inventory) {
        u->Error("Can't control any more balrogs.");
        return false;
    }

    const unsigned int level = u->GetSkill(Skills::Types::S_SUMMON_BALROG);

    int num = (static_cast<int>(level) * ItemDefs[Items::Types::I_BALROG].mOut + getrandom(100)) / 100;
    const int num_balrog = static_cast<int>(u->items.GetNum(Items::Types::I_BALROG));
    if (num_balrog + num > ItemDefs[Items::Types::I_BALROG].max_inventory)
    {
        num = ItemDefs[Items::Types::I_BALROG].max_inventory - num_balrog;
    }

    u->items.SetNum(Items::Types::I_BALROG, static_cast<size_t>(num_balrog + num));
    u->Event(AString("Summons ") + ItemString(Items::Types::I_BALROG, num) + ".");
    return true;
}

int Game::RunSummonDemon(const ARegion::Handle& r,const Unit::Handle&u)
{
    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    int level = u->GetSkill(Skills::Types::S_SUMMON_DEMON);
    int num = (level * ItemDefs[I_DEMON].mOut + getrandom(100)) / 100;
    num = RandomiseSummonAmount(num);
    if (num < 1)
        num = 1;
    u->items.SetNum(I_DEMON,u->items.GetNum(I_DEMON) + num);
    u->Event(AString("Summons ") + ItemString(I_DEMON,num) + ".");
    return 1;
}

int Game::RunSummonImps(const ARegion::Handle& r,const Unit::Handle&u)
{
    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    int level = u->GetSkill(Skills::Types::S_SUMMON_IMPS);
    int num = (level * ItemDefs[I_IMP].mOut + getrandom(100)) / 100;
    num = RandomiseSummonAmount(num);

    u->items.SetNum(I_IMP,u->items.GetNum(I_IMP) + num);
    u->Event(AString("Summons ") + ItemString(I_IMP,num) + ".");
    return 1;
}

int Game::RunCreateArtifact(const ARegion::Handle& r,const Unit::Handle& u, int skill,int item)
{
    int level = u->GetSkill(skill);
    if (level < ItemDefs[item].mLevel) {
        u->Error("CAST: Skill level isn't that high.");
        return 0;
    }
    unsigned int c;
    for (c = 0; c < sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
        if (ItemDefs[item].mInput[c].item == -1) continue;
        int amt = u->GetSharedNum(ItemDefs[item].mInput[c].item);
        int cost = ItemDefs[item].mInput[c].amt;
        if (amt < cost) {
            u->Error(AString("Doesn't have sufficient ") +
                    ItemDefs[ItemDefs[item].mInput[c].item].name +
                    " to create that.");
            return 0;
        }
    }

    // Deduct the costs
    for (c = 0; c < sizeof(ItemDefs[item].mInput)/sizeof(Materials); c++) {
        if (ItemDefs[item].mInput[c].item == -1) continue;
        int cost = ItemDefs[item].mInput[c].amt;
        u->ConsumeShared(ItemDefs[item].mInput[c].item, cost);
    }

    int num = (level * ItemDefs[item].mOut + getrandom(100))/100;

    if (ItemDefs[item].type & IT_SHIP) {
        if (num > 0)
            CreateShip(r, u, item);
    } else {
        u->items.SetNum(item,u->items.GetNum(item) + num);
    }
    u->Event(AString("Creates ") + ItemString(item,num) + ".");
    if (num == 0) return 0;
    return 1;
}

int Game::RunSummonLich(const ARegion::Handle& r,const Unit::Handle&u)
{
    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    int level = u->GetSkill(Skills::Types::S_SUMMON_LICH);

    int chance = level * ItemDefs[I_LICH].mOut;
    if (chance < 1)
        chance = level * level * 2;
    int num = (chance + getrandom(100))/100;

    u->items.SetNum(I_LICH,u->items.GetNum(I_LICH) + num);
    u->Event(AString("Summons ") + ItemString(I_LICH,num) + ".");
    if (num == 0) return 0;
    return 1;
}

int Game::RunRaiseUndead(const ARegion::Handle& r,const Unit::Handle&u)
{
    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    int level = u->GetSkill(Skills::Types::S_RAISE_UNDEAD);

    int chance = level * ItemDefs[I_UNDEAD].mOut;
    if (chance < 1)
        chance = level * level * 10;
    int num = (chance + getrandom(100))/100;
    num = RandomiseSummonAmount(num);

    u->items.SetNum(I_UNDEAD,u->items.GetNum(I_UNDEAD) + num);
    u->Event(AString("Raises ") + ItemString(I_UNDEAD,num) + ".");
    if (num == 0) return 0;
    return 1;
}

int Game::RunSummonSkeletons(const ARegion::Handle& r,const Unit::Handle&u)
{
    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    int level = u->GetSkill(Skills::Types::S_SUMMON_SKELETONS);

    int chance = level * ItemDefs[I_SKELETON].mOut;
    if (chance < 1)
        chance = level * level * 40;
    int num = (chance + getrandom(100))/100;
    num = RandomiseSummonAmount(num);

    u->items.SetNum(I_SKELETON,u->items.GetNum(I_SKELETON) + num);
    u->Event(AString("Summons ") + ItemString(I_SKELETON,num) + ".");
    if (num == 0) return 0;
    return 1;
}

int Game::RunDragonLore(const ARegion::Handle& r, const Unit::Handle&u)
{
    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    int level = u->GetSkill(Skills::Types::S_DRAGON_LORE);

    int num = u->items.GetNum(I_DRAGON);
    if (num >= level) {
        u->Error("Mage may not summon more dragons.");
        return 0;
    }

    int chance = level * ItemDefs[I_DRAGON].mOut;
    if (chance < 1)
        chance = level * level * 4;
    if (getrandom(100) < chance) {
        u->items.SetNum(I_DRAGON,num + 1);
        u->Event("Summons a dragon.");
        num = 1;
    } else {
        u->Event("Attempts to summon a dragon, but fails.");
        num = 0;
    }
    if (num == 0) return 0;
    return 1;
}

int Game::RunBirdLore(const ARegion::Handle& r,const Unit::Handle&u)
{
    CastIntOrder *order = (CastIntOrder *) u->castorders;
    int type = regions.GetRegionArray(r->zloc)->levelType;

    if (type != ARegionArray::LEVEL_SURFACE) {
        AString error = "CAST: Bird Lore may only be cast on the surface of ";
        error += Globals->WORLD_NAME;
        error += ".";
        u->Error(error.Str());
        return 0;
    }

    if (order->level < 3) {
        int dir = order->target;
        const ARegion::Handle&tar = r->neighbors[dir];
        if (!tar) {
            u->Error("CAST: No such region.");
            return 0;
        }

        Farsight *f = new Farsight;
        f->faction = u->faction;
        f->level = u->GetSkill(Skills::Types::S_BIRD_LORE);
        tar->farsees.Add(f);
        u->Event(AString("Sends birds to spy on ") +
                tar->Print( &regions ) + ".");
        return 1;
    }

    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    int level = u->GetSkill(Skills::Types::S_BIRD_LORE) - 2;
    int max = level * level * 2;
    int num = (level * ItemDefs[I_EAGLE].mOut + getrandom(100)) / 100;
    num = RandomiseSummonAmount(num);
    if (num < 1)
        num = 1;

    if (u->items.GetNum(I_EAGLE) >= max) {
        u->Error("CAST: Mage can't summon more eagles.");
        return 0;
    }

    if (u->items.GetNum(I_EAGLE) + num > max)
        num = max - u->items.GetNum(I_EAGLE);

    u->items.SetNum(I_EAGLE,u->items.GetNum(I_EAGLE) + num);
    u->Event(AString("Summons ") + ItemString(I_EAGLE,num) + ".");
    return 1;
}

int Game::RunWolfLore(const ARegion::Handle& r,const Unit::Handle&u)
{
    if (TerrainDefs[r->type].similar_type != Regions::Types::R_MOUNTAIN &&
        TerrainDefs[r->type].similar_type != Regions::Types::R_FOREST) {
        u->Error("CAST: Can only summon wolves in mountain and "
                 "forest regions.");
        return 0;
    }

    int level = u->GetSkill(Skills::Types::S_WOLF_LORE);
    int max = level * level * 4;

    int curr = u->items.GetNum(I_WOLF);
    int num = (level * ItemDefs[I_WOLF].mOut + getrandom(100)) / 100;
    num = RandomiseSummonAmount(num);

    if (num + curr > max)
        num = max - curr;
    if (num < 0) num = 0;

    u->Event(AString("Casts Wolf Lore, summoning ") +
            ItemString(I_WOLF,num) + ".");
    u->items.SetNum(I_WOLF,num + curr);
    if (num == 0) return 0;
    return 1;
}

int Game::RunInvisibility(const ARegion::Handle& r,const Unit::Handle&u)
{
    CastUnitsOrder *order = (CastUnitsOrder *) u->castorders;
    int max = u->GetSkill(Skills::Types::S_INVISIBILITY);
    max = max * max;

    int num = 0;
    r->DeduplicateUnitList(&order->units, u->faction->num);
    forlist (&(order->units)) {
        const Unit::Handle&tar = r->GetUnitId((UnitId *) elem,u->faction->num);
        if (!tar) continue;
        if (tar->GetAttitude(r,u) < A_FRIENDLY) continue;
        num += tar->GetSoldiers();
    }

    if (num > max) {
        u->Error("CAST: Can't render that many men or creatures invisible.");
        return 0;
    }

    if (!num) {
        u->Error("CAST: No valid targets to turn invisible.");
        return 0;
    }
    forlist_reuse (&(order->units)) {
        const Unit::Handle&tar = r->GetUnitId((UnitId *) elem,u->faction->num);
        if (!tar) continue;
        if (tar->GetAttitude(r,u) < A_FRIENDLY) continue;
        tar->SetFlag(FLAG_INVIS,1);
        tar->Event(AString("Is rendered invisible by ") +
                *(u->name) + ".");
    }

    u->Event("Casts invisibility.");
    return 1;
}

int Game::RunPhanDemons(const ARegion::Handle& r,const Unit::Handle&u)
{
    CastIntOrder *order = (CastIntOrder *) u->castorders;
    int level = u->GetSkill(Skills::Types::S_CREATE_PHANTASMAL_DEMONS);
    int create,max;

    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    if (order->level < 3) {
        create = I_IIMP;
        max = level * level * 4;
    } else {
        if (order->level < 5) {
            create = I_IDEMON;
            max = (level - 2) * (level - 2);
        } else {
            create = I_IBALROG;
            max = 1;
        }
    }

    if (order->target > max || order->target <= 0) {
        u->Error("CAST: Can't create that many Phantasmal Demons.");
        return 0;
    }

    u->items.SetNum(create,order->target);
    u->Event("Casts Create Phantasmal Demons.");
    return 1;
}

int Game::RunPhanUndead(const ARegion::Handle& r,const Unit::Handle&u)
{
    CastIntOrder *order = (CastIntOrder *) u->castorders;
    int level = u->GetSkill(Skills::Types::S_CREATE_PHANTASMAL_UNDEAD);
    int create,max;

    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    if (order->level < 3) {
        create = I_ISKELETON;
        max = level * level * 4;
    } else {
        if (order->level < 5) {
            create = I_IUNDEAD;
            max = (level - 2) * (level - 2);
        } else {
            create = I_ILICH;
            max = 1;
        }
    }

    if (order->target > max || order->target <= 0) {
        u->Error("CAST: Can't create that many Phantasmal Undead.");
        return 0;
    }

    u->items.SetNum(create,order->target);
    u->Event("Casts Create Phantasmal Undead.");
    return 1;
}

int Game::RunPhanBeasts(const ARegion::Handle& r,const Unit::Handle&u)
{
    CastIntOrder *order = (CastIntOrder *) u->castorders;
    int level = u->GetSkill(Skills::Types::S_CREATE_PHANTASMAL_BEASTS);
    int create,max;

    if (r->type == Regions::Types::R_NEXUS) {
        u->Error("Can't summon creatures in the nexus.");
        return 0;
    }

    if (order->level < 3) {
        create = I_IWOLF;
        max = level * level * 4;
    } else {
        if (order->level < 5) {
            create = I_IEAGLE;
            max = (level - 2) * (level - 2);
        } else {
            create = I_IDRAGON;
            max = 1;
        }
    }

    if (order->target > max || order->target <= 0) {
        u->Error("CAST: Can't create that many Phantasmal Beasts.");
        return 0;
    }

    u->items.SetNum(create,order->target);
    u->Event("Casts Create Phantasmal Beasts.");
    return 1;
}

int Game::RunEarthLore(const ARegion::Handle& r,const Unit::Handle&u)
{
    int level = u->GetSkill(Skills::Types::S_EARTH_LORE);

    if (level > r->earthlore) r->earthlore = level;
    int amt = r->Wages() * level * 2 / 10;

    u->items.SetNum(I_SILVER,u->items.GetNum(I_SILVER) + amt);
    u->Event(AString("Casts Earth Lore, raising ") + amt + " silver.");
    return 1;
}

int Game::RunClearSkies(const ARegion::Handle& r, const Unit::Handle&u)
{
    const ARegion::Handle&tar = r;
    AString temp = "Casts Clear Skies";
    int val;

    CastRegionOrder *order = (CastRegionOrder *)u->castorders;

    RangeType *range = FindRange(SkillDefs[Skills::Types::S_CLEAR_SKIES].range);
    if (range != NULL) {
        tar = regions.GetRegion(order->xloc, order->yloc, order->zloc);
        val = GetRegionInRange(r, tar, u, Skills::Types::S_CLEAR_SKIES);
        if (!val) return 0;
        temp += " on ";
        temp += tar->ShortPrint(&regions);
    }
    temp += ".";
    int level = u->GetSkill(Skills::Types::S_CLEAR_SKIES);
    if (level > r->clearskies) r->clearskies = level;
    u->Event(temp);
    return 1;
}

int Game::RunWeatherLore(const ARegion::Handle& r, const Unit::Handle&u)
{
    const ARegion::Handle&tar;
    int val, i;

    CastRegionOrder *order = (CastRegionOrder *)u->castorders;

    tar = regions.GetRegion(order->xloc, order->yloc, order->zloc);
    val = GetRegionInRange(r, tar, u, Skills::Types::S_WEATHER_LORE);
    if (!val) return 0;

    int level = u->GetSkill(Skills::Types::S_WEATHER_LORE);
    int months = 3;
    if (level >= 5) months = 12;
    else if (level >= 3) months = 6;

    AString temp = "Casts Weather Lore on ";
    temp += tar->ShortPrint(&regions);
    temp += ". It will be ";
    int weather, futuremonth;
    for (i = 0; i <= months; i++) {
        futuremonth = (month + i)%12;
        weather=regions.GetWeather(tar, futuremonth);
        temp += SeasonNames[weather];
        temp += " in ";
        temp += MonthNames[futuremonth];
        if (i < (months-1))
            temp += ", ";
        else if (i == (months-1))
            temp += " and ";
        else
            temp += ".";
    }
    u->Event(temp);
    return 1;
}

int Game::RunFarsight(const ARegion::Handle& r,const Unit::Handle&u)
{
    const ARegion::Handle&tar;
    int val;

    CastRegionOrder *order = (CastRegionOrder *)u->castorders;

    tar = regions.GetRegion(order->xloc, order->yloc, order->zloc);
    val = GetRegionInRange(r, tar, u, Skills::Types::S_FARSIGHT);
    if (!val) return 0;

    Farsight *f = new Farsight;
    f->faction = u->faction;
    f->level = u->GetSkill(Skills::Types::S_FARSIGHT);
    f->unit = u;
    f->observation = u->GetAttribute("observation");
    tar->farsees.Add(f);
    AString temp = "Casts Farsight on ";
    temp += tar->ShortPrint(&regions);
    temp += ".";
    u->Event(temp);
    return 1;
}

int Game::RunDetectGates(const ARegion::Handle& r, Object *, const Unit::Handle&u)
{
    int level = u->GetSkill(Skills::Types::S_GATE_LORE);

    if (level == 1) {
        u->Error("CAST: Casting Gate Lore at level 1 has no effect.");
        return 0;
    }

    u->Event("Casts Gate Lore, detecting nearby Gates:");
    int found = 0;
    if ((r->gate) && (!r->gateopen)) {
        u->Event(AString("Identified local gate number ") + (r->gate) +
        " in " + r->ShortPrint(&regions) + ".");
    }
    for (int i=0; i<NDIRS; i++) {
        const ARegion::Handle&tar = r->neighbors[i];
        if (tar) {
            if (tar->gate) {
                if (Globals->DETECT_GATE_NUMBERS) {
                    u->Event(tar->Print(&regions) +
                        " contains Gate " + tar->gate +
                        ".");
                } else {
                    u->Event(tar->Print(&regions) +
                        " contains a Gate.");
                }
                found = 1;
            }
        }
    }
    if (!found)
        u->Event("There are no nearby Gates.");
    return 1;
}

int Game::RunTeleport(const ARegion::Handle& r, Object *, const Unit::Handle&u)
{
    const ARegion::Handle&tar;
    int val;

    CastRegionOrder *order = (CastRegionOrder *)u->teleportorders;

    tar = regions.GetRegion(order->xloc, order->yloc, order->zloc);
    val = GetRegionInRange(r, tar, u, Skills::Types::S_TELEPORTATION);
    if (!val) return 0;

    int level = u->GetSkill(Skills::Types::S_TELEPORTATION);
    int maxweight = level * 15;

    if (u->Weight() > maxweight) {
        u->Error("CAST: Can't carry that much when teleporting.");
        return 0;
    }

    // Presume they had to open the portal to see if target is ocean
    if (TerrainDefs[tar->type].similar_type == Regions::Types::R_OCEAN) {
        u->Error(AString("CAST: ") + tar->Print(&regions) +
            " is an ocean.");
        return 1;
    }
    u->DiscardUnfinishedShips();
    u->Event(AString("Teleports to ") + tar->Print(&regions) + ".");
    u->MoveUnit(tar->GetDummy());
    return 1;
}

int Game::RunGateJump(const ARegion::Handle& r, Object *, const Unit::Handle&u)
{
    int level = u->GetSkill(Skills::Types::S_GATE_LORE);
    int nexgate = 0;
    if ( !level ) {
        u->Error( "CAST: Unit doesn't have that skill." );
        return 0;
    }

    TeleportOrder *order = u->teleportorders;

    if ((order->gate > 0 && level < 3) ||
            (order->gate == -2 && level < 2)) {
        u->Error("CAST: Unit Doesn't know Gate Lore at that level.");
        return 0;
    }

    nexgate = Globals->NEXUS_GATE_OUT &&
        (TerrainDefs[r->type].similar_type == Regions::Types::R_NEXUS);
    if (!r->gate && !nexgate) {
        u->Error("CAST: There is no gate in that region.");
        return 0;
    }

    if (!r->gateopen) {
        u->Error("CAST: Gate not open at this time of year.");
        return 0;
    }

    int maxweight = 10;
    if (order->gate != -1) level -= 2;
    switch (level) {
        case 1:
            maxweight = 15;
            break;
        case 2:
            maxweight = 100;
            break;
        case 3:
        case 4:
        case 5:
            maxweight = 1000;
            break;
    }

    int weight = u->Weight();

    r->DeduplicateUnitList(&order->units, u->faction->num);
    forlist (&(order->units)) {
        const Unit::Handle&taru = r->GetUnitId((UnitId *) elem,u->faction->num);
        if (taru && taru != u) weight += taru->Weight();
    }

    if (weight > maxweight) {
        u->Error( "CAST: Can't carry that much weight through a Gate.");
        return 0;
    }

    const ARegion::Handle&tar;
    if (order->gate < 0) {
        int good = 0;

        do {
            tar = regions.FindGate(-1);
            if (!tar)
                continue;

            if (tar->zloc == r->zloc)
                good = 1;
            if (order->gate == -2) {
                good = 1;
            }
            if (nexgate && tar->zloc == ARegionArray::LEVEL_SURFACE)
                good = 1;
            if (!tar->gateopen)
                good = 0;
        } while (!good);

        u->Event("Casts Random Gate Jump.");
    } else {
        tar = regions.FindGate(order->gate);
        if (!tar) {
            u->Error("CAST: No such target gate.");
            return 0;
        }
        if (!tar->gateopen) {
            u->Error("CAST: Target gate is not open at this time of year.");
            return 0;
        }

        u->Event("Casts Gate Jump.");
    }

    int comma = 0;
    AString unitlist; {
        forlist(&(order->units)) {
            Location *loc = r->GetLocation((UnitId *) elem,u->faction->num);
            if (loc) {
                /* Don't do the casting unit yet */
                if (loc->unit == u) {
                    delete loc;
                    continue;
                }

                if (loc->unit->GetAttitude(r,u) < A_ALLY) {
                    u->Error("CAST: Unit is not allied.");
                } else {
                    if (comma) {
                        unitlist += AString(", ") + AString(loc->unit->num);
                    } else {
                        unitlist += AString(loc->unit->num);
                        comma = 1;
                    }
                    loc->unit->DiscardUnfinishedShips();
                    loc->unit->Event(AString("Is teleported through a ") +
                            "Gate to " + tar->Print(&regions) + " by " +
                            *u->name + ".");
                    loc->unit->MoveUnit( tar->GetDummy() );
                    if (loc->unit != u) loc->unit->ClearCastOrders();
                }
                delete loc;
            } else {
                u->Error("CAST: No such unit.");
            }
        }
    }
    u->DiscardUnfinishedShips();
    u->Event(AString("Jumps through a Gate to ") +
            tar->Print( &regions ) + ".");
    if (comma) {
        u->Event(unitlist + " follow through the Gate.");
    }
    u->MoveUnit( tar->GetDummy() );
    return 1;
}

int Game::RunPortalLore(const ARegion::Handle& r, Object *, const Unit::Handle&u)
{
    int level = u->GetSkill(Skills::Types::S_PORTAL_LORE);
    TeleportOrder *order = u->teleportorders;

    if (!level) {
        u->Error("CAST: Doesn't know Portal Lore.");
        return 0;
    }

    if (!u->items.GetNum(I_PORTAL)) {
        u->Error("CAST: Unit doesn't have a Portal.");
        return 0;
    }

    int maxweight = 50 * level;
    r->DeduplicateUnitList(&order->units, u->faction->num);
    int weight = 0;
    forlist (&(order->units)) {
        const Unit::Handle&taru = r->GetUnitId((UnitId *) elem,u->faction->num);
        if (taru) weight += taru->Weight();
    }

    if (weight > maxweight) {
        u->Error("CAST: That mage cannot teleport that much weight through a "
                "Portal.");
        return 0;
    }

    Location *tar = regions.FindUnit(order->gate);
    if (!tar) {
        u->Error("CAST: No such target mage.");
        return 0;
    }

    if (tar->unit->faction->GetAttitude(u->faction->num) < A_FRIENDLY) {
        u->Error("CAST: Target mage is not friendly.");
        return 0;
    }

    if (tar->unit->type != U_MAGE) {
        u->Error("CAST: Target is not a mage.");
        return 0;
    }

    if (!tar->unit->items.GetNum(I_PORTAL)) {
        u->Error("CAST: Target does not have a Portal.");
        return 0;
    }

    if (!GetRegionInRange(r, tar->region, u, Skills::Types::S_PORTAL_LORE)) return 0;

    u->Event("Casts Portal Jump.");

    {
        forlist(&(order->units)) {
            Location *loc = r->GetLocation((UnitId *) elem,u->faction->num);
            if (loc) {
                if (loc->unit->GetAttitude(r,u) < A_ALLY) {
                    u->Error("CAST: Unit is not allied.");
                } else {
                    loc->unit->DiscardUnfinishedShips();
                    loc->unit->Event(AString("Is teleported to ") +
                            tar->region->Print( &regions ) +
                            " by " + *u->name + ".");
                    loc->unit->MoveUnit( tar->obj );
                    if (loc->unit != u) loc->unit->ClearCastOrders();
                }
                delete loc;
            } else {
                u->Error("CAST: No such unit.");
            }
        }
    }

    delete tar;
    return 1;
}

int Game::RunTransmutation(const ARegion::Handle&, const Unit::Handle&u)
{
    CastTransmuteOrder *order;
    int level, num, source;

    order = (CastTransmuteOrder *) u->castorders;
    level = u->GetSkill(Skills::Types::S_TRANSMUTATION);
    if (!level) {
        u->Error("CAST: Unit doesn't have that skill.");
        return 0;
    }
    if (level < order->level) {
        u->Error("CAST: Can't create that by transmutation.");
        return 0;
    }
    
    switch(order->item) {
        case I_MITHRIL:
            source = I_IRON;
            break;
        case I_ROOTSTONE:
            source = I_STONE;
            break;
        case I_FLOATER:
            source = I_FUR;
            break;
        case I_IRONWOOD:
        case I_YEW:
            source = I_WOOD;
            break;
        case I_WHORSE:
            source = I_HORSE;
            break;
    }
    
    num = u->GetSharedNum(source);
    if (num > level)
        num = level;
    if (order->number != -1 && num > order->number)
        num = order->number;
    if (num < order->number)
        u->Error("CAST: Can't create that many.");
    u->ConsumeShared(source, num);
    u->items.SetNum(order->item, u->items.GetNum(order->item) + num);
    u->Event(AString("Transmutes ") +
            ItemString(source, num) +
            " into " +
            ItemString(order->item, num) +
            ".");
    
    return 1;
}

int Game::RunBlasphemousRitual(const ARegion::Handle& r, const Unit::Handle&mage)
{
    int level, num, sactype, sacrifices, i, sac, max;
    Object *o, *tower;
    const Unit::Handle& u,  *victim;
    Item *item;
    const ARegion::Handle&start;
    AString message;

    level = mage->GetSkill(Skills::Types::S_BLASPHEMOUS_RITUAL);
    if (level < 1) {
        mage->Error("CAST: Unit doesn't have that skill.");
        return 0;
    }
    if (TerrainDefs[r->type].similar_type == Regions::Types::R_OCEAN) {
        mage->Error(AString("CAST: Can't build a ") +
            ObjectDefs[O_BKEEP].name +
            " on water.");
        return 0;
    }
    num = mage->GetSharedNum(I_ROOTSTONE);
    if (num > level)
        num = level;
    tower = 0;
    sactype = IT_LEADER;
    sacrifices = 0;
    forlist(&r->objects) {
        o = (Object *) elem;
        if (o->type == O_BKEEP)
            tower = o;
        forlist(&o->units) {
            u = (const Unit::Handle&) elem;
            if (u->faction->num == mage->faction->num) {
                forlist(&u->items) {
                    item = (Item *) elem;
                    if (ItemDefs[item->type].type & sactype)
                        sacrifices += item->num;
                }
            }
        }
    }
    if (num > sacrifices)
        num = sacrifices;
    if (num < 1) {
        mage->Error("CAST: Don't have the required materials.");
        return 0;
    }
    if (!tower) {
        for (i = 1; i < 100; i++)
            if (!r->GetObject(i))
                break;
        if (i < 100) {
            tower = new Object(r);
            tower->type = O_BKEEP;
            tower->incomplete = ObjectDefs[tower->type].cost;
            tower->num = i;
            tower->SetName(new AString("Building"));
            r->objects.Add(tower);
            WriteTimesArticle("The earth shakes as a blasphemous word is uttered.");
        } else {
            mage->Error("CAST: The region is full.");
            return 0;
        }
    }
    if (num > tower->incomplete)
        num = tower->incomplete;
    while (num-- > 0) {
        victim = 0;
        i = getrandom(sacrifices);
        forlist(&r->objects) {
            o = (Object *) elem;
            forlist(&o->units) {
                u = (const Unit::Handle&) elem;
                if (u->faction->num == mage->faction->num) {
                    forlist(&u->items) {
                        item = (Item *) elem;
                        if (ItemDefs[item->type].type & sactype) {
                            if (!victim && i < item->num) {
                                victim = u;
                                sac = item->type;
                            }
                            i -= item->num;
                        }
                    }
                }
            }
        }
        mage->ConsumeShared(I_ROOTSTONE, 1);
        victim->SetMen(sac, victim->GetMen(sac) - 1);
        sacrifices--;
        tower->incomplete--;
        max = ObjectDefs[tower->type].cost;
        if (tower->incomplete == max * 9 / 10) {
            // 10% complete
            message = "Vile rituals are being performed in the ";
            message += TerrainDefs[r->type].name;
            message += " of ";
            message += *r->name;
            message += "!";
            WriteTimesArticle(message);
        }
        if (tower->incomplete == max * 2 / 3) {
            // 33% complete
            Directions dir;
            start = regions.FindNearestStartingCity(r, dir);
            message = "A blasphemous construction is taking shape in ";
            if (start == r) {
                message += *start->town->name;
                message += ", in ";
            }
            message += "the ";
            message += TerrainDefs[r->type].name;
            message += " of ";
            message += *r->name;
            if (start && start != r && dir.isValid()) {
                message += ", ";
                if (r->zloc != start->zloc && dir != MOVE_IN)
                    message += "through a shaft ";
                switch (dir) {
                    case D_NORTH:
                    case D_NORTHWEST:
                        message += "north of";
                        break;
                    case D_NORTHEAST:
                        message += "east of";
                        break;
                    case D_SOUTH:
                    case D_SOUTHEAST:
                        message += "south of";
                        break;
                    case D_SOUTHWEST:
                        message += "west of";
                        break;
                    case MOVE_IN:
                        message += "through a shaft in";
                        break;
                }
                message += " ";
                message += *start->town->name;
            }
            message += "!";
            WriteTimesArticle(message);
        }
        if (tower->incomplete == max / 3) {
            // 66% complete
            message = "The blasphemous tower in the ";
            message += r->ShortPrint(&regions);
            message += " is nearing completion!";
            WriteTimesArticle(message);
        }
        if (tower->incomplete == max / 10) {
            // 90% complete
            message = *u->faction->name;
            message += " have almost completed a blasphemous tower in the ";
            message += r->ShortPrint(&regions);
            message += ".  Their folly will doom everyone!";
            WriteTimesArticle(message);
        }
        mage->Event(AString("Sacrifices ") + ItemDefs[sac].name + " from " + victim->name->Str());
        if (!victim->GetMen())
            r->Kill(victim);
        if (!mage->GetMen())
            break;
    }

    // If the player chooses to go down the dark path,
    // then erase any progress they may have made down the light path
    forlist_reuse(&regions) {
        r = (const ARegion::Handle&) elem;
        forlist(&r->objects) {
            Object * o = (Object *) elem;
            forlist(&o->units) {
                const Unit::Handle& u = (const Unit::Handle&) elem;
                if (u->faction->num == mage->faction->num) {
                    u->items.SetNum(I_RELICOFGRACE, 0);
                }
            }
        }
    }

    return 1;
}

void Game::RunTeleportOrders()
{
    int val = 1;
    forlist(&regions) {
        const ARegion::Handle& r = (const ARegion::Handle&) elem;
        forlist(&r->objects) {
            Object * o = (Object *) elem;
            int foundone = 1;
            while (foundone) {
                foundone = 0;
                forlist(&o->units) {
                    const Unit::Handle& u = (const Unit::Handle&) elem;
                    if (u->teleportorders) {
                        foundone = 1;
                        switch (u->teleportorders->spell) {
                            case Skills::Types::S_GATE_LORE:
                                val = RunGateJump(r,o,u);
                                break;
                            case Skills::Types::S_TELEPORTATION:
                                val = RunTeleport(r,o,u);
                                break;
                            case Skills::Types::S_PORTAL_LORE:
                                val = RunPortalLore(r,o,u);
                                break;
                        }
                        if (val)
                            u->Practice(u->teleportorders->spell);
                        delete u->teleportorders;
                        u->teleportorders = 0;
                        break;
                    }
                }
            }
        }
    }
}
