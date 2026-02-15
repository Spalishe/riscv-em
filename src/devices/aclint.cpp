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

/*
    FIXME:
        ACLINT has a major problem: it inconsistent
        Need to slow it down somehow.
        Reject idea with chrono, i tried it. It consistent, but linux excepts something else??? 
        
        !! UPDATE !!:
        It seems that i whatever i used host time or the consistent time, it still fails.
        Seems like smth inside ACLINT is broken, rather than MTIME.
        I just rn looked up through multiple repos on git, including spike, and they all use simple mtime++ 
*/

ACLINT::ACLINT(uint64_t base, Machine& cpu, fdt_node* fdt)
    : Device(base, 0x10000, cpu),
        msip(cpu.core_count, 0),
        mtimecmp(cpu.core_count)
{   
    //timer_init(&mtime,ACLINT_FREQ_HZ);
    //for(int i = 0; i < cpu.core_count; i++) {
    //    timecmp_init(&mtimecmp[i],&mtime);
    //}
    
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

void ACLINT::tick() {
    mtime += 1;
    for(HART* hrt : cpu.harts) {
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
        //return timer_get(&mtime);
        return mtime;
    }
    //return timecmp_get(&mtimecmp[hart_id]);
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
        //timer_set(&mtime,value);
        mtime = value;
        return;
    }

    //timecmp_set(&mtimecmp[hart_id],value);
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

    if (mtime >= hart->csrs[STIMECMP] && hart->csrs[STIMECMP] != 0)
        mip |= (1ULL << MIP_STIP);
    else
        mip &= ~(1ULL << MIP_STIP);

    hart->csrs[MIP] = mip;
}