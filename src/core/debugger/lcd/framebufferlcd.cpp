#include "framebufferlcd.h"

ILCDDebug::State DebuggableFrameBufferLCD::getState() {
    return {
        .x = x,
        .y = y
    };
}

uint8_t DebuggableFrameBufferLCD::readLCDC() const {
    uint8_t value = FrameBufferLCD::readLCDC();
    if (observer)
        observer->onReadLCDC(value);
    return value;
}

void DebuggableFrameBufferLCD::writeLCDC(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readLCDC();
    FrameBufferLCD::writeLCDC(value);
    if (observer)
        observer->onWriteLCDC(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readSTAT() const {
    uint8_t value = FrameBufferLCD::readSTAT();
    if (observer)
        observer->onReadSTAT(value);
    return value;
}

void DebuggableFrameBufferLCD::writeSTAT(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readSTAT();
    FrameBufferLCD::writeSTAT(value);
    if (observer)
        observer->onWriteSTAT(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readSCY() const {
    uint8_t value = FrameBufferLCD::readSCY();
    if (observer)
        observer->onReadSCY(value);
    return value;
}

void DebuggableFrameBufferLCD::writeSCY(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readSCY();
    FrameBufferLCD::writeSCY(value);
    if (observer)
        observer->onWriteSCY(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readSCX() const {
    uint8_t value = FrameBufferLCD::readSCX();
    if (observer)
        observer->onReadSCX(value);
    return value;
}

void DebuggableFrameBufferLCD::writeSCX(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readSCX();
    FrameBufferLCD::writeSCX(value);
    if (observer)
        observer->onWriteSCX(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readLY() const {
    uint8_t value = FrameBufferLCD::readLY();
    if (observer)
        observer->onReadLY(value);
    return value;
}

void DebuggableFrameBufferLCD::writeLY(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readLY();
    FrameBufferLCD::writeLY(value);
    if (observer)
        observer->onWriteLY(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readLYC() const {
    uint8_t value = FrameBufferLCD::readLYC();
    if (observer)
        observer->onReadLYC(value);
    return value;
}

void DebuggableFrameBufferLCD::writeLYC(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readLYC();
    FrameBufferLCD::writeLYC(value);
    if (observer)
        observer->onWriteLYC(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readDMA() const {
    uint8_t value = FrameBufferLCD::readDMA();
    if (observer)
        observer->onReadDMA(value);
    return value;
}

void DebuggableFrameBufferLCD::writeDMA(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readDMA();
    FrameBufferLCD::writeDMA(value);
    if (observer)
        observer->onWriteDMA(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readBGP() const {
    uint8_t value = FrameBufferLCD::readBGP();
    if (observer)
        observer->onReadBGP(value);
    return value;
}

void DebuggableFrameBufferLCD::writeBGP(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readBGP();
    FrameBufferLCD::writeBGP(value);
    if (observer)
        observer->onWriteBGP(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readOBP0() const {
    uint8_t value = FrameBufferLCD::readOBP0();
    if (observer)
        observer->onReadOBP0(value);
    return value;
}

void DebuggableFrameBufferLCD::writeOBP0(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readOBP0();
    FrameBufferLCD::writeOBP0(value);
    if (observer)
        observer->onWriteOBP0(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readOBP1() const {
    uint8_t value = FrameBufferLCD::readOBP1();
    if (observer)
        observer->onReadOBP1(value);
    return value;
}

void DebuggableFrameBufferLCD::writeOBP1(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readOBP1();
    FrameBufferLCD::writeOBP1(value);
    if (observer)
        observer->onWriteOBP1(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readWY() const {
    uint8_t value = FrameBufferLCD::readWY();
    if (observer)
        observer->onReadWY(value);
    return value;
}

void DebuggableFrameBufferLCD::writeWY(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readWY();
    FrameBufferLCD::writeWY(value);
    if (observer)
        observer->onWriteWY(oldValue, value);
}

uint8_t DebuggableFrameBufferLCD::readWX() const {
    uint8_t value = FrameBufferLCD::readWX();
    if (observer)
        observer->onReadWX(value);
    return value;
}

void DebuggableFrameBufferLCD::writeWX(uint8_t value) {
    uint8_t oldValue = FrameBufferLCD::readWX();
    FrameBufferLCD::writeWX(value);
    if (observer)
        observer->onWriteWX(oldValue, value);
}

void DebuggableFrameBufferLCD::setObserver(ILCDIODebug::Observer *o) {
    observer = o;
}
