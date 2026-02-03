/*
Copyright 2026 Spalishe

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#pragma once
#include <stdint.h>

//      Name        Number   Priv       Description
//------------------------------------------------------------------------------------

// User Trap Setup
#define USTATUS     0x000 // URW User status register.
#define UIE         0x004 // URW User interrupt-enable register.
#define UTVEC       0x005 // URW User trap handler base address.

//User Trap Handling
#define USCRATCH    0x040 // URW Scratch register for user trap handlers.
#define UEPC        0x041 // URW User exception program counter.
#define UCAUSE      0x042 // URW User trap cause.
#define UTVAL       0x043 // URW User bad address or instruction.
#define UIP         0x044 // URW User interrupt pending.

//User Floating-Point CSRs
#define FFLAGS      0x001 // URW Floating-Point Accrued Exceptions.
#define FRM         0x002 // URW Floating-Point Dynamic Rounding Mode.
#define FCSR        0x003 // URW Floating-Point Control and Status Register (frm + fflags)

//User Counter/Timers
#define CYCLE       0xC00 // URO Cycle counter for RDCYCLE instruction.
#define TIME        0xC01 // URO Timer for RDTIME instruction.
#define INSTRET     0xC02 // URO Instructions-retired counter for RDINSTRET instruction.
#define HPMCOUNTER3 0xC03 // URO Performance-monitoring counter.
#define HPMCOUNTER4 0xC04 // URO Performance-monitoring counter.
// ... hpm counter 4-31 (TODO)
#define HPMCOUNTER31 0xC1F // URO Performance-monitoring counter.
#define CYCLEH      0xC80  // URO Upper 32 bits of cycle, RV32I only.
#define TIMEH       0xC81  // URO Upper 32 bits of time, RV32I only.
#define INSTRETH    0xC82  // URO Upper 32 bits of instret, RV32I only.
#define HPMCOUNTER3H 0xC83 // URO Upper 32 bits of hpmcounter3, RV32I only.
#define HPMCOUNTER4H 0xC84 // URO Upper 32 bits of hpmcounter4, RV32I only.
// ... hpm counter 4-31 (TODO)
#define HPMCOUNTER31H 0xC9F URO 


//Supervisor Trap Setup
#define SSTATUS     0x100 // SRW Supervisor status register.
#define SEDELEG     0x102 // SRW Supervisor exception delegation register.
#define SIDELEG     0x103 // SRW Supervisor interrupt delegation register.
#define SIE         0x104 // SRW Supervisor interrupt-enable register.
#define STVEC       0x105 // SRW Supervisor trap handler base address.
#define SCOUNTEREN  0x106 // SRW Supervisor counter enable.

//Supervisor Trap Handling
#define SSCRATCH    0x140 // SRW Scratch register for supervisor trap handlers.
#define SEPC        0x141 // SRW Supervisor exception program counter.
#define SCAUSE      0x142 // SRW Supervisor trap cause.
#define STVAL       0x143 // SRW Supervisor bad address or instruction.
#define SIP         0x144 // SRW Supervisor interrupt pending.

//https://lists.riscv.org/g/tech-privileged/topic/fast_track_stimecmp/78649672
#define STIMECMP    0x14D // SRW Supervisor timer compare
#define STIMECMPH   0x15D // SRW Supervisor timer compare high

//Supervisor Protection and Translation
#define SATP        0x180 // SRW Supervisor address translation and protection.

//Machine Information Registers
#define MVENDORID   0xF11 // MRO Vendor ID.
#define MARCHID     0xF12 // MRO Architecture ID.
#define MIMPID      0xF13 // MRO Implementation ID.
#define MHARTID     0xF14 // MRO Hardware thread ID.

//Machine Trap Setup
#define MSTATUS     0x300 // MRW Machine status register.
#define MISA        0x301 // MRW ISA and extensions
#define MEDELEG     0x302 // MRW Machine exception delegation register.
#define MIDELEG     0x303 // MRW Machine interrupt delegation register.
#define MIE         0x304 // MRW Machine interrupt-enable register.
#define MTVEC       0x305 // MRW Machine trap-handler base address.
#define MCOUNTEREN  0x306 // MRW Machine counter enable.

//Machine Trap Handling
#define MSCRATCH    0x340 // MRW Scratch register for machine trap handlers.
#define MEPC        0x341 // MRW Machine exception program counter.
#define MCAUSE      0x342 // MRW Machine trap cause.
#define MTVAL       0x343 // MRW Machine bad address or instruction.
#define MIP         0x344 // MRW Machine interrupt pending.

//Machine Memory Protection
#define PMPCFG0     0x3A0 // MRW Physical memory protection configuration.
#define PMPCFG1     0x3A1 // MRW Physical memory protection configuration, RV32 only.
#define PMPCFG2     0x3A2 // MRW Physical memory protection configuration.
#define PMPCFG3     0x3A3 // MRW Physical memory protection configuration, RV32 only.
#define PMPCFG4     0x3A4 // MRW Physical memory protection configuration.
#define PMPCFG5     0x3A5 // MRW Physical memory protection configuration, RV32 only.
#define PMPCFG6     0x3A6 // MRW Physical memory protection configuration.
#define PMPCFG7     0x3A7 // MRW Physical memory protection configuration, RV32 only.
#define PMPCFG8     0x3A8 // MRW Physical memory protection configuration.
#define PMPCFG9     0x3A9 // MRW Physical memory protection configuration, RV32 only.
#define PMPCFG10    0x3AA // MRW Physical memory protection configuration.
#define PMPCFG11    0x3AB // MRW Physical memory protection configuration, RV32 only.
#define PMPCFG12    0x3AC // MRW Physical memory protection configuration.
#define PMPCFG13    0x3AD // MRW Physical memory protection configuration, RV32 only.
#define PMPCFG14    0x3AE // MRW Physical memory protection configuration.
#define PMPCFG15    0x3AF // MRW Physical memory protection configuration, RV32 only.

#define PMPADDR0    0x3B0 // MRW Physical memory protection address register.
#define PMPADDR1    0x3B1 // MRW Physical memory protection address register.
#define PMPADDR2    0x3B2 // MRW Physical memory protection address register.
#define PMPADDR3    0x3B3 // MRW Physical memory protection address register.
#define PMPADDR4    0x3B4 // MRW Physical memory protection address register.
#define PMPADDR5    0x3B5 // MRW Physical memory protection address register.
#define PMPADDR6    0x3B6 // MRW Physical memory protection address register.
#define PMPADDR7    0x3B7 // MRW Physical memory protection address register.
#define PMPADDR8    0x3B8 // MRW Physical memory protection address register.
#define PMPADDR9    0x3B9 // MRW Physical memory protection address register.
#define PMPADDR10   0x3BA // MRW Physical memory protection address register.
#define PMPADDR11   0x3BB // MRW Physical memory protection address register.
#define PMPADDR12   0x3BC // MRW Physical memory protection address register.
#define PMPADDR13   0x3BD // MRW Physical memory protection address register.
#define PMPADDR14   0x3BE // MRW Physical memory protection address register.
#define PMPADDR15   0x3BF // MRW Physical memory protection address register.
#define PMPADDR16   0x3C0 // MRW Physical memory protection address register.
#define PMPADDR17   0x3C1 // MRW Physical memory protection address register.
#define PMPADDR18   0x3C2 // MRW Physical memory protection address register.
#define PMPADDR19   0x3C3 // MRW Physical memory protection address register.
#define PMPADDR20   0x3C4 // MRW Physical memory protection address register.
#define PMPADDR21   0x3C5 // MRW Physical memory protection address register.
#define PMPADDR22   0x3C6 // MRW Physical memory protection address register.
#define PMPADDR23   0x3C7 // MRW Physical memory protection address register.
#define PMPADDR24   0x3C8 // MRW Physical memory protection address register.
#define PMPADDR25   0x3C9 // MRW Physical memory protection address register.
#define PMPADDR26   0x3CA // MRW Physical memory protection address register.
#define PMPADDR27   0x3CB // MRW Physical memory protection address register.
#define PMPADDR28   0x3CC // MRW Physical memory protection address register.
#define PMPADDR29   0x3CD // MRW Physical memory protection address register.
#define PMPADDR30   0x3CE // MRW Physical memory protection address register.
#define PMPADDR31   0x3CF // MRW Physical memory protection address register.
#define PMPADDR32   0x3D0 // MRW Physical memory protection address register.
#define PMPADDR33   0x3D1 // MRW Physical memory protection address register.
#define PMPADDR34   0x3D2 // MRW Physical memory protection address register.
#define PMPADDR35   0x3D3 // MRW Physical memory protection address register.
#define PMPADDR36   0x3D4 // MRW Physical memory protection address register.
#define PMPADDR37   0x3D5 // MRW Physical memory protection address register.
#define PMPADDR38   0x3D6 // MRW Physical memory protection address register.
#define PMPADDR39   0x3D7 // MRW Physical memory protection address register.
#define PMPADDR40   0x3D8 // MRW Physical memory protection address register.
#define PMPADDR41   0x3D9 // MRW Physical memory protection address register.
#define PMPADDR42   0x3DA // MRW Physical memory protection address register.
#define PMPADDR43   0x3DB // MRW Physical memory protection address register.
#define PMPADDR44   0x3DC // MRW Physical memory protection address register.
#define PMPADDR45   0x3DD // MRW Physical memory protection address register.
#define PMPADDR46   0x3DE // MRW Physical memory protection address register.
#define PMPADDR47   0x3DF // MRW Physical memory protection address register.
#define PMPADDR48   0x3E0 // MRW Physical memory protection address register.
#define PMPADDR49   0x3E1 // MRW Physical memory protection address register.
#define PMPADDR50   0x3E2 // MRW Physical memory protection address register.
#define PMPADDR51   0x3E3 // MRW Physical memory protection address register.
#define PMPADDR52   0x3E4 // MRW Physical memory protection address register.
#define PMPADDR53   0x3E5 // MRW Physical memory protection address register.
#define PMPADDR54   0x3E6 // MRW Physical memory protection address register.
#define PMPADDR55   0x3E7 // MRW Physical memory protection address register.
#define PMPADDR56   0x3E8 // MRW Physical memory protection address register.
#define PMPADDR57   0x3E9 // MRW Physical memory protection address register.
#define PMPADDR58   0x3EA // MRW Physical memory protection address register.
#define PMPADDR59   0x3EB // MRW Physical memory protection address register.
#define PMPADDR60   0x3EC // MRW Physical memory protection address register.
#define PMPADDR61   0x3ED // MRW Physical memory protection address register.
#define PMPADDR62   0x3EE // MRW Physical memory protection address register.
#define PMPADDR63   0x3EF // MRW Physical memory protection address register.

//Machine Counter/Timers
#define MCYCLE       0xB00 // MRW Machine cycle counter.
#define MINSTRET     0xB02 // MRW Machine instructions-retired counter.
#define MHPMCOUNTER3 0xB03 // MRW Machine performance-monitoring counter.
#define MHPMCOUNTER4 0xB04 // MRW Machine performance-monitoring counter.
// #define ...
#define MHPMCOUNTER31 0xB1F // MRW Machine performance-monitoring counter.
#define MCYCLEH       0xB80 // MRW Upper 32 bits of mcycle, RV32I only.
#define MINSTRETH     0xB82 // MRW Upper 32 bits of minstret, RV32I only.
#define MHPMCOUNTER3H 0xB83 // MRW Upper 32 bits of mhpmcounter3, RV32I only.
#define MHPMCOUNTER4H 0xB84 // MRW Upper 32 bits of mhpmcounter4, RV32I only.
// ...
#define MHPMCOUNTER31H 0xB9F // MRW Upper 32 bits of mhpmcounter31, RV32I only.

//Machine Counter Setup
#define MCOUNTINHIBIT 0x320 // MRW Machine counter-inhibit register.
#define MHPMEVENT3    0x323 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT4    0x324 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT5    0x325 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT6    0x326 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT7    0x327 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT8    0x328 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT9    0x329 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT10    0x32A // MRW Machine performance-monitoring event selector.
#define MHPMEVENT11    0x32B // MRW Machine performance-monitoring event selector.
#define MHPMEVENT12    0x32C // MRW Machine performance-monitoring event selector.
#define MHPMEVENT13    0x32D // MRW Machine performance-monitoring event selector.
#define MHPMEVENT14    0x32E // MRW Machine performance-monitoring event selector.
#define MHPMEVENT15    0x32F // MRW Machine performance-monitoring event selector.
#define MHPMEVENT16    0x330 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT17    0x331 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT18    0x332 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT19    0x333 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT20    0x334 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT21    0x335 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT22    0x336 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT23    0x337 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT24    0x338 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT25    0x339 // MRW Machine performance-monitoring event selector.
#define MHPMEVENT26    0x33A // MRW Machine performance-monitoring event selector.
#define MHPMEVENT27    0x33B // MRW Machine performance-monitoring event selector.
#define MHPMEVENT28    0x33C // MRW Machine performance-monitoring event selector.
#define MHPMEVENT29    0x33D // MRW Machine performance-monitoring event selector.
#define MHPMEVENT30    0x33E // MRW Machine performance-monitoring event selector.
#define MHPMEVENT31 0x33F // MRW Machine performance-monitoring event selector.

//Debug/Trace Registers (shared with Debug Mode)
#define TSELECT     0x7A0 // MRW Debug/Trace trigger register select.
#define TDATA1      0x7A1 // MRW First Debug/Trace trigger data register.
#define TDATA2      0x7A2 // MRW Second Debug/Trace trigger data register.
#define TDATA3      0x7A3 // MRW Third Debug/Trace trigger data register.

//Debug Mode Registers
#define DCSR        0x7B0 // DRW Debug control and status register.
#define DPC         0x7B1 // DRW Debug PC.
#define DSCRATCH0   0x7B2 // DRW Debug scratch register 0.
#define DSCRATCH1   0x7B3 // DRW Debug scratch register 1.

// mcause / scause exception codes (Interrupt bit is set separately)
#define EXC_INST_ADDR_MISALIGNED  0
#define EXC_INST_ACCESS_FAULT     1
#define EXC_ILLEGAL_INSTRUCTION   2
#define EXC_BREAKPOINT            3
#define EXC_LOAD_ADDR_MISALIGNED  4
#define EXC_LOAD_ACCESS_FAULT     5
#define EXC_STORE_ADDR_MISALIGNED 6
#define EXC_STORE_ACCESS_FAULT    7
#define EXC_ENV_CALL_FROM_U       8
#define EXC_ENV_CALL_FROM_S       9
#define EXC_ENV_CALL_FROM_M       11
#define EXC_INST_PAGE_FAULT       12
#define EXC_LOAD_PAGE_FAULT       13
#define EXC_STORE_PAGE_FAULT      15

// Interrupt cause codes (these are the Exception_Code field when Interrupt bit = 1)
#define IRQ_USW   0   // user software
#define IRQ_SSW   1   // supervisor software
#define IRQ_MSW   3   // machine software
#define IRQ_UTIMER 4
#define IRQ_STIMER 5
#define IRQ_MTIMER 7
#define IRQ_UEXT   8
#define IRQ_SEXT   9
#define IRQ_MEXT   11

// SSTATUS bits (these are aliases typically)
#define SSTATUS_SIE_BIT   1
#define SSTATUS_SPIE_BIT  5
#define SSTATUS_SPP_BIT   8

// mtvec/stvec mode masks
#define TVEC_MODE_MASK 0x3ULL
#define TVEC_BASE_MASK (~0x3ULL)

// MIP/MIE bit positions (spec Figure 3.10/3.11)
#define MIP_MEIP 11
#define MIP_SEIP 9
#define MIP_UEIP 8
#define MIP_MTIP 7
#define MIP_HTIP 6
#define MIP_STIP 5
#define MIP_UTIP 4
#define MIP_MSIP 3
#define MIP_HSIP 2
#define MIP_SSIP 1
#define MIP_USIP 0

#define MIE_MEIE_BIT 11
#define MIE_MSIE_BIT 3
#define MIE_MTIP_BIT 7
#define SIE_SEIE_BIT 9
#define MIE_SEIE_BIT 9
#define SIE_SSIE_BIT 1
#define MIE_SSIE_BIT 1
#define SIE_STIE_BIT 5
#define MIE_STIE_BIT 5
#define SIP_STIP_BIT 5



#define MSTATUS_MPRV 17
#define MSTATUS_MPP_LOW 11
#define MSTATUS_MPP_HIGH 12
#define MSTATUS_MIE 3
#define MSTATUS_MPIE 7
#define MSTATUS_TSR 22
#define MSTATUS_SPP 8
#define MSTATUS_SIE 1
#define MSTATUS_SPIE 5
#define MSTATUS_MXR 19
#define MSTATUS_SUM 18

#define SSTATUS_MASK 0x80000003000DE762ULL
#define SE_MASK 0x222

static const int irq_priority[] = {
    11, // MEI
    9,  // SEI
    3,  // MSI
    1,  // SSI
    7,  // MTI
    5   // STI
};