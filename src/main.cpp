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

#include <iostream>
#include "../include/cpu.h"
#include "../include/argparser.h"

#include "../include/devices/mmio.h"
#include "../include/devices/aclint.hpp"
#include "../include/devices/rom.hpp"
#include "../include/devices/plic.hpp"
#include "../include/devices/uart.hpp"
#include "../include/devices/dtb.hpp"
#include "../include/devices/syscon.hpp"
#include "../include/devices/virtio_blk.hpp"

#include "../include/memory_map.h"
#include "../include/libfdt.hpp"
#include "../include/gdbstub.hpp"
#include <string>
#include <vector>
#include <random>

#include "../include/main.hpp"

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <filesystem>

/*
	What should i add to functionality if i want to add 2 harts:
		- FENCE
		- THE REAL ATOMICITY

	The Broken Insts:
		none
	TODO:
		Entire core btw
		- Counter CSRs
		-RTC GoldFish

		-Zba - bit manip
		-Zbb - bit manip
		-Zbc - carry-less mul
		-Zicbom - non-coherent DMA
		-Zicboz - fast memory zeroing

		-MMU
		-RV32F
		-RV64F
		-RV32F for RV32C
		-RV64F for RV64C
		-RV32D
		-RV64D
		-RV32D for RV32C
		-RV64D for RV64C
*/

MemoryMap memmap;

std::string file;

bool gdb_stub = false;

std::string kernel_path;
bool kernel_has = false;

std::string dtb_path;
bool dtb_has = false;
uint64_t dtb_path_in_memory = 0;

std::string dtb_dump_path;
bool dtb_dump_has = false;

std::string image_path;
bool image_has = false;

std::string cmdline_append;

fdt_node* fdt; 

HART* hart;
MMIO* mmio;
ACLINT* clint;

std::vector<HART*> hart_list;
std::unordered_map<HART*,std::thread> hart_list_threads;

void add_devices_and_map() {
	memmap.add_region(DRAM_BASE, DRAM_SIZE);

	dtb_path_in_memory = DRAM_BASE + DRAM_SIZE - 0x20000;

	if (!dtb_has) {
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
		fdt_node_add_prop_reg(memory, "reg", DRAM_BASE, DRAM_SIZE);
		fdt_node_add_child(fdt, memory);

		// FDT /cpus node
		struct fdt_node* cpus = fdt_node_create("cpus");
		fdt_node_add_prop_u32(cpus, "#address-cells", 1);
		fdt_node_add_prop_u32(cpus, "#size-cells", 0);
		for(uint32_t i=0; i < 1; i++) {
			struct fdt_node* cpu = fdt_node_create_reg("cpu", i);

			fdt_node_add_prop_str(cpu, "device_type", "cpu");
			fdt_node_add_prop_u32(cpu, "reg", i);
			fdt_node_add_prop_u32(cpu, "riscv,cbop-block-size", 0x40);
			fdt_node_add_prop_u32(cpu, "riscv,cboz-block-size", 0x40);
			fdt_node_add_prop_u32(cpu, "riscv,cbom-block-size", 0x40);

			fdt_node_add_prop(cpu, "compatible", "riscv\0", 6);
			fdt_node_add_prop(cpu, "status", "okay\0", 5);
			fdt_node_add_prop(cpu, "riscv,isa", "rv64ima_zicsr_zifencei\0", 23);
			fdt_node_add_prop_str(cpu, "mmu-type", "riscv,none");
			
			struct fdt_node* clic = fdt_node_create("interrupt-controller");
			fdt_node_add_prop_u32(clic, "#interrupt-cells", 1);
			fdt_node_add_prop(clic, "interrupt-controller", NULL, 0);
			fdt_node_add_prop_str(clic, "compatible", "riscv,cpu-intc");

			fdt_node_add_child(cpu, clic);

			fdt_node_add_child(cpus, cpu);
		}

		fdt_node_add_child(fdt, cpus);

		for(uint32_t i=0; i < 1; i++) {
			fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"cpus"),"cpu",i));
			fdt_node_get_phandle(fdt_node_find(fdt_node_find_reg(fdt_node_find(fdt,"cpus"),"cpu",i),"interrupt-controller"));
		}

		// FDT cpu-map
		struct fdt_node* cpu_map = fdt_node_create("cpu-map");
		struct fdt_node* cluster0 = fdt_node_create("cluster0");
		for(uint32_t i=0; i < 1; i++) {
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
	}

	uint64_t irq_num = 1;

	memmap.add_region(0x00001000, 1024*1024*4); // Do not make it round 16 mb, or it will override syscon
	ROM* rom = new ROM(0x00001000,1024*1024*4,hart->dram);
	mmio->add(rom);

	memmap.add_region(0x0C000000, 0x400000);
	PLIC* plic = new PLIC(0x0C000000,0x400000,hart->dram,64,(dtb_has ? NULL : fdt),1);
	mmio->add(plic);
	
	memmap.add_region(0x1000000, 0x1000);
	SYSCON* syscon = new SYSCON(0x1000000,0x1000,hart->dram,(dtb_has ? NULL : fdt));
	mmio->add(syscon);
	
	memmap.add_region(0x10000000, 0x100);
	UART* uart = new UART(0x10000000,hart->dram,plic,irq_num,(dtb_has ? NULL : fdt),1);
	mmio->add(uart);
	irq_num ++;

	memmap.add_region(0x02000000, 0x10000);
	clint = new ACLINT(0x02000000,hart->dram,1,(dtb_has ? NULL : fdt));
	uint64_t clint_freq = 100000;
	if(!dtb_has) fdt_node_add_prop_u32(fdt_node_find(fdt,"cpus"), "timebase-frequency", clint_freq);
	clint->start_timer(clint_freq);
	mmio->add(clint);

	/*memmap.add_region(0x30000000, 0x10000000);
	PCI* pci = new PCI(0x30000000,0x10000000,hart->dram,(dtb_has ? NULL : fdt));
	mmio->add(pci);*/

	if(image_has) {
		memmap.add_region(0x10001000, 0x1000);
		std::cout << "Loading image: " << image_path << std::endl;
		VirtIO_BLK* virtio_blk = new VirtIO_BLK(0x10001000,0x1000,hart->dram,plic,(dtb_has ? NULL : fdt),irq_num,image_path);
		mmio->add(virtio_blk);
		irq_num ++;
	}
}

void poweroff() {
	// FIXME
}

void reset() {
	// FIXME
	/*if(isNotMain) {
		reset_pending = true;
		return;
	} else {
		reset_pending = false;
		shutdown = true;
		kb_running = false;
		if (kb_t.joinable())
			kb_t.join();
		clint->stop_timer_thread();

		fdt_node_free(fdt);

		for(HART* hrt : hart_list) {
			hrt->god_said_to_destroy_this_thread = true;
			if (hart_list_threads[hrt].joinable())
				hart_list_threads[hrt].join();
			delete hrt;
		}

		hart_list.clear();

		delete mmio;
		delete clint;
		
		hart = new HART();
		hart->dram.mmap = &memmap;
		mmio = new MMIO(hart->dram);
		hart->mmio = mmio;

		hart_list.push_back(hart);
		
		if(using_SDL) fb->clear();

		add_devices_and_map();

		if(!dtb_has) {
			size_t dtb_size = fdt_size(fdt);
			void* buffer = malloc(dtb_size);

			size_t size = fdt_serialize(fdt,buffer,0x1000,0);

			if(dtb_dump_has) {
				FILE* f = fopen(dtb_dump_path.c_str(), "wb");
				fwrite(buffer, 1, size, f);
				fclose(f);
			}

			uint8_t* bytes = static_cast<uint8_t*>(buffer);
			for (size_t i = 0; i < dtb_size; ++i) {
				dram_store(&hart->dram,dtb_path_in_memory+i,8,bytes[i]);
			}	

			free(buffer);
		} else {
			hart->cpu_readfile(dtb_path, dtb_path_in_memory, false);
		}

		if (kernel_has) {
			hart->cpu_readfile(kernel_path, DRAM_BASE + 0x200000,false);
		}

		if (!file.empty()) {
			hart->cpu_readfile(file, DRAM_BASE,false);
		}

		for (HART* hrt : hart_list) {
			hart_list_threads[hrt] = std::thread(&HART::cpu_start,hart,debug,dtb_path_in_memory,gdb_stub);
		}
		sdl_loop();
	}*/
}

int main(int argc, char* argv[]) {
	Argparser::Argparser parser(argc,argv);
	parser.setProgramName("RISC-V EM");
	parser.addArgument("--bios", "File with Machine Level program (bootloader)",false,false,Argparser::ArgumentType::str);

	parser.addArgument("--kernel", "File with Supervisor Level program",false,false,Argparser::ArgumentType::str);
	parser.addArgument("--image", "File with Image file that will put on VirtIO-BLK",false,false,Argparser::ArgumentType::str);

	parser.addArgument("--dtb", "Use specified FDT instead of auto-generated",false,false,Argparser::ArgumentType::str);
	parser.addArgument("--dumpdtb", "Dumps auto-generated FDT to file",false,false,Argparser::ArgumentType::str);
	parser.addArgument("--gdb", "Starts GDB Stub on port 1512", false, false, Argparser::ArgumentType::def);
	parser.addArgument("--append", "Append command line arguments", false, false, Argparser::ArgumentType::str);

	parser.parse();

	if(parser.getDefined(6)) {
		cmdline_append = parser.getString(6);
		std::cout << "Custom cmdargs defined: " << cmdline_append << std::endl;
	}

	kernel_has = parser.getDefined(1);
	if(kernel_has) 
		kernel_path = parser.getString(1);
		
	image_has = parser.getDefined(2);
	if(image_has)
		image_path = parser.getString(2);

	dtb_has = parser.getDefined(3);
	if(dtb_has)
		dtb_path = parser.getString(3);

	dtb_dump_has = parser.getDefined(4);
	if(dtb_dump_has)
		dtb_dump_path = parser.getString(4);
	
	gdb_stub = parser.getDefined(5);

	bool file_has = parser.getDefined(0);
	if(file_has) {
		file = parser.getString(0);
		std::cout << "Opening bootloader: " << file << std::endl;
	}

	hart = new HART();
	hart->dram.mmap = &memmap;
	mmio = new MMIO(hart->dram);
	hart->mmio = mmio;

	hart_list.push_back(hart);

	add_devices_and_map();
	
	//memmap.add_region(dtb_path_in_memory, 0x1000);
	//DTB* dtb = new DTB(dtb_path_in_memory,0x1000,hart->dram);
	//mmio->add(dtb);
	//commented cuz path in memory rn > 0x80000000

	if(!dtb_has) {
		size_t dtb_size = fdt_size(fdt);
		void* buffer = malloc(dtb_size);

		size_t size = fdt_serialize(fdt,buffer,0x1000,0);

		if(dtb_dump_has) {
			FILE* f = fopen(dtb_dump_path.c_str(), "wb");
			fwrite(buffer, 1, size, f);
			fclose(f);
		}

		uint8_t* bytes = static_cast<uint8_t*>(buffer);
		for (size_t i = 0; i < dtb_size; ++i) {
			dram_store(&hart->dram,dtb_path_in_memory+i,8,bytes[i]);
		}	

		free(buffer);
	} else {
		// places our dtb in memory

		// FIXME: Load dtb to memory
		//hart->cpu_readfile(dtb_path, dtb_path_in_memory, false);
	}

	if (kernel_has) {
		std::cout << "Loading kernel: " << kernel_path << std::endl;
		// FIXME: Load kernel to memory
		//hart->cpu_readfile(kernel_path, DRAM_BASE + 0x200000,false);
	}

	if (!file.empty()) {
		// FIXME: Load bios to memory
		//hart->cpu_readfile(file, DRAM_BASE,false);
	}

	for (HART* hrt : hart_list) {
		hart_list_threads[hrt] = std::thread(&HART::cpu_start,hrt,dtb_path_in_memory,gdb_stub);
	}
	if(gdb_stub) GDB_Create(hart_list[0]);

	for(HART* hrt : hart_list) {
		hart_list_threads[hrt].join();
	}
	
	return 0;
	
}
