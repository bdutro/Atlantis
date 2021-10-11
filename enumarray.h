#ifndef ENUM_ARRAY_H
#define ENUM_ARRAY_H

#include <array>
#include <cstddef>
#include <vector>
#include "validenum.h"

template<typename EnumT, typename T, size_t N>
class EnumArray : public std::array<T, N> {
    public:
        using std::array<T, N>::operator[];

        template<typename U>
        typename std::enable_if<std::is_unsigned<U>::value || std::is_same<EnumT, U>::value, T&>::type
        operator[](const U& idx) {
            return operator[](static_cast<size_t>(idx));
        }

        template<typename U>
        typename std::enable_if<std::is_unsigned<U>::value || std::is_same<EnumT, U>::value, const T&>::type
        operator[](const U& idx) const {
            return operator[](static_cast<size_t>(idx));
        }

};

template<typename EnumT, typename T, size_t N, EnumT EndType>
class EnumArray<ValidEnum<EnumT, EndType>, T, N> : public std::array<T, N> {
    public:
        using std::array<T, N>::operator[];

        template<typename U>
        typename std::enable_if<std::is_unsigned<U>::value || std::is_same<EnumT, U>::value, T&>::type
        operator[](const U& idx) {
            return operator[](static_cast<size_t>(idx));
        }

        template<typename U>
        typename std::enable_if<std::is_unsigned<U>::value || std::is_same<EnumT, U>::value, const T&>::type
        operator[](const U& idx) const {
            return operator[](static_cast<size_t>(idx));
        }

};

template<typename EnumT, typename DataT>
class EnumVector : public std::vector<DataT> {
    public:
        using std::vector<DataT>::operator[];

        template<typename U>
        typename std::enable_if<std::is_unsigned<U>::value || std::is_same<EnumT, U>::value, DataT&>::type
        operator[](const U& idx) {
            return operator[](static_cast<size_t>(idx));
        }

        template<typename U>
        typename std::enable_if<std::is_unsigned<U>::value || std::is_same<EnumT, U>::value, const DataT&>::type
        operator[](const U& idx) const {
            return operator[](static_cast<size_t>(idx));
        }
};

#endif
