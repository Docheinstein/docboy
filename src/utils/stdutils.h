#ifndef STDUTILS_H
#define STDUTILS_H

template <template <typename...> class Container, typename T>
const T& pop(Container<T>& container);

template <template <typename...> class Container, typename T>
const T& pop_front(Container<T>& container);

template <template <typename...> class Container, typename T, typename Predicate>
void erase_if(Container<T>& cont, Predicate pred);

template <template <typename...> class Container, typename T>
bool contains(Container<T>& container, const T& value);

#include "stdutils.tpp"

#endif // STDUTILS_H