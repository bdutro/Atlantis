#ifndef GAME_DATA_ARRAY_H
#define GAME_DATA_ARRAY_H

#include <vector>
#include <type_traits>

template<typename T>
class GameDataArray : public std::vector<T>
{
    public:
        explicit GameDataArray(const std::initializer_list<T>& lst) :
            std::vector<T>(lst)
        {
        }

        using std::vector<T>::operator[];

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
