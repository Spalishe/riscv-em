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
#include "../../include/devices/plic.hpp"
#include <vector>
#include <cstdint>

#ifndef PLIC_CTX_THRESHOLD
#define PLIC_CTX_THRESHOLD 0x0
#endif
#ifndef PLIC_CTX_CLAIMCOMP
#define PLIC_CTX_CLAIMCOMP 0x4
#endif

PLIC::PLIC(uint64_t base, uint64_t size, Machine& cpu, uint32_t num_sources, fdt_node* fdt)
    : Device(base, size, cpu),
      base_addr(base),
      size_bytes(size),
      num_sources(num_sources),
      num_harts(cpu.core_count),
      num_contexts(cpu.core_count * 2),
      priority(num_sources + 1, 1),
      pending(num_sources + 1, 0),
      active(num_sources + 1, 0),
      threshold(num_contexts, 0),
      last_claimed(num_contexts, 0)
{
    uint32_t words = (num_sources + 31) / 32;
    enable.assign(num_contexts, std::vector<uint32_t>(words, 0));

    if (fdt) {
        struct fdt_node* cpus = fdt_node_find(fdt, "cpus");
        std::vector<uint32_t> irq_ext;
        for (int i = 0; i < (int)cpu.core_count; ++i) {
            struct fdt_node* cpu = fdt_node_find_reg(cpus, "cpu", i);
            struct fdt_node* cpu_irq = fdt_node_find(cpu, "interrupt-controller");
            uint32_t irq_phandle = fdt_node_get_phandle(cpu_irq);
            if (irq_phandle) {
                irq_ext.push_back(irq_phandle);
                irq_ext.push_back(0x0b);
                irq_ext.push_back(irq_phandle);
                irq_ext.push_back(0x09);
            }
        }

        struct fdt_node* plic_fdt = fdt_node_create_reg("plic", base);
        fdt_node_add_prop_reg(plic_fdt, "reg", base, size);
        fdt_node_add_prop_str(plic_fdt, "compatible", "sifive,plic-1.0.0");
        fdt_node_add_prop(plic_fdt, "interrupt-controller", nullptr, 0);
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

bool PLIC::is_enabled(uint32_t ctx, uint32_t src) {
    if (ctx >= num_contexts || src == 0 || src > num_sources) return false;
    uint32_t idx = (src - 1) / 32;
    uint32_t bit = (src - 1) % 32;
    return ((enable[ctx][idx] >> bit) & 1U) != 0;
}

uint32_t PLIC::pick_claimable(uint32_t ctx) {
    if (ctx >= num_contexts) return 0;
    uint32_t best_src = 0;
    uint32_t best_pri = 0;
    uint32_t th = threshold[ctx];

    for (uint32_t src = 1; src <= num_sources; ++src) {
        if (!pending[src] || active[src] || !is_enabled(ctx, src)) continue;
        uint32_t pri = priority[src];
        if (pri == 0 || pri <= th) continue;
        if (pri > best_pri || (pri == best_pri && (best_src == 0 || src < best_src))) {
            best_pri = pri;
            best_src = src;
        }
    }
    return best_src;
}

uint64_t PLIC::read(HART* hart, uint64_t addr, uint64_t size) {
    if (addr < base_addr || addr >= base_addr + size_bytes) return 0;
    uint64_t off = addr - base_addr;

    uint64_t pr_sz = (num_sources + 1) * 4;
    uint64_t pend_sz = ((num_sources + 31) / 32) * 4;
    uint64_t en_sz = num_contexts * pend_sz;
    uint64_t ctx_base = pr_sz + pend_sz + en_sz;

    // PRIORITY
    if (off < pr_sz) return priority[off / 4];

    // PENDING
    if (off >= pr_sz && off < pr_sz + pend_sz) {
        uint32_t word_idx = (off - pr_sz) / 4;
        uint32_t word = 0;
        for (uint32_t i = 0; i < 32; ++i) {
            uint32_t src = word_idx * 32 + 1 + i;
            if (src > num_sources) break;
            if (pending[src]) word |= (1u << i);
        }
        return word;
    }

    // ENABLE
    if (off >= pr_sz + pend_sz && off < ctx_base) {
        uint64_t local = off - pr_sz - pend_sz;
        uint32_t ctx = local / pend_sz;
        uint32_t word_index = (local % pend_sz) / 4;
        if (ctx < num_contexts && word_index < enable[ctx].size())
            return enable[ctx][word_index];
        return 0;
    }

    // CONTEXT THRESHOLD / CLAIMCOMP
    if (off >= ctx_base && off < ctx_base + num_contexts * 0x1000) {
        uint32_t ctx = (off - ctx_base) / 0x1000;
        uint64_t roff = (off - ctx_base) % 0x1000;
        if (ctx >= num_contexts) return 0;
        if (roff == PLIC_CTX_THRESHOLD) return threshold[ctx];
        if (roff == PLIC_CTX_CLAIMCOMP) {
            uint32_t src = pick_claimable(ctx);
            if (src) {
                pending[src] = 0;
                active[src] = 1;
                last_claimed[ctx] = src;
            }
            return src;
        }
        return 0;
    }

    return 0;
}

void PLIC::write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    if (addr < base_addr || addr >= base_addr + size_bytes) return;
    uint64_t off = addr - base_addr;

    uint64_t pr_sz = (num_sources + 1) * 4;
    uint64_t pend_sz = ((num_sources + 31) / 32) * 4;
    uint64_t en_sz = num_contexts * pend_sz;
    uint64_t ctx_base = pr_sz + pend_sz + en_sz;

    // PRIORITY
    if (off < pr_sz) {
        uint32_t idx = off / 4;
        if (idx > 0 && idx <= num_sources) priority[idx] = value & 0x7;
        return;
    }

    // ENABLE
    if (off >= pr_sz + pend_sz && off < ctx_base) {
        uint64_t local = off - pr_sz - pend_sz;
        uint32_t ctx = local / pend_sz;
        uint32_t word_index = (local % pend_sz) / 4;
        if (ctx < num_contexts && word_index < enable[ctx].size())
            enable[ctx][word_index] = (uint32_t)value;
        return;
    }

    // CONTEXT
    if (off >= ctx_base && off < ctx_base + num_contexts * 0x1000) {
        uint32_t ctx = (off - ctx_base) / 0x1000;
        uint64_t roff = (off - ctx_base) % 0x1000;
        if (ctx >= num_contexts) return;
        if (roff == PLIC_CTX_THRESHOLD) threshold[ctx] = value & 0x7;
        else if (roff == PLIC_CTX_CLAIMCOMP) {
            uint32_t src = value;
            if (src > 0 && src <= num_sources && active[src]) {
                active[src] = 0;
                last_claimed[ctx] = 0;
            }
        }
        return;
    }
}

void PLIC::plic_service(HART* hart) {
    uint32_t ctx = hart->id;
    uint32_t best_src = pick_claimable(ctx);

    if (best_src == 0) return;

    pending[best_src] = 0;
    active[best_src]  = 1;
    last_claimed[ctx] = best_src;

    if (hart->mode == PrivilegeMode::Machine) {
        uint64_t mip = hart->csrs[MIP];
        mip |= (1ULL << MIP_MEIP);
        hart->csrs[MIP] = mip;
        hart_trap(*hart,IRQ_MEXT, 0, true);
    } else if (hart->mode == PrivilegeMode::Supervisor) {
        uint64_t sip = hart->csrs[SIP];
        sip |= (1ULL << MIP_SEIP);
        hart->csrs[SIP] = sip;
        hart_trap(*hart,IRQ_SEXT, 0, true);
    }
}