#include "interrupts.h"

uint8_t Interrupts::readIF() const {
    return IF;
}

void Interrupts::writeIF(uint8_t value) {
    IF = value;
}

uint8_t Interrupts::readIE() const {
    return IE;
}

void Interrupts::writeIE(uint8_t value) {
    IE = value;
}
