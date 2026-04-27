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
#include "machine.hpp"


PLIC::PLIC(uint64_t base, uint64_t size, Machine& cpu,
                       uint32_t max_sources, uint32_t num_harts, fdt_node* fdt)
    : Device(base, size, cpu),
      max_sources(max_sources),
      num_harts(num_harts),
      num_contexts(num_harts * 2)
{
    pending_words_count = (max_sources + 31) / 32;
    enable_words_count = pending_words_count;

    priorities.resize(max_sources + 1, 0);
    pending_words.resize(pending_words_count, 0);

    contexts.resize(num_contexts);
    for (uint32_t i = 0; i < num_contexts; ++i) {
        contexts[i].enables.resize(enable_words_count, 0);
        contexts[i].threshold = 0;
        contexts[i].interrupt_line = false;

        contexts[i].hart_id = static_cast<int>(i / 2);
        contexts[i].supervisor_mode = (i % 2) == 1;
    }

    if (fdt) {
        struct fdt_node* cpus = fdt_node_find(fdt, "cpus");
        std::vector<uint32_t> irq_ext;

        for (uint32_t i = 0; i < cpu.core_count; ++i) {
            struct fdt_node* cpu_node = fdt_node_find_reg(cpus, "cpu", i);
            if (!cpu_node) continue;

            struct fdt_node* cpu_irq = fdt_node_find(cpu_node, "interrupt-controller");
            if (!cpu_irq) continue;

            uint32_t irq_phandle = fdt_node_get_phandle(cpu_irq);
            if (!irq_phandle) continue;

            // M-mode external interrupt
            irq_ext.push_back(irq_phandle);
            irq_ext.push_back(MIE_MEIE_BIT);

            // S-mode external interrupt
            irq_ext.push_back(irq_phandle);
            irq_ext.push_back(MIE_SEIE_BIT);
        }

        struct fdt_node* plic_fdt = fdt_node_create_reg("plic", base);
        fdt_node_add_prop_reg(plic_fdt, "reg", base, size);
        fdt_node_add_prop_str(plic_fdt, "compatible", "sifive,plic-1.0.0");
        fdt_node_add_prop(plic_fdt, "interrupt-controller", nullptr, 0);
        fdt_node_add_prop_u32(plic_fdt, "#interrupt-cells", 1);
        fdt_node_add_prop_u32(plic_fdt, "#address-cells", 0);
        fdt_node_add_prop_u32(plic_fdt, "riscv,ndev", max_sources);

        if (!irq_ext.empty()) {
            fdt_node_add_prop_cells(plic_fdt, "interrupts-extended", irq_ext, irq_ext.size());
        }

        fdt_node_add_child(fdt_node_find(fdt, "soc"), plic_fdt);
    }
}

void PLIC::set_pending(uint32_t source_id, bool pending) {
    if (source_id == 0 || source_id > max_sources)
        return;

    uint32_t word_idx = source_id / 32;
    uint32_t bit_mask = 1U << (source_id % 32);

    if (pending) {
        pending_words[word_idx] |= bit_mask;
    }
    else
        pending_words[word_idx] &= ~bit_mask;

    update_all_contexts();
}

void PLIC::plic_service() {
    update_all_contexts();
}

uint64_t PLIC::read(HART*, uint64_t addr, uint64_t size) {
    if (size != 32) return 0;

    uint64_t offset = addr - base;

    // Priority registers
    if (offset < 0x1000) {
        uint32_t idx = static_cast<uint32_t>(offset / 4);
        if (idx >= 1 && idx <= max_sources)
            return priorities[idx];
        return 0;
    }

    // Pending (read-only)
    if (offset >= 0x1000 && offset < 0x1080) {
        uint32_t word_idx = static_cast<uint32_t>((offset - 0x1000) / 4);
        if (word_idx < pending_words_count)
            return pending_words[word_idx];
        return 0;
    }

    // Enable registers
    if (offset >= 0x2000 && offset < 0x200000) {
        uint32_t ctx_idx = static_cast<uint32_t>((offset - 0x2000) / 0x80);
        if (ctx_idx >= num_contexts) return 0;

        uint32_t word_offset = static_cast<uint32_t>(((offset - 0x2000) % 0x80) / 4);
        if (word_offset < enable_words_count)
            return contexts[ctx_idx].enables[word_offset];
        return 0;
    }

    // Threshold and Claim
    if (offset >= 0x200000 && offset < 0x200000 + num_contexts * 0x1000) {
        uint32_t ctx_idx = static_cast<uint32_t>((offset - 0x200000) / 0x1000);
        if (ctx_idx >= num_contexts) return 0;

        uint32_t sub_offset = static_cast<uint32_t>((offset - 0x200000) % 0x1000);
        if (sub_offset == 0) {
            return contexts[ctx_idx].threshold;
        } else if (sub_offset == 4) {
            return claim_interrupt(ctx_idx);
        }
    }
    return 0;
}

void PLIC::write(HART*, uint64_t addr, uint64_t size, uint64_t value) {
    if (size != 32) return;

    uint64_t offset = addr - base;

    // Priority
    if (offset < 0x1000) {
        uint32_t idx = static_cast<uint32_t>(offset / 4);
        if (idx >= 1 && idx <= max_sources) {
            priorities[idx] = static_cast<uint32_t>(value & 0x7FFFFFFF);
            update_all_contexts();
        }
        return;
    }

    // Pending is read-only
    if (offset >= 0x1000 && offset < 0x1080) return;

    // Enable
    if (offset >= 0x2000 && offset < 0x200000) {
        uint32_t ctx_idx = static_cast<uint32_t>((offset - 0x2000) / 0x80);
        if (ctx_idx >= num_contexts) return;

        uint32_t word_offset = static_cast<uint32_t>(((offset - 0x2000) % 0x80) / 4);
        if (word_offset < enable_words_count) {
            contexts[ctx_idx].enables[word_offset] = static_cast<uint32_t>(value);
            update_context(ctx_idx);
        }
        return;
    }

    // Threshold and Complete
    if (offset >= 0x200000 && offset < 0x200000 + num_contexts * 0x1000) {
        uint32_t ctx_idx = static_cast<uint32_t>((offset - 0x200000) / 0x1000);
        if (ctx_idx >= num_contexts) return;

        uint32_t sub_offset = static_cast<uint32_t>((offset - 0x200000) % 0x1000);
        if (sub_offset == 0) {
            contexts[ctx_idx].threshold = static_cast<uint32_t>(value & 0x7FFFFFFF);
            update_context(ctx_idx);
        } else if (sub_offset == 4) {
            complete_interrupt(ctx_idx, static_cast<uint32_t>(value));
        }
        return;
    }
}

uint32_t PLIC::find_highest_pending_enabled(uint32_t ctx_idx) {
    const Context& ctx = contexts[ctx_idx];
    uint32_t best_id = 0;
    uint32_t best_priority = 0;

    for (uint32_t id = 1; id <= max_sources; ++id) {
        uint32_t word = id / 32;
        uint32_t bit = id % 32;

        if (!(pending_words[word] & (1U << bit))) continue;
        if (!(ctx.enables[word] & (1U << bit))) continue;

        uint32_t prio = priorities[id];
        if (prio > ctx.threshold) {
            if (prio > best_priority || (prio == best_priority && id < best_id)) {
                best_priority = prio;
                best_id = id;
            }
        }
    }
    return best_id;
}

uint32_t PLIC::claim_interrupt(uint32_t ctx_idx) {
    uint32_t id = find_highest_pending_enabled(ctx_idx);
    if (id != 0) {
        uint32_t word = id / 32;
        uint32_t bit = id % 32;
        pending_words[word] &= ~(1U << bit);
        update_context(ctx_idx);
    }
    return id;
}

void PLIC::complete_interrupt(uint32_t ctx_idx, uint32_t id) {
    if (id == 0 || id > max_sources) return;
    uint32_t word = id / 32;
    uint32_t bit = id % 32;
    pending_words[word] &= ~(1U << bit);
    update_context(ctx_idx);
}

void PLIC::update_context(uint32_t ctx_idx) {
    Context& ctx = contexts[ctx_idx];
    bool should_raise = (find_highest_pending_enabled(ctx_idx) != 0);
    if (should_raise != ctx.interrupt_line) {
        ctx.interrupt_line = should_raise;
        signal_cpu_interrupt(ctx.hart_id, ctx.supervisor_mode, should_raise);
    }
}

void PLIC::update_all_contexts() {
    for (uint32_t i = 0; i < num_contexts; ++i)
        update_context(i);
}

void PLIC::signal_cpu_interrupt(int hart_id, bool supervisor_mode, bool level) {
    if (hart_id < 0 || hart_id >= 64)
        return;

    HART* hart = cpu.harts[hart_id];
    if (!hart) return;

    const uint64_t MEIP_BIT = 1ULL << 11;
    const uint64_t SEIP_BIT = 1ULL << 9;

    if (supervisor_mode) {
        if (level)
            hart->ip |= SEIP_BIT;
        else
            hart->ip &= ~SEIP_BIT;
    } else {
        if (level)
            hart->ip |= MEIP_BIT;
        else
            hart->ip &= ~MEIP_BIT;
    }
}