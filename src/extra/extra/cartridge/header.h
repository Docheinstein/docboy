#ifndef CARTRIDGEHEADERHELPERS_H
#define CARTRIDGEHEADERHELPERS_H

#include "docboy/cartridge/header.h"

#include <string>

namespace CartridgeHeaderHelpers {
std::string title_as_string(const CartridgeHeader& header);
std::string cgb_flag_description(const CartridgeHeader& header);
std::string new_licensee_code_as_string(const CartridgeHeader& header);
std::string new_licensee_code_description(const CartridgeHeader& header);
std::string cartridge_type_description(const CartridgeHeader& header);
std::string rom_size_description(const CartridgeHeader& header);
std::string ram_size_description(const CartridgeHeader& header);
std::string old_licensee_code_description(const CartridgeHeader& header);
} // namespace CartridgeHeaderHelpers

#endif // CARTRIDGEHEADERHELPERS_H