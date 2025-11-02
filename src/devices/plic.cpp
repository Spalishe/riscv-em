/*
Copyright 2025 Spalishe

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

#include "../../include/devices/plic.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>

#ifndef PLIC_CTX_THRESHOLD
#define PLIC_CTX_THRESHOLD 0x0
#endif
#ifndef PLIC_CTX_CLAIMCOMP
#define PLIC_CTX_CLAIMCOMP 0x4
#endif

PLIC::PLIC(uint64_t base, uint64_t size, DRAM& ram, uint32_t num_sources, fdt_node* fdt, uint8_t hartcount)
    : Device(base, size, ram),
      base_addr(base),
      size_bytes(size),
      num_sources(num_sources),
      num_harts(hartcount),
      num_contexts(hartcount * 2),
      priority(num_sources + 1, 0),
      pending(num_sources + 1, 0),
      active(num_sources + 1, 0),
      threshold(num_contexts, 0),
      last_claimed(num_contexts, 0)
{
    uint32_t words = (num_sources + 31) / 32;
    enable.assign(num_contexts, std::vector<uint32_t>(words, 0));

    if (fdt != NULL) {
        struct fdt_node* cpus = fdt_node_find(fdt, "cpus");
        std::vector<uint32_t> irq_ext;
        for (int i = 0; i < (int)hartcount; ++i) {
            struct fdt_node* cpu = fdt_node_find_reg(cpus, "cpu", i);
            struct fdt_node* cpu_irq = fdt_node_find(cpu, "interrupt-controller");
            uint32_t irq_phandle = fdt_node_get_phandle(cpu_irq);
            if (irq_phandle) {
                irq_ext.push_back(irq_phandle);
                irq_ext.push_back(0x0);
            }
        }

        struct fdt_node* plic_fdt = fdt_node_create_reg("plic", base);
        fdt_node_add_prop_reg(plic_fdt, "reg", base, size);
        fdt_node_add_prop_str(plic_fdt, "compatible", "sifive,plic-1.0.0");
        fdt_node_add_prop(plic_fdt, "interrupt-controller", NULL, 0);
        fdt_node_add_prop_u32(plic_fdt, "#interrupt-cells", 1);
        fdt_node_add_prop_u32(plic_fdt, "#address-cells", 0);
        fdt_node_add_prop_u32(plic_fdt, "riscv,ndev", num_sources);
        if (!irq_ext.empty()) {
            fdt_node_add_prop_cells(plic_fdt, "interrupts-extended", irq_ext, irq_ext.size());
        }
        fdt_node_add_child(fdt_node_find(fdt,"soc"), plic_fdt);
    }
}

void PLIC::raise(uint32_t src) {
    if (src == 0 || src > num_sources) return;
    pending[src] = 1;
}

void PLIC::clear(uint32_t src) {
    if (src == 0 || src > num_sources) return;
    pending[src] = 0;
}
inline bool PLIC::plic_is_priority(uint64_t off) const {
    return off < (uint64_t)(num_sources + 1) * 4;
}
inline bool PLIC::plic_is_pending(uint64_t off) const {
    uint64_t pr_sz = (uint64_t)(num_sources + 1) * 4;
    uint64_t pend_base = pr_sz;
    uint64_t pend_words = (num_sources + 31) / 32;
    return off >= pend_base && off < pend_base + pend_words * 4;
}
inline bool PLIC::plic_is_enable(uint64_t off) const {
    uint64_t pr_sz = (uint64_t)(num_sources + 1) * 4;
    uint64_t pend_sz = (uint64_t)((num_sources + 31) / 32) * 4;
    uint64_t en_base = pr_sz + pend_sz;
    uint64_t en_sz = (uint64_t)num_contexts * ((num_sources + 31) / 32) * 4;
    return off >= en_base && off < en_base + en_sz;
}
inline bool PLIC::plic_is_ctx(uint64_t off) const {
    uint64_t pr_sz = (uint64_t)(num_sources + 1) * 4;
    uint64_t pend_sz = (uint64_t)((num_sources + 31) / 32) * 4;
    uint64_t en_sz = (uint64_t)num_contexts * ((num_sources + 31) / 32) * 4;
    uint64_t ctx_base = pr_sz + pend_sz + en_sz;
    return off >= ctx_base && off < ctx_base + (uint64_t)num_contexts * 0x1000;
}

inline uint32_t PLIC::plic_priority_index(uint64_t off) const {
    return (uint32_t)(off / 4);
}
inline uint32_t PLIC::plic_pending_word_index(uint64_t off) const {
    uint64_t pr_sz = (uint64_t)(num_sources + 1) * 4;
    return (uint32_t)((off - pr_sz) / 4);
}
inline uint32_t PLIC::plic_enable_context(uint64_t off) const {
    uint64_t pr_sz = (uint64_t)(num_sources + 1) * 4;
    uint64_t pend_sz = (uint64_t)((num_sources + 31) / 32) * 4;
    uint64_t en_base = pr_sz + pend_sz;
    uint64_t local = off - en_base;
    uint32_t words = (num_sources + 31) / 32;
    return (uint32_t)(local / (words * 4));
}
inline uint32_t PLIC::plic_enable_word_index(uint64_t off) const {
    uint64_t pr_sz = (uint64_t)(num_sources + 1) * 4;
    uint64_t pend_sz = (uint64_t)((num_sources + 31) / 32) * 4;
    uint64_t en_base = pr_sz + pend_sz;
    uint64_t local = off - en_base;
    uint32_t words = (num_sources + 31) / 32;
    return (uint32_t)((local % (words * 4)) / 4);
}
inline uint32_t PLIC::plic_ctx_index(uint64_t off) const {
    uint64_t pr_sz = (uint64_t)(num_sources + 1) * 4;
    uint64_t pend_sz = (uint64_t)((num_sources + 31) / 32) * 4;
    uint64_t en_sz = (uint64_t)num_contexts * ((num_sources + 31) / 32) * 4;
    uint64_t ctx_base = pr_sz + pend_sz + en_sz;
    return (uint32_t)((off - ctx_base) / 0x1000);
}
inline uint64_t PLIC::plic_ctx_reg_offset(uint64_t off) const {
    uint64_t pr_sz = (uint64_t)(num_sources + 1) * 4;
    uint64_t pend_sz = (uint64_t)((num_sources + 31) / 32) * 4;
    uint64_t en_sz = (uint64_t)num_contexts * ((num_sources + 31) / 32) * 4;
    uint64_t ctx_base = pr_sz + pend_sz + en_sz;
    return (off - ctx_base) % 0x1000;
}

// read
uint64_t PLIC::read(HART* hart, uint64_t addr, uint64_t size) {
    if (addr < base_addr || addr >= base_addr + size_bytes) return 0;
    uint64_t off = addr - base_addr;

    // PRIORITY array
    if (plic_is_priority(off)) {
        uint32_t idx = plic_priority_index(off);
        if (idx <= num_sources) return priority[idx];
        return 0;
    }

    // PENDING bits
    if (plic_is_pending(off)) {
        uint32_t word_idx = plic_pending_word_index(off);
        uint32_t word = 0;
        uint32_t maxb = num_sources;
        uint32_t base_src = word_idx * 32 + 1;
        for (uint32_t i = 0; i < 32; ++i) {
            uint32_t src = base_src + i;
            if (src > maxb) break;
            if (pending[src]) word |= (1u << i);
        }
        return word;
    }

    // ENABLE bitmap
    if (plic_is_enable(off)) {
        uint32_t ctx = plic_enable_context(off);
        uint32_t word_index = plic_enable_word_index(off);
        if (ctx < num_contexts && word_index < enable[ctx].size()) {
            return enable[ctx][word_index];
        }
        return 0;
    }

    // CONTEXT registers (THRESHOLD / CLAIM)
    if (plic_is_ctx(off)) {
        uint32_t ctx = plic_ctx_index(off);
        uint64_t roff = plic_ctx_reg_offset(off);
        if (ctx < num_contexts) {
            if (roff == PLIC_CTX_THRESHOLD) {
                return threshold[ctx];
            } else if (roff == PLIC_CTX_CLAIMCOMP) {
                uint32_t src = pick_claimable(ctx);
                if (src) {
                    pending[src] = 0;
                    active[src] = 1;
                    last_claimed[ctx] = src;
                }
                return src;
            }
        }
        return 0;
    }

    return 0;
}

// write
void PLIC::write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    if (addr < base_addr || addr >= base_addr + size_bytes) return;
    uint64_t off = addr - base_addr;

    // PRIORITY array
    if (plic_is_priority(off)) {
        uint32_t idx = plic_priority_index(off);
        if (idx <= num_sources) {
            priority[idx] = (uint32_t)(value & 0x7);
        }
        return;
    }

    // PENDING read-only
    if (plic_is_pending(off)) {
        return;
    }

    // ENABLE bitmap
    if (plic_is_enable(off)) {
        uint32_t ctx = plic_enable_context(off);
        uint32_t word_index = plic_enable_word_index(off);
        if (ctx < num_contexts && word_index < enable[ctx].size()) {
            enable[ctx][word_index] = (uint32_t)value;
        }
        return;
    }

    // CONTEXT THRESHOLD / CLAIMCOMPLETE
    if (plic_is_ctx(off)) {
        uint32_t ctx = plic_ctx_index(off);
        uint64_t roff = plic_ctx_reg_offset(off);
        if (ctx < num_contexts) {
            if (roff == PLIC_CTX_THRESHOLD) {
                threshold[ctx] = (uint32_t)(value & 0x7);
            } else if (roff == PLIC_CTX_CLAIMCOMP) {
                uint32_t src = (uint32_t)value;
                if (src > 0 && src <= num_sources && active[src]) {
                    active[src] = 0;
                    last_claimed[ctx] = 0;
                }
            }
        }
        return;
    }
}

uint32_t PLIC::pick_claimable(uint32_t ctx) {
    if (ctx >= num_contexts) return 0;
    uint32_t best_src = 0;
    uint32_t best_pri = 0;
    uint32_t th = threshold[ctx];

    for (uint32_t src = 1; src <= num_sources; ++src) {
        if (!pending[src]) continue;
        if (active[src]) continue;
        if (!is_enabled(ctx, src)) continue;
        uint32_t pri = priority[src];
        if (pri == 0) continue;
        if (pri <= th) continue;
        if (pri > best_pri || (pri == best_pri && (best_src == 0 || src < best_src))) {
            best_pri = pri;
            best_src = src;
        }
    }
    return best_src;
}

bool PLIC::is_enabled(uint32_t ctx, uint32_t src) {
    if (ctx >= num_contexts) return false;
    uint32_t idx = (src - 1) / 32;
    uint32_t bit = (src - 1) % 32;
    if (idx >= enable[ctx].size()) return false;
    return ((enable[ctx][idx] >> bit) & 1U) != 0;
}
void PLIC::plic_service(HART* hart) {
    uint32_t ctx = hart->id;

    uint32_t th = threshold[ctx];

    uint32_t best_src = 0;
    uint32_t best_pri = 0;
    for (uint32_t src = 1; src <= num_sources; ++src) {
        if (!pending[src]) continue;
        if (active[src]) continue;
        if (!is_enabled(ctx, src)) continue;
        uint32_t pri = priority[src];
        if (pri == 0 || pri <= th) continue;

        if (pri > best_pri || (pri == best_pri && src < best_src)) {
            best_pri = pri;
            best_src = src;
        }
    }

    if (best_src == 0) {
        return;
    }

    pending[best_src] = 0;
    active[best_src]  = 1;
    last_claimed[ctx] = best_src;

    if (hart->mode == 3) {
        uint64_t mip = hart->h_cpu_csr_read(MIP);
        mip |= (1ULL << MIP_MEIP); // set machine external interrupt pending
        hart->h_cpu_csr_write(MIP, mip);
        hart->cpu_trap(IRQ_MEXT, 0, true);
    } else if (hart->mode == 1) {
        uint64_t sip = hart->h_cpu_csr_read(SIP);
        sip |= (1ULL << MIP_SEIP); // set supervisor external interrupt pending
        hart->h_cpu_csr_write(SIP, sip);
        hart->cpu_trap(IRQ_SEXT, 0, true);
    }
}


// utility set bit
inline uint64_t PLIC::set_bit(uint64_t value, int pos, bool state) {
    return state ? (value | (1ULL << pos)) : (value & ~(1ULL << pos));
}
