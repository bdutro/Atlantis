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
#ifndef FILE_IO
#define FILE_IO

#include <iostream>
#include <fstream>
#include <exception>
#include <memory>

#include "astring.h"

class Ainfile {
    public:
        Ainfile();
        ~Ainfile() = default;

        void Open(const AString &);
        int OpenByName(const AString &);
        void Close();

        AString GetStr();
        AString GetStrNoSkip();

        template<typename T>
        T GetInt()
        {
            T x;
            *file >> x;
            return x;
        }


        std::unique_ptr<std::ifstream> file;
};

class Aoutfile {
    public:
        using Handle = std::shared_ptr<Aoutfile>;

        Aoutfile();
        ~Aoutfile() = default;

        void Open(const AString &);
        int OpenByName(const AString &);
        void Close();

        void PutStr(char const *);
        void PutStr(const AString &);
        void PutInt(int);
        void PutInt(unsigned int);
        void PutInt(ssize_t);
        void PutInt(size_t);
        void PutBool(bool);

        std::unique_ptr<std::ofstream> file;
};

class Aorders {
    public:
        Aorders();
        ~Aorders() = default;

        void Open(const AString &);
        int OpenByName(const AString &);
        void Close();

        AString GetLine();

        std::unique_ptr<std::ifstream> file;
};

class Areport {
    public:
        Areport();
        ~Areport() = default;

        void Open(const AString &);
        int OpenByName(const AString &);
        void Close();

        void AddTab();
        void DropTab();
        void ClearTab();

        void PutStr(const AString &,int = 0);
        void PutNoFormat(const AString &);
        void EndLine();

        std::unique_ptr<std::ofstream> file;
        int tabs;
};

class Arules {
    public:
        Arules();
        ~Arules() = default;

        void Open(const AString &);
        int OpenByName(const AString &);
        void Close();

        void AddTab();
        void DropTab();
        void ClearTab();

        void AddWrapTab();
        void DropWrapTab();
        void ClearWrapTab();

        void PutStr(const AString &);
        void WrapStr(const AString &);
        void PutNoFormat(const AString &);
        void EndLine();

        void Enclose(int flag, const AString &tag);
        void TagText(const AString &tag, const AString &text);
        void ClassTagText(const AString &tag, const AString &cls,
                const AString &text);
        void Paragraph(const AString &text);
        void CommandExample(const AString &header, const AString &examp);
        AString Link(const AString &href, const AString &text);
        void LinkRef(const AString &name);

        std::unique_ptr<std::ofstream> file;
        int tabs;
        int wraptab;
};

class AFileException : public std::exception
{
};

#endif
