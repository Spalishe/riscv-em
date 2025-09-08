#include "../include/mmio.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include "../include/dram.h"
#include <optional>
#include <iostream>

std::optional<uint64_t> MMIO::load(HART* hart, uint64_t addr, uint64_t size) {
    if((addr % (size/8)) != 0) {
        cpu_trap(hart,EXC_LOAD_ADDR_MISALIGNED,addr,false);
        return std::nullopt;
    }

    // DRAM
    if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        return dram_load(&ram,addr,size);
    }

    // Devices
    for (auto* dev : devices) {
        if (addr >= dev->base && addr < dev->base + dev->size) {
            return dev->read(hart, addr, size);
        }
    }

    // Fault
    cpu_trap(hart,EXC_LOAD_ACCESS_FAULT,addr,false);
    return std::nullopt;
}

void MMIO::store(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    if(addr % (size/8) != 0) {
        cpu_trap(hart,EXC_STORE_ADDR_MISALIGNED,addr,false);
        return;
    }

    // DRAM
    if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        dram_store(&ram,addr,size,value);
        return;
    }

    // Devices
    for (auto* dev : devices) {
        if (addr >= dev->base && addr < dev->base + dev->size) {
            dev->write(hart, addr, size, value);
            return;
        }
    }
    
    // Fault
    cpu_trap(hart,EXC_STORE_ACCESS_FAULT,addr,false);
}

uint64_t ROM::read(HART* hart,uint64_t addr,uint64_t size) {
    return dram_load(&ram,addr,size);
}

void ROM::write(HART* hart, uint64_t addr, uint64_t size,uint64_t val) {
    dram_store(&ram,addr,size,val);
    //cpu_trap(hart,EXC_STORE_ACCESS_FAULT,addr,false);
}

PLIC::PLIC(uint64_t base, uint64_t size, DRAM& ram, uint32_t num_sources, fdt_node* fdt, uint8_t hartcount)
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

void PLIC::raise(uint32_t src) {
    if (src == 0 || src > num_sources) return;
    pending[src] = 1;
}
void PLIC::clear(uint32_t src) {
    if (src == 0 || src > num_sources) return;
    // If level-sensitive cleared while active, spec allows it to remain active until complete.
    pending[src] = 0;
}

uint64_t PLIC::read(HART* hart, uint64_t addr,uint64_t size) {
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
                    pending[src] = 0;
                    active[src]  = 1;
                    last_claimed[ctx] = src;
                }
                return src;
            }
        }
        return 0;
    }

    return 0;
}

void PLIC::write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    if (addr < base || addr >= base + size) return;
    uint64_t off = addr - base;

    // PRIORITY array
    if (plic_is_priority(off)) {
        uint32_t idx = plic_priority_index(off);
        if (idx <= num_sources) {
            priority[idx] = value & 0x7;
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
            } else if (roff == PLIC_CTX_CLAIMCOMP) {
                uint32_t src = value;
                if (src > 0 && src <= num_sources && active[src]) {
                    active[src] = 0;
                }
            }
        }
        return;
    }
}

void PLIC::plic_service(HART* hart) {
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

// pick highest-priority enabled pending source above threshold for given ctx
uint32_t PLIC::pick_claimable(uint32_t ctx) {
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

bool PLIC::is_enabled(uint32_t ctx, uint32_t src) {
    uint32_t word = src >> 5;       // src/32
    uint32_t bit  = src & 31;       // src%32
    if (word >= enable[ctx].size()) return false;
    return ( (enable[ctx][word] >> bit) & 1U ) != 0;
}

inline uint64_t PLIC::set_bit(uint64_t value, int pos, bool state) {
    return state ? (value | (1ULL << pos)) : (value & ~(1ULL << pos));
}

UART::UART(uint64_t base, DRAM& ram, PLIC* plic, int irq_num, fdt_node* fdt, uint8_t hartcount)
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

void UART::trigger_irq() {
    if (ier & 0x01) { // TX interrupt enabled
        plic->raise(irq_num);
    }
}

void UART::clear_irq() {
    plic->clear(irq_num);
}

void UART::update_iir() {
    if ((ier & 0x02) && (lsr & 0x01)) { // RX interrupt enabled and data ready
        iir = 0x04; // RX data available interrupt
    } else if ((ier & 0x01) && (lsr & 0x20)) { // TX interrupt enabled and THR empty
        iir = 0x02; // TX holding register empty interrupt
    } else {
        iir = 0x01; // No interrupt pending
    }
}

uint64_t UART::read(HART* hart, uint64_t addr, uint64_t size) {
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
            break;
    }

    return value;
}

void UART::write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
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

void UART::receive_byte(uint8_t byte) {
    rhr = byte;
    lsr |= 0x01; // Data Ready
    update_iir();
    if (ier & 0x02) trigger_irq(); // RX interrupt enabled
}

// idk why but why not
void UART::reset() {
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



CLINT::CLINT(uint64_t base, DRAM& ram, uint32_t num_harts, fdt_node* fdt, uint8_t hartcount)
    : Device(base, 0x10000, ram),
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

void CLINT::start_timer(uint64_t freq_hz, HART* hart) {
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

void CLINT::stop_timer_thread() {
    stop_timer = true;
    if (timer_thread.joinable())
        timer_thread.join();
}

uint64_t CLINT::read(HART* hart, uint64_t addr, uint64_t size) {
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

void CLINT::write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
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

void CLINT::update_mip(HART* hart) {
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