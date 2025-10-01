# riscv-em
A software emulator for the RISC-V instruction set architecture (ISA) written in C++. This project aims to provide a functional emulator, capable of running XV6, OpenSBI, U-Boot, and many others.
The emulator supports RV64IMA ISA, Zicsr, Zifencei, privileged ISA, CSRs, peripheral devices (UART, CLINT, PLIC, Virtio-BLK), and FDT. See the ["Features List" section](https://github.com/Spalishe/riscv-em#features-list) section for the details of features.

## Build
```bash
git clone https://github.com/Spalishe/riscv-em
cd riscv-em
make
```
Output program will be located in ./build folder
## Running
Available arguments are:
```
  --bios: File with Machine Level program (bootloader)
  --kernel: File with Supervisor Level program
  --image: File with Image file that will put on VirtIO-BLK
  --dtb: Use specified FDT instead of auto-generated
  --dumpdtb: Dumps auto-generated FDT to file
  --debug: Enables DEBUG mode
  --tests: Enables TESTING mode
```

Running:
```bash
./build/riscvem --bios fw_jump.bin --kernel u-boot.bin
```

## Features List

The emulator supports the following features:
- [x] RV64G ISA
  - [x] RV64I
  - [x] RV64M
  - [x] RV64A (No atomicity for now)
  - [ ] RV64F
  - [ ] RV64D
  - [x] Zifencei: (`fence.i` does nothing for
    now)
  - [x] Zicsr: (No atomicity for now)
- [ ] RV64C
- [x] Privileged ISA
- [x] Control and status registers (CSRs)
  - [x] Machine-level CSRs
  - [x] Supervisor-level CSRs
  - [ ] User-level CSRs
- [x] Devices
  - [x] UART: universal asynchronous receiver-transmitter
  - [x] CLINT: core local interruptor
  - [x] PLIC: platform level interrupt controller
  - [x] Virtio-BLK: virtual I/O Block Device
- [x] FDT

## Dependencies
You can install all required dependencies using:

Arch:
```bash
sudo pacman -S cmake make gcc llvm
```

Ubuntu:
```bash
sudo apt install cmake make gcc llvm
```

## License
This project is licensed under the Apache 2.0 License – see the [LICENSE](https://github.com/Spalishe/riscv-em/blob/main/LICENSE)
