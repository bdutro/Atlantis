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
#ifndef SHIELD_CLASS
#define SHIELD_CLASS

#include <list>

class Shield {
    public:
        int shieldtype;
        int shieldskill;
};

class ShieldList {
    private:
        using list_type = std::list<Shield>;
        list_type shields_;

    public:
        using iterator = list_type::iterator;
        using const_iterator = list_type::const_iterator;
        const_iterator GetHighShield(int) const;

        iterator begin()
        {
            return shields_.begin();
        }

        iterator end()
        {
            return shields_.end();
        }

        const_iterator begin() const
        {
            return shields_.begin();
        }

        const_iterator end() const
        {
            return shields_.end();
        }

        const_iterator cbegin() const
        {
            return shields_.cbegin();
        }
        
        const_iterator cend() const
        {
            return shields_.cend();
        }

        void erase(const_iterator it)
        {
            shields_.erase(it);
        }

        void clear()
        {
            shields_.clear();
        }
};

#endif /* SHIELD_CLASS */
