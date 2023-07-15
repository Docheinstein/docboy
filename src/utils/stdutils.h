#ifndef STDUTILS_H
#define STDUTILS_H

#include <vector>

template<typename T>
const T& pop(std::vector<T> &vector);

#include "stdutils.tpp"

#endif // STDUTILS_H