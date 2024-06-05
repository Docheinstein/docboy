#ifndef VIDEO_H
#define VIDEO_H

#include "docboy/bootrom/helpers.h"
#include "docboy/dma/dma.h"
#include "utils/parcel.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/common/specs.h"
#endif

class VideoIO {
public:
    explicit VideoIO(Dma& dma) :
        dma_controller {dma} {
    }

    void write_stat(uint8_t value) {
        stat = 0b10000000 | (value & 0b01111000) | keep_bits<3>(stat);
    }

    void write_dma(uint8_t value) {
        dma = value;
        dma_controller.start_transfer(dma << 8);
    }

    void save_state(Parcel& parcel) const {
        parcel.write_uint8(lcdc);
        parcel.write_uint8(stat);
        parcel.write_uint8(scy);
        parcel.write_uint8(scx);
        parcel.write_uint8(ly);
        parcel.write_uint8(lyc);
        parcel.write_uint8(dma);
        parcel.write_uint8(bgp);
        parcel.write_uint8(obp0);
        parcel.write_uint8(obp1);
        parcel.write_uint8(wy);
        parcel.write_uint8(wx);
    }

    void load_state(Parcel& parcel) {
        lcdc = parcel.read_uint8();
        stat = parcel.read_uint8();
        scy = parcel.read_uint8();
        scx = parcel.read_uint8();
        ly = parcel.read_uint8();
        lyc = parcel.read_uint8();
        dma = parcel.read_uint8();
        bgp = parcel.read_uint8();
        obp0 = parcel.read_uint8();
        obp1 = parcel.read_uint8();
        wy = parcel.read_uint8();
        wx = parcel.read_uint8();
    }

    void reset() {
        lcdc = if_bootrom_else(0, 0x91);
        stat = 0x80;
        stat = if_bootrom_else(0x80, 0x85);
        scy = 0;
        scx = 0;
        ly = 0;
        lyc = 0;
        dma = 0xFF;
        bgp = if_bootrom_else(0, 0xFC);
        obp0 = 0;
        obp1 = 0;
        wy = 0;
        wx = 0;
    }

    byte lcdc {make_byte(Specs::Registers::Video::LCDC)};
    byte stat {make_byte(Specs::Registers::Video::STAT)};
    byte scy {make_byte(Specs::Registers::Video::SCY)};
    byte scx {make_byte(Specs::Registers::Video::SCX)};
    byte ly {make_byte(Specs::Registers::Video::LY)};
    byte lyc {make_byte(Specs::Registers::Video::LYC)};
    byte dma {make_byte(Specs::Registers::Video::DMA)};
    byte bgp {make_byte(Specs::Registers::Video::BGP)};
    byte obp0 {make_byte(Specs::Registers::Video::OBP0)};
    byte obp1 {make_byte(Specs::Registers::Video::OBP1)};
    byte wy {make_byte(Specs::Registers::Video::WY)};
    byte wx {make_byte(Specs::Registers::Video::WX)};

private:
    Dma& dma_controller;
};

#endif // VIDEO_H