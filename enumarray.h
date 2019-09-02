#ifndef ENUM_ARRAY_H
#define ENUM_ARRAY_H

#include <array>

template<typename T, size_t N>
class EnumArray : public std::array<T, N>
{
    public:
        using std::array<T, N>::operator[];

        template<typename U>
        typename std::enable_if<std::is_unsigned<U>::value || std::is_enum<U>::value, T&>::type operator[](const U& idx)
        {
            return operator[](static_cast<size_t>(idx));
        }

        template<typename U>
        typename std::enable_if<std::is_unsigned<U>::value || std::is_enum<U>::value, const T&>::type operator[](const U& idx) const
        {
            return operator[](static_cast<size_t>(idx));
        }

};

#endif
