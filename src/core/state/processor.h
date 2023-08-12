#ifndef STATEPROCESSOR_H
#define STATEPROCESSOR_H

#include "loader.h"
#include "saver.h"

class IStateProcessor : public IStateLoader, public IStateSaver {};

#endif // STATEPROCESSOR_H