#ifndef VIDEO_H
#define VIDEO_H

#include "docboy/bootrom/macros.h"
#include "docboy/dma/dma.h"
#include "docboy/memory/byte.hpp"
#include "utils/parcel.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/shared/specs.h"
#endif

class VideoIO {
public:
    explicit VideoIO(Dma& dma) :
        dma(dma) {
    }

    void writeSTAT(uint8_t value) {
        STAT = 0b10000000 | (value & 0b01111000) | keep_bits<3>(STAT);
    }

    void writeDMA(uint8_t value) {
        DMA = value;
        dma.startTransfer(DMA << 8);
    }

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(LCDC);
        parcel.writeUInt8(STAT);
        parcel.writeUInt8(SCY);
        parcel.writeUInt8(SCX);
        parcel.writeUInt8(LY);
        parcel.writeUInt8(LYC);
        parcel.writeUInt8(DMA);
        parcel.writeUInt8(BGP);
        parcel.writeUInt8(OBP0);
        parcel.writeUInt8(OBP1);
        parcel.writeUInt8(WY);
        parcel.writeUInt8(WX);
    }

    void loadState(Parcel& parcel) {
        LCDC = parcel.readUInt8();
        STAT = parcel.readUInt8();
        SCY = parcel.readUInt8();
        SCX = parcel.readUInt8();
        LY = parcel.readUInt8();
        LYC = parcel.readUInt8();
        DMA = parcel.readUInt8();
        BGP = parcel.readUInt8();
        OBP0 = parcel.readUInt8();
        OBP1 = parcel.readUInt8();
        WY = parcel.readUInt8();
        WX = parcel.readUInt8();
    }

    void reset() {
        LCDC = IF_BOOTROM_ELSE(0, 0x91);
        STAT = IF_BOOTROM_ELSE(0x80, 0x85);
        SCY = 0;
        SCX = 0;
        LY = 0;
        LYC = 0;
        DMA = 0;
        BGP = IF_BOOTROM_ELSE(0, 0xFC);
        OBP0 = 0;
        OBP1 = 0;
        WY = 0;
        WX = 0;
    }

    byte LCDC {make_byte(Specs::Registers::Video::LCDC)};
    byte STAT {make_byte(Specs::Registers::Video::STAT)};
    byte SCY {make_byte(Specs::Registers::Video::SCY)};
    byte SCX {make_byte(Specs::Registers::Video::SCX)};
    byte LY {make_byte(Specs::Registers::Video::LY)};
    byte LYC {make_byte(Specs::Registers::Video::LYC)};
    byte DMA {make_byte(Specs::Registers::Video::DMA)};
    byte BGP {make_byte(Specs::Registers::Video::BGP)};
    byte OBP0 {make_byte(Specs::Registers::Video::OBP0)};
    byte OBP1 {make_byte(Specs::Registers::Video::OBP1)};
    byte WY {make_byte(Specs::Registers::Video::WY)};
    byte WX {make_byte(Specs::Registers::Video::WX)};

private:
    Dma& dma;
};

#endif // VIDEO_H