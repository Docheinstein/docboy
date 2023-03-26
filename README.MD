# DocBoy

GameBoy emulator written in C++20.

Work in Progress.

### Features

* CPU: m-cycle accurate emulation
* PPU/LCD (WIP)
* Cartridges: NoMBC, MBC1
* CLI debugger: supports breakpoints, watchpoints, disassembler, step by step execution (by instruction or micro-instruction), memory viewer, interrupts viewer, IO viewer
* Serial Link
* Bootroom support
* Joypad
* DMA

### TODO
* Sound
* Save/load state
* Support other platforms: Windows, macOs
* Compile as libretro core

### Building

#### Linux

```
mkdir build
cd build
cmake ..
make
```


### Usage

```
usage:  [--boot-rom BOOT-ROM] [--debugger] [--serial] [--scaling SCALING] 
        [--help] rom

positional arguments:
  rom                     ROM

optional arguments:
  -b, --boot-rom BOOT-ROM Boot ROM
  -d, --debugger          Attach CLI debugger
  -s, --serial            Display serial output
  -z, --scaling SCALING   Scaling factor
  -h, --help              Display this help message and exit.
```


### Screenshots

![DMG boot](images/dmg_boot.png)
![Blargg's cpu_instr boot](images/cpu_instrs.png)

### Tests

State of [blargg's test roms](https://github.com/retrio/gb-test-roms).

| Rom          | Passed | Status             |
|--------------|--------|--------------------|
| dmg_sound    |        |                    |
| cpu_instrs   | 11/11  | :white_check_mark: |
| instr_timing |        |                    |
| mem_timing   |        |                    |
| mem_timing-2 |        |                    |
| oam_bug      |        |                    |



