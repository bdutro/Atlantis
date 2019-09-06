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

#include <algorithm>
#include <cstring>
#include <cstdio>

#include "astring.h"

template<>
AString::AString(char c, void*)
{
    str_ = c;
}

AString & AString::operator=(char c)
{
    str_ = c;
    return *this;
}

AString & AString::operator=(const char *c)
{
    str_ = c;
    return *this;
}

bool AString::operator==(char *s) const
{
    return isEqual(s);
}

bool AString::operator==(const char *s) const
{
    return isEqual(s);
}

bool AString::operator==(const AString &s) const
{
    return isEqual(s.str_);
}

bool AString::operator!=(char *s) const
{
    return !isEqual(s);
}

bool AString::operator!=(const char *s) const
{
    return !isEqual(s);
}

bool AString::operator!=(const AString &s) const
{
    return !isEqual(s.str_);
}

bool AString::isEqual(const std::string& rhs) const
{
    return std::equal(str_.begin(), str_.end(),
                      rhs.begin(), rhs.end(),
                      [](char a, char b) {
                          if(a == '_')
                          {
                              a = ' ';
                          }
                          if(b == '_')
                          {
                              b = ' ';
                          }
                          return tolower(a) == tolower(b);
                      });
}

AString AString::operator+(const AString &s) const
{
    return str_ + s.str_;
}

AString &AString::operator+=(const AString &s)
{
    str_ += s.str_;
    return *this;
}


const char *AString::Str() const
{
    return str_.c_str();
}

size_t AString::Len() const
{
    return str_.size();
}

static inline std::string::iterator skipWhitespace(std::string::iterator start,
                                                   const std::string::iterator& end)
{
    while(start != end && isspace(*start))
    {
        ++start;
    }

    return start;
}

AString AString::gettoken()
{
    std::string::iterator it = skipWhitespace(str_.begin(), str_.end());

    if(it == str_.end())
    {
        return AString();
    }

    if(*it == ';')
    {
        return AString();
    }

    std::string::iterator tok_start = str_.end();
    std::string::iterator tok_end = str_.end();

    if(*it == '"')
    {
        ++it;
        if(it != str_.end())
        {
            tok_start = it;
            while(it != str_.end() && *it != '"')
            {
                tok_end = ++it;
            }

            if(it != str_.end())
            {
                ++it;
            }
            else
            {
                str_.clear();
                return AString();
            }
        }
    }
    else
    {
        if(it != str_.end())
        {
            tok_start = it;

            while(it != str_.end() && !isspace(*it) && *it != ';')
            {
                tok_end = ++it;
            }
        }
    }

    std::string new_str(tok_start, tok_end);

    if(it == str_.end() || *it == ';')
    {
        str_.clear();
        return AString(new_str);
    }

    str_.erase(str_.begin(), it);
    return AString(new_str);

#if 0
    char buf[1024];
    size_t place = 0;
    size_t place2 = 0;
    while (place < len && (str[place] == ' ' || str[place] == '\t'))
        place++;
    if (place >= len) return 0;
    if (str[place] == ';') return 0;

    if (str[place] == '"') {
        place++;
        while (place < len && str[place] != '"') {
            buf[place2++] = str[place++];
        }
        if (place != len) {
            /* Get rid of extra " */
            place++;
        } else {
            /* Unmatched "" return 0 */
            delete[] str;
            str = new char[1];
            len = 0;
            str[0] = '\0';
            return 0;
        }
    } else {
        while (place<len &&
                (str[place]!=' ' && str[place]!='\t' && str[place]!=';')) {
            buf[place2++] = str[place++];
        }
    }
    buf[place2] = '\0';
    if (place == len || str[place] == ';') {
        delete[] str;
        str = new char[1];
        len = 0;
        str[0] = '\0';
        return new AString(buf);
    }
    char * buf2 = new char[len-place2+1];
    size_t newlen = 0;
    place2 = 0;
    while (place < len) {
        buf2[place2++] = str[place++];
        newlen++;
    }
    buf2[place2] = '\0';
    len = newlen;
    delete[] str;
    str = buf2;
    return new AString(buf);
#endif
}

AString AString::StripWhite() const
{
    auto it = str_.begin();
    while(it != str_.end() && isspace(*it))
    {
        ++it;
    }

    if(it == str_.end())
    {
        return AString();
    }

    return AString(std::string(it, str_.end()));

#if 0
    size_t place = 0;
    while (place < len && (str[place] == ' ' || str[place] == '\t')) {
        place++;
    }
    if (place >= len) {
        return 0;
    }
    return( new AString( &str[ place ] ));
#endif
}

bool AString::getat()
{
    auto it = str_.begin();
    while(it != str_.end() && isspace(*it))
    {
        ++it;
    }

    if(it == str_.end())
    {
        return false;
    }

    if(*it == '@')
    {
        *it = ' ';
        return true;
    }

    return false;

#if 0
    size_t place = 0;
    while (place < len && (str[place] == ' ' || str[place] == '\t'))
        place++;
    if (place >= len) return 0;
    if (str[place] == '@') {
        str[place] = ' ';
        return 1;
    }
    return 0;
#endif
}

bool islegal(char c);
bool islegal(char c)
{
    if ((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') ||
            c=='!' || c=='[' || c==']' || c==',' || c=='.' || c==' ' ||
            c=='{' || c=='}' || c=='@' || c=='#' || c=='$' || c=='%' ||
            c=='^' || c=='&' || c=='*' || c=='-' || c=='_' || c=='+' ||
            c=='=' || c==';' || c==':' || c=='<' || c=='>' || c=='?' ||
            c=='/' || c=='~' || c=='\'' || c== '\\' || c=='`')
    {
        return true;
    }
    return false;
}

AString AString::getlegal() const
{
    std::string ret;

    bool j = false;

    for(const auto c: str_)
    {
        if(islegal(c))
        {
            ret.push_back(c);
            if(c != ' ')
            {
                j = true;
            }
        }
    }

    if(!j)
    {
        return AString();
    }

    return ret;

#if 0
    char * temp = new char[len+1];
    char * temp2 = temp;
    int j = 0;
    for (size_t i=0; i<len; i++) {
        if (islegal(str[i])) {
            *temp2 = str[i];
            if (str[i] != ' ') j=1;
            temp2++;
        }
    }

    if (!j) {
        delete[] temp;
        return 0;
    }

    *temp2 = '\0';
    AString * retval = new AString(temp);
    delete[] temp;
    return retval;
#endif
}

bool AString::CheckPrefix(const AString &s)
{
    if (Len() < s.Len())
    {
        return false;
    }

    return std::equal(str_.begin(), std::next(str_.begin(), static_cast<ssize_t>(s.Len())),
                      s.str_.begin(), s.str_.end());
}

AString AString::Trunc(size_t val, size_t back)
{
    size_t l = Len();
    if (l <= val)
    {
        return AString();
    }

    size_t pos = str_.find_first_of("\r\n", 0, val);
    if(pos != std::string::npos)
    {
        std::string substr = str_.substr(pos + 1);
        str_.resize(pos);
        return AString(substr);
    }

    pos = str_.find_last_of(" ", val - back + 1);
    if(pos != std::string::npos)
    {
        std::string substr = str_.substr(pos + 1);
        str_.resize(pos);
        return AString(substr);
    }

    std::string substr = str_.substr(val);
    str_.resize(val);
    return AString(substr);

#if 0
    for (size_t i = 0; i < val; i++) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0';
            return new AString(&(str[i+1]));
        }
    }
    for (size_t i=val; i>(val-back); i--) {
        if (str[i] == ' ') {
            str[i] = '\0';
            return new AString(&(str[i+1]));
        }
    }
    AString * temp = new AString(&(str[val]));
    str[val] = '\0';
    return temp;
#endif
}

void AString::clear()
{
    str_.clear();
}

std::ostream & operator <<(std::ostream & os, const AString & s)
{
    os << s.str_;
    return os;
}

std::istream & operator >>(std::istream & is, AString & s)
{
    is >> s.str_;
    return is;
}
