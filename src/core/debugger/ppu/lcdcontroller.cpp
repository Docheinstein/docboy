#include "lcdcontroller.h"

DebuggableLCDController::DebuggableLCDController(IDMA& dma) :
    LCDController(dma),
    observer() {
}

uint8_t DebuggableLCDController::readLCDC() const {
    uint8_t value = LCDController::readLCDC();
    if (observer)
        observer->onReadLCDC(value);
    return value;
}

void DebuggableLCDController::writeLCDC(uint8_t value) {
    uint8_t oldValue = LCDController::readLCDC();
    LCDController::writeLCDC(value);
    if (observer)
        observer->onWriteLCDC(oldValue, value);
}

uint8_t DebuggableLCDController::readSTAT() const {
    uint8_t value = LCDController::readSTAT();
    if (observer)
        observer->onReadSTAT(value);
    return value;
}

void DebuggableLCDController::writeSTAT(uint8_t value) {
    uint8_t oldValue = LCDController::readSTAT();
    LCDController::writeSTAT(value);
    if (observer)
        observer->onWriteSTAT(oldValue, value);
}

uint8_t DebuggableLCDController::readSCY() const {
    uint8_t value = LCDController::readSCY();
    if (observer)
        observer->onReadSCY(value);
    return value;
}

void DebuggableLCDController::writeSCY(uint8_t value) {
    uint8_t oldValue = LCDController::readSCY();
    LCDController::writeSCY(value);
    if (observer)
        observer->onWriteSCY(oldValue, value);
}

uint8_t DebuggableLCDController::readSCX() const {
    uint8_t value = LCDController::readSCX();
    if (observer)
        observer->onReadSCX(value);
    return value;
}

void DebuggableLCDController::writeSCX(uint8_t value) {
    uint8_t oldValue = LCDController::readSCX();
    LCDController::writeSCX(value);
    if (observer)
        observer->onWriteSCX(oldValue, value);
}

uint8_t DebuggableLCDController::readLY() const {
    uint8_t value = LCDController::readLY();
    if (observer)
        observer->onReadLY(value);
    return value;
}

void DebuggableLCDController::writeLY(uint8_t value) {
    uint8_t oldValue = LCDController::readLY();
    LCDController::writeLY(value);
    if (observer)
        observer->onWriteLY(oldValue, value);
}

uint8_t DebuggableLCDController::readLYC() const {
    uint8_t value = LCDController::readLYC();
    if (observer)
        observer->onReadLYC(value);
    return value;
}

void DebuggableLCDController::writeLYC(uint8_t value) {
    uint8_t oldValue = LCDController::readLYC();
    LCDController::writeLYC(value);
    if (observer)
        observer->onWriteLYC(oldValue, value);
}

uint8_t DebuggableLCDController::readDMA() const {
    uint8_t value = LCDController::readDMA();
    if (observer)
        observer->onReadDMA(value);
    return value;
}

void DebuggableLCDController::writeDMA(uint8_t value) {
    uint8_t oldValue = LCDController::readDMA();
    LCDController::writeDMA(value);
    if (observer)
        observer->onWriteDMA(oldValue, value);
}

uint8_t DebuggableLCDController::readBGP() const {
    uint8_t value = LCDController::readBGP();
    if (observer)
        observer->onReadBGP(value);
    return value;
}

void DebuggableLCDController::writeBGP(uint8_t value) {
    uint8_t oldValue = LCDController::readBGP();
    LCDController::writeBGP(value);
    if (observer)
        observer->onWriteBGP(oldValue, value);
}

uint8_t DebuggableLCDController::readOBP0() const {
    uint8_t value = LCDController::readOBP0();
    if (observer)
        observer->onReadOBP0(value);
    return value;
}

void DebuggableLCDController::writeOBP0(uint8_t value) {
    uint8_t oldValue = LCDController::readOBP0();
    LCDController::writeOBP0(value);
    if (observer)
        observer->onWriteOBP0(oldValue, value);
}

uint8_t DebuggableLCDController::readOBP1() const {
    uint8_t value = LCDController::readOBP1();
    if (observer)
        observer->onReadOBP1(value);
    return value;
}

void DebuggableLCDController::writeOBP1(uint8_t value) {
    uint8_t oldValue = LCDController::readOBP1();
    LCDController::writeOBP1(value);
    if (observer)
        observer->onWriteOBP1(oldValue, value);
}

uint8_t DebuggableLCDController::readWY() const {
    uint8_t value = LCDController::readWY();
    if (observer)
        observer->onReadWY(value);
    return value;
}

void DebuggableLCDController::writeWY(uint8_t value) {
    uint8_t oldValue = LCDController::readWY();
    LCDController::writeWY(value);
    if (observer)
        observer->onWriteWY(oldValue, value);
}

uint8_t DebuggableLCDController::readWX() const {
    uint8_t value = LCDController::readWX();
    if (observer)
        observer->onReadWX(value);
    return value;
}

void DebuggableLCDController::writeWX(uint8_t value) {
    uint8_t oldValue = LCDController::readWX();
    LCDController::writeWX(value);
    if (observer)
        observer->onWriteWX(oldValue, value);
}

void DebuggableLCDController::setObserver(ILCDIODebug::Observer* o) {
    observer = o;
}
