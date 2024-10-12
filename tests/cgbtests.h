#ifndef CGBTESTS_H
#define CGBTESTS_H

#include "testutils/catch.h"
#include "testutils/runners.h"

#include "utils/strings.h"

#define WIP_ONLY_TEST_ROMS 0

#define RUN_TEST_ROMS(...)                                                                                             \
    static RunnerAdapter adapter {TESTS_ROOT_FOLDER "/roms/cgb/", TESTS_ROOT_FOLDER "/results/cgb/"};                  \
    const auto [params] = TABLE(RunnerAdapter::Params, ({__VA_ARGS__}));                                               \
    REQUIRE(adapter.run(params))

TEST_CASE("cgb", "[emulation]") {
#if !WIP_ONLY_TEST_ROMS
    SECTION("boot") {
        RUN_TEST_ROMS(
            // docboy
            F {"docboy/boot/boot_bg_palettes.gbc", "docboy/ok.png"},
            F {"docboy/boot/boot_regs_interrupts.gbc", "docboy/ok.png"},
            F {"docboy/boot/boot_regs_joypad.gbc", "docboy/ok.png"},
            F {"docboy/boot/boot_regs_serial.gbc", "docboy/ok.png"},
            F {"docboy/boot/boot_regs_timers.gbc", "docboy/ok.png"},
            F {"docboy/boot/boot_regs_video.gbc", "docboy/ok.png"},
            F {"docboy/boot/boot_vram1.gbc", "docboy/ok.png"}, );
    }

    SECTION("banks") {
        RUN_TEST_ROMS(
            // docboy
            F {"docboy/banks/vram_bank_switch.gbc", "docboy/ok.png"},
            F {"docboy/banks/wram_bank_switch.gbc", "docboy/ok.png"});
    }

    SECTION("ir") {
        RUN_TEST_ROMS(
            // docboy
            F {"docboy/ir/rp_write_read.gbc", "docboy/ok.png"}, );
    }

    SECTION("undocumented") {
        RUN_TEST_ROMS(
            // docboy
            F {"docboy/undocumented/ff72_write_read.gbc", "docboy/ok.png"},
            F {"docboy/undocumented/ff73_write_read.gbc", "docboy/ok.png"},
            F {"docboy/undocumented/ff74_write_read.gbc", "docboy/ok.png"},
            F {"docboy/undocumented/ff75_write_read.gbc", "docboy/ok.png"});
    }

    SECTION("ppu") {
        RUN_TEST_ROMS(
            // mattcurrie
            F {"mattcurrie/cgb-acid2.gbc", "mattcurrie/cgb-acid2.png"},

            // magen
            F {"magen/bg_oam_priority.gbc", "magen/bg_oam_priority.png"},
            F {"magen/oam_internal_priority.gbc", "magen/oam_internal_priority.png"},

            // samesuite
            F {"samesuite/ppu/blocking_bgpi_increase.gb", "samesuite/ppu/blocking_bgpi_increase.png"},

            // docboy
            F {"docboy/ppu/rendering/bg_attr_bank.gbc", "docboy/ppu/bg_attr_bank.png"},
            F {"docboy/ppu/rendering/bg_attr_horizontal_flip.gbc", "docboy/ppu/bg_attr_horizontal_flip.png"},
            F {"docboy/ppu/rendering/bg_attr_palette.gbc", "docboy/ppu/bg_attr_palette.png"},
            F {"docboy/ppu/rendering/bg_attr_priority.gbc", "docboy/ppu/bg_attr_priority.png"},
            F {"docboy/ppu/rendering/bg_attr_priority_color_0.gbc", "docboy/ppu/bg_attr_priority_color_0.png"},
            F {"docboy/ppu/rendering/bg_attr_priority_color_0_lcdc_bit_0.gbc",
               "docboy/ppu/bg_attr_priority_color_0_lcdc_bit_0.png"},
            F {"docboy/ppu/rendering/bg_attr_priority_lcdc_bit_0.gbc", "docboy/ppu/bg_attr_priority_lcdc_bit_0.png"},
            F {"docboy/ppu/rendering/bg_attr_vertical_flip_scy.gbc", "docboy/ppu/bg_attr_vertical_flip_scy.png"},
            F {"docboy/ppu/rendering/bg_attrs_lcdc_bit_3.gbc", "docboy/ppu/bg_attrs_lcdc_bit_3.png"},
            F {"docboy/ppu/rendering/lcdc_bit_0.gbc", "docboy/ppu/lcdc_bit_0.png"},
            F {"docboy/ppu/rendering/obj_attr_bank.gbc", "docboy/ppu/obj_attr_bank.png"},
            F {"docboy/ppu/rendering/obj_attr_palette.gbc", "docboy/ppu/obj_attr_palette.png"},
            F {"docboy/ppu/rendering/win_attr_bank.gbc", "docboy/ppu/win_attr_bank.png"},
            F {"docboy/ppu/rendering/win_attr_palette.gbc", "docboy/ppu/win_attr_palette.png"},
            F {"docboy/ppu/rendering/win_attrs_lcdc_bit_6.gbc", "docboy/ppu/win_attrs_lcdc_bit_6.png"},
            F {"docboy/ppu/bcpd_write_read.gbc", "docboy/ok.png"}, F {"docboy/ppu/bcps_increment.gbc", "docboy/ok.png"},
            F {"docboy/ppu/bcps_increment_overflow.gbc", "docboy/ok.png"},
            F {"docboy/ppu/ly_timing_round0.gbc", "docboy/ok.png"},
            F {"docboy/ppu/ly_timing_round1.gbc", "docboy/ok.png"},
            F {"docboy/ppu/ocpd_write_read.gbc", "docboy/ok.png"}, F {"docboy/ppu/ocps_increment.gbc", "docboy/ok.png"},
            F {"docboy/ppu/ocps_increment_overflow.gbc", "docboy/ok.png"},
            F {"docboy/ppu/opri_write_read.gbc", "docboy/ok.png"},

        );
    }

    SECTION("apu") {
        RUN_TEST_ROMS(
            // docboy
            F {"docboy/apu/ch1/square_position_retrigger_round1.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch1/square_position_retrigger_round2.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch1/square_position_round1.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch1/square_position_round2.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch1/volume_sweep_round_1.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch1/volume_sweep_round_2.gbc", "docboy/ok.png"},

            F {"docboy/apu/ch2/square_position_retrigger_round1.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch2/square_position_retrigger_round2.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch2/square_position_round1.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch2/square_position_round2.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch2/volume_sweep_round_1.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch2/volume_sweep_round_2.gbc", "docboy/ok.png"},

            F {"docboy/apu/ch4/volume_sweep_round_1.gbc", "docboy/ok.png"},
            F {"docboy/apu/ch4/volume_sweep_round_2.gbc", "docboy/ok.png"},

            // samesuite
            F {"samesuite/apu/div_trigger_volume_10.gb", "samesuite/apu/div_trigger_volume_10.png",
               COLOR_TOLERANCE_MEDIUM},
            F {"samesuite/apu/div_write_trigger.gb", "samesuite/apu/div_write_trigger.png", COLOR_TOLERANCE_MEDIUM},
            F {"samesuite/apu/div_write_trigger_10.gb", "samesuite/apu/div_write_trigger_10.png",
               COLOR_TOLERANCE_MEDIUM},
            F {"samesuite/apu/div_trigger_volume_10.gb", "samesuite/apu/div_trigger_volume_10.png",
               COLOR_TOLERANCE_MEDIUM},
            F {"samesuite/apu/div_write_trigger_volume.gb", "samesuite/apu/div_write_trigger_volume.png",
               COLOR_TOLERANCE_MEDIUM},
            F {"samesuite/apu/div_write_trigger_volume_10.gb", "samesuite/apu/div_write_trigger_volume_10.png",
               COLOR_TOLERANCE_MEDIUM}, );
    }
#else
    SECTION("wip") {
        RUN_TEST_ROMS(

        );
    }
#endif
}

#endif // CGBTESTS_H