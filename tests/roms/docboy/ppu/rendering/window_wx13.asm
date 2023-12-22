INCLUDE "hardware.inc"
INCLUDE "common.inc"

; Render with window enabled and WX=13.

EntryPoint:
    ; Disable PPU
    DisablePPU

    ; Place a tile to VRAM Tile Data[1]
    Memset $9010, $ff, $10

    ; Reset VRAM Tile Map
    Memset $9C00, $00, $0400

    ; Write Index 1 to VRAM Tile Map[0]
    Memset $9C00, $01, $01

    ; Set WX
    ld a, 13
    ldh [rWX], a

    ; Enable PPU with window on
    ld a, LCDCF_ON | LCDCF_BGON | LCDCF_WINON | LCDCF_WIN9C00
    ldh [rLCDC], a

    WaitVBlank

    halt
    nop
