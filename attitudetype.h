#ifndef ATTITUDE_TYPE_H
#define ATTITUDE_TYPE_H

#include "validenum.h"

enum class _Attitudes : size_t {
    A_HOSTILE,
    A_UNFRIENDLY,
    A_NEUTRAL,
    A_FRIENDLY,
    A_ALLY,
    NATTITUDES
};

class Attitudes : public ValidEnum<_Attitudes, _Attitudes::NATTITUDES>
{
    public:
        Attitudes() :
            ValidEnum()
        {
        }

        Attitudes(size_t type) :
            ValidEnum(type)
        {
        }

        Attitudes(const Types& type) :
            ValidEnum(type)
        {
        }

        Attitudes(const ValidEnum& rhs) :
            ValidEnum(rhs)
        {
        }

        Attitudes(ssize_t type) :
            ValidEnum(static_cast<size_t>(type))
        {
        }

        Attitudes(int type) :
            ValidEnum(type)
        {
        }

        bool isNotFriendly() const
        {
            return *this <= Attitudes(Attitudes::Types::A_NEUTRAL);
        }
};

#endif

