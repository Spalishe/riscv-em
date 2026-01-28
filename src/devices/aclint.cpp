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

#include "../../include/devices/aclint.hpp"
#include "../../include/libfdt.hpp"
#include "../../include/main.hpp"

ACLINT::ACLINT(uint64_t base, Machine& cpu, fdt_node* fdt)
    : Device(base, 0x10000, cpu),
        msip(cpu.core_count, 0),
        mtimecmp(cpu.core_count, 0)
{    
    if(fdt != NULL) {
        struct fdt_node* cpus = fdt_node_find(fdt, "cpus");
        std::vector<uint32_t> irq_ext = {}; 
        for(int i=0; i < cpu.core_count; i++) {
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

void ACLINT::tick(const std::chrono::nanoseconds time_passed) {
    uint64_t ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(time_passed).count()
        * ACLINT_FREQ_HZ / 1'000'000'000;

    mtime += ticks;

    for(HART* hrt : cpu.harts) {
        hrt->csrs[TIME] = mtime;
        update_mip(hrt);
    }
}

uint64_t ACLINT::read(HART* hart, uint64_t addr, uint64_t size) {
    uint64_t off = addr - base;

    if(off < ACLINT_MSWI_SIZE) {
        return read_mswi(hart,off);
    } else if(off >= ACLINT_MSWI_SIZE && off < ACLINT_MSWI_SIZE+ACLINT_MTIMER_SIZE) {
        return read_mtimer(hart,off-ACLINT_MSWI_SIZE);
    }

    return 0;
}

void ACLINT::write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    uint64_t off = addr - base;
    
    if(off < ACLINT_MSWI_SIZE) {
        write_mswi(hart,off,value);
    } else if(off >= ACLINT_MSWI_SIZE && off < ACLINT_MSWI_SIZE+ACLINT_MTIMER_SIZE) {
        write_mtimer(hart,off-ACLINT_MSWI_SIZE,value);
    }
}

uint64_t ACLINT::read_mswi(HART* hart, uint64_t offset) {
    uint64_t hart_id = offset >> 2;
    return msip[hart_id];
}
uint64_t ACLINT::read_mtimer(HART* hart, uint64_t offset) {
    uint64_t hart_id = offset >> 3;
    if(offset == 0x7FF8) {
        return (uint64_t)mtime.load();
    }
    return mtimecmp[hart_id];
}
void ACLINT::write_mswi(HART* hart, uint64_t offset, uint64_t value) {
    uint64_t hart_id = offset >> 2;
    msip[hart_id] = value;
    update_mip(cpu.harts[hart_id]);
}
void ACLINT::write_mtimer(HART* hart, uint64_t offset, uint64_t value) {
    uint64_t hart_id = offset >> 3;
    if (offset == 0x7FF8) {
        mtime.store(value);
        return;
    }

    mtimecmp[hart_id] = value;
    update_mip(cpu.harts[hart_id]);
}

void ACLINT::update_mip(HART* hart) {
    uint32_t hart_id = hart->id;

    uint64_t mip = hart->csrs[MIP];

    if (msip[hart_id] & 1)
        mip |= (1ULL << MIP_MSIP);
    else
        mip &= ~(1ULL << MIP_MSIP);

    if (mtime >= mtimecmp[hart_id])
        mip |= (1ULL << MIP_MTIP);
    else
        mip &= ~(1ULL << MIP_MTIP);

    if (mtime >= hart->csrs[STIMECMP])
        mip |= (1ULL << MIP_STIP);
    else
        mip &= ~(1ULL << MIP_STIP);

    hart->csrs[MIP] = mip;
}