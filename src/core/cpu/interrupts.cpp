#include "interrupts.h"

void Interrupts::loadState(IReadableState& state) {
    IF = state.readBool();
    IE = state.readBool();
}

void Interrupts::saveState(IWritableState& state) {
    state.writeBool(IF);
    state.writeBool(IE);
}

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