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
#include "orders.h"

const std::vector<std::string> OrderStrs = {
    "#atlantis",
    "#end",
    "unit",
    "address",
    "advance",
    "armor",
    "assassinate",
    "attack",
    "autotax",
    "avoid",
    "behind",
    "build",
    "buy",
    "cast",
    "claim",
    "combat",
    "consume",
    "declare",
    "describe",
    "destroy",
    "distribute",
    "end",
    "endturn",
    "enter",
    "entertain",
    "evict",
    "exchange",
    "faction",
    "find",
    "forget",
    "form",
    "give",
    "guard",
    "hold",
    "idle",
    "join",
    "leave",
    "move",
    "name",
    "noaid",
    "nocross",
    "nospoils",
    "option",
    "password",
    "pillage",
    "prepare",
    "produce",
    "promote",
    "quit",
    "restart",
    "reveal",
    "sail",
    "sell",
    "share",
    "show",
    "spoils",
    "steal",
    "study",
    "take",
    "tax",
    "teach",
    "transport",
    "turn",
    "weapon",
    "withdraw",
    "work",
};

Orders Parse1Order(AString *token)
{
    for (auto i = Orders::begin(); i != Orders::end(); ++i)
    {
        if (*token == OrderStrs[*i])
        {
            return *i;
        }
    }
    return Orders();
}

Order::Order()
{
    type = *Orders::end();
    quiet = 0;
}

ExchangeOrder::ExchangeOrder()
{
    type = Orders::Types::O_EXCHANGE;
    exchangeStatus = -1;
}

TurnOrder::TurnOrder()
{
    type = Orders::Types::O_TURN;
    repeating = 0;
}

MoveOrder::MoveOrder()
{
    type = Orders::Types::O_MOVE;
}

ForgetOrder::ForgetOrder()
{
    type = Orders::Types::O_FORGET;
}

WithdrawOrder::WithdrawOrder()
{
    type = Orders::Types::O_WITHDRAW;
}

GiveOrder::GiveOrder()
{
    type = Orders::Types::O_GIVE;
    unfinished = 0;
    merge = 0;
}

StudyOrder::StudyOrder()
{
    type = Orders::Types::O_STUDY;
}

TeachOrder::TeachOrder()
{
    type = Orders::Types::O_TEACH;
}

ProduceOrder::ProduceOrder()
{
    type = Orders::Types::O_PRODUCE;
}

BuyOrder::BuyOrder()
{
    type = Orders::Types::O_BUY;
}

SellOrder::SellOrder()
{
    type = Orders::Types::O_SELL;
}

AttackOrder::AttackOrder()
{
    type = Orders::Types::O_ATTACK;
}

BuildOrder::BuildOrder()
{
    type = Orders::Types::O_BUILD;
}

SailOrder::SailOrder()
{
    type = Orders::Types::O_SAIL;
}

FindOrder::FindOrder()
{
    type = Orders::Types::O_FIND;
}

StealOrder::StealOrder()
{
    type = Orders::Types::O_STEAL;
}

AssassinateOrder::AssassinateOrder()
{
    type = Orders::Types::O_ASSASSINATE;
}

CastOrder::CastOrder()
{
    type = Orders::Types::O_CAST;
}

CastMindOrder::CastMindOrder()
{
    id.invalidate();
}

EvictOrder::EvictOrder()
{
    type = Orders::Types::O_EVICT;
}

IdleOrder::IdleOrder()
{
    type = Orders::Types::O_IDLE;
}

TransportOrder::TransportOrder()
{
    type = Orders::Types::O_TRANSPORT;
    item.invalidate();
    amount = 0;
    except = 0;
    target.invalidate();
}

JoinOrder::JoinOrder()
{
    type = Orders::Types::O_JOIN;
}

