#ifndef NOTUSABLEFWD_H
#define NOTUSABLEFWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/memoryfwd.h"

using NotUsable = Memory<Specs::MemoryLayout::NOT_USABLE::START, Specs::MemoryLayout::NOT_USABLE::END>;

#endif // NOTUSABLEFWD_H