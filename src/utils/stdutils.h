#ifndef STDUTILS_H
#define STDUTILS_H

template <template <typename> class CONTAINER, typename T>
const T& pop(CONTAINER<T>& container);

template <template <typename> class CONTAINER, typename T>
const T& pop_front(CONTAINER<T>& container);

#include "stdutils.tpp"

#endif // STDUTILS_H