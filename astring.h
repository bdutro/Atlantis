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
#ifndef ASTRING_CLASS
#define ASTRING_CLASS

#include <memory>
#include <iostream>
#include "alist.h"

class AString : public AListElem {
    friend std::ostream & operator <<(std::ostream &os, const AString &);
    friend std::istream & operator >>(std::istream &is, AString &);
public:
    using Handle = std::shared_ptr<AString>;

    AString();
    AString(char *);
    AString(const char * const);
    AString(int);
    AString(unsigned int);
    AString(size_t);
    AString(char);
    AString(const AString &);
    ~AString();

    bool operator==(const AString &) const;
    bool operator==(char *) const;
    bool operator==(const char *) const;
    int CheckPrefix(const AString &);
    AString operator+(const AString &);
    AString & operator+=(const AString &);

    AString & operator=(const AString &);
    AString & operator=(const char *);

    char *Str();
    size_t Len();

    AString *gettoken();
    int getat();
    AString *getlegal();
    AString *Trunc(size_t, size_t back=30);
    int value();
    AString *StripWhite();

private:

    size_t len;
    char *str;
    bool isEqual(const char *) const;
};

#endif
