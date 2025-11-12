#ifndef NOTUSABLE_H
#define NOTUSABLE_H

#include "docboy/memory/memory.h"

class NotUsable : public Memory<Specs::MemoryLayout::NOT_USABLE::START, Specs::MemoryLayout::NOT_USABLE::END> {};

#endif // NOTUSABLE_H