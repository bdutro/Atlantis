#ifndef PTR_LIST_H
#define PTR_LIST_H

#include <list>
#include <memory>

#include "safe_iterator.h"

template<typename pointer_type>
class BasePtrList
{
    private:
        template<typename T>
        struct is_shared_ptr : std::false_type {};

        template<typename T>
        struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

        template<typename T>
        struct is_unique_ptr : std::false_type {};

        template<typename T>
        struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

        using list_type = std::list<pointer_type>;
        list_type list_;

    public:
        using value_type = typename list_type::value_type;
        using iterator = SafeIterator<typename list_type::iterator>;
        using const_iterator = ConstSafeIterator<typename list_type::const_iterator>;
        using reference = typename list_type::reference;
        using const_reference = typename list_type::const_reference;

        BasePtrList() = default;

        const_iterator begin() const
        {
            return const_iterator(list_.begin());
        }

        const_iterator end() const
        {
            return const_iterator(list_.end());
        }

        const_iterator cbegin() const
        {
            return const_iterator(list_.cbegin());
        }

        const_iterator cend() const
        {
            return const_iterator(list_.cend());
        }

        iterator begin()
        {
            return iterator(list_.begin());
        }

        iterator end()
        {
            return iterator(list_.end());
        }

        iterator erase(const_iterator it)
        {
            return iterator(list_.erase(it));
        }

        size_t size() const
        {
            return list_.size();
        }

        void push_front(const value_type& v)
        {
            list_.push_front(v);
        }

        void push_front(value_type&& v)
        {
            list_.push_front(v);
        }

        void push_back(const value_type& v)
        {
            list_.push_back(v);
        }

        void push_back(value_type&& v)
        {
            list_.push_back(v);
        }

        void pop_front()
        {
            list_.pop_front();
        }

        void pop_back()
        {
            list_.pop_back();
        }

        reference emplace_front(pointer_type&& p)
        {
            return list_.emplace_front(std::move(p));
        }

        reference emplace_back(pointer_type&& p)
        {
            return list_.emplace_back(std::move(p));
        }

        reference emplace_front(const pointer_type& p)
        {
            return list_.emplace_front(p);
        }

        reference emplace_back(const pointer_type& p)
        {
            return list_.emplace_back(p);
        }

        template<typename... Args, typename U = pointer_type>
        typename std::enable_if<is_shared_ptr<U>::value, reference>::type emplace_front(Args&&... args)
        {
            return list_.emplace_front(std::make_shared<typename U::element_type>(std::forward<Args>(args)...));
        }

        template<typename... Args, typename U = pointer_type>
        typename std::enable_if<is_shared_ptr<U>::value, reference>::type emplace_back(Args&&... args)
        {
            return list_.emplace_back(std::make_shared<typename U::element_type>(std::forward<Args>(args)...));
        }

        template<typename... Args, typename U = pointer_type>
        typename std::enable_if<is_unique_ptr<U>::value, reference>::type emplace_front(Args&&... args)
        {
            return list_.emplace_front(std::make_unique<typename U::element_type>(std::forward<Args>(args)...));
        }

        template<typename... Args, typename U = pointer_type>
        typename std::enable_if<is_unique_ptr<U>::value, reference>::type emplace_back(Args&&... args)
        {
            return list_.emplace_back(std::make_unique<typename U::element_type>(std::forward<Args>(args)...));
        }

        void clear()
        {
            list_.clear();
        }

        bool empty() const
        {
            return list_.empty();
        }

        reference front()
        {
            return list_.front();
        }

        const_reference front() const
        {
            return list_.front();
        }

        reference back()
        {
            return list_.back();
        }

        const_reference back() const
        {
            return list_.back();
        }

        template<typename UnaryPredicate>
        void remove_if(UnaryPredicate p)
        {
            list_.remove_if(p);
        }

        void remove(const value_type& value)
        {
            list_.remove(value);
        }

        template<typename UnaryPredicate>
        void for_each(UnaryPredicate p) const
        {
            for(const auto& elem: list_)
            {
                p(elem);
            }
        }

        template<typename UnaryPredicate>
        void for_each(UnaryPredicate p)
        {
            for(auto& elem: list_)
            {
                p(elem);
            }
        }

        void splice(const_iterator pos, BasePtrList& other)
        {
            list_.splice(pos, other.list_);
        }

        void splice(const_iterator pos, BasePtrList&& other)
        {
            list_.splice(pos, other.list_);
        }

        void splice(const_iterator pos, BasePtrList& other, const_iterator it)
        {
            list_.splice(pos, other.list_, it);
        }

        void splice(const_iterator pos, BasePtrList&& other, const_iterator it)
        {
            list_.splice(pos, other.list_, it);
        }

        void splice(const_iterator pos, BasePtrList& other, const_iterator first, const_iterator last)
        {
            list_.splice(pos, other.list_, first, last);
        }

        void splice(const_iterator pos, BasePtrList&& other, const_iterator first, const_iterator last)
        {
            list_.splice(pos, other.list_, first, last);
        }
};

template<typename T>
using PtrList = BasePtrList<std::shared_ptr<T>>;

template<typename T>
using WeakPtrList = BasePtrList<std::weak_ptr<T>>;

template<typename T>
using UniquePtrList = BasePtrList<std::unique_ptr<T>>;

#endif
