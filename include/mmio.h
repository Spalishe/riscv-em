#pragma once

#include <stdint.h>
#include <vector>
#include <dram.h>
#include <csr.h>
#include <optional>
#include <cstdio>
#include <atomic>
#include <thread>
#include <iostream>
#include "libfdt.hpp"

struct HART;
void cpu_trap(struct HART *hart, uint64_t cause, uint64_t tval, bool is_interrupt);

uint8_t h_cpu_id(struct HART *hart);
uint64_t h_cpu_csr_read(struct HART *hart, uint64_t addr);
void h_cpu_csr_write(struct HART *hart, uint64_t addr, uint64_t value);


struct Device {
    uint64_t base;
    uint64_t size;
    DRAM& ram;

    Device(uint64_t base, uint64_t size, DRAM& ram)
        : base(base), size(size), ram(ram) {}

    virtual uint64_t read(HART* hart,uint64_t addr,uint64_t size) = 0;
    virtual void write(HART* hart,uint64_t addr, uint64_t size,uint64_t value) = 0;
};

struct ROM : public Device {
    ROM(uint64_t base, uint64_t size, DRAM& ram)
        : Device(base, size, ram) {}

    uint64_t read(HART* hart,uint64_t addr,uint64_t size);

    void write(HART* hart, uint64_t addr, uint64_t size,uint64_t val);
};

#ifndef MIE_MEIE_BIT
#define MIE_MEIE_BIT MIP_MEIP
#endif
#ifndef SIE_SEIE_BIT
#define SIE_SEIE_BIT MIP_SEIP
#endif

static inline uint64_t mcause_encode_ext_irq(bool machine_level) {
    const uint64_t intr = 1ULL << 63;
    const uint64_t code = machine_level ? (uint64_t)IRQ_MEXT : (uint64_t)IRQ_SEXT;
    return intr | code;
}

static constexpr uint64_t PLIC_OFF_PRIORITY    = 0x0000;      // +4*src
static constexpr uint64_t PLIC_OFF_PENDING     = 0x1000;      // read-only
static constexpr uint64_t PLIC_OFF_ENABLE_BASE = 0x2000;      // +0x80*ctx
static constexpr uint64_t PLIC_OFF_CTX_BASE    = 0x200000;    // +0x1000*ctx
static constexpr uint64_t PLIC_CTX_THRESHOLD   = 0x0;
static constexpr uint64_t PLIC_CTX_CLAIMCOMP   = 0x4;

static constexpr uint32_t PLIC_NUM_CONTEXTS = 2;   // 0: M-mode, 1: S-mode

// helpers: idx calc
static inline bool plic_is_priority(uint64_t off) { return off >= PLIC_OFF_PRIORITY && off < PLIC_OFF_PENDING; }
static inline bool plic_is_pending(uint64_t off)  { return off == PLIC_OFF_PENDING; }
static inline bool plic_is_enable(uint64_t off)   { return (off >= PLIC_OFF_ENABLE_BASE) && (off < PLIC_OFF_CTX_BASE); }
static inline bool plic_is_ctx(uint64_t off)      { return off >= PLIC_OFF_CTX_BASE; }
static inline uint64_t plic_ctx_prio(uint64_t ctx)       { return (ctx & 1) ? 0x9 : 0xB; }

static inline uint32_t plic_priority_index(uint64_t off) { return (off - PLIC_OFF_PRIORITY) >> 2; }
static inline uint32_t plic_enable_context(uint64_t off) { return (off - PLIC_OFF_ENABLE_BASE) / 0x80; }
static inline uint64_t plic_enable_offset_in_ctx(uint64_t off) { return (off - PLIC_OFF_ENABLE_BASE) % 0x80; }

static inline uint32_t plic_ctx_index(uint64_t off) { return (off - PLIC_OFF_CTX_BASE) / 0x1000; }
static inline uint64_t plic_ctx_reg_offset(uint64_t off) { return (off - PLIC_OFF_CTX_BASE) % 0x1000; }

struct PLIC : public Device {
    // Configuration
    uint32_t num_sources; // source IDs: 1..num_sources ; 0 is reserved

    // State
    std::vector<uint32_t> priority;                 // [0..num_sources], 0th unused
    std::vector<uint8_t>  pending;                  // pending[src] = 0/1
    std::vector<uint8_t>  active;                   // active[src] = 0/1 (claimed but not completed)
    std::vector<std::vector<uint32_t>> enable;      // enable[ctx][word], 1 bit per source
    std::vector<uint32_t> threshold;                // threshold[ctx]
    std::vector<uint32_t> last_claimed;             // last claimed source per context (for debug)

    // Construct
    PLIC(uint64_t base, uint64_t size, DRAM& ram, uint32_t num_sources, fdt_node* fdt, uint8_t hartcount);

    void raise(uint32_t src);
    void clear(uint32_t src);
    uint64_t read(HART* hart, uint64_t addr,uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value);
    void plic_service(HART* hart);

private:
    uint32_t pick_claimable(uint32_t ctx);
    bool is_enabled(uint32_t ctx, uint32_t src);
    inline uint64_t set_bit(uint64_t value, int pos, bool state = true);
};

struct MMIO {
    std::vector<Device*> devices;
    DRAM& ram;

    MMIO(DRAM& ram)
        : ram(ram) {}

    void add(Device* dev) {
        devices.push_back(dev);
    }

    std::optional<uint64_t> load(HART* hart, uint64_t addr, uint64_t size);
    void store(HART* hart, uint64_t addr, uint64_t size, uint64_t value);
};


struct UART : public Device {
    // UART registers
    uint8_t rhr = 0;      // Receiver Holding Register (read)
    uint8_t thr = 0;      // Transmitter Holding Register (write)
    uint8_t ier = 0;      // Interrupt Enable Register
    uint8_t iir = 0x01;   // Interrupt Identification Register (no interrupt pending)
    uint8_t fcr = 0;      // FIFO Control Register
    uint8_t lcr = 0;      // Line Control Register
    uint8_t mcr = 0;      // Modem Control Register
    uint8_t lsr = 0x60;   // Line Status Register (THR empty + line idle)
    uint8_t msr = 0;      // Modem Status Register
    uint8_t scr = 0;      // Scratch Register
    
    // Divisor latch registers (when LCR[7] = 1)
    uint8_t dll = 0;      // Divisor Latch Low
    uint8_t dlm = 0;      // Divisor Latch High
    
    PLIC* plic;
    int irq_num;
    bool dlab = false;    // Divisor Latch Access Bit (from LCR[7])

    UART(uint64_t base, DRAM& ram, PLIC* plic, int irq_num, fdt_node* fdt, uint8_t hartcount);

    void trigger_irq();
    void clear_irq();
    void update_iir();
    uint64_t read(HART* hart, uint64_t addr, uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value);
    void receive_byte(uint8_t byte);
    void reset();
};

struct DTB : public Device {
    DTB(uint64_t base, uint64_t size, DRAM& ram)
        : Device(base, size, ram) {}

    uint64_t read(HART* hart,uint64_t addr, uint64_t size) override {
        return dram_load(&ram,addr,size);
    }

    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t val) override {
        //dram_store(&ram,addr,32,val);
        cpu_trap(hart,EXC_STORE_ACCESS_FAULT,addr,false);
    }
};


struct CLINT : public Device {
    // Memory-mapped registers
    std::vector<uint32_t> msip;      // one per HART, 32-bit
    std::vector<uint64_t> mtimecmp;  // one per HART, 64-bit
    std::atomic<uint64_t> mtime = 0; // global timer
    bool stop_timer = false;
    std::thread timer_thread;

    CLINT(uint64_t base, DRAM& ram, uint32_t num_harts, fdt_node* fdt, uint8_t hartcount);
    void start_timer(uint64_t freq_hz, HART* hart);
    void stop_timer_thread();
    uint64_t read(HART* hart, uint64_t addr, uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value);

private:
    void update_mip(HART* hart);
};