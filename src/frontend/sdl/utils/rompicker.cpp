#include "rompicker.h"

#ifdef NFD

#include "nfd.h"

std::optional<std::string> open_rom_picker() {
    std::optional<std::string> rom {};

    nfdchar_t* nfd_path;
#ifdef ENABLE_CGB
    static constexpr nfdfilteritem_t filter_items[1] {{"GameBoy ROM", "gb,gbc"}};
#else
    static constexpr nfdfilteritem_t filter_items[1] {{"GameBoy ROM", "gb"}};
#endif

    if (nfdresult_t result = NFD_OpenDialog(&nfd_path, filter_items, 1, nullptr); result == NFD_OKAY) {
        rom = std::string {nfd_path};
        NFD_FreePath(nfd_path);
    }

    return rom;
}

#endif
