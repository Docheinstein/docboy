#include "rompicker.h"

#ifdef NFD

#include "nfd.h"

std::optional<std::string> openRomPicker() {
    std::optional<std::string> rom {};

    nfdchar_t* nfdPath;
    nfdfilteritem_t filterItem[2] = {{"GameBoy ROM", "gb,gbc"}};

    if (nfdresult_t result = NFD_OpenDialog(&nfdPath, filterItem, 1, nullptr); result == NFD_OKAY) {
        rom = std::string {nfdPath};
        NFD_FreePath(nfdPath);
    }

    return rom;
}

#endif
