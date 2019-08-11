#ifndef GAME_DATA_ARRAY_H
#define GAME_DATA_ARRAY_H

#include <vector>
#include <type_traits>
#include <exception>

#include "astring.h"

class NoSuchItemException : public std::exception
{
};

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

        const T& FindItemByAbbr(char const* abbr) const
        {
            if(!abbr)
            {
                throw NoSuchItemException();
            }
            return FindItemByAbbr(AString(abbr));
        }

        const T& FindItemByAbbr(const AString& abbr) const
        {
            for (const auto& item: *this) {
                if (item.abbr == nullptr)
                {
                    continue;
                }
                if (abbr == item.abbr)
                {
                    return item;
                }
            }
        
            throw NoSuchItemException();
        }

        T& FindItemByAbbr(char const* abbr)
        {
            return const_cast<T&>(const_cast<const GameDataArray*>(this)->FindItemByAbbr(abbr));
        }

        T& FindItemByAbbr(const AString& abbr)
        {
            return const_cast<T&>(const_cast<const GameDataArray*>(this)->FindItemByAbbr(abbr));
        }

        const T& FindItemByKey(char const* key) const
        {
            if(!key)
            {
                throw NoSuchItemException();
            }
            return FindItemByKey(AString(key));
        }

        const T& FindItemByKey(const AString& key) const
        {
            for (const auto& item: *this) {
                if (item.key == nullptr)
                {
                    continue;
                }
                if (key == item.key)
                {
                    return item;
                }
            }

            throw NoSuchItemException();
        }

        T& FindItemByKey(char const* key)
        {
            return const_cast<T&>(const_cast<const GameDataArray*>(this)->FindItemByKey(key));
        }

        T& FindItemByKey(const AString& key)
        {
            return const_cast<T&>(const_cast<const GameDataArray*>(this)->FindItemByKey(key));
        }

        const T& FindItemByName(char const* name) const
        {
            if(!name)
            {
                throw NoSuchItemException();
            }
            return FindItemByName(AString(name));
        }

        const T& FindItemByName(const AString& name) const
        {
            for (const auto& item: *this) {
                if (item.name == nullptr)
                {
                    continue;
                }
                if (name == item.name)
                {
                    return item;
                }
            }

            throw NoSuchItemException();
        }

        T& FindItemByName(char const* name)
        {
            return const_cast<T&>(const_cast<const GameDataArray*>(this)->FindItemByName(name));
        }

        T& FindItemByName(const AString& name)
        {
            return const_cast<T&>(const_cast<const GameDataArray*>(this)->FindItemByName(name));
        }
};

#endif
