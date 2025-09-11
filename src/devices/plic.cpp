#include "../include/devices/plic.hpp"

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
    uint64_t mstatus = hart->h_cpu_csr_read(MSTATUS);
    uint64_t sstatus = hart->h_cpu_csr_read(SSTATUS);
    bool m_glb = ((mstatus >> MSTATUS_MIE_BIT) & 1ULL) != 0;
    bool s_glb = ((sstatus >> SSTATUS_SIE_BIT) & 1ULL) != 0;

    // Local enables in mie/sie
    uint64_t mie = hart->h_cpu_csr_read(MIE);
    uint64_t sie = hart->h_cpu_csr_read(SIE);
    bool me_ie = ((mie >> MIE_MEIE_BIT) & 1ULL) != 0;
    bool se_ie = ((sie >> SIE_SEIE_BIT) & 1ULL) != 0;

    // Determine claimable for M and S
    uint32_t m_src = pick_claimable(/*ctx=*/0);
    uint32_t s_src = pick_claimable(/*ctx=*/1);

    // Priority: Machine ext > Supervisor ext (higher privilege first)
    if (m_src && m_glb && me_ie) {
        // Set MEIP and take trap
        uint64_t mip = hart->h_cpu_csr_read(MIP);
        mip |= (1ULL << MIP_MEIP); 
        hart->h_cpu_csr_write(MIP, mip);
        hart->cpu_trap(IRQ_MEXT, 0, /*is_interrupt=*/true);
        return;
    }
    if (s_src && s_glb && se_ie) {
        // Set SEIP and take trap
        uint64_t sip = hart->h_cpu_csr_read(SIP);
        sip |= (1ULL << MIP_SEIP);
        hart->h_cpu_csr_write(SIP, sip);

        hart->cpu_trap(IRQ_SEXT, 0, /*is_interrupt=*/true);
        return;
    }

    // If not taking now, just reflect line level into MIP/SIP for software visibility
    uint64_t mip = hart->h_cpu_csr_read(MIP);
    mip = set_bit(mip, MIP_MEIP, m_src != 0);
    hart->h_cpu_csr_write(MIP, mip);

    uint64_t sip = hart->h_cpu_csr_read(SIP);
    sip = set_bit(sip, MIP_SEIP, s_src != 0);
    hart->h_cpu_csr_write(SIP, sip);
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