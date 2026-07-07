# riscv-em
A software emulator for the RISC-V instruction set architecture (ISA) written in C++. This project aims to provide a functional emulator, capable of running XV6, OpenSBI, U-Boot, and many others.
The emulator supports RV64IMA ISA, privileged ISA and peripheral devices. See the ["Features List" section](https://github.com/Spalishe/riscv-em#features-list) section for the details of features.

## Running
Available arguments are:
```
  --bios: File with Machine Level program (bootloader)
  --kernel: File with Supervisor Level program
  --image: File with Image file that will put on VirtIO-BLK
  --dtb: Use specified FDT instead of auto-generated
  --dumpdtb: Dumps auto-generated FDT to file
  --gdb: Starts GDB Stub on port 1512
  --append: Append command line arguments
```

Running:
```bash
./build/riscvem --bios fw_jump.bin --kernel u-boot.bin
```

## Features List

The emulator supports the following features:
- [ ] RV64G ISA
  - [x] RV64I
  - [x] RV64M
  - [x] RV64A (No atomicity for now)
  - [x] RV64F
  - [ ] RV64D
  - [x] Zifencei
  - [x] Zicsr
- [ ] RV64C
- [ ] Bit manipulations
  - [x] Zba
  - [x] Zbb
  - [x] Zbc
  - [x] Zbs
  - [ ] Zbkb
- [ ] Zicboz (for faster memory zeroing)
- [x] Privileged ISA
- [x] Control and status registers (CSRs)
  - [x] Machine-level CSRs
  - [x] Supervisor-level CSRs
  - [x] User-level CSRs
- [x] Devices
  - [x] UART: universal asynchronous receiver-transmitter
  - [x] CLINT: core local interruptor
  - [x] PLIC: platform level interrupt controller
  - [x] Virtio-BLK: virtual I/O Block Device
  - [x] Framebuffer: virtual simple screen
- [x] FDT

## Build
```bash
git clone https://github.com/Spalishe/riscv-em
cd riscv-em
make
```
Output program will be located in corresponding target and architecture folder(f.e. build.linux.x86_64/)
You can also compile emulator as lib:
```bash
git clone https://github.com/Spalishe/riscv-em
cd riscv-em
make lib
```
And static lib as well:
```bash
git clone https://github.com/Spalishe/riscv-em
cd riscv-em
make slib
```

## Dependencies
You can install all required dependencies using:

Arch:
```bash
sudo pacman -S make gcc
```

Ubuntu:
```bash
sudo apt install make gcc
```

## License
This project is licensed under the Apache 2.0 License – see the [LICENSE](https://github.com/Spalishe/riscv-em/blob/main/LICENSE)
