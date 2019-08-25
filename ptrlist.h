#ifndef PTR_LIST_H
#define PTR_LIST_H

#include <list>
#include <memory>

template<typename T>
using PtrList = std::list<std::shared_ptr<T>>;

template<typename T>
using WeakPtrList = std::list<std::weak_ptr<T>>;

template<typename T>
using UniquePtrList = std::list<std::unique_ptr<T>>;

#endif
