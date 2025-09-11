#include "../include/devices/clint.hpp"
#include "../include/libfdt.hpp"

CLINT::CLINT(uint64_t base, DRAM& ram, uint32_t num_harts, fdt_node* fdt)
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