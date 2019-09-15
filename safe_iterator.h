#ifndef SAFE_ITERATOR_H
#define SAFE_ITERATOR_H

#include <iterator>

template<typename iterator_type>
class ConstSafeIterator : public std::iterator<std::bidirectional_iterator_tag, typename iterator_type::value_type>
{
    protected:
        using iterator_traits = std::iterator<std::bidirectional_iterator_tag, typename iterator_type::value_type>;

        iterator_type it_;
        iterator_type next_it_;
        iterator_type prev_it_;
        
    public:
        using value_type = typename iterator_traits::value_type;
        using difference_type = typename iterator_traits::difference_type;
        using reference = typename iterator_traits::reference;
        using const_reference = const typename iterator_traits::value_type&;
        using pointer = typename iterator_traits::pointer;
        using const_pointer = const typename iterator_traits::pointer;

        ConstSafeIterator(const iterator_type& it) :
            it_(it),
            next_it_(std::next(it)),
            prev_it_(std::prev(it))
        {
        }

        const_reference operator*() const
        {
            return *it_;
        }

        const_pointer operator->() const
        {
            return it_.operator->();
        }

        ConstSafeIterator& operator--()
        {
            next_it_ = it_;
            it_ = prev_it_;
            prev_it_ = std::prev(it_);
            return *this;
        }

        ConstSafeIterator& operator++()
        {
            prev_it_ = it_;
            it_ = next_it_;
            next_it_ = std::next(it_);
            return *this;
        }

        ConstSafeIterator operator--(int)
        {
            ConstSafeIterator copy = *this;
            --(*this);
            return copy;
        }

        ConstSafeIterator operator++(int)
        {
            ConstSafeIterator copy = *this;
            ++(*this);
            return copy;
        }

        operator iterator_type()
        {
            return it_;
        }

        template<typename const_iterator>
        operator const_iterator() const
        {
            return const_iterator(it_);
        }

        bool operator== (const ConstSafeIterator& rhs) const
        {
            return it_ == rhs.it_;
        }

        bool operator!= (const ConstSafeIterator& rhs) const
        {
            return it_ != rhs.it_;
        }
};

template<typename iterator_type>
class SafeIterator : public ConstSafeIterator<iterator_type>
{
    private:
        using iterator_traits = ConstSafeIterator<iterator_type>;
        using iterator_traits::it_;

    public:
        using value_type = typename iterator_traits::value_type;
        using difference_type = typename iterator_traits::difference_type;
        using reference = typename iterator_traits::reference;
        using const_reference = const typename iterator_traits::value_type&;
        using pointer = typename iterator_traits::pointer;
        using const_pointer = typename iterator_traits::pointer;

        SafeIterator(const iterator_type& it) :
            iterator_traits(it)
        {
        }

        using iterator_traits::operator*;

        reference operator*()
        {
            return *it_;
        }

        using iterator_traits::operator->;

        pointer operator->()
        {
            return it_.operator->();
        }

        template<typename const_iterator_type>
        operator ConstSafeIterator<const_iterator_type>() const
        {
            return ConstSafeIterator(it_);
        }

        using iterator_traits::operator==;

        bool operator==(const SafeIterator& rhs) const
        {
            return it_ == rhs.it_;
        }

        using iterator_traits::operator!=;

        bool operator!=(const SafeIterator& rhs) const
        {
            return it_ != rhs.it_;
        }
};

#endif
