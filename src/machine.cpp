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

#include <fstream>
#include <cerrno>
#include <cstring>
#include <random>
#include "../include/machine.hpp"

void machine_run(Machine& cpu) {
    cpu.state = cpu.gdb ? MachineState::Halted : MachineState::Running;

    while(cpu.state != MachineState::PoweredOff) {
		if (cpu.state == MachineState::PoweringOff) {
			cpu.state = MachineState::PoweredOff;
			break;
		}
        if (cpu.state == MachineState::Halted) {
			if(cpu.gdb_single_step) {
				cpu.state = MachineState::Running;
			}
            std::this_thread::yield();
            continue;
        }
        for (auto& h : cpu.harts) {
			cpu.plic->plic_service(h);
			h->GPR[0] = 0;
            if(h->WFI) {
                hart_check_interrupts(*h);
                continue;
            }
            hart_step(*h);
        }

        cpu.clint->tick();
		if(cpu.gdb_single_step) {
			cpu.gdb_single_step = false;
			cpu.state = MachineState::Halted;
		}
    }
    machine_destroy_harts(cpu);
}

void machine_create_memory(Machine& cpu) {
    cpu.memmap.add_region(DRAM_BASE,cpu.memsize);
    cpu.mmio = new MMIO(cpu.dram);
	cpu.mmu = new MMU();
	cpu.mmu->cpu = &cpu;
	cpu.mmu->dram = &cpu.dram;
    cpu.dram.mmap = &cpu.memmap;
    cpu.mmio->ram = cpu.dram;
	cpu.mmio->mmu = cpu.mmu;
}

void machine_create_fdt(Machine& cpu, const string file_dtb = "", const string cmdline_append = "", const string dtb_dump_path = "") {
    uint64_t dtb_path_in_memory = DRAM_BASE + cpu.memsize - 0x20000;
    if(file_dtb.length() == 0) {
        // Generate FDT
        fdt_node* fdt = cpu.fdt;
        fdt = fdt_node_create(NULL);
		fdt_node_add_prop_u32(fdt, "#address-cells", 2);
		fdt_node_add_prop_u32(fdt, "#size-cells", 2);
		fdt_node_add_prop_str(fdt, "model", "Risc-V EM");
		fdt_node_add_prop(fdt, "compatible", "riscv-virtio\0simple-bus\0", 24);

		// FDT /chosen node
		struct fdt_node* chosen = fdt_node_create("chosen");
		fdt_node_add_prop_str(chosen, "bootargs", cmdline_append.c_str());
		std::vector<uint32_t> rngseed;
		std::random_device rd;
		std::mt19937 gen(rd());
		
		std::uniform_int_distribution<int32_t> dist(
			std::numeric_limits<int32_t>::min(),
			std::numeric_limits<int32_t>::max()
		);
		for(int i=0; i < 16; i++) {
			rngseed.push_back(dist(gen));
		}
		fdt_node_add_prop_cells(chosen, "rng-seed", rngseed,rngseed.size());
		fdt_node_add_child(fdt, chosen);

		// FDT /memory node
		struct fdt_node* memory = fdt_node_create_reg("memory", DRAM_BASE);
		fdt_node_add_prop_str(memory, "device_type", "memory");
		fdt_node_add_prop_reg(memory, "reg", DRAM_BASE, cpu.memsize);
		fdt_node_add_child(fdt, memory);

		// FDT /cpus node
		struct fdt_node* cpus = fdt_node_create("cpus");
		fdt_node_add_prop_u32(cpus, "#address-cells", 1);
		fdt_node_add_prop_u32(cpus, "#size-cells", 0);
		for(uint32_t i=0; i < cpu.core_count; i++) {
			struct fdt_node* cpu = fdt_node_create_reg("cpu", i);

			fdt_node_add_prop_str(cpu, "device_type", "cpu");
			fdt_node_add_prop_u32(cpu, "reg", i);

			fdt_node_add_prop(cpu, "compatible", "riscv\0", 6);
			fdt_node_add_prop(cpu, "status", "okay\0", 5);
			fdt_node_add_prop(cpu, "riscv,isa", "rv64ima_zicsr_zifencei_zba_zbb_zbc_zbs\0", 23);
			fdt_node_add_prop_str(cpu, "mmu-type", "riscv,sv39");
			
			struct fdt_node* clic = fdt_node_create("interrupt-controller");
			fdt_node_add_prop_u32(clic, "#interrupt-cells", 1);
			fdt_node_add_prop(clic, "interrupt-controller", NULL, 0);
			fdt_node_add_prop_str(clic, "compatible", "riscv,cpu-intc");

			fdt_node_add_child(cpu, clic);

			fdt_node_add_child(cpus, cpu);
		}

		fdt_node_add_child(fdt, cpus);

		for(uint32_t i=0; i < cpu.core_count; i++) {
			fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"cpus"),"cpu",i));
			fdt_node_get_phandle(fdt_node_find(fdt_node_find_reg(fdt_node_find(fdt,"cpus"),"cpu",i),"interrupt-controller"));
		}

		// FDT cpu-map
		struct fdt_node* cpu_map = fdt_node_create("cpu-map");
		struct fdt_node* cluster0 = fdt_node_create("cluster0");
		for(uint32_t i=0; i < cpu.core_count; i++) {
			std::string text = "core" + std::to_string(i);
			struct fdt_node* core = fdt_node_create(text.c_str());
			fdt_node_add_prop_u32(core,"cpu",fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"cpus"),"cpu",i)));
		}

		// FDT /soc node
		struct fdt_node* soc = fdt_node_create("soc");
		fdt_node_add_prop_u32(soc, "#address-cells", 2);
		fdt_node_add_prop_u32(soc, "#size-cells", 2);
		fdt_node_add_prop_str(soc, "compatible", "simple-bus");
		fdt_node_add_prop(soc, "ranges", NULL, 0);
		fdt_node_add_child(fdt, soc);


        size_t dtb_size = fdt_size(fdt);
		void* buffer = malloc(dtb_size);

		size_t size = fdt_serialize(fdt,buffer,0x1000,0);

		if(dtb_dump_path.length() != 0) {
			FILE* f = fopen(dtb_dump_path.c_str(), "wb");
			fwrite(buffer, 1, size, f);
			fclose(f);
		}

		char* bytes = static_cast<char*>(buffer);
		cpu.memmap.load_buffer(dtb_path_in_memory, bytes, size);

		free(buffer);
    } else {
        cpu.memmap.load_file(dtb_path_in_memory, file_dtb);
        cpu.dtb_is_file = true;
    }
}

void machine_create_devices(Machine& cpu, const string image_path = "") {
    uint64_t irq_num = 1;

	cpu.memmap.add_region(0x00001000, 1024*1024*4); // Do not make it round 16 mb, or it will override syscon
	ROM* rom = new ROM(0x00001000,1024*1024*4,cpu);
	cpu.mmio->add(rom);

	cpu.memmap.add_region(0x0C000000, 0x400000);
	PLIC* plic = new PLIC(0x0C000000,0x400000,cpu,64,(cpu.dtb_is_file ? NULL : cpu.fdt));
	cpu.mmio->add(plic);
    cpu.plic = plic;
	
	cpu.memmap.add_region(0x1000000, 0x1000);
	SYSCON* syscon = new SYSCON(0x1000000,0x1000,cpu,(cpu.dtb_is_file ? NULL : cpu.fdt));
	cpu.mmio->add(syscon);
    cpu.syscon = syscon;
	
	cpu.memmap.add_region(0x10000000, 0x100);
	UART* uart = new UART(0x10000000,cpu,plic,irq_num,(cpu.dtb_is_file ? NULL : cpu.fdt));
	cpu.mmio->add(uart);
    cpu.uart = uart;
	irq_num ++;

	cpu.memmap.add_region(0x02000000, 0x10000);
	ACLINT* clint = new ACLINT(0x02000000,cpu,(cpu.dtb_is_file ? NULL : cpu.fdt));
	if(!cpu.dtb_is_file) fdt_node_add_prop_u32(fdt_node_find(cpu.fdt,"cpus"), "timebase-frequency", ACLINT_FREQ_HZ);
    cpu.clint = clint;
	cpu.mmio->add(clint);

	if(image_path.length() != 0) {
		cpu.memmap.add_region(0x10001000, 0x1000);
		VirtIO_BLK* virtio_blk = new VirtIO_BLK(0x10001000,0x1000,cpu,plic,(cpu.dtb_is_file ? NULL : cpu.fdt),irq_num,image_path);
		cpu.mmio->add(virtio_blk);
        cpu.image = virtio_blk;
		irq_num ++;
	}
}

void machine_poweroff(Machine& cpu) {
    cpu.state = MachineState::PoweringOff;
}

void machine_reset(Machine& cpu) {
    
}

void machine_create_harts(Machine& cpu) {
    uint64_t dtb_path_in_memory = DRAM_BASE + cpu.memsize - 0x20000;

    for(uint8_t i = 0; i < cpu.core_count; i++) {
        HART* hart = new HART();
        hart->mmio = cpu.mmio;
        hart->id = i;
        hart_reset(*hart,dtb_path_in_memory);
		cpu.harts.push_back(hart);
    }
}

void machine_destroy_harts(Machine& cpu) {
    cpu.harts.clear();
}