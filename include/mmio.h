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

    uint64_t read(HART* hart,uint64_t addr,uint64_t size) override {
        return dram_load(&ram,addr,size);
    }

    void write(HART* hart, uint64_t addr, uint64_t size,uint64_t val) override {
        dram_store(&ram,addr,size,val);
        //cpu_trap(hart,EXC_STORE_ACCESS_FAULT,addr,false);
    }
};

// mie/sie enable bits for external interrupts (spec)
#ifndef MIE_MEIE_BIT
#define MIE_MEIE_BIT MIP_MEIP
#endif
#ifndef SIE_SEIE_BIT
#define SIE_SEIE_BIT MIP_SEIP
#endif

// mcause encoder (XLEN=64 here)
static inline uint64_t mcause_encode_ext_irq(bool machine_level) {
    const uint64_t intr = 1ULL << 63;
    const uint64_t code = machine_level ? (uint64_t)IRQ_MEXT : (uint64_t)IRQ_SEXT;
    return intr | code;
}

// ---------------- PLIC layout constants -------------------------------------
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
    PLIC(uint64_t base, uint64_t size, DRAM& ram, uint32_t num_sources, fdt_node* fdt, uint8_t hartcount)
    : Device(base, size, ram),
      num_sources(num_sources),
      priority(num_sources+1, 0),
      pending(num_sources+1, 0),
      active(num_sources+1, 0),
      enable(PLIC_NUM_CONTEXTS, std::vector<uint32_t>((num_sources+31)/32, 0)),
      threshold(PLIC_NUM_CONTEXTS, 0),
      last_claimed(PLIC_NUM_CONTEXTS, 0)
    {
        if(fdt != NULL) {
            struct fdt_node* cpus = fdt_node_find(fdt, "cpus");
            std::vector<uint32_t> irq_ext = {}; 
            for(int i=0; i < hartcount; i++) {
                struct fdt_node* cpu = fdt_node_find_reg(cpus, "cpu", i);
                struct fdt_node* cpu_irq = fdt_node_find(cpu, "interrupt-controller");
                uint32_t irq_phandle = fdt_node_get_phandle(cpu_irq);

                if (irq_phandle) {
                    irq_ext.push_back(irq_phandle);
                    irq_ext.push_back(plic_ctx_prio(0));
                    irq_ext.push_back(irq_phandle);
                    irq_ext.push_back(plic_ctx_prio(1));
                } else {
                    // missing interrupt controller in hart i
                }
            }
            
            struct fdt_node* plic_fdt = fdt_node_create_reg("plic", base);
            fdt_node_add_prop_reg(plic_fdt, "reg", base, size);
            fdt_node_add_prop_str(plic_fdt, "compatible", "sifive,plic-1.0.0");
            fdt_node_add_prop(plic_fdt, "interrupt-controller", NULL, 0);
            fdt_node_add_prop_u32(plic_fdt, "#interrupt-cells", 1);
            fdt_node_add_prop_u32(plic_fdt, "#address-cells", 0);
            fdt_node_add_prop_cells(plic_fdt, "interrupts-extended", irq_ext, irq_ext.size());
            fdt_node_add_prop_u32(plic_fdt, "riscv,ndev", 32 - 1);

            fdt_node_add_child(fdt_node_find(fdt,"soc"), plic_fdt);
            fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"soc"),"plic",base));
        }
    }

    // External device API: raise/clear source level
    // For level-triggered sources, call raise when line is high; clear when low.
    void raise(uint32_t src) {
        if (src == 0 || src > num_sources) return;
        pending[src] = 1;
        recompute_lines(); // update MEIP/SEIP based on new pending
    }
    void clear(uint32_t src) {
        if (src == 0 || src > num_sources) return;
        // If level-sensitive cleared while active, spec allows it to remain active until complete.
        pending[src] = 0;
        recompute_lines();
    }

    // Read MMIO
    uint64_t read(HART* hart, uint64_t addr,uint64_t size) override {
        if (addr < base || addr >= base + size) return 0;
        uint64_t off = addr - base;

        // PRIORITY array
        if (plic_is_priority(off)) {
            uint32_t idx = plic_priority_index(off);
            if (idx <= num_sources) return priority[idx];
            return 0;
        }
        // PENDING bits
        if (plic_is_pending(off)) {
            // pack pending bits into a 32-bit word (sources 1..32).
            // For >32 sources you may need multiple words; here we return lower 32 as example.
            uint32_t word = 0;
            uint32_t maxb = std::min<uint32_t>(num_sources, 32);
            for (uint32_t i = 1; i <= maxb; ++i)
                if (pending[i]) word |= (1u << i);
            return word;
        }
        // ENABLE bitmap per context
        if (plic_is_enable(off)) {
            uint32_t ctx = plic_enable_context(off);
            uint64_t inoff = plic_enable_offset_in_ctx(off);
            uint32_t word_index = inoff >> 2;
            if (ctx < PLIC_NUM_CONTEXTS && word_index < enable[ctx].size())
                return enable[ctx][word_index];
            return 0;
        }
        // Context registers: THRESHOLD, CLAIM/COMPLETE
        if (plic_is_ctx(off)) {
            uint32_t ctx = plic_ctx_index(off);
            uint64_t roff = plic_ctx_reg_offset(off);
            if (ctx < PLIC_NUM_CONTEXTS) {
                if (roff == PLIC_CTX_THRESHOLD) {
                    return threshold[ctx];
                } else if (roff == PLIC_CTX_CLAIMCOMP) {
                    // CLAIM: select highest-priority enabled pending source > threshold
                    uint32_t src = pick_claimable(ctx);
                    if (src) {
                        // Per spec: reading claim clears pending and sets active for that context.
                        pending[src] = 0;
                        active[src]  = 1;
                        last_claimed[ctx] = src;
                        recompute_lines();
                    }
                    return src;
                }
            }
            return 0;
        }

        return 0;
    }

    // Write MMIO
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) override {
        if (addr < base || addr >= base + size) return;
        uint64_t off = addr - base;

        // PRIORITY array
        if (plic_is_priority(off)) {
            uint32_t idx = plic_priority_index(off);
            if (idx <= num_sources) {
                priority[idx] = value & 0x7; // typical implementations use small priority width
                recompute_lines();
            }
            return;
        }
        // PENDING is read-only
        if (plic_is_pending(off)) {
            return;
        }
        // ENABLE bitmaps
        if (plic_is_enable(off)) {
            uint32_t ctx = plic_enable_context(off);
            uint64_t inoff = plic_enable_offset_in_ctx(off);
            uint32_t word_index = inoff >> 2;
            if (ctx < PLIC_NUM_CONTEXTS && word_index < enable[ctx].size()) {
                enable[ctx][word_index] = value;
                recompute_lines();
            }
            return;
        }
        // Context THRESHOLD / CLAIM/COMPLETE
        if (plic_is_ctx(off)) {
            uint32_t ctx = plic_ctx_index(off);
            uint64_t roff = plic_ctx_reg_offset(off);
            if (ctx < PLIC_NUM_CONTEXTS) {
                if (roff == PLIC_CTX_THRESHOLD) {
                    threshold[ctx] = value & 0x7; // typical small width
                    recompute_lines();
                } else if (roff == PLIC_CTX_CLAIMCOMP) {
                    // COMPLETE: value must be the source id previously claimed
                    uint32_t src = value;
                    if (src > 0 && src <= num_sources && active[src]) {
                        active[src] = 0;
                        // If source is still asserted level-high externally, it should
                        // be re-pended by device via raise(src) later.
                        recompute_lines();
                    }
                }
            }
            return;
        }
    }

    // Replace your cpu_check_pending_interrupts_and_handle():
    // Call this periodically (e.g., before each instruction) to TAKE external IRQs.
    void plic_service(HART* hart) {
        // Global enables
        uint64_t mstatus = h_cpu_csr_read(hart,MSTATUS);
        uint64_t sstatus = h_cpu_csr_read(hart,SSTATUS);
        bool m_glb = ((mstatus >> MSTATUS_MIE_BIT) & 1ULL) != 0;
        bool s_glb = ((sstatus >> SSTATUS_SIE_BIT) & 1ULL) != 0;

        // Local enables in mie/sie
        uint64_t mie = h_cpu_csr_read(hart,MIE);
        uint64_t sie = h_cpu_csr_read(hart,SIE);
        bool me_ie = ((mie >> MIE_MEIE_BIT) & 1ULL) != 0;
        bool se_ie = ((sie >> SIE_SEIE_BIT) & 1ULL) != 0;

        // Determine claimable for M and S
        uint32_t m_src = pick_claimable(/*ctx=*/0);
        uint32_t s_src = pick_claimable(/*ctx=*/1);

        // Priority: Machine ext > Supervisor ext (higher privilege first)
        if (m_src && m_glb && me_ie) {
            // Set MEIP and take trap
            uint64_t mip = h_cpu_csr_read(hart, MIP);
            mip |= (1ULL << MIP_MEIP); 
            h_cpu_csr_write(hart, MIP, mip);
            cpu_trap(hart, IRQ_MEXT, 0, /*is_interrupt=*/true);
            return;
        }
        if (s_src && s_glb && se_ie) {
            // Set SEIP and take trap
            uint64_t sip = h_cpu_csr_read(hart, SIP);
            sip |= (1ULL << MIP_SEIP);
            h_cpu_csr_write(hart, SIP, sip);

            cpu_trap(hart, IRQ_SEXT, 0, /*is_interrupt=*/true);
            return;
        }

        // If not taking now, just reflect line level into MIP/SIP for software visibility
        uint64_t mip = h_cpu_csr_read(hart, MIP);
        mip = set_bit(mip, MIP_MEIP, m_src != 0);
        h_cpu_csr_write(hart, MIP, mip);

        uint64_t sip = h_cpu_csr_read(hart, SIP);
        sip = set_bit(sip, MIP_SEIP, s_src != 0);
        h_cpu_csr_write(hart, SIP, sip);
    }

private:
    // pick highest-priority enabled pending source above threshold for given ctx
    uint32_t pick_claimable(uint32_t ctx) const {
        if (ctx >= PLIC_NUM_CONTEXTS) return 0;

        uint32_t best_src = 0;
        uint32_t best_pri = 0;
        uint32_t th = threshold[ctx];

        for (uint32_t src = 1; src <= num_sources; ++src) {
            if (!pending[src]) continue;
            if (active[src]) continue; // already claimed by some context
            if (!is_enabled(ctx, src)) continue;
            uint32_t pri = priority[src];
            if (pri == 0) continue;         // priority 0 is masked
            if (pri <= th) continue;        // must be > threshold
            // priority tie-breaker: lowest ID wins per spec; we prefer higher pri first then smaller id
            if (pri > best_pri || (pri == best_pri && src < best_src)) {
                best_pri = pri;
                best_src = src;
            }
        }
        return best_src;
    }

    bool is_enabled(uint32_t ctx, uint32_t src) const {
        uint32_t word = src >> 5;       // src/32
        uint32_t bit  = src & 31;       // src%32
        if (word >= enable[ctx].size()) return false;
        return ( (enable[ctx][word] >> bit) & 1U ) != 0;
    }

    inline uint64_t set_bit(uint64_t value, int pos, bool state = true) {
        return state ? (value | (1ULL << pos)) : (value & ~(1ULL << pos));
    }

    // Reflect current claimable status into MEIP/SEIP bits
    void recompute_lines() {
        // NOTE: We do not have a global pointer to HART here; lines are updated in plic_service().
        // If you want "live" MIP/SIP reflection, inject HART* into this method or call plic_service().
    }
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

    UART(uint64_t base, DRAM& ram, PLIC* plic, int irq_num, fdt_node* fdt, uint8_t hartcount)
        : Device(base, 0x100, ram), plic(plic), irq_num(irq_num)
    {
        struct fdt_node* uart_fdt = fdt_node_create_reg("uart", base);
        fdt_node_add_prop_reg(uart_fdt, "reg", base, 0x100);
        fdt_node_add_prop_str(uart_fdt, "compatible", "ns16550a");
        fdt_node_add_prop_u32(uart_fdt, "clock-frequency", 20000000);
        fdt_node_add_prop_u32(uart_fdt, "fifo-size", 16);
        fdt_node_add_prop_str(uart_fdt, "status", "okay");
        fdt_node_add_prop_u32(uart_fdt, "interrupt-parent", hartcount*2+1);
        fdt_node_add_prop_u32(uart_fdt, "interrupts", 1);
        fdt_node_add_child(fdt_node_find(fdt,"soc"), uart_fdt);

        struct fdt_node* chosen = fdt_node_find(fdt, "chosen");
        fdt_node_add_prop_str(chosen, "stdout-path", "/soc/uart@10000000");
    }

    void trigger_irq() {
        if (ier & 0x01) { // TX interrupt enabled
            plic->raise(irq_num);
        }
    }

    void clear_irq() {
        plic->clear(irq_num);
    }

    void update_iir() {
        // Simple implementation: prioritize TX empty interrupt if enabled
        if ((ier & 0x02) && (lsr & 0x01)) { // RX interrupt enabled and data ready
            iir = 0x04; // RX data available interrupt
        } else if ((ier & 0x01) && (lsr & 0x20)) { // TX interrupt enabled and THR empty
            iir = 0x02; // TX holding register empty interrupt
        } else {
            iir = 0x01; // No interrupt pending
        }
    }

    uint64_t read(HART* hart, uint64_t addr, uint64_t size) override {
        uint64_t offset = addr - base;
        uint8_t reg = offset;
        uint64_t value = 0;

        switch (reg) {
            case 0: // RHR (read) or DLL (if DLAB=1)
                if (dlab) {
                    value = dll;
                } else {
                    value = rhr;
                    lsr &= ~0x01; // Clear data ready flag after read
                    update_iir();
                }
                break;
                
            case 1: // IER (read) or DLM (if DLAB=1)
                if (dlab) {
                    value = dlm;
                } else {
                    value = ier;
                }
                break;
                
            case 2: // IIR (read)
                value = iir;
                // Reading IIR clears highest priority interrupt
                if (iir == 0x02) { // TX interrupt
                    lsr &= ~0x20; // Clear THR empty flag temporarily
                } else if (iir == 0x04) { // RX interrupt
                    lsr &= ~0x01; // Clear data ready flag
                }
                update_iir();
                break;
                
            case 3: // LCR (read)
                value = lcr;
                break;
                
            case 4: // MCR (read)
                value = mcr;
                break;
                
            case 5: // LSR (read) - CRITICAL FOR OPENSBI!
                value = lsr;
                break;
                
            case 6: // MSR (read)
                value = msr;
                break;
                
            case 7: // SCR (read)
                value = scr;
                break;
                
            default:
                // For unhandled registers, return 0
                break;
        }

        return value;
    }

    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) override {
        uint64_t offset = addr - base;
        uint8_t reg = offset;
        uint8_t byte_value = value & 0xFF;

        switch (reg) {
            case 0: // THR (write) or DLL (if DLAB=1)
                if (dlab) {
                    dll = byte_value;
                } else {
                    thr = byte_value;
                    std::putchar(thr);
                    std::fflush(stdout);
                    lsr |= 0x20; // THR empty
                    lsr &= ~0x40; // Transmission not in progress
                    update_iir();
                    trigger_irq();
                }
                break;
                
            case 1: // IER (write) or DLM (if DLAB=1)
                if (dlab) {
                    dlm = byte_value;
                } else {
                    ier = byte_value & 0x0F; // Only lower 4 bits are valid
                    update_iir();
                }
                break;
                
            case 2: // FCR (write) - FIFO Control Register
                fcr = byte_value;
                // Simple implementation: ignore FIFO for now
                break;
                
            case 3: // LCR (write)
                lcr = byte_value;
                dlab = (lcr & 0x80) != 0; // Update DLAB from bit 7
                break;
                
            case 4: // MCR (write)
                mcr = byte_value;
                break;
                
            case 5: // LSR (read-only)
                // Ignore writes to read-only register
                break;
                
            case 6: // MSR (read-only)
                // Ignore writes to read-only register
                break;
                
            case 7: // SCR (write)
                scr = byte_value;
                break;
                
            default:
                // Ignore writes to unknown registers
                break;
        }
    }

    void receive_byte(uint8_t byte) {
        rhr = byte;
        lsr |= 0x01; // Data Ready
        update_iir();
        if (ier & 0x02) trigger_irq(); // RX interrupt enabled
    }

    // Method to simulate incoming data (for testing)
    void simulate_rx_data(const char* data) {
        for (size_t i = 0; data[i] != '\0'; i++) {
            receive_byte(data[i]);
        }
    }

    // Reset UART to default state
    void reset() {
        rhr = 0;
        thr = 0;
        ier = 0;
        iir = 0x01;
        fcr = 0;
        lcr = 0;
        mcr = 0;
        lsr = 0x60; // THR empty + line idle
        msr = 0;
        scr = 0;
        dll = 0;
        dlm = 0;
        dlab = false;
    }
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

    CLINT(uint64_t base, DRAM& ram, uint32_t num_harts, fdt_node* fdt, uint8_t hartcount)
        : Device(base, 0x10000, ram), // size arbitrary, enough to cover addr space
          msip(num_harts, 0),
          mtimecmp(num_harts, 0)
    {    
        if(fdt != NULL) {
            struct fdt_node* cpus = fdt_node_find(fdt, "cpus");
            std::vector<uint32_t> irq_ext = {}; 
            for(int i=0; i < hartcount; i++) {
                struct fdt_node* cpu = fdt_node_find_reg(cpus, "cpu", i);
                struct fdt_node* cpu_irq = fdt_node_find(cpu, "interrupt-controller");
                uint32_t irq_phandle = fdt_node_get_phandle(cpu_irq);

                if (irq_phandle) {
                    irq_ext.push_back(irq_phandle);
                    irq_ext.push_back(0x3); // RISCV_INTERRUPT_MSOFTWARE
                    irq_ext.push_back(irq_phandle);
                    irq_ext.push_back(0x7); // RISCV_INTERRUPT_MTIMER
                } else {
                    // missing interrupt controller in hart i
                }
            }
            
            struct fdt_node* clint_fdt = fdt_node_create_reg("clint", base);
            fdt_node_add_prop_reg(clint_fdt, "reg", base, size);
            fdt_node_add_prop(clint_fdt, "compatible", "sifive,clint0\0riscv,clint0\0",27);
            fdt_node_add_prop_cells(clint_fdt, "interrupts-extended", irq_ext, irq_ext.size());

            fdt_node_add_child(fdt_node_find(fdt,"soc"), clint_fdt);
        }
    }

    void start_timer(uint64_t freq_hz, HART* hart) {
        stop_timer = false;
        timer_thread = std::thread([this, freq_hz, hart]() {
            using namespace std::chrono;
            auto period = nanoseconds(1'000'000'000ULL / freq_hz);
            while (!stop_timer) {
                std::this_thread::sleep_for(period);
                mtime.fetch_add(1, std::memory_order_relaxed);
                update_mip(hart);
            }
        });
    }

    void stop_timer_thread() {
        stop_timer = true;
        if (timer_thread.joinable())
            timer_thread.join();
    }

    uint64_t read(HART* hart, uint64_t addr, uint64_t size) override {
        uint64_t off = addr - base;

        // MSIP
        if (off < 0x4000) {
            uint32_t hart_id = off / 4;
            if (hart_id < msip.size())
                return msip[hart_id];
        }
        // MTIMECMP
        else if (off >= 0x4000 && off < 0xBFF8) {
            uint32_t hart_id = (off - 0x4000) / 8;
            if (hart_id < mtimecmp.size())
                return mtimecmp[hart_id];
        }
        // MTIME
        else if (off == 0xBFF8)
            return (uint32_t)(mtime & 0xFFFFFFFFULL);
        else if (off == 0xBFFC)
            return (uint32_t)(mtime >> 32);

        return 0;
    }

    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) override {
        uint64_t off = addr - base;

        // MSIP
        if (off < 0x4000) {
            uint32_t hart_id = off / 4;
            if (hart_id < msip.size()) {
                msip[hart_id] = value & 1;
            }
        }
        // MTIMECMP
        else if (off >= 0x4000 && off < 0xBFF8) {
            uint32_t hart_id = (off - 0x4000) / 8;
            if (hart_id < mtimecmp.size()) {
                mtimecmp[hart_id] = value;
            }
        }
    }

private:
    void update_mip(HART* hart) {
        uint64_t mip = h_cpu_csr_read(hart, MIP);
        uint32_t hart_id = h_cpu_id(hart);

        // MSIP -> software interrupt
        if (msip[hart_id] & 0x1)
            mip |= (1ULL << MIP_MSIP);
        else
            mip &= ~(1ULL << MIP_MSIP);

        // MTIMECMP -> timer interrupt
        if (mtime >= mtimecmp[hart_id])
            mip |= (1ULL << MIP_MTIP);
        else
            mip &= ~(1ULL << MIP_MTIP);

        h_cpu_csr_write(hart, MIP, mip);
    }
};