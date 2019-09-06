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
#include <string>
#include <sstream>
#include "validvalue.h"

class AString
{
    friend std::ostream & operator <<(std::ostream &os, const AString &);
    friend std::istream & operator >>(std::istream &is, AString &);
public:
    using Handle = std::shared_ptr<AString>;

    AString() = default;
    AString(char * s) :
        str_(s)
    {
    }

    AString(const char * const s) :
        str_(s)
    {
    }

    template<typename T>
    AString(T l, typename std::enable_if<std::is_integral<T>::value>::type* = 0)
    {
        std::stringstream ss;
        ss << l;
        str_ = ss.str();
    }

    AString(const AString &) = default;

    template<typename T>
    AString(const ValidValue<T> & rhs) :
        AString(static_cast<T>(rhs))
    {
    }

    AString(const std::string& s) :
        AString(s.c_str())
    {
    }

    ~AString() = default;

    bool operator==(const AString &) const;
    bool operator==(char *) const;
    bool operator==(const char *) const;
    bool operator!=(const AString &) const;
    bool operator!=(char *) const;
    bool operator!=(const char *) const;
    bool CheckPrefix(const AString &);
    AString operator+(const AString &) const;
    AString & operator+=(const AString &);

    AString & operator=(const AString &) = default;
    AString & operator=(const char *);
    AString & operator=(char);
    AString & operator=(int) = delete;

    const char* Str() const;
    size_t Len() const;

    AString gettoken();
    bool getat();
    AString getlegal() const;
    AString Trunc(size_t, size_t back=30);

    template<typename T>
    typename std::enable_if<!std::is_signed<T>::value, T>::type value() const
    {
        T ret = 0;
        T last_ret;

        auto it = str_.begin();
        while(it != str_.end() && *it >= '0' && *it <= '9')
        {
            last_ret = ret;
            ret *= 10;
            if (ret < last_ret)
            {
                return 0;
            }
            ret += static_cast<T>(*(it++) - '0');
        }
        return ret;

    #if 0
        int place = 0;
        int ret = 0;
        while ((str[place] >= '0') && (str[place] <= '9')) {
            ret *= 10;
            // Fix bug where int could be overflowed.
            if (ret < 0) return 0;
            ret += (str[place++] - '0');
        }
        return ret;
    #endif
    }

    template<typename T>
    typename std::enable_if<std::is_signed<T>::value, T>::type value() const
    {
        T ret = 0;
        auto it = str_.begin();
        while(it != str_.end() && *it >= '0' && *it <= '9')
        {
            ret *= 10;
            if (ret < 0)
            {
                return 0;
            }
            ret += *(it++) - '0';
        }
        return ret;

    #if 0
        int place = 0;
        int ret = 0;
        while ((str[place] >= '0') && (str[place] <= '9')) {
            ret *= 10;
            // Fix bug where int could be overflowed.
            if (ret < 0) return 0;
            ret += (str[place++] - '0');
        }
        return ret;
    #endif
    }

    AString StripWhite() const;
    void clear();

private:

    std::string str_;
    bool isEqual(const std::string&) const;
};

#endif
