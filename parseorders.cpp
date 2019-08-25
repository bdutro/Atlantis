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

#include <stdlib.h>

#include "game.h"
#include "gameio.h"
#include "orders.h"
#include "skills.h"
#include "gamedata.h"

OrdersCheck::OrdersCheck()
{
    pCheckFile = nullptr;
    numshows = 0;
    numerrors = 0;
    dummyFaction = std::make_shared<Faction>();
    dummyUnit = std::make_shared<Unit>();
    dummyUnit->monthorders = 0;
}

void OrdersCheck::Error(const AString &strError)
{
    if (pCheckFile) {
        pCheckFile->PutStr("");
        pCheckFile->PutStr("");
        pCheckFile->PutStr(AString("*** Error: ") + strError + " ***");
    }
    numerrors++;
}

Directions Game::ParseDir(AString *token)
{
    for (auto i = Directions::begin(); i != Directions::end(); ++i) {
        if (*token == DirectionStrs[*i])
        {
            return *i;
        }
        if (*token == DirectionAbrs[*i])
        {
            return *i;
        }
    }
    if (*token == "in")
    {
        return Directions::MOVE_IN;
    }
    if (*token == "out")
    {
        return Directions::MOVE_OUT;
    }
    if (*token == "pause" || *token == "p")
    {
        return Directions::MOVE_PAUSE;
    }

    int num = token->value();
    if (num)
    {
        return Directions::MOVE_ENTER + static_cast<unsigned int>(num);
    }
    return Directions();
}

int ParseTF(AString *token);

int ParseTF(AString *token)
{
    if (*token == "true") return 1;
    if (*token == "false") return 0;
    if (*token == "t") return 1;
    if (*token == "f") return 0;
    if (*token == "on") return 1;
    if (*token == "off") return 0;
    if (*token == "yes") return 1;
    if (*token == "no") return 0;
    if (*token == "1") return 1;
    if (*token == "0") return 0;
    return -1;
}

UnitId::Handle Game::ParseUnit(AString *s)
{
    AString *token = s->gettoken();
    if (!token) return nullptr;

    if (*token == "0") {
        delete token;
        UnitId::Handle id = std::make_shared<UnitId>();
        id->unitnum.invalidate();
        id->alias = 0;
        id->faction = 0;
        return id;
    }

    if (*token == "faction") {
        delete token;
        /* Get faction number */
        token = s->gettoken();
        if (!token) return nullptr;

        int fn = token->value();
        delete token;
        if (!fn) return nullptr;

        /* Next token should be "new" */
        token = s->gettoken();
        if (!token) return nullptr;

        if (!(*token == "new")) {
            delete token;
            return nullptr;
        }
        delete token;

        /* Get alias number */
        token = s->gettoken();
        if (!token) return nullptr;

        int un = token->value();
        delete token;
        if (!un) return nullptr;

        /* Return UnitId */
        UnitId::Handle id = std::make_shared<UnitId>();
        id->unitnum = 0;
        id->alias = un;
        id->faction = static_cast<size_t>(fn);
        return id;
    }

    if (*token == "new") {
        delete token;
        token = s->gettoken();
        if (!token) return nullptr;

        int un = token->value();
        delete token;
        if (!un) return nullptr;

        UnitId::Handle id = std::make_shared<UnitId>();
        id->unitnum = 0;
        id->alias = un;
        id->faction = 0;
        return id;
    } else {
        int un = token->value();
        delete token;
        if (!un) return nullptr;

        UnitId::Handle id = std::make_shared<UnitId>();
        id->unitnum = static_cast<size_t>(un);
        id->alias = 0;
        id->faction = 0;
        return id;
    }
}

int ParseFactionType(AString *o, int *type);

int ParseFactionType(AString *o, int *type)
{
    int i;
    for (i=0; i<NFACTYPES; i++) type[i] = 0;

    AString *token = o->gettoken();
    if (!token) return -1;

    if (*token == "generic") {
        delete token;
        for (i=0; i<NFACTYPES; i++) type[i] = 1;
        return 0;
    }

    while(token) {
        int foundone = 0;
        for (i=0; i<NFACTYPES; i++) {
            if (*token == FactionStrs[i]) {
                delete token;
                token = o->gettoken();
                if (!token) return -1;
                type[i] = token->value();
                delete token;
                foundone = 1;
                break;
            }
        }
        if (!foundone) {
            delete token;
            return -1;
        }
        token = o->gettoken();
    }

    int tot = 0;
    for (i=0; i<NFACTYPES; i++) {
        tot += type[i];
    }
    if (tot > Globals->FACTION_POINTS) return -1;

    return 0;
}

void Game::ParseError(const OrdersCheck::Handle& pCheck,
                      const Unit::Handle& pUnit,
                      const Faction::Handle& pFaction,
                      const AString &strError)
{
    if (pCheck) pCheck->Error(strError);
    else if (pUnit) pUnit->Error(strError);
    else if (pFaction) pFaction->Error(strError);
}

void Game::ParseOrders(size_t faction, Aorders& f, const OrdersCheck::Handle& pCheck)
{
    Faction::Handle fac = nullptr;
    Unit::Handle unit = nullptr;
    int indent = 0, i;

    AString* order = f.GetLine();
    while (order) {
        AString saveorder = *order;
        int getatsign = order->getat();
        AString *token = order->gettoken();
        Orders code;

        if (token) {
            code = Parse1Order(token);
            if(!code.isValid())
            {
                ParseError(pCheck, unit, fac, *token+" is not a valid order.");
            }
            else
            {
                switch (code.asEnum()) {
                    case Orders::Types::O_ATLANTIS:
                        if (fac)
                            ParseError(pCheck, 0, fac, "No #END statement given.");
                        delete token;
                        token = order->gettoken();
                        if (!token) {
                            ParseError(pCheck, 0, 0,
                                    "No faction number given on #atlantis line.");
                            fac = nullptr;
                            break;
                        }
                        if (pCheck) {
                            fac = pCheck->dummyFaction;
                            pCheck->numshows = 0;
                        } else {
                            fac = GetFaction(factions, static_cast<size_t>(token->value()));
                        }

                        if (!fac) break;

                        delete token;
                        token = order->gettoken();

                        if (pCheck) {
                            if (!token) {
                                ParseError(pCheck, 0, fac,
                                        "Warning: No password on #atlantis line.");
                                ParseError(pCheck, 0, fac,
                                        "If this is your first turn, ignore this "
                                        "error.");
                            }
                        } else {
                            if (!(*(fac->password) == "none")) {
                                if (!token || !(*(fac->password) == *token)) {
                                    ParseError(pCheck, 0, fac,
                                            "Incorrect password on #atlantis line.");
                                    fac = nullptr;
                                    break;
                                }
                            }

                            if (fac->num == monfaction || fac->num == guardfaction) {
                                fac = nullptr;
                                break;
                            }
                            if (!Globals->LASTORDERS_MAINTAINED_BY_SCRIPTS)
                                fac->lastorders = TurnNumber();
                        }

                        unit = nullptr;
                        break;

                    case Orders::Types::O_END:
                        indent = 0;
                        while (unit) {
                            const auto& former = unit->former;
                            if (unit->inTurnBlock)
                            {
                                ParseError(pCheck, unit, fac, "TURN: without ENDTURN");
                            }
                            if (unit->former)
                            {
                                ParseError(pCheck, unit, fac, "FORM: without END.");
                            }
                            if (unit && pCheck)
                            {
                                unit->ClearOrders();
                            }
                            unit = former;
                        }

                        unit = nullptr;
                        fac = nullptr;
                        break;

                    case Orders::Types::O_UNIT:
                        indent = 0;
                        if (fac) {
                            while (unit) {
                                const auto& former = unit->former;
                                if (unit->inTurnBlock)
                                {
                                    ParseError(pCheck, unit, fac, "TURN: without ENDTURN");
                                }
                                if (unit->former)
                                {
                                    ParseError(pCheck, unit, fac, "FORM: without END.");
                                }
                                if (unit && pCheck)
                                {
                                    unit->ClearOrders();
                                }
                                unit = former;
                            }
                            unit = nullptr;
                            delete token;

                            token = order->gettoken();
                            if (!token) {
                                ParseError(pCheck, 0, fac, "UNIT without unit number.");
                                unit = nullptr;
                                break;
                            }

                            if (pCheck) {
                                if (!token->value()) {
                                    ParseError(pCheck, 0, fac, "Invalid unit number.");
                                } else {
                                    unit = pCheck->dummyUnit;
                                    unit->monthorders = 0;
                                }
                            } else {
                                unit = GetUnit(token->value()).lock();
                                if (!unit || unit->faction.lock() != fac) {
                                    fac->Error(*token + " is not your unit.");
                                    unit = nullptr;
                                } else {
                                    unit->ClearOrders();
                                }
                            }
                        }
                        break;
                    case Orders::Types::O_FORM:
                        if (fac) {
                            if (unit) {
                                if (unit->former && !unit->inTurnBlock) {
                                    ParseError(pCheck, unit, fac, "FORM: cannot nest.");
                                }
                                else {
                                    unit = ProcessFormOrder(unit, order, pCheck, getatsign);
                                    if (!pCheck && unit && unit->former && unit->former->format)
                                    {
                                        unit->former->oldorders.emplace_back(std::make_shared<AString>(saveorder));
                                    }
                                    if (!pCheck) {
                                        if (unit) unit->ClearOrders();
                                    }
                                }
                            } else {
                                ParseError(pCheck, 0, fac,
                                        "Order given without a unit selected.");
                            }
                        }
                        break;
                    case Orders::Types::O_ENDFORM:
                        if (fac) {
                            if (unit && unit->former) {
                                const auto& former = unit->former;

                                if (unit->inTurnBlock)
                                {
                                    ParseError(pCheck, unit, fac, "TURN: without ENDTURN");
                                }
                                if (!pCheck && unit->former && unit->former->format)
                                {
                                    unit->former->oldorders.emplace_back(std::make_shared<AString>(saveorder));
                                }
                                unit = former;
                            } else {
                                ParseError(pCheck, unit, fac, "END: without FORM.");
                            }
                        }
                        break;
                    case Orders::Types::O_TURN:
                        if (unit && unit->inTurnBlock) {
                            ParseError(pCheck, unit, fac, "TURN: cannot nest");
                        } else if (!unit)
                            ParseError(pCheck, 0, fac, "Order given without a unit selected.");
                        else {
                            // faction is 0 if checking syntax only, not running turn.
                            if (faction != 0) {
                                AString *retval;
                                if (!pCheck && unit->former && unit->former->format)
                                {
                                    unit->former->oldorders.emplace_back(std::make_shared<AString>(saveorder));
                                }
                                retval = ProcessTurnOrder(unit, f, pCheck, getatsign);
                                if (retval) {
                                    delete order;
                                    order = retval;
                                    continue;
                                }
                            } else {
                                unit->inTurnBlock = 1;
                                unit->presentMonthOrders = unit->monthorders;
                                unit->monthorders = nullptr;
                                unit->presentTaxing = unit->taxing;
                                unit->taxing = 0;
                            }
                        }
                        break;
                    case Orders::Types::O_ENDTURN:
                        if (unit && unit->inTurnBlock) {
                            unit->monthorders = unit->presentMonthOrders;
                            unit->presentMonthOrders = nullptr;
                            unit->taxing = unit->presentTaxing;
                            unit->presentTaxing = 0;
                            unit->inTurnBlock = 0;
                            if (!pCheck && unit->former && unit->former->format)
                            {
                                unit->former->oldorders.emplace_back(std::make_shared<AString>(saveorder));
                            }
                        } else
                            ParseError(pCheck, unit, fac, "ENDTURN: without TURN.");
                        break;
                    default:
                        if (fac) {
                            if (unit) {
                                if (!pCheck && getatsign)
                                {
                                    unit->oldorders.emplace_back(std::make_shared<AString>(saveorder));
                                }
                                if (!pCheck && unit->former && unit->former->format)
                                {
                                    unit->former->oldorders.emplace_back(std::make_shared<AString>(saveorder));
                                }

                                ProcessOrder(code, unit, order, pCheck);
                            } else {
                                ParseError(pCheck, 0, fac,
                                        "Order given without a unit selected.");
                            }
                        }
                }
            }
            SAFE_DELETE(token);
        } else {
            code = *Orders::end();
            if (!pCheck) {
                if (getatsign && fac && unit)
                {
                    unit->oldorders.emplace_back(std::make_shared<AString>(saveorder));
                }
            }
        }

        delete order;
        if (pCheck) {
            if (code == Orders::Types::O_ENDTURN || code == Orders::Types::O_ENDFORM)
            {
                indent--;
            }
            AString prefix;
            for (i = 0, prefix = ""; i < indent; i++)
            {
                prefix += "  ";
            }
            pCheck->pCheckFile->PutStr(prefix + saveorder);
            if (code == Orders::Types::O_TURN || code == Orders::Types::O_FORM)
            {
                indent++;
            }
        }

        order = f.GetLine();
    }

    while (unit) {
        const auto& former = unit->former;
        if (unit->inTurnBlock)
        {
            ParseError(pCheck, 0, fac, "TURN: without ENDTURN");
        }
        if (unit->former)
        {
            ParseError(pCheck, 0, fac, "FORM: without END.");
        }
        if (unit && pCheck)
        {
            unit->ClearOrders();
        }
        unit = former;
    }

    if (pCheck) {
        pCheck->pCheckFile->PutStr("");
        if (!pCheck->numerrors) {
            pCheck->pCheckFile->PutStr("No errors found.");
        } else {
            AString str = pCheck->numerrors;
            str += " error";
            if (pCheck->numerrors != 1)
                str += "s";
            str += " found!";
            pCheck->pCheckFile->PutStr(str);
        }
    }
}

void Game::ProcessOrder(const Orders& orderNum,
                        const Unit::Handle& unit,
                        AString *o,
                        const OrdersCheck::Handle& pCheck)
{
    switch(orderNum.asEnum()) {
        case Orders::Types::O_ADDRESS:
            ProcessAddressOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_ADVANCE:
            ProcessAdvanceOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_ASSASSINATE:
            ProcessAssassinateOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_ATTACK:
            ProcessAttackOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_AUTOTAX:
            ProcessAutoTaxOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_AVOID:
            ProcessAvoidOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_IDLE:
            ProcessIdleOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_BEHIND:
            ProcessBehindOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_BUILD:
            ProcessBuildOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_BUY:
            ProcessBuyOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_CAST:
            ProcessCastOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_CLAIM:
            ProcessClaimOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_COMBAT:
            ProcessCombatOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_CONSUME:
            ProcessConsumeOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_DECLARE:
            ProcessDeclareOrder(unit->faction.lock(), o, pCheck);
            break;
        case Orders::Types::O_DESCRIBE:
            ProcessDescribeOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_DESTROY:
            ProcessDestroyOrder(unit, pCheck);
            break;
        case Orders::Types::O_ENTER:
            ProcessEnterOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_ENTERTAIN:
            ProcessEntertainOrder(unit, pCheck);
            break;
        case Orders::Types::O_EVICT:
            ProcessEvictOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_EXCHANGE:
            ProcessExchangeOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_FACTION:
            ProcessFactionOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_FIND:
            ProcessFindOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_FORGET:
            ProcessForgetOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_WITHDRAW:
            ProcessWithdrawOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_GIVE:
            ProcessGiveOrder(orderNum, unit, o, pCheck);
            break;
        case Orders::Types::O_GUARD:
            ProcessGuardOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_HOLD:
            ProcessHoldOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_JOIN:
            ProcessJoinOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_LEAVE:
            ProcessLeaveOrder(unit, pCheck);
            break;
        case Orders::Types::O_MOVE:
            ProcessMoveOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_NAME:
            ProcessNameOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_NOAID:
            ProcessNoaidOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_NOCROSS:
            ProcessNocrossOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_NOSPOILS:
            ProcessNospoilsOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_OPTION:
            ProcessOptionOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_PASSWORD:
            ProcessPasswordOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_PILLAGE:
            ProcessPillageOrder(unit, pCheck);
            break;
        case Orders::Types::O_PREPARE:
            ProcessPrepareOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_WEAPON:
            ProcessWeaponOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_ARMOR:
            ProcessArmorOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_PRODUCE:
            ProcessProduceOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_PROMOTE:
            ProcessPromoteOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_QUIT:
            ProcessQuitOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_RESTART:
            ProcessRestartOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_REVEAL:
            ProcessRevealOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_SAIL:
            ProcessSailOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_SELL:
            ProcessSellOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_SHARE:
            ProcessShareOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_SHOW:
            ProcessReshowOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_SPOILS:
            ProcessSpoilsOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_STEAL:
            ProcessStealOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_STUDY:
            ProcessStudyOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_TAKE:
            ProcessGiveOrder(orderNum, unit, o, pCheck);
            break;
        case Orders::Types::O_TAX:
            ProcessTaxOrder(unit, pCheck);
            break;
        case Orders::Types::O_TEACH:
            ProcessTeachOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_WORK:
            ProcessWorkOrder(unit, 0, pCheck);
            break;
        case Orders::Types::O_TRANSPORT:
            ProcessTransportOrder(unit, o, pCheck);
            break;
        case Orders::Types::O_DISTRIBUTE:
            ProcessDistributeOrder(unit, o, pCheck);
            break;
        default:
            break;
    }
}

void Game::ProcessPasswordOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if (pCheck) return;

    AString *token = o->gettoken();
    const auto u_fac = u->faction.lock();
    if (u_fac->password)
    {
        delete u_fac->password;
    }
    if (token) {
        u_fac->password = token;
        u_fac->Event(AString("Password is now: ") + *token);
    } else {
        u_fac->password = new AString("none");
        u_fac->Event("Password cleared.");
    }
}

void Game::ProcessOptionOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "OPTION: What option?");
        return;
    }

    const auto u_fac = u->faction.lock();
    if (*token == "times") {
        delete token;
        if (!pCheck) {
            u_fac->Event("Times will be sent to your faction.");
            u_fac->times = 1;
        }
        return;
    }

    if (*token == "notimes") {
        delete token;
        if (!pCheck) {
            u_fac->Event("Times will not be sent to your faction.");
            u_fac->times = 0;
        }
        return;
    }

    if (*token == "showattitudes") {
        delete token;
        if (!pCheck) {
            u_fac->Event("Units will now have a leading sign to show your " 
                        "attitude to them.");
            u_fac->showunitattitudes = 1;
        }
        return;
    }

    if (*token == "dontshowattitudes") {
        delete token;
        if (!pCheck) {
            u_fac->Event("Units will now have a leading minus sign regardless"
                        " of your attitude to them.");
            u_fac->showunitattitudes = 0;
        }
        return;
    }

    if (*token == "template") {
        delete token;

        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, u, 0, "OPTION: No template type specified.");
            return;
        }

        int newformat = -1;
        if (*token == "off") {
            newformat = TEMPLATE_OFF;
        }
        if (*token == "short") {
            newformat = TEMPLATE_SHORT;
        }
        if (*token == "long") {
            newformat = TEMPLATE_LONG;
        }
        // DK
        if (*token == "map") {
            newformat = TEMPLATE_MAP;
        }
        delete token;

        if (newformat == -1) {
            ParseError(pCheck, u, 0, "OPTION: Invalid template type.");
            return;
        }

        if (!pCheck) {
            u_fac->temformat = newformat;
        }

        return;
    }

    delete token;

    ParseError(pCheck, u, 0, "OPTION: Invalid option.");
}

void Game::ProcessReshowOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    Skills sk;
    size_t lvl;
    AString *token;
    
    token = o->gettoken();
    if (!token) {
        // LLS
        ParseError(pCheck, u, 0, "SHOW: Show what?");
        return;
    }

    const auto u_fac = u->faction.lock();
    if (pCheck) {
        if (pCheck->numshows++ > 100) {
            if (pCheck->numshows == 102) {
                ParseError(pCheck, 0, 0, "Too many SHOW orders.");
            }
            return;
        }
    } else {
        if (u_fac->numshows++ > 100) {
            if (u_fac->numshows == 102) {
                u->Error("Too many SHOW orders.");
            }
            return;
        }
    }

    if (*token == "skill") {
        delete token;

        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, u, 0, "SHOW: Show what skill?");
            return;
        }
        sk = ParseSkill(token);
        delete token;

        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, u, 0, "SHOW: No skill level given.");
            return;
        }
        lvl = static_cast<size_t>(token->value());
        delete token;

        if (!pCheck) {
            if (!sk.isValid() ||
                    SkillDefs[sk].flags & SkillType::DISABLED ||
                    (SkillDefs[sk].flags & SkillType::APPRENTICE &&
                                                     !Globals->APPRENTICES_EXIST) ||
                    lvl > u_fac->skills.GetDays(sk)) {
                u->Error("SHOW: Faction doesn't have that skill.");
                return;
            }

            u_fac->shows.emplace_back(std::make_shared<ShowSkill>(sk, lvl));
        }
        return;
    }

    if (*token == "object") {
        delete token;
        token = o->gettoken();

        if (!token) {
            ParseError(pCheck, u, 0, "SHOW: Show which object?");
            return;
        }

        ssize_t obj = ParseShipObject(token);
        delete token;

        if (!pCheck && obj >= -1) {
            const Objects obj2 = Objects(obj);
            if (obj == -1 ||
                    obj2 == Objects::Types::O_DUMMY ||
                    (ObjectDefs[obj2].flags & ObjectType::DISABLED)) {
                u->Error("SHOW: No such object.");
                return;
            }
            u_fac->objectshows.Add(ObjectDescription(obj2));
        }
        if (obj >= -1)
            return;
        token = new AString("item");
        o = new AString(ItemDefs[static_cast<size_t>(-(obj + 1))].abr);
    }

    if (*token == "item") {
        delete token;
        token = o->gettoken();

        if (!token) {
            ParseError(pCheck, u, 0, "SHOW: Show which item?");
            return;
        }

        const Items item = ParseEnabledItem(token);
        delete token;

        if (!pCheck) {
            if (!item.isValid() || (ItemDefs[item].flags & ItemType::DISABLED)) {
                u->Error("SHOW: You don't know anything about that item.");
                return;
            }
            if (ItemDefs[item].pSkill) {
                token = new AString(ItemDefs[item].pSkill);
                sk = LookupSkill(*token);
                delete token;
                if (ItemDefs[item].pLevel <= u_fac->skills.GetDays(sk)) {
                    u_fac->DiscoverItem(item, 1, 1);
                    return;
                }
            }
            if (ItemDefs[item].mSkill) {
                token = new AString(ItemDefs[item].mSkill);
                sk = LookupSkill(*token);
                delete token;
                if (static_cast<size_t>(ItemDefs[item].mLevel) <= u_fac->skills.GetDays(sk)) {
                    u_fac->DiscoverItem(item, 1, 1);
                    return;
                }
            }
            if (u_fac->items.GetNum(item)) {
                u_fac->DiscoverItem(item, 1, 0);
                return;
            }
            if (ItemDefs[item].type & (IT_MAN | IT_NORMAL | IT_TRADE | IT_MONSTER)) {
                u_fac->DiscoverItem(item, 1, 0);
                return;
            }

            u->Error("SHOW: You don't know anything about that item.");
        }
        return;
    }

    ParseError(pCheck, u, 0, "SHOW: Show what?");
}

void Game::ProcessForgetOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "FORGET: No skill given.");
        return;
    }

    const Skills sk = ParseSkill(token);
    delete token;

    if (!sk.isValid()) {
        ParseError(pCheck, u, 0, "FORGET: Invalid skill.");
        return;
    }

    if (!pCheck) {
        auto& ord = u->forgetorders.emplace_back(std::make_shared<ForgetOrder>());
        ord->skill = sk;
    }
}

void Game::ProcessEntertainOrder(const Unit::Handle& unit, const OrdersCheck::Handle& pCheck)
{
    if (unit->monthorders ||
            (Globals->TAX_PILLAGE_MONTH_LONG &&
                ((unit->taxing == TAX_TAX) ||
                    (unit->taxing == TAX_PILLAGE)))) {
        AString err = "ENTERTAIN: Overwriting previous ";
        if (unit->inTurnBlock)
        {
            err += "DELAYED ";
        }
        err += "month-long order.";
        ParseError(pCheck, unit, 0, err);
    }
    ProduceOrder::Handle o = std::make_shared<ProduceOrder>();
    o->item = Items::Types::I_SILVER;
    o->skill = Skills::Types::S_ENTERTAINMENT;
    o->target = 0;
    unit->monthorders = o;
}

void Game::ProcessCombatOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        if (!pCheck) {
            u->combat.invalidate();
            u->Event("Combat spell set to none.");
        }
        return;
    }
    const Skills sk = ParseSkill(token);
    delete token;

    if (!pCheck) {
        if (!sk.isValid()) {
            ParseError(pCheck, u, 0, "COMBAT: Invalid skill.");
            return;
        }
        if (!(SkillDefs[sk].flags & SkillType::MAGIC)) {
            ParseError(pCheck, u, 0, "COMBAT: That is not a magic skill.");
            return;
        }
        if (!(SkillDefs[sk].flags & SkillType::COMBAT)) {
            ParseError(pCheck, u, 0,
                    "COMBAT: That skill cannot be used in combat.");
            return;
        }

        if (u->type != U_MAGE) {
            u->Error("COMBAT: That unit is not a mage.");
            return;
        }
        if (!u->GetSkill(sk)) {
            u->Error("COMBAT: Unit does not possess that skill.");
            return;
        }

        u->combat = sk;
        AString temp = AString("Combat spell set to ") + SkillDefs[sk].name;
        if (Globals->USE_PREPARE_COMMAND) {
            u->readyItem.invalidate();
            temp += " and prepared item set to none";
        }
        temp += ".";

        u->Event(temp);
    }
}

// Lacandon's prepare command
void Game::ProcessPrepareOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if (!(Globals->USE_PREPARE_COMMAND)) {
        ParseError(pCheck, u, 0, "PREPARE is not a valid order.");
        return;
    }

    AString *token = o->gettoken();
    if (!token) {
        if (!pCheck) {
            u->readyItem.invalidate();
            u->Event("Prepared battle item set to none.");
        }
        return;
    }

    const Items it = ParseEnabledItem(token);

    try
    {
        const auto& bt = FindBattleItem(token->Str());
        delete token;
        if (!pCheck) {
            AString temp;
            if (!it.isValid() || !u->items.GetNum(it)) {
                u->Error("PREPARE: Unit does not possess that item.");
                return;
            }

            if (!(bt.flags & BattleItemType::SPECIAL)) {
                u->Error("PREPARE: That item cannot be prepared.");
                return;
            }

            if ((bt.flags & BattleItemType::MAGEONLY) &&
                !((u->type == U_MAGE) || (u->type == U_APPRENTICE) ||
                    (u->type == U_GUARDMAGE))) {
                temp = "PREPARE: Only a mage";
                if (Globals->APPRENTICES_EXIST) {
                    temp += " or ";
                    temp += Globals->APPRENTICE_NAME;
                }
                temp += " may use this item.";
                u->Error(temp);
                return;
            }
            u->readyItem = it;
            temp = AString("Prepared item set to ") + ItemDefs[it].name;
            if (u->combat.isValid()) {
                u->combat.invalidate();
                temp += " and combat spell set to none";
            }
            temp += ".";
            u->Event(temp);
        }
    }
    catch(const NoSuchItemException&)
    {
        if(!pCheck)
        {
            u->Error("PREPARE: That item cannot be prepared.");
            return;
        }
    }
}

void Game::ProcessWeaponOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if (!(Globals->USE_WEAPON_ARMOR_COMMAND)) {
        ParseError(pCheck, u, 0, "WEAPON is not a valid order.");
        return;
    }

    AString *token = o->gettoken();
    if (!token) {
        if (!pCheck) {
            for (auto& r: u->readyWeapon)
            {
                r.invalidate();
            }
            u->Event("Preferred weapons set to none.");
        }
        return;
    }
    if (pCheck) {
        delete token;
        return;
    }
    Items items[MAX_READY];
    size_t i = 0;
    const auto u_fac = u->faction.lock();
    while (token && (i < MAX_READY)) {
        const Items it = ParseEnabledItem(token);
        delete token;
        if (!it.isValid() || u_fac->items.GetNum(it) < 1) {
            u->Error("WEAPON: Unknown item.");
        } else if (!(ItemDefs[it].type & IT_WEAPON)) {
            u->Error("WEAPON: Item is not a weapon.");
        } else {
            if (!pCheck) items[i++] = it;
        }
        token = o->gettoken();
    }
    if (token) delete token;

    while (i < MAX_READY) {
        items[i++].invalidate();
    }
    if (!items[0].isValid())
    {
        return;
    }
    AString temp = "Preferred weapons set to: ";
    for (i = 0; i < MAX_READY; ++i) {
        u->readyWeapon[i] = items[i];
        if (items[i].isValid()) {
            if (i > 0) temp += ", ";
            temp += ItemDefs[items[i]].name;
        }
    }
    temp += ".";
    u->Event(temp);
}

void Game::ProcessArmorOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if (!(Globals->USE_WEAPON_ARMOR_COMMAND)) {
        ParseError(pCheck, u, 0, "ARMOR is not a valid order.");
        return;
    }

    AString *token = o->gettoken();
    if (!token) {
        if (!pCheck) {
            for (auto& r: u->readyArmor)
            {
                r.invalidate();
            }
            u->Event("Preferred armor set to none.");
        }
        return;
    }
    if (pCheck) {
        delete token;
        return;
    }
    Items items[MAX_READY];
    size_t i = 0;
    const auto u_fac = u->faction.lock();
    while (token && (i < MAX_READY)) {
        const Items it = ParseEnabledItem(token);
        delete token;
        if (!it.isValid() || u_fac->items.GetNum(it) < 1) {
            u->Error("ARMOR: Unknown item.");
        } else if (!(ItemDefs[it].type & IT_ARMOR)) {
            u->Error("ARMOR: Item is not armor.");
        } else {
            if (!pCheck) items[i++] = it;
        }
        token = o->gettoken();
    }
    if (token) delete token;

    while (i < MAX_READY) {
        items[i++].invalidate();
    }
    if (!items[0] .isValid()) return;
    AString temp = "Preferred armor set to: ";
    for (i=0; i<MAX_READY;++i) {
        u->readyArmor[i] = items[i];
        if (items[i].isValid()) {
            if (i > 0) temp += ", ";
            temp += ItemDefs[items[i]].name;
        }
    }
    temp += ".";
    u->Event(temp);
}

void Game::ProcessClaimOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "CLAIM: No amount given.");
        return;
    }

    int value = token->value();
    delete token;
    if (!value) {
        ParseError(pCheck, u, 0, "CLAIM: No amount given.");
        return;
    }

    if (!pCheck) {
        const auto u_fac = u->faction.lock();
        if (value > static_cast<int>(u_fac->unclaimed)) {
            u->Error("CLAIM: Don't have that much unclaimed silver.");
            value = static_cast<int>(u_fac->unclaimed);
        }
        u_fac->unclaimed -= static_cast<size_t>(value);
        u->SetMoney(u->GetMoney() + value);
        u_fac->DiscoverItem(Items::Types::I_SILVER, 0, 1);
        u->Event(AString("Claims $") + value + ".");
    }
}

void Game::ProcessFactionOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if (Globals->FACTION_LIMIT_TYPE != GameDefs::FACLIM_FACTION_TYPES) {
        ParseError(pCheck, u, 0,
                "FACTION: Invalid order, no faction types in this game.");
        return;
    }

    int oldfactype[NFACTYPES];
    int factype[NFACTYPES];

    const auto u_fac = u->faction.lock();
    if (!pCheck) {
        for (size_t i = 0; i < NFACTYPES; i++) {
            oldfactype[i] = u_fac->type[i];
        }
    }

    int retval = ParseFactionType(o, factype);
    if (retval == -1) {
        ParseError(pCheck, u, 0, "FACTION: Bad faction type.");
        return;
    }

    if (!pCheck) {
        const size_t m = CountMages(u_fac);
        const size_t a = CountApprentices(u_fac);

        for (size_t i = 0; i < NFACTYPES; i++)
        {
            u_fac->type[i] = factype[i];
        }

        if (m > AllowedMages(*u_fac)) {
            u->Error(AString("FACTION: Too many mages to change to that "
                             "faction type."));

            for (size_t i = 0; i < NFACTYPES; i++)
            {
                u_fac->type[i] = oldfactype[i];
            }

            return;
        }

        if (a > AllowedApprentices(*u_fac)) {
            u->Error(AString("FACTION: Too many ") +
                Globals->APPRENTICE_NAME +
                "s to change to that "
                 "faction type.");

            for (size_t i = 0; i < NFACTYPES; i++)
            {
                u_fac->type[i] = oldfactype[i];
            }

            return;
        }

        if (Globals->FACTION_LIMIT_TYPE == GameDefs::FACLIM_FACTION_TYPES) {
            const size_t q = CountQuarterMasters(u_fac);
            if ((Globals->TRANSPORT & GameDefs::ALLOW_TRANSPORT) &&
                    (q > AllowedQuarterMasters(*u_fac))) {
                u->Error(AString("FACTION: Too many quartermasters to "
                            "change to that faction type."));

                for (size_t i = 0; i < NFACTYPES; i++)
                {
                    u_fac->type[i] = oldfactype[i];
                }

                return;
            }
        }

        u_fac->lastchange = static_cast<int>(TurnNumber());
        u_fac->DefaultOrders();
    }
}

void Game::ProcessAssassinateOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    UnitId::Handle id = ParseUnit(o);
    if (!id || !id->unitnum.isValid()) {
        ParseError(pCheck, u, 0, "ASSASSINATE: No target given.");
        return;
    }
    if (!pCheck) {
        AssassinateOrder::Handle ord = std::make_shared<AssassinateOrder>();
        ord->target = *id;
        u->stealorders = ord;
    }
}

void Game::ProcessStealOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    UnitId::Handle id = ParseUnit(o);
    if (!id || !id->unitnum.isValid()) {
        ParseError(pCheck, u, 0, "STEAL: No target given.");
        return;
    }
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "STEAL: No item given.");
        return;
    }
    const Items i = ParseEnabledItem(token);
    delete token;
    if (!pCheck) {
        if (!i.isValid()) {
            u->Error("STEAL: Bad item given.");
            return;
        }

        if (IsSoldier(i)) {
            u->Error("STEAL: Can't steal that.");
            return;
        }
        StealOrder::Handle ord = std::make_shared<StealOrder>();
        ord->target = *id;
        ord->item = i;
        u->stealorders = ord;
    }
}

void Game::ProcessQuitOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if (!pCheck) {
        const auto u_fac = u->faction.lock();
        if (u_fac->password && !(*(u_fac->password) == "none")) {
            AString *token = o->gettoken();
            if (!token) {
                u_fac->Error("QUIT: Must give the correct password.");
                return;
            }

            if (!(*token == *(u_fac->password))) {
                delete token;
                u_fac->Error("QUIT: Must give the correct password.");
                return;
            }

            delete token;
        }

        if (u_fac->quit != QUIT_AND_RESTART) {
            u_fac->quit = QUIT_BY_ORDER;
        }
    }
}

void Game::ProcessRestartOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if (!pCheck) {
        const auto u_fac = u->faction.lock();
        if (u_fac->password && !(*(u_fac->password) == "none")) {
            AString *token = o->gettoken();
            if (!token) {
                u_fac->Error("RESTART: Must give the correct password.");
                return;
            }

            if (!(*token == *(u_fac->password))) {
                delete token;
                u_fac->Error("RESTART: Must give the correct password.");
                return;
            }

            delete token;
        }

        if (u_fac->quit != QUIT_AND_RESTART) {
            u_fac->quit = QUIT_AND_RESTART;
            Faction::Handle pFac = AddFaction(0, NULL);
            pFac->SetAddress(*(u_fac->address));
            AString *pass = new AString(*(u_fac->password));
            pFac->password = pass;
            AString facstr = AString("Restarting ") + *(pFac->address) + ".";
            newfactions.push_back(facstr);
        }
    }
}

void Game::ProcessDestroyOrder(const Unit::Handle& u, const OrdersCheck::Handle& pCheck)
{
    if (!pCheck) {
        u->destroy = 1;
    }
}

void Game::ProcessFindOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "FIND: No faction number given.");
        return;
    }
    int n = token->value();
    int is_all = (*token == "all");
    delete token;
    if (n==0 && !is_all) {
        ParseError(pCheck, u, 0, "FIND: No faction number given.");
        return;
    }
    if (!pCheck) {
        auto& f = u->findorders.emplace_back(std::make_shared<FindOrder>());
        f->find = n;
    }
}

void Game::ProcessConsumeOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (token) {
        if (*token == "unit") {
            if (!pCheck) {
                u->SetFlag(FLAG_CONSUMING_UNIT, 1);
                u->SetFlag(FLAG_CONSUMING_FACTION, 0);
            }
            delete token;
            return;
        }

        if (*token == "faction") {
            if (!pCheck) {
                u->SetFlag(FLAG_CONSUMING_UNIT, 0);
                u->SetFlag(FLAG_CONSUMING_FACTION, 1);
            }
            delete token;
            return;
        }

        if (*token == "none") {
            if (!pCheck) {
                u->SetFlag(FLAG_CONSUMING_UNIT, 0);
                u->SetFlag(FLAG_CONSUMING_FACTION, 0);
            }
            delete token;
            return;
        }

        delete token;
        ParseError(pCheck, u, 0, "CONSUME: Invalid value.");
    } else {
        if (!pCheck) {
            u->SetFlag(FLAG_CONSUMING_UNIT, 0);
            u->SetFlag(FLAG_CONSUMING_FACTION, 0);
        }
    }
}

void Game::ProcessRevealOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (token) {
        if (*token == "unit") {
            u->reveal = REVEAL_UNIT;
            delete token;
            return;
        }
        if (*token == "faction") {
            delete token;
            u->reveal = REVEAL_FACTION;
            return;
        }
        if (*token == "none") {
            delete token;
            u->reveal = REVEAL_NONE;
            return;
        }
        ParseError(pCheck, u, 0, "REVEAL: Invalid value.");
    } else {
        u->reveal = REVEAL_NONE;
    }
}

void Game::ProcessTaxOrder(const Unit::Handle& u, const OrdersCheck::Handle& pCheck)
{
    if (u->taxing == TAX_PILLAGE) {
        ParseError(pCheck, u, 0, "TAX: The unit is already pillaging.");
        return;
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG && u->monthorders) {
        u->monthorders.reset();
        AString err = "TAX: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
    }

    u->taxing = TAX_TAX;
}

void Game::ProcessPillageOrder(const Unit::Handle& u, const OrdersCheck::Handle& pCheck)
{
    if (u->taxing == TAX_TAX) {
        ParseError(pCheck, u, 0, "PILLAGE: The unit is already taxing.");
        return;
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG && u->monthorders) {
        u->monthorders.reset();
        AString err = "PILLAGE: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
    }

    u->taxing = TAX_PILLAGE;
}

void Game::ProcessPromoteOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    UnitId::Handle id = ParseUnit(o);
    if (!id || !id->unitnum.isValid()) {
        ParseError(pCheck, u, 0, "PROMOTE: No target given.");
        return;
    }
    if (!pCheck) {
        u->promote = id;
    }
}

void Game::ProcessLeaveOrder(const Unit::Handle& u, const OrdersCheck::Handle& pCheck)
{
    if (!pCheck) {    
        // if the unit isn't already trying to enter a building,
        // then set it to leave.
        if (u->enter == 0) u->enter = -1;
    }
}

void Game::ProcessEnterOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token;
    int i = 0;

    token = o->gettoken();
    if (token) {
        i = token->value();
        delete token;
    }
    if (i) {
        if (!pCheck) {
            u->enter = i;
        }
    } else {
        ParseError(pCheck, u, 0, "ENTER: No object specified.");
    }
}

void Game::ProcessBuildOrder(const Unit::Handle& unit, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString * token = o->gettoken();
    BuildOrder::Handle order = std::make_shared<BuildOrder>();
    int maxbuild, i;

    // 'incomplete' for ships:
    maxbuild = 0;
    unit->build = 0;
    
    if (token) {
        if (*token == "help") {
            // "build help unitnum"
            UnitId::Handle targ = nullptr;
            delete token;
            if (!pCheck) {
                targ = ParseUnit(o);
                if (!targ) {
                    unit->Error("BUILD: Non-existent unit to help.");
                    return;
                }
                if (!targ->unitnum.isValid()) {
                    unit->Error("BUILD: Non-existent unit to help.");
                    return;
                }
            }
            order->target = *targ;    // set the order's target to the unit number helped
        } else {
            // token exists and != "help": must be something like 'build tower'
            const ssize_t ot = ParseShipObject(token);
            delete token;
            if (ot == -1) {
                ParseError(pCheck, unit, 0, "BUILD: Not a valid object name.");
                return;
            }
            
            if (!pCheck) {
                const auto u_obj = unit->object.lock();
                const auto reg = u_obj->region.lock();
                if (TerrainDefs[reg->type].similar_type == Regions::Types::R_OCEAN){
                    unit->Error("BUILD: Can't build in an ocean.");
                    return;
                }
                
                if (ot < 0) {
                    /* Build SHIP item */
                    const Items st(static_cast<size_t>(abs(ot+1)));
                    if (ItemDefs[st].flags & ItemType::DISABLED) {
                        ParseError(pCheck, unit, 0, "BUILD: Not a valid object name.");
                        return;
                    }
                    const auto& flying = ItemDefs[st].fly;
                    if (!reg->IsCoastalOrLakeside() && (flying <= 0)) {
                        unit->Error("BUILD: Can't build ship in "
                                "non-coastal or lakeside region.");
                        return;
                    }
                    unit->build = -static_cast<int>(st);
                    maxbuild = ItemDefs[st].pMonths;
                    // if we already have an unfinished
                    // ship, see how much work is left
                    if (unit->items.GetNum(st) > 0)
                        maxbuild = static_cast<int>(unit->items.GetNum(st));
                    // Don't create a fleet yet    
                } else {
                    const size_t ot2 = static_cast<size_t>(ot);
                    /* build standard OBJECT */
                    if (ObjectDefs[ot2].flags & ObjectType::DISABLED) {
                        ParseError(pCheck, unit, 0, "BUILD: Not a valid object name.");
                        return;
                    }
                    if (!(ObjectDefs[ot2].flags & ObjectType::CANENTER)) {
                        ParseError(pCheck, unit, 0, "BUILD: Can't build that.");
                        return;
                    }
                    AString skname = ObjectDefs[ot2].skill;
                    const Skills sk = LookupSkill(skname);
                    if (!sk.isValid()) {
                        ParseError(pCheck, unit, 0, "BUILD: Can't build that.");
                        return;
                    }
                    for (i = 1; i < 100; i++)
                    {
                        if (reg->GetObject(i).expired())
                        {
                            break;
                        }
                    }
                    if (i < 100) {
                        auto& obj = reg->objects.emplace_back(std::make_shared<Object>(reg));
                        obj->type = ot2;
                        obj->incomplete = ObjectDefs[obj->type].cost;
                        obj->num = i;
                        obj->SetName(new AString("Building"));
                        unit->build = obj->num;
                        unit->MoveUnit(obj);
                    } else {
                        unit->Error("BUILD: The region is full.");
                        return;
                    }
                }
            }
            order->target.invalidate(); // Not helping anyone...
        }
    } else {
        // just a 'build' order
        order->target.invalidate();
        if (!pCheck) {
            // look for an incomplete ship type in inventory
            ssize_t st = static_cast<ssize_t>(Objects::Types::O_DUMMY);
            for(const auto& it: unit->items) {
                if ((ItemDefs[it->type].type & IT_SHIP)
                    && (!(ItemDefs[it->type].flags & ItemType::DISABLED))) {
                        st = -static_cast<ssize_t>(it->type);
                        break;
                }
            }
            
            if (st == static_cast<ssize_t>(Objects::Types::O_DUMMY)) {
                // Build whatever we happen to be in when
                // we get to the build phase
                unit->build = 0;
            } else {
                unit->build = static_cast<int>(st);
                maxbuild = static_cast<int>(unit->items.GetNum(Items(-st)));
            }
        }
    }
    // set neededtocomplete
    if (maxbuild != 0) order->needtocomplete = maxbuild;
    
    
    // Now do all of the generic bits...
    // Check that the unit isn't doing anything else important
    if (unit->monthorders ||
            (Globals->TAX_PILLAGE_MONTH_LONG &&
                ((unit->taxing == TAX_TAX) || 
                    (unit->taxing == TAX_PILLAGE)))) {
        AString err = "BUILD: Overwriting previous ";
        if (unit->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, unit, 0, err);
    }
    
    // reset their taxation status if taxing is a month-long order
    if (Globals->TAX_PILLAGE_MONTH_LONG) unit->taxing = TAX_NONE;
    unit->monthorders = order;
}

void Game::ProcessAttackOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    UnitId::Handle id = ParseUnit(o);
    while (id && id->unitnum.isValid()) {
        if (!pCheck) {
            if (!u->attackorders)
            {
                u->attackorders = std::make_shared<AttackOrder>();
            }
            u->attackorders->targets.push_back(*id);
        }
        id = ParseUnit(o);
    }
}

void Game::ProcessSellOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "SELL: Number to sell not given.");
        return;
    }
    int num = 0;
    if (*token == "ALL") {
        num = -1;
    } else {
        num = token->value();
    }
    delete token;
    if (!num) {
        ParseError(pCheck, u, 0, "SELL: Number to sell not given.");
        return;
    }
    token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "SELL: Item not given.");
        return;
    }
    const Items it = ParseGiveableItem(token);
    delete token;

    if (!pCheck) {
        auto& s = u->sellorders.emplace_back(std::make_shared<SellOrder>());
        s->item = it;
        s->num = num;
    }
}

void Game::ProcessBuyOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "BUY: Number to buy not given.");
        return;
    }
    int num = 0;
    if (*token == "ALL") {
        num = -1;
    } else {
        num = token->value();
    }
    delete token;
    if (!num) {
        ParseError(pCheck, u, 0, "BUY: Number to buy not given.");
        return;
    }
    token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "BUY: Item not given.");
        return;
    }
    Items it = ParseGiveableItem(token);
    if (!it.isValid()) {
        if (*token == "peasant" || *token == "peasants" || *token == "peas") {
            if (pCheck) {
                it.invalidate();
                for (auto i = Items::begin(); i != Items::end(); ++i) {
                    if (ItemDefs[*i].flags & ItemType::DISABLED) continue;
                    if (ItemDefs[*i].type & IT_LEADER) continue;
                    if (ItemDefs[*i].type & IT_MAN) {
                        it = *i;
                        break;
                    }
                }
            } else {
                it = u->object.lock()->region.lock()->race;
            }
        }
    }
    delete token;

    if (!pCheck) {
        auto& b = u->buyorders.emplace_back(std::make_shared<BuyOrder>());
        b->item = it;
        b->num = num;
    }
}

void Game::ProcessProduceOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    int target = 0;
    AString *token = o->gettoken();

    if (token && token->value() > 0)
    {
        target = token->value();
        token = o->gettoken();
    }
    if (!token) {
        ParseError(pCheck, u, 0, "PRODUCE: No item given.");
        return;
    }
    const Items it = ParseEnabledItem(token);
    delete token;

    ProduceOrder::Handle p = std::make_shared<ProduceOrder>();
    p->item = it;
    if (it.isValid()) {
        AString skname = ItemDefs[it].pSkill;
        p->skill = LookupSkill(skname);
    } else {
        p->skill.invalidate();
    }
    p->target = target;
    if (u->monthorders ||
        (Globals->TAX_PILLAGE_MONTH_LONG &&
         ((u->taxing == TAX_TAX) || (u->taxing == TAX_PILLAGE)))) {
        AString err = "PRODUCE: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG) u->taxing = TAX_NONE;
    u->monthorders = p;
}

void Game::ProcessWorkOrder(const Unit::Handle& u, bool quiet, const OrdersCheck::Handle& pCheck)
{
    ProduceOrder::Handle order = std::make_shared<ProduceOrder>();
    order->skill.invalidate();
    order->item = Items::Types::I_SILVER;
    order->target = 0;
    if (quiet)
    {
        order->quiet = true;
    }
    if (u->monthorders ||
        (Globals->TAX_PILLAGE_MONTH_LONG &&
         ((u->taxing == TAX_TAX) || (u->taxing == TAX_PILLAGE)))) {
        AString err = "WORK: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG) u->taxing = TAX_NONE;
    u->monthorders = order;
}

void Game::ProcessTeachOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    TeachOrder::Handle order = nullptr;

    if (u->monthorders && u->monthorders->type == Orders::Types::O_TEACH) {
        order = std::dynamic_pointer_cast<TeachOrder>(u->monthorders);
    } else {
        order = std::make_shared<TeachOrder>();
    }

    int students = 0;
    UnitId::Handle id = ParseUnit(o);
    while (id && id->unitnum.isValid()) {
        students++;
        if (order) {
            order->targets.push_back(*id);
        }
        id = ParseUnit(o);
    }

    if (!students) {
        ParseError(pCheck, u, 0, "TEACH: No students given.");
        return;
    }

    if ((u->monthorders && u->monthorders->type != Orders::Types::O_TEACH) ||
        (Globals->TAX_PILLAGE_MONTH_LONG &&
         ((u->taxing == TAX_TAX) || (u->taxing == TAX_PILLAGE)))) {
        AString err = "TEACH: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG) u->taxing = TAX_NONE;
    u->monthorders = order;
}

void Game::ProcessStudyOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "STUDY: No skill given.");
        return;
    }
    Skills sk = ParseSkill(token);
    delete token;

    StudyOrder::Handle order = std::make_shared<StudyOrder>();
    order->skill = sk;
    order->days = 0;
    // parse study level:
    token = o->gettoken();
    if (token)
    {
        order->level = static_cast<unsigned int>(token->value());
        delete token;
    }
    else
    {
        order->level.invalidate();
    }
    
    if (u->monthorders ||
        (Globals->TAX_PILLAGE_MONTH_LONG &&
         ((u->taxing == TAX_TAX) || (u->taxing == TAX_PILLAGE)))) {
        AString err = "STUDY: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG) u->taxing = TAX_NONE;
    u->monthorders = order;
}

void Game::ProcessDeclareOrder(const Faction::Handle& f, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, 0, f, "DECLARE: No faction given.");
        return;
    }
    ValidValue<size_t> fac;
    if (*token != "default") {
        fac = static_cast<size_t>(token->value());
    }
    delete token;

    if (!pCheck) {
        Faction::Handle target;
        if (fac.isValid()) {
            target = GetFaction(factions, fac);
            if (!target) {
                f->Error(AString("DECLARE: Non-existent faction ") + static_cast<int>(fac) + ".");
                return;
            }
            if (target == f) {
                f->Error(AString("DECLARE: Can't declare towards your own "
                                 "faction."));
                return;
            }
        }
    }

    token = o->gettoken();
    if (!token) {
        if (fac.isValid()) {
            if (!pCheck) {
                f->SetAttitude(fac, -1);
            }
        }
        return;
    }

    int att = ParseAttitude(token);
    delete token;
    if (att == -1) {
        ParseError(pCheck, 0, f, "DECLARE: Invalid attitude.");
        return;
    }

    if (!pCheck) {
        if (!fac.isValid()) {
            f->defaultattitude = att;
        } else {
            f->SetAttitude(fac, att);
        }
    }
}

void Game::ProcessWithdrawOrder(const Unit::Handle& unit, AString *o, const OrdersCheck::Handle& pCheck)
{
    if (!(Globals->ALLOW_WITHDRAW)) {
        ParseError(pCheck, unit, 0, "WITHDRAW is not a valid order.");
        return;
    }

    AString *token = o->gettoken();
    if (!token) {
        ParseError (pCheck, unit, 0, "WITHDRAW: No amount given.");
        return;
    }
    int amt = token->value();
    if (amt < 1) {
        amt = 1;
    } else {
        delete token;
        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, unit, 0, "WITHDRAW: No item given.");
            return;
        }
    }
    const Items item = ParseGiveableItem(token);
    delete token;

    if (!item.isValid()) {
        ParseError(pCheck, unit, 0, "WITHDRAW: Invalid item.");
        return;
    }
    if (ItemDefs[item].flags & ItemType::DISABLED) {
        ParseError(pCheck, unit, 0, "WITHDRAW: Invalid item.");
        return;
    }
    if (!(ItemDefs[item].type & IT_NORMAL)) {
        ParseError(pCheck, unit, 0, "WITHDRAW: Invalid item.");
        return;
    }
    if (item == Items::Types::I_SILVER) {
        ParseError(pCheck, unit, 0, "WITHDRAW: Invalid item.");
        return;
    }

    if (!pCheck) {
        auto& order = unit->withdraworders.emplace_back(std::make_shared<WithdrawOrder>());
        order->item = item;
        order->amount = amt;
    }
    return;
}

AString *Game::ProcessTurnOrder(const Unit::Handle& unit, Aorders& f, const OrdersCheck::Handle& pCheck, int repeat)
{
    int turnDepth = 1;
    int turnLast = 1;
    int formDepth = 0;
    TurnOrder::Handle tOrder = std::make_shared<TurnOrder>();
    tOrder->repeating = repeat;

    AString *order, *token;

    while (turnDepth) {
        // get the next line
        order = f.GetLine();
        if (!order) {
            // Fake end of commands to invoke appropriate processing
            order = new AString("#end");
        }
        AString    saveorder = *order;
        token = order->gettoken();

        if (token) {
            const Orders i = Parse1Order(token);
            switch (i.asEnum()) {
                case Orders::Types::O_TURN:
                    if (turnLast) {
                        ParseError(pCheck, unit, 0, "TURN: cannot nest.");
                        break;
                    }
                    turnDepth++;
                    tOrder->turnOrders.emplace_back(std::make_shared<AString>(saveorder));
                    turnLast = 1;
                    break;
                case Orders::Types::O_FORM:
                    if (!turnLast) {
                        ParseError(pCheck, unit, 0, "FORM: cannot nest.");
                        break;
                    }
                    turnLast = 0;
                    formDepth++;
                    tOrder->turnOrders.emplace_back(std::make_shared<AString>(saveorder));
                    break;
                case Orders::Types::O_ENDFORM:
                    if (turnLast) {
                        if (!(formDepth + (unit->former != 0))) {
                            ParseError(pCheck, unit, 0, "END: without FORM.");
                            break;
                        } else {
                            ParseError(pCheck, unit, 0, "TURN: without ENDTURN.");
                            if (!--turnDepth) {
                                unit->turnorders.push_back(tOrder);
                                return new AString(saveorder);
                            }
                        }
                    }
                    formDepth--;
                    tOrder->turnOrders.emplace_back(std::make_shared<AString>(saveorder));
                    turnLast = 1;
                    break;
                case Orders::Types::O_UNIT:
                case Orders::Types::O_END:
                    if (!turnLast)
                        ParseError(pCheck, unit, 0, "FORM: without END.");
                    while (--turnDepth) {
                        ParseError(pCheck, unit, 0, "TURN: without ENDTURN.");
                        ParseError(pCheck, unit, 0, "FORM: without END.");
                    }
                    ParseError(pCheck, unit, 0, "TURN: without ENDTURN.");
                    unit->turnorders.push_back(tOrder);
                    return new AString(saveorder);
                    break;
                case Orders::Types::O_ENDTURN:
                    if (!turnLast) {
                        ParseError(pCheck, unit, 0, "ENDTURN: without TURN.");
                    } else {
                        if (--turnDepth)
                        {
                            tOrder->turnOrders.emplace_back(std::make_shared<AString>(saveorder));
                        }
                        turnLast = 0;
                    }
                    break;
                default:
                    tOrder->turnOrders.emplace_back(std::make_shared<AString>(saveorder));
                    break;
            }
            if (!pCheck && unit->former && unit->former->format)
            {
                unit->former->oldorders.emplace_back(std::make_shared<AString>(saveorder));
            }
            delete token;
        }
        delete order;
    }

    unit->turnorders.push_back(tOrder);

    return nullptr;
}

void Game::ProcessExchangeOrder(const Unit::Handle& unit, AString *o, const OrdersCheck::Handle& pCheck)
{
    UnitId::Handle t = ParseUnit(o);
    if (!t) {
        ParseError(pCheck, unit, 0, "EXCHANGE: Invalid target.");
        return;
    }
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, unit, 0, "EXCHANGE: No amount given.");
        return;
    }

    const int amtGive = token->value();
    delete token;

    if (amtGive < 0) {
        ParseError(pCheck, unit, 0, "EXCHANGE: Illegal amount given.");
        return;
    }

    token = o->gettoken();
    if (!token) {
        ParseError(pCheck, unit, 0, "EXCHANGE: No item given.");
        return;
    }

    const Items itemGive = ParseGiveableItem(token);
    delete token;

    if (!itemGive.isValid()) {
        ParseError(pCheck, unit, 0, "EXCHANGE: Invalid item.");
        return;
    }
    if (ItemDefs[itemGive].type & IT_SHIP) {
        ParseError(pCheck, unit, 0, "EXCHANGE: Can't exchange ships.");
        return;
    }

    token = o->gettoken();
    if (!token) {
        ParseError(pCheck, unit, 0, "EXCHANGE: No amount expected.");
        return;
    }

    const int amtExpected = token->value();
    delete token;

    if (amtExpected < 0) {
        ParseError(pCheck, unit, 0, "EXCHANGE: Illegal amount given.");
        return;
    }

    token = o->gettoken();
    if (!token) {
        ParseError(pCheck, unit, 0, "EXCHANGE: No item expected.");
        return;
    }

    const Items itemExpected = ParseGiveableItem(token);
    delete token;

    if (!itemExpected.isValid()) {
        ParseError(pCheck, unit, 0, "EXCHANGE: Invalid item.");
        return;
    }
    if (ItemDefs[itemExpected].type & IT_SHIP) {
        ParseError(pCheck, unit, 0, "EXCHANGE: Can't exchange ships.");
        return;
    }

    if (!pCheck) {
        auto& order = unit->exchangeorders.emplace_back(std::make_shared<ExchangeOrder>());
        order->giveItem = itemGive;
        order->giveAmount = amtGive;
        order->expectAmount = amtExpected;
        order->expectItem = itemExpected;
        order->target = *t;
    }
}

void Game::ProcessGiveOrder(const Orders& order,
                            const Unit::Handle& unit,
                            AString *o,
                            const OrdersCheck::Handle& pCheck)
{
    AString *token, ord;
    int unfinished, amt, excpt;

    if (order == Orders::Types::O_TAKE) {
        ord = "TAKE";
        token = o->gettoken();
        if (!token || !(*token == "from")) {
            ParseError(pCheck, unit, 0, "TAKE: Missing FROM.");
            return;
        }
    } else
        ord = "GIVE";

    UnitId::Handle t = ParseUnit(o);
    if (!t) {
        ParseError(pCheck, unit, 0, ord + ": Invalid target.");
        return;
    }
    if (!t->unitnum.isValid() && order == Orders::Types::O_TAKE) {
        ParseError(pCheck, unit, 0, ord + ": Invalid target.");
        return;
    }
    token = o->gettoken();
    if (!token) {
        ParseError(pCheck, unit, 0, ord + ": No amount given.");
        return;
    }
    if (*token == "unit" && order == Orders::Types::O_GIVE) {
        amt = -1;
    } else if (*token == "all") {
        amt = -2;
    } else {
        amt = token->value();
        if (amt < 1) {
            ParseError(pCheck, unit, 0, ord + ": Illegal amount given.");
            return;
        }
    }
    delete token;
    Items item;
    ssize_t item_s = -1;
    unfinished = 0;
    if (amt != -1) {
        token = o->gettoken();
        if (token && *token == "unfinished") {
            unfinished = 1;
            token = o->gettoken();
        }
        if (token) {
            if (!t->unitnum.isValid())
            {
                item = ParseEnabledItem(token);
            }
            else
            {
                item = ParseGiveableItem(token);
            }

            item_s = item.isValid() ? static_cast<ssize_t>(item) : -1;
            if (amt == -2) {
                bool found = false;
                if (*token == "normal") {
                    item_s = -IT_NORMAL;
                    found = true;
                } else if (*token == "advanced") {
                    item_s = -IT_ADVANCED;
                    found = true;
                } else if (*token == "trade") {
                    item_s = -IT_TRADE;
                    found = true;
                } else if ((*token == "man") || (*token == "men")) {
                    item_s = -IT_MAN;
                    found = true;
                } else if ((*token == "monster") || (*token == "monsters")) {
                    item_s = -IT_MONSTER;
                    found = true;
                } else if (*token == "magic") {
                    item_s = -IT_MAGIC;
                    found = true;
                } else if ((*token == "weapon") || (*token == "weapons")) {
                    item_s = -IT_WEAPON;
                    found = true;
                } else if (*token == "armor") {
                    item_s = -IT_ARMOR;
                    found = true;
                } else if ((*token == "mount") || (*token == "mounts")) {
                    item_s = -IT_MOUNT;
                    found = true;
                } else if (*token == "battle") {
                    item_s = -IT_BATTLE;
                    found = true;
                } else if (*token == "special") {
                    item_s = -IT_SPECIAL;
                    found = true;
                } else if (*token == "food") {
                    item_s = -IT_FOOD;
                    found = true;
                } else if ((*token == "tool") || (*token == "tools")) {
                    item_s = -IT_TOOL;
                    found = true;
                } else if ((*token == "item_s") || (*token == "items")) {
                    item_s = -static_cast<ssize_t>(*Items::end());
                    found = true;
                } else if ((*token == "ship") || (*token == "ships")) {
                    item_s = -IT_SHIP;
                    found = true;
                } else if (item.isValid()) {
                    found = true;
                }
                if (!found) {
                    ParseError(pCheck, unit, 0,
                            ord + ": Invalid item or item class.");
                    return;
                }
            } else if (!item.isValid()) {
                ParseError(pCheck, unit, 0, ord + ": Invalid item.");
                return;
            }

            if(item_s >= 0)
            {
                item = static_cast<size_t>(item_s);
            }

            if (unfinished &&
                    item_s != -IT_SHIP &&
                    item_s != -static_cast<ssize_t>(*Items::end()) &&
                    !(item.isValid() &&
                    ItemDefs[item].type & IT_SHIP)) {
                ParseError(pCheck, unit, 0, ord + ": That item does not have an unfinished version.");
                return;
            }
        } else {
            ParseError(pCheck, unit, 0, ord + ": No item given.");
            return;
        }
        delete token;
    }

    token = o->gettoken();
    excpt = 0;
    if (token && *token == "except") {
        if (amt == -2) {
            delete token;
            if (item_s < 0) {
                ParseError(pCheck, unit, 0,
                        ord + ": EXCEPT only valid with specific items.");
                return;
            }
            token = o->gettoken();
            if (!token) {
                ParseError(pCheck, unit, 0, ord + ": EXCEPT requires a value.");
                return;
            }
            excpt = token->value();
            if (excpt <= 0) {
                ParseError(pCheck, unit, 0, ord + ": Invalid EXCEPT value.");
                return;
            }
        } else {
            ParseError(pCheck, unit, 0, ord + ": EXCEPT only valid with ALL");
            return;
        }
        delete token;
    }

    if (!pCheck) {
        auto& go = unit->giveorders.emplace_back(std::make_shared<GiveOrder>());
        go->type = order;
        go->item = item_s;
        go->target = *t;
        go->amount = amt;
        go->except = excpt;
        go->unfinished = unfinished;
    }
    return;
}

void Game::ProcessDescribeOrder(const Unit::Handle& unit, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, unit, 0, "DESCRIBE: No argument.");
        return;
    }
    if (*token == "unit") {
        delete token;
        token = o->gettoken();
        if (!pCheck) {
            unit->SetDescribe(token);
        }
        return;
    }
    if (*token == "ship" || *token == "building" || *token == "object" ||
        *token == "structure") {
        delete token;
        token = o->gettoken();
        if (!pCheck) {
            // ALT, 25-Jul-2000
            // Fix to prevent non-owner units from describing objects
            const auto u_obj = unit->object.lock();
            if (unit != u_obj->GetOwner().lock()) {
                unit->Error("DESCRIBE: Unit is not owner.");
                return;
            }
            u_obj->SetDescribe(token);
        }
        return;
    }
    ParseError(pCheck, unit, 0, "DESCRIBE: Can't describe that.");
}

void Game::ProcessNameOrder(const Unit::Handle& unit, AString *o, const OrdersCheck::Handle& pCheck)
{
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, unit, 0, "NAME: No argument.");
        return;
    }
    
    if (*token == "faction") {
        delete token;
        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, unit, 0, "NAME: No name given.");
            return;
        }
        if (!pCheck) {
            unit->faction.lock()->SetName(token);
        }
        return;
    }

    if (*token == "unit") {
        delete token;
        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, unit, 0, "NAME: No name given.");
            return;
        }
        if (!pCheck) {
            unit->SetName(token);
        }
        return;
    }

    const auto u_obj = unit->object.lock();
    if (*token == "building" || *token == "ship" || *token == "object" ||
        *token == "structure") {
        delete token;
        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, unit, 0, "NAME: No name given.");
            return;
        }
        if (!pCheck) {
            // ALT, 25-Jul-2000
            // Fix to prevent non-owner units from renaming objects
            if (unit != u_obj->GetOwner().lock()) {
                unit->Error("NAME: Unit is not owner.");
                return;
            }
            if (!u_obj->CanModify()) {
                unit->Error("NAME: Can't name this type of object.");
                return;
            }
            u_obj->SetName(token);
        }
        return;
    }

    // ALT, 26-Jul-2000
    // Allow some units to rename cities. Unit must be at least the owner
    // of tower to rename village, fort to rename town and castle to
    // rename city.
    if (*token == "village" || *token == "town" || *token == "city") {
        delete token;
        token = o->gettoken();

        if (!token) {
            ParseError(pCheck, unit, 0, "NAME: No name given.");
            return;
        }

        if (!pCheck) {
            if (!u_obj) {
                unit->Error("NAME: Unit is not in a structure.");
                return;
            }

            const auto u_obj_reg = u_obj->region.lock();
            if (!u_obj_reg->town) {
                unit->Error("NAME: Unit is not in a village, town or city.");
                return;
            }
            size_t cost = 0;
            const auto towntype = u_obj_reg->town->TownType();
            AString tstring;
            switch(towntype) {
                case TownTypeEnum::TOWN_VILLAGE:
                    tstring = "village";
                    break;
                case TownTypeEnum::TOWN_TOWN:
                    tstring = "town";
                    break;
                case TownTypeEnum::TOWN_CITY:
                    tstring = "city";
                    break;
                default:
                    break;
            }
            if (Globals->CITY_RENAME_COST) {
                cost = (static_cast<size_t>(towntype)+1)* Globals->CITY_RENAME_COST;
            }
            int ok = 0;
            switch(towntype) {
                case TownTypeEnum::TOWN_VILLAGE:
                    switch(u_obj->type.asEnum()) {
                        case Objects::Types::O_TOWER:
                        case Objects::Types::O_MTOWER:
                            ok = 1;
                        default:
                            break;
                    }
                    /* FALLTHRU */
                case TownTypeEnum::TOWN_TOWN:
                    switch(u_obj->type.asEnum()) {
                        case Objects::Types::O_FORT:
                        case Objects::Types::O_MFORTRESS:
                            ok = 1;
                        default:
                            break;
                    }
                    /* FALLTHRU */
                case TownTypeEnum::TOWN_CITY:
                    switch(u_obj->type.asEnum()) {
                        case Objects::Types::O_CASTLE:
                        case Objects::Types::O_CITADEL:
                        case Objects::Types::O_MCASTLE:
                        case Objects::Types::O_MCITADEL:
                            ok = 1;
                        default:
                            break;
                    }
                default:
                    break;
            }
            if (!ok) {
                unit->Error(AString("NAME: Unit is not in a large ")+
                            "enough structure to rename a "+tstring+".");
                return;
            }
            if (unit != u_obj->GetOwner().lock()) {
                unit->Error(AString("NAME: Cannot name ")+tstring+
                            ".  Unit is not the owner of object.");
                return;
            }
            if (u_obj->incomplete > 0) {
                unit->Error(AString("NAME: Cannot name ")+tstring+
                            ".  Object is not finished.");
                return;
            }

            AString *newname = token->getlegal();
            if (!newname) {
                unit->Error("NAME: Illegal name.");
                return;
            }
            if (cost) {
                size_t silver = unit->items.GetNum(Items::Types::I_SILVER);
                if (silver < cost) {
                    unit->Error(AString("NAME: Unit doesn't have enough ")+
                                "silver to rename a "+tstring+".");
                    return;
                }
                unit->items.SetNum(Items::Types::I_SILVER, silver-cost);
            }

            unit->Event(AString("Renames ") +
                    *(u_obj_reg->town->name) + " to " +
                    *newname + ".");
            u_obj_reg->NotifyCity(unit,
                    *(u_obj_reg->town->name), *newname);
            delete u_obj_reg->town->name;
            u_obj_reg->town->name = newname;
        }
        return;
    }

    delete token;
    ParseError(pCheck, unit, 0, "NAME: Can't name that.");
}

void Game::ProcessGuardOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* This is an instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "GUARD: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    delete token;
    if (val==-1) {
        ParseError(pCheck, u, 0, "GUARD: Invalid value.");
        return;
    }
    if (!pCheck) {
        if (val==0) {
            if (u->guard != GUARD_AVOID)
                u->guard = GUARD_NONE;
        } else {
            if (u->guard != GUARD_GUARD)
                u->guard = GUARD_SET;
        }
    }
}

void Game::ProcessBehindOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* This is an instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "BEHIND: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    if (val == -1) {
        ParseError(pCheck, u, 0, "BEHIND: Invalid value.");
        return;
    }
    if (!pCheck) {
        u->SetFlag(FLAG_BEHIND, val);
    }
}

void Game::ProcessNoaidOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* Instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "NOAID: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    delete token;
    if (val==-1) {
        ParseError(pCheck, u, 0, "NOAID: Invalid value.");
        return;
    }
    if (!pCheck) {
        u->SetFlag(FLAG_NOAID, val);
    }
}

void Game::ProcessSpoilsOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* Instant order */
    AString *token = o->gettoken();
    int flag = 0;
    int val = 1;
    if (token) {
        if (*token == "none") flag = FLAG_NOSPOILS;
        else if (*token == "walk") flag = FLAG_WALKSPOILS;
        else if (*token == "ride") flag = FLAG_RIDESPOILS;
        else if (*token == "fly") flag = FLAG_FLYSPOILS;
        else if (*token == "swim") flag = FLAG_SWIMSPOILS;
        else if (*token == "sail") flag = FLAG_SAILSPOILS;
        else if (*token == "all") val = 0;
        else ParseError(pCheck, u, 0, "SPOILS: Bad argument.");
        delete token;
    }

    if (!pCheck) {
        /* Clear all the flags */
        u->SetFlag(FLAG_NOSPOILS, 0);
        u->SetFlag(FLAG_WALKSPOILS, 0);
        u->SetFlag(FLAG_RIDESPOILS, 0);
        u->SetFlag(FLAG_FLYSPOILS, 0);
        u->SetFlag(FLAG_SWIMSPOILS, 0);
        u->SetFlag(FLAG_SAILSPOILS, 0);

        /* Set the flag we're trying to set */
        if (flag) {
            u->SetFlag(flag, val);
        }
    }
}

void Game::ProcessNospoilsOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    ParseError(pCheck, u, 0, "NOSPOILS: This command is deprecated.  "
            "Use the 'SPOILS' command instead");

    /* Instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "NOSPOILS: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    delete token;
    if (val==-1) {
        ParseError(pCheck, u, 0, "NOSPILS: Invalid value.");
        return;
    }
    if (!pCheck) {
        u->SetFlag(FLAG_FLYSPOILS, 0);
        u->SetFlag(FLAG_RIDESPOILS, 0);
        u->SetFlag(FLAG_WALKSPOILS, 0);
        u->SetFlag(FLAG_NOSPOILS, val);
    }
}

void Game::ProcessNocrossOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    bool move_over_water = false;

    if (Globals->FLIGHT_OVER_WATER != GameDefs::WFLIGHT_NONE)
    {
        move_over_water = true;
    }
    if (!move_over_water) {
        for (const auto& i: ItemDefs) {
            if (i.flags & ItemType::DISABLED)
            {
                continue;
            }
            if (i.swim > 0)
            {
                move_over_water = true;
            }
        }
    }
    if (!move_over_water) {
        ParseError(pCheck, u, 0, "NOCROSS is not a valid order.");
        return;
    }

    /* Instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "NOCROSS: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    delete token;
    if (val==-1) {
        ParseError(pCheck, u, 0, "NOCROSS: Invalid value.");
        return;
    }
    if (!pCheck) {
        u->SetFlag(FLAG_NOCROSS_WATER, val);
    }
}

void Game::ProcessHoldOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* Instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "HOLD: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    delete token;
    if (val==-1) {
        ParseError(pCheck, u, 0, "HOLD: Invalid value.");
        return;
    }
    if (!pCheck) {
        u->SetFlag(FLAG_HOLDING, val);
    }
}

void Game::ProcessAutoTaxOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* Instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "AUTOTAX: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    delete token;
    if (val==-1) {
        ParseError(pCheck, u, 0, "AUTOTAX: Invalid value.");
        return;
    }
    if (!pCheck) {
        u->SetFlag(FLAG_AUTOTAX, val);
    }
}

void Game::ProcessAvoidOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* This is an instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "AVOID: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    delete token;
    if (val==-1) {
        ParseError(pCheck, u, 0, "AVOID: Invalid value.");
        return;
    }
    if (!pCheck) {
        if (val==1) {
            u->guard = GUARD_AVOID;
        } else {
            if (u->guard == GUARD_AVOID) {
                u->guard = GUARD_NONE;
            }
        }
    }
}

Unit::Handle Game::ProcessFormOrder(const Unit::Handle& former,
                                    AString *o,
                                    const OrdersCheck::Handle& pCheck,
                                    int atsign)
{
    AString *t = o->gettoken();
    if (!t) {
        ParseError(pCheck, former, 0, "Must give alias in FORM order.");
        return nullptr;
    }

    int an = t->value();
    delete t;
    if (!an) {
        ParseError(pCheck, former, 0, "Must give alias in FORM order.");
        return nullptr;
    }
    if (pCheck) {
        Unit::Handle retval = std::make_shared<Unit>();
        retval->former = former;
        former->format = atsign;
        return retval;
    } else {
        const auto f_obj = former->object.lock();
        if (!f_obj->region.lock()->GetUnitAlias(an, former->faction.lock()->num).expired()) {
            former->Error("Alias multiply defined.");
            return nullptr;
        }
        Unit::Handle temp = GetNewUnit(former->faction.lock(), an);
        temp->CopyFlags(former);
        temp->DefaultOrders(f_obj);
        temp->MoveUnit(former->object);
        temp->former = former;
        former->format = atsign;
        return temp;
    }
}

void Game::ProcessAddressOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* This is an instant order */
    AString *token = o->gettoken();
    if (token) {
        if (!pCheck) {
            u->faction.lock()->address = token;
        }
    } else {
        ParseError(pCheck, u, 0, "ADDRESS: No address given.");
    }
}

void Game::ProcessAdvanceOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if ((u->monthorders && u->monthorders->type != Orders::Types::O_ADVANCE) ||
        (Globals->TAX_PILLAGE_MONTH_LONG &&
         ((u->taxing == TAX_TAX) || (u->taxing == TAX_PILLAGE)))) {
        AString err = "ADVANCE: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long orders";
        ParseError(pCheck, u, 0, err);
        u->monthorders.reset();
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG) u->taxing = TAX_NONE;
    if (!u->monthorders) {
        u->monthorders = std::make_shared<MoveOrder>();
        u->monthorders->type = Orders::Types::O_ADVANCE;
    }

    const auto m = std::dynamic_pointer_cast<MoveOrder>(u->monthorders);
    m->advancing = 1;

    for (;;) {
        AString *t = o->gettoken();
        if (!t) return;
        const Directions d = ParseDir(t);
        delete t;
        if (d.isValid()) {
            if (!pCheck) {
                auto& x = m->dirs.emplace_back(std::make_shared<MoveDir>());
                x->dir = d;
            }
        } else {
            ParseError(pCheck, u, 0, "ADVANCE: Warning, bad direction.");
            return;
        }
    }
}

void Game::ProcessMoveOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if ((u->monthorders && u->monthorders->type != Orders::Types::O_MOVE) ||
        (Globals->TAX_PILLAGE_MONTH_LONG &&
         ((u->taxing == TAX_TAX) || (u->taxing == TAX_PILLAGE)))) {
        AString err = "MOVE: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
        u->monthorders.reset();
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG) u->taxing = TAX_NONE;
    if (!u->monthorders) {
        u->monthorders = std::make_shared<MoveOrder>();
    }
    const auto m = std::dynamic_pointer_cast<MoveOrder>(u->monthorders);
    m->advancing = 0;

    for (;;) {
        AString *t = o->gettoken();
        if (!t) return;
        const Directions d = ParseDir(t);
        delete t;
        if (d.isValid()) {
            if (!pCheck) {
                auto& x = m->dirs.emplace_back(std::make_shared<MoveDir>());
                x->dir = d;
            }
        } else {
            ParseError(pCheck, u, 0, "MOVE: Warning, bad direction.");
            return;
        }
    }
}

void Game::ProcessSailOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    if ((u->monthorders && u->monthorders->type != Orders::Types::O_SAIL) ||
        (Globals->TAX_PILLAGE_MONTH_LONG &&
         ((u->taxing == TAX_TAX) || (u->taxing == TAX_PILLAGE)))) {
        AString err = "SAIL: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
        u->monthorders.reset();
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG) u->taxing = TAX_NONE;
    if (!u->monthorders) {
        u->monthorders = std::make_shared<SailOrder>();
    }

    const auto m = std::dynamic_pointer_cast<SailOrder>(u->monthorders);

    for (;;) {
        AString *t = o->gettoken();
        if (!t) return;
        const Directions d = ParseDir(t);
        delete t;
        if (!d.isValid()) {
            ParseError(pCheck, u, 0, "SAIL: Warning, bad direction.");
            return;
        } else {
            if (d.isRegularDirection() || d.isMovePause()) {
                if (!pCheck) {
                    auto& x = m->dirs.emplace_back(std::make_shared<MoveDir>());
                    x->dir = d;
                }
            } else {
                ParseError(pCheck, u, 0, "SAIL: Warning, bad direction.");
                return;
            }
        }
    }
}

void Game::ProcessEvictOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    UnitId::Handle id = ParseUnit(o);
    while (id && id->unitnum.isValid()) {
        if (!pCheck) {
            if (!u->evictorders)
            {
                u->evictorders = std::make_shared<EvictOrder>();
            }
            u->evictorders->targets.push_back(*id);
        }
        id = ParseUnit(o);
    }
}

void Game::ProcessIdleOrder(const Unit::Handle& u, AString *, const OrdersCheck::Handle& pCheck)
{
    if (u->monthorders || (Globals->TAX_PILLAGE_MONTH_LONG &&
        ((u->taxing == TAX_TAX) || (u->taxing == TAX_PILLAGE)))) {
        AString err = "IDLE: Overwriting previous ";
        if (u->inTurnBlock) err += "DELAYED ";
        err += "month-long order.";
        ParseError(pCheck, u, 0, err);
    }
    if (Globals->TAX_PILLAGE_MONTH_LONG) u->taxing = TAX_NONE;
    u->monthorders = std::make_shared<IdleOrder>();
}

void Game::ProcessTransportOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    UnitId::Handle tar = ParseUnit(o);
    if (!tar) {
        ParseError(pCheck, u, 0, "TRANSPORT: Invalid target.");
        return;
    }
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "TRANSPORT: No amount given.");
        return;
    }

    int amt;
    if (*token == "all")
        amt = -1;
    else
        amt = token->value();
    delete token;
    token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "TRANSPORT: No item given.");
        return;
    }
    const Items item = ParseTransportableItem(token);
    delete token;
    if (!item.isValid()) {
        ParseError(pCheck, u, 0, "TRANSPORT: Invalid item.");
        return;
    }

    int except = 0;
    token = o->gettoken();
    if (token && *token == "except") {
        delete token;
        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, u, 0, "TRANSPORT: EXCEPT requires a value.");
            return;
        }
        except = token->value();
        delete token;
        if (except <= 0) {
            ParseError(pCheck, u, 0, "TRANSPORT: Invalid except value.");
            return;
        }
    }

    if (!pCheck) {
        auto& order = u->transportorders.emplace_back(std::make_shared<TransportOrder>());
        order->item = item;
        order->target = *tar;
        order->amount = amt;
        order->except = except;
    }
    return;
}

void Game::ProcessDistributeOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    UnitId::Handle tar = ParseUnit(o);
    if (!tar) {
        ParseError(pCheck, u, 0, "DISTRIBUTE: Invalid target.");
        return;
    }
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "DISTRIBUTE: No amount given.");
        return;
    }

    int amt;
    if (*token == "all")
        amt = -1;
    else
        amt = token->value();
    delete token;
    token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "DISTRIBUTE: No item given.");
        return;
    }
    const Items item = ParseTransportableItem(token);
    delete token;
    if (!item.isValid()) {
        ParseError(pCheck, u, 0, "DISTRIBUTE: Invalid item.");
        return;
    }

    int except = 0;
    token = o->gettoken();
    if (token && *token == "except") {
        delete token;
        token = o->gettoken();
        if (!token) {
            ParseError(pCheck, u, 0, "DISTRIBUTE: EXCEPT requires a value.");
            return;
        }
        except = token->value();
        delete token;
        if (except <= 0) {
            ParseError(pCheck, u, 0, "DISTRIBUTE: Invalid except value.");
            return;
        }
    }

    if (!pCheck) {
        auto& order = u->transportorders.emplace_back(std::make_shared<TransportOrder>());
        order->type = Orders::Types::O_DISTRIBUTE;
        order->item = item;
        order->target = *tar;
        order->amount = amt;
        order->except = except;
    }
    return;
}

void Game::ProcessShareOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    /* Instant order */
    AString *token = o->gettoken();
    if (!token) {
        ParseError(pCheck, u, 0, "SHARE: Invalid value.");
        return;
    }
    int val = ParseTF(token);
    delete token;
    if (val==-1) {
        ParseError(pCheck, u, 0, "SHARE: Invalid value.");
        return;
    }
    if (!pCheck) {
        u->SetFlag(FLAG_SHARING, val);
    }
}

void Game::ProcessJoinOrder(const Unit::Handle& u, AString *o, const OrdersCheck::Handle& pCheck)
{
    int overload = 1;
    int merge = 0;

    UnitId::Handle id = ParseUnit(o);
    if (!id || !id->unitnum.isValid()) {
        ParseError(pCheck, u, 0, "JOIN: No target given.");
        return;
    }
    AString *token = o->gettoken();
    if (token) {
        if (*token == "nooverload")
            overload = 0;
        else if (*token == "merge")
            merge = 1;
        delete token;
    }
    if (!pCheck) {
        JoinOrder::Handle ord = std::make_shared<JoinOrder>();
        ord->target = *id;
        ord->overload = overload;
        ord->merge = merge;
        u->joinorders = ord;
    }
}

