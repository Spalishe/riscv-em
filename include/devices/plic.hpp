#pragma once

#include "mmio.h"

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
