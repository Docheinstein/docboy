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
        STAT = 0b10000000 | value;
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

    BYTE(LCDC, Specs::Registers::Video::LCDC, IF_BOOTROM_ELSE(0, 0x91));
    BYTE(STAT, Specs::Registers::Video::STAT, IF_BOOTROM_ELSE(0x80, 0x85));
    BYTE(SCY, Specs::Registers::Video::SCY);
    BYTE(SCX, Specs::Registers::Video::SCX);
    BYTE(LY, Specs::Registers::Video::LY);
    BYTE(LYC, Specs::Registers::Video::LYC);
    BYTE(DMA, Specs::Registers::Video::DMA);
    BYTE(BGP, Specs::Registers::Video::BGP, IF_BOOTROM_ELSE(0, 0xFC));
    BYTE(OBP0, Specs::Registers::Video::OBP0);
    BYTE(OBP1, Specs::Registers::Video::OBP1);
    BYTE(WY, Specs::Registers::Video::WY);
    BYTE(WX, Specs::Registers::Video::WX);

private:
    Dma& dma;
};

#endif // VIDEO_H