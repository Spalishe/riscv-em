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

#include "../../include/devices/aclint.hpp"
#include "../../include/libfdt.hpp"
#include "../../include/main.hpp"

#define ACLINT_MSWI_SIZE   0x4000
#define ACLINT_MTIMER_SIZE 0x8000

ACLINT::ACLINT(uint64_t base, DRAM& ram, uint32_t num_harts, fdt_node* fdt)
    : Device(base, 0x10000, ram),
        msip(num_harts, 0),
        mtimecmp(num_harts, 0)
{    
    if(fdt != NULL) {
        struct fdt_node* cpus = fdt_node_find(fdt, "cpus");
        std::vector<uint32_t> irq_ext = {}; 
        for(int i=0; i < num_harts; i++) {
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

void ACLINT::start_timer(uint64_t freq_hz) {
    stop_timer = false;
    timer_thread = std::thread([this, freq_hz]() {
        using namespace std::chrono;
        auto period = nanoseconds(1'000'000'000ULL / freq_hz);
        auto next_time = steady_clock::now();
        while (!stop_timer) {
            next_time += period;
            std::this_thread::sleep_until(next_time);
            uint64_t current_time = mtime.fetch_add(1, std::memory_order_seq_cst) + 1;
            for(HART* hrt : hart_list) {
                hrt->csrs[TIME] = current_time;
                update_mip(hrt);
            }
        }
    });
}

void ACLINT::stop_timer_thread() {
    stop_timer = true;
    if (timer_thread.joinable())
        timer_thread.join();
}

uint64_t ACLINT::read(HART* hart, uint64_t addr, uint64_t size) {
    uint64_t off = addr - base;

    if(off < ACLINT_MSWI_SIZE) {
        return read_mswi(hart,off);
    } else if(off >= ACLINT_MSWI_SIZE && off < ACLINT_MTIMER_SIZE) {
        return read_mtimer(hart,off-ACLINT_MSWI_SIZE);
    }

    return 0;
}

void ACLINT::write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    uint64_t off = addr - base;

    if(off < ACLINT_MSWI_SIZE) {
        write_mswi(hart,off,value);
    } else if(off >= ACLINT_MSWI_SIZE && off < ACLINT_MTIMER_SIZE) {
        write_mtimer(hart,off-ACLINT_MSWI_SIZE,value);
    }
}

uint64_t ACLINT::read_mswi(HART* hart, uint64_t offset) {
    uint64_t hart_id = offset >> 2;
    uint64_t pending = hart_list[hart_id]->csrs[MIP] & hart_list[hart_id]->csrs[MIE];
    return (pending >> IRQ_MSW) & 0x1;
}
uint64_t ACLINT::read_mtimer(HART* hart, uint64_t offset) {
    uint64_t hart_id = offset >> 3;
    if (offset == 0x7FF8) {
        return mtime.load(std::memory_order_seq_cst);
    }
    return mtimecmp[hart_id];
}
void ACLINT::write_mswi(HART* hart, uint64_t offset, uint64_t value) {
    uint64_t hart_id = offset >> 2;
    msip[hart_id] = value;
    update_mip(hart_list[hart_id]);
}
void ACLINT::write_mtimer(HART* hart, uint64_t offset, uint64_t value) {
    uint64_t hart_id = offset >> 3;
    if (offset == 0x7FF8) {
        mtime.store(value, std::memory_order_seq_cst);
        return;
    }

    mtimecmp[hart_id] = value;
    update_mip(hart_list[hart_id]);
}

void ACLINT::update_mip(HART* hart) {
    uint64_t mip = hart->h_cpu_csr_read(MIP);
    uint32_t hart_id = hart->h_cpu_id();

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

    hart->h_cpu_csr_write(MIP, mip);
}