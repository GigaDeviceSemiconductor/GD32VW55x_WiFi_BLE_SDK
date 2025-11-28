# Welcome to the GD32VW55x Wi-Fi BLE SDK

Welcome to the GD32VW55x Wi-Fi BLE SDK.The GD32VW553 series MCU supports Wi-Fi 6 and Bluetooth LE 5.2 wireless connectivity. To fulfill the requirements of real-time processing and efficient communication, the GD32VW553 series MCU incorporates a new open-source instruction set architecture with a RISC-V processor core, boasting a maximum clock frequency of 160 MHz. The integrated 2.4 GHz Wi-Fi 6 RF module adheres to the IEEE 802.11ax standard and is backward compatible with the IEEE 802.11b/g/n standard, ensuring compatibility with different network environments.

GD32VW553 series MCU：
- Support Wi-Fi 6 and Bluetooth LE 5.2 wireless connectivity
- Based on RISC-V core, up to 160MHz operating frequency.
- Integrated 4MB Flash and 320KB SRAM.

This repository contains the GD32VW55x Wi-Fi and BLE SDK including core SDK,drivers and examples, a comprehensive toolkit for building embedded IoT applications.

# Getting start

## EmbeddedBuilder IDE
1. Download IDE [EmbeddedBuilder](https://www.gd32mcu.com/data/documents/toolSoftware/GD32EmbeddedBuilder_v1.5.5_Rel.7z)
2. Please reference to ~/docs/EN/AN154 GD32VW553 Quick Development Guide.pdf

## Cmake project
### Build Setup
#### Windows
1. Install CMake
- Download: https://cmake.org/download/
- Please choose version 3.15 or later; recommended: `cmake-3.20.3-windows-x86_64`
2. Install GNU Make
- Download: http://ftp.gnu.org/gnu/make/
- Recommended: `make-3.81`
3. Install 7-Zip
4. Add to system `PATH`
- After installing to the default locations, add the following paths to the system environment variable `PATH`. If you installed to different locations, add those paths instead.

```bash
C:\Program Files (x86)\GnuWin32\bin
C:\Program Files\CMake\bin
```

#### Ubuntu
1. System requirement: Ubuntu 18.04 or newer
2. Install any missing packages following the build instructions below

### Build Steps
1. Enter SDK root directory

2. Build commands
- Windows
```bash
cmake_build.bat <app> {make user command}
# Examples:
cmake_build.bat app VERBOSE=1
cmake_build.bat app clean
```
- Ubuntu
```bash
./cmake_build.sh <app> {make user command}
# Examples:
./cmake_build.sh app VERBOSE=1
./cmake_build.sh app clean
```
- The first build will extract the toolchain

3. Firmware locations
- Full flash firmware:
    `~/scripts/images/image_all.bin`
- Debug files (elf, dump, map, lst):
    `~/MSDK/projects/cmake/output/bin/`
    Contains: `MSDK.bin  MSDK.dump  MSDK.elf  MSDK.hex  MSDK.lst  MSDK.map`

### FAQ
1. If you run into executable permission issues on Ubuntu:
```bash
chmod +x cmake_build.sh
chmod +x MBL/project/mbl_afterbuild.sh
chmod +x MSDK/projects/image_afterbuild.sh
chmod +x MSDK/projects/image_prebuild.sh
```

### Development Notes
1. The toolchain and OpenOCD debugging tools are placed in `Tools`
2. `main.c` is under `MSDK/app`. You can add your own source files in the `CMakeLists.txt` in that directory.

