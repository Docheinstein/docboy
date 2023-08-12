#include "lcdcontroller.h"
#include "core/dma/dma.h"

LCDController::LCDController(IDMA& dma) :
    dma(dma),
    LCDC(0b10010001),
    STAT(),
    SCY(),
    SCX(),
    LY(),
    LYC(),
    DMA(),
    BGP(),
    OBP0(),
    OBP1(),
    WY(),
    WX() {
}

void LCDController::loadState(IReadableState& state) {
    LCDC = state.readUInt8();
    STAT = state.readUInt8();
    SCY = state.readUInt8();
    SCX = state.readUInt8();
    LY = state.readUInt8();
    LYC = state.readUInt8();
    DMA = state.readUInt8();
    BGP = state.readUInt8();
    OBP0 = state.readUInt8();
    OBP1 = state.readUInt8();
    WY = state.readUInt8();
    WX = state.readUInt8();
}

void LCDController::saveState(IWritableState& state) {
    state.writeUInt8(LCDC);
    state.writeUInt8(STAT);
    state.writeUInt8(SCY);
    state.writeUInt8(SCX);
    state.writeUInt8(LY);
    state.writeUInt8(LYC);
    state.writeUInt8(DMA);
    state.writeUInt8(BGP);
    state.writeUInt8(OBP0);
    state.writeUInt8(OBP1);
    state.writeUInt8(WY);
    state.writeUInt8(WX);
}

uint8_t LCDController::readLCDC() const {
    return LCDC;
}

void LCDController::writeLCDC(uint8_t value) {
    LCDC = value;
}

uint8_t LCDController::readSTAT() const {
    return STAT;
}

void LCDController::writeSTAT(uint8_t value) {
    STAT = value;
}

uint8_t LCDController::readSCY() const {
    return SCY;
}

void LCDController::writeSCY(uint8_t value) {
    SCY = value;
}

uint8_t LCDController::readSCX() const {
    return SCX;
}

void LCDController::writeSCX(uint8_t value) {
    SCX = value;
}

uint8_t LCDController::readLY() const {
    return LY;
}

void LCDController::writeLY(uint8_t value) {
    LY = value;
}

uint8_t LCDController::readLYC() const {
    return LYC;
}

void LCDController::writeLYC(uint8_t value) {
    LYC = value;
}

uint8_t LCDController::readDMA() const {
    return DMA;
}

void LCDController::writeDMA(uint8_t value) {
    DMA = value;
    dma.transfer((uint16_t)DMA << 8);
}

uint8_t LCDController::readBGP() const {
    return BGP;
}

void LCDController::writeBGP(uint8_t value) {
    BGP = value;
}

uint8_t LCDController::readOBP0() const {
    return OBP0;
}

void LCDController::writeOBP0(uint8_t value) {
    OBP0 = value;
}

uint8_t LCDController::readOBP1() const {
    return OBP1;
}

void LCDController::writeOBP1(uint8_t value) {
    OBP1 = value;
}

uint8_t LCDController::readWY() const {
    return WY;
}

void LCDController::writeWY(uint8_t value) {
    WY = value;
}

uint8_t LCDController::readWX() const {
    return WX;
}

void LCDController::writeWX(uint8_t value) {
    WX = value;
}
