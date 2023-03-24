#include "lcd.h"
#include "utils/binutils.h"
#include "core/definitions.h"

void LCD::pushPixel(ILCD::Pixel pixel) {
    putPixel(pixel, x, y);
    x++;
    if (x >= 160) {
        x = 0;
        y++;
    }
    if (y >= 144) {
        x = 0;
        y = 0;
    }
}

LCD::LCD() :
    LCDC(), STAT(),
    SCY(), SCX(),
    LY(), LYC(), DMA(),
    BGP(), OBP0(), OBP1(),
    WY(), WX(),
    x(), y() {

}



void LCD::putPixel(Pixel pixel, uint8_t x_, uint8_t y_) {
    // nop
}

uint8_t LCD::readLCDC() const {
    return LCDC;
}

void LCD::writeLCDC(uint8_t value) {
    LCDC = value;
    if (!get_bit<Bits::LCD::LCDC::LCD_ENABLE>(LCDC))
        reset();
}

uint8_t LCD::readSTAT() const {
    return STAT;
}

void LCD::writeSTAT(uint8_t value) {
    STAT = value;
}

uint8_t LCD::readSCY() const {
    return SCY;
}

void LCD::writeSCY(uint8_t value) {
    SCY = value;
}

uint8_t LCD::readSCX() const {
    return SCX;
}

void LCD::writeSCX(uint8_t value) {
    SCX = value;
}

uint8_t LCD::readLY() const {
    return LY;
}

void LCD::writeLY(uint8_t value) {
    LY = value;
}

uint8_t LCD::readLYC() const {
    return LYC;
}

void LCD::writeLYC(uint8_t value) {
    LYC = value;
}

uint8_t LCD::readDMA() const {
    return DMA;
}

void LCD::writeDMA(uint8_t value) {
    DMA = value;
}

uint8_t LCD::readBGP() const {
    return BGP;
}

void LCD::writeBGP(uint8_t value) {
    BGP = value;
}

uint8_t LCD::readOBP0() const {
    return OBP0;
}

void LCD::writeOBP0(uint8_t value) {
    OBP0 = value;
}

uint8_t LCD::readOBP1() const {
    return OBP1;
}

void LCD::writeOBP1(uint8_t value) {
    OBP1 = value;
}

uint8_t LCD::readWY() const {
    return WY;
}

void LCD::writeWY(uint8_t value) {
    WY = value;
}

uint8_t LCD::readWX() const {
    return WX;
}

void LCD::writeWX(uint8_t value) {
    WX = value;
}

void LCD::reset() {
    x = 0;
    y = 0;
}

//bool LCD::isOn() const {
//    return get_bit<Bits::LCD::LCDC::LCD_ENABLE>(LCDC);
//}


