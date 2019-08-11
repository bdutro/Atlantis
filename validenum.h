#ifndef VALID_ENUM_H
#define VALID_ENUM_H

#include <iterator>

#include "validvalue.h"

template<typename EnumType, EnumType EndType>
class ValidEnum : public ValidValue<size_t>
{
    private:
        bool overflowed_ = false;

    public:
        using Types = EnumType;

        class iterator
        {
            private:
                size_t it_;

            public:
                using value_type = ValidEnum;
                using pointer = ValidEnum*;
                using reference = ValidEnum&;
                using iterator_category = std::bidirectional_iterator_tag;
                using difference_type = ptrdiff_t;

                iterator(Types it) :
                    iterator(static_cast<size_t>(it))
                {
                }

                iterator(size_t it) :
                    it_(it)
                {
                }

                iterator& operator++()
                {
                    ++it_;
                    return *this;
                }

                iterator& operator--()
                {
                    --it_;
                    return *this;
                }

                iterator operator++(int)
                {
                    iterator copy = *this;
                    ++(*this);
                    return copy;
                }

                iterator operator--(int)
                {
                    iterator copy = *this;
                    --(*this);
                    return copy;
                }

                bool operator==(const iterator& rhs) const
                {
                    return it_ == rhs.it_;
                }

                bool operator!=(const iterator& rhs) const
                {
                    return it_ != rhs.it_;
                }

                ValidEnum operator*()
                {
                    return ValidEnum(it_);
                }
        };

        using ValidValue<size_t>::operator=;
        using ValidValue<size_t>::operator size_t;

        ValidEnum() :
            ValidValue<size_t>()
        {
        }

        ValidEnum(size_t type) :
            ValidValue<size_t>(type)
        {
            if(type >= size())
            {
                overflowed_ = true;
            }
        }
        
        ValidEnum(const Types& type) :
            ValidEnum(static_cast<size_t>(type))
        {
        }

        ValidEnum(int type) :
            ValidEnum(static_cast<size_t>(type))
        {
            if(type < 0)
            {
                invalidate();
            }
        }

        constexpr ValidEnum(const ValidEnum& rhs) = default;

        ValidEnum& operator=(const ValidEnum& rhs) = default;

        ValidEnum& operator=(const Types& rhs)
        {
            ValidValue<size_t>::operator=(static_cast<size_t>(rhs));
            return *this;
        }

        operator Types() const
        {
            return static_cast<Types>(ValidValue<size_t>::operator size_t());
        }

        static constexpr iterator begin()
        {
            return 0;
        }

        static constexpr iterator end()
        {
            return size();
        }

        Types asEnum() const
        {
            return *this;
        }

        bool operator==(const Types& rhs) const
        {
            return *this == ValidEnum(rhs);
        }

        bool operator!=(const Types& rhs) const
        {
            return *this != ValidEnum(rhs);
        }

        static constexpr size_t size()
        {
            return static_cast<size_t>(EndType);
        }

        bool isValid() const override
        {
            return !overflowed_ && ValidValue::isValid();
        }

        bool overflowed() const
        {
            return overflowed_;
        }
};

#endif
