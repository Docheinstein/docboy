// Copyright (c) 2018, 2019, 2020 Marko Mahnič
// License: MPL2. See LICENSE in the root of the project.

#pragma once

#include "argumentstream.h"

#include <fstream>
#include <memory>
#include <string>

namespace argumentum {

// A virtual filesystem for opening streams of arguments.
class Filesystem
{
public:
   virtual ~Filesystem() = default;
   virtual std::unique_ptr<ArgumentStream> open( const std::string& filename ) = 0;
};

class DefaultFilesystem : public Filesystem
{
public:
   std::unique_ptr<ArgumentStream> open( const std::string& filename ) override
   {
      return std::make_unique<StdStreamArgumentStream>(
            std::make_unique<std::ifstream>( filename ) );
   }
};

}   // namespace argumentum
