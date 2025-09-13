#include <iostream>
#include "../include/cpu.h"

#include "../include/devices/mmio.h"
#include "../include/devices/clint.hpp"
#include "../include/devices/rom.hpp"
#include "../include/devices/plic.hpp"
#include "../include/devices/uart.hpp"
#include "../include/devices/virtio.hpp"
#include "../include/devices/dtb.hpp"

#include "../include/memory_map.h"
#include "../include/libfdt.hpp"
#include <string>
#include <vector>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <filesystem>

/*
	The Broken Insts:
		- MULHSU
		- REMUW
		- DIVUW
	TODO:
		-RV32F
		-RV64F
		-RV32F for RV32C
		-RV64F for RV64C
		-RV32D
		-RV64D
		-RV32D for RV32C
		-RV64D for RV64C
*/

bool hasValue(char* arr[], int arrlen, std::string match) {
	bool has = false;
	for (int i = 1; i < arrlen; ++i) {
        if (std::string(arr[i]) == match) {
            has = true;
            break;
        }
	}
	return has;
}

int main(int argc, char* argv[]) {
	if(argc > 1) {
		std::vector<std::string> filtered_args;

		bool debug = false;

    	std::string kernel_path;
		bool kernel_has = false;

    	std::string dtb_path;
		bool dtb_has = false;
		uint64_t dtb_path_in_memory = 0;

    	std::string dtb_dump_path;
		bool dtb_dump_has = false;
		
    	std::string image_path;
		bool image_has = false;

		bool testing_enabled = false; // This must be enabled only if cpu testing required. For this to work, you need to compile https://github.com/riscv-software-src/riscv-tests
		std::vector<std::string> testing_files;

		fdt_node* fdt; 

		for (int i = 1; i < argc; ++i) {
			std::string arg(argv[i]);

			if (arg == "--debug") {
				debug = true;
				std::cout << "[DEBUG] Entered debug mode." << std::endl;
			}
			if (arg == "--tests") {
				testing_enabled = true;
				std::cout << "[TESTING] You entered testing mode." << std::endl;
				std::cout << "[TESTING] In this mode, ./tests folder will be iterated by .bin files" << std::endl;
				std::cout << "[TESTING] Those bin files must be tests from repository https://github.com/riscv-software-src/riscv-tests" << std::endl;
				DIR *dp;
				int i = 0;
				struct dirent *ep;     
				dp = opendir ("./tests");
				if (dp != NULL)
				{
					while (ep = readdir (dp))
					i++;
					(void) closedir (dp);
				}
				else {
					std::cerr << "[TESTING] Couldn't open the \"tests\" directory" << std::endl;
					return 1;
				}
				std::cout << "[TESTING] CPU will iterate over " << i << " files" << std::endl;
				
				using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
				for (const auto& dirEntry : recursive_directory_iterator("./tests"))
					testing_files.push_back(dirEntry.path());
			}

			else if (arg == "--kernel" || arg == "-k") {
				kernel_has = true;
				if (i + 1 < argc) {
					kernel_path = argv[i + 1];
					++i;
				} else {
					std::cerr << "Error: no path provided after " << arg << std::endl;
				}
			}
			
			else if (arg == "--image" || arg == "-i") {
				image_has = true;
				if (i + 1 < argc) {
					image_path = argv[i + 1];
					++i;
				} else {
					std::cerr << "Error: no path provided after " << arg << std::endl;
				}
			}
			
			else if (arg == "--dtb" || arg == "-d") {
				dtb_has = true;
				if (i + 1 < argc) {
					dtb_path = argv[i + 1];
					++i;
				} else {
					std::cerr << "Error: no path provided after " << arg << std::endl;
				}
			}

			else if (arg == "--dumpdtb" || arg == "-dd") {
				dtb_dump_has = true;
				if (i + 1 < argc) {
					dtb_dump_path = argv[i + 1];
					++i;
				} else {
					std::cerr << "Error: no path provided after " << arg << std::endl;
				}
			}

			else if (arg.rfind("-",0) != 0) {
				filtered_args.push_back(arg);
			}
		}

		std::string file;
		if (!filtered_args.empty()) {
			file = filtered_args[0];
			std::cout << "Opening bootloader: " << file << std::endl;
		}

		MemoryMap memmap;

		HART* hart = new HART();
		hart->dram.mmap = &memmap;
		MMIO* mmio = new MMIO(hart->dram);
		hart->mmio = mmio;

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
			fdt_node_add_prop_str(chosen, "bootargs", "console=ttyS0");
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
				fdt_node_add_prop(cpu, "compatible", "riscv\0", 6);
				fdt_node_add_prop(cpu, "status", "okay\0", 5);
				fdt_node_add_prop(cpu, "riscv,isa", "rv64ima_zicsr_zifencei\0", 23);
				
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

			// FDT /soc node
			struct fdt_node* soc = fdt_node_create("soc");
			fdt_node_add_prop_u32(soc, "#address-cells", 2);
			fdt_node_add_prop_u32(soc, "#size-cells", 2);
			fdt_node_add_prop_str(soc, "compatible", "simple-bus");
			fdt_node_add_prop(soc, "ranges", NULL, 0);
			fdt_node_add_child(fdt, soc);
		}

		memmap.add_region(0x00000000, 1024*1024*16);
		ROM* rom = new ROM(0,1024*1024*16,hart->dram);
		mmio->add(rom);

		memmap.add_region(0x0C000000, 0x400000);
		PLIC* plic = new PLIC(0x0C000000,0x400000,hart->dram,64,(dtb_has ? NULL : fdt),1);
		mmio->add(plic);

		memmap.add_region(0x10000000, 0x100);
		UART* uart = new UART(0x10000000,hart->dram,plic,1,(dtb_has ? NULL : fdt),1);
		mmio->add(uart);

		memmap.add_region(0x02000000, 0x10000);
		CLINT* clint = new CLINT(0x02000000,hart->dram,1,(dtb_has ? NULL : fdt));
		uint64_t clint_freq = 10'000'000;
		if(!dtb_has) fdt_node_add_prop_u32(fdt_node_find(fdt,"cpus"), "timebase-frequency", clint_freq);
		clint->start_timer(clint_freq,hart);
		mmio->add(clint);

		if(image_has) {
			memmap.add_region(0x10001000, 0x1000);
			VirtioBlkDevice* virtio_blk = new VirtioBlkDevice(0x10001000,0x1000,hart->dram,plic,(dtb_has ? NULL : fdt),2,image_path);
			mmio->add(virtio_blk);
		}
		
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

			hart->cpu_readfile(dtb_path, dtb_path_in_memory, false);
		}

		if (kernel_has) {
			std::cout << "Loading kernel: " << kernel_path << std::endl;
			hart->cpu_readfile(kernel_path, DRAM_BASE + 0x200000,false);
		}

		if (!file.empty()) {
			hart->cpu_readfile(file, DRAM_BASE,false);
		}

		if(!testing_enabled) {
			hart->cpu_start(debug,dtb_path_in_memory);
		} else {
			std::vector<std::string> failed;
			int succeded = 0;
			
			for(std::string &val : testing_files) {
				for(int i=0; i < 0x800; i++) {
					dram_store(&hart->dram,DRAM_BASE + i, 8, 0);
				}
				std::cout << "[TESTING] Executing file " << val << "... ";
				hart->cpu_readfile(val, DRAM_BASE,false);
				int out = hart->cpu_start_testing();
				if(out != 0) {
					std::cout << "[TESTING] [\033[31mFAIL\033[0m] A0 = " << out << std::endl;	
					failed.push_back(val);
				} else {
					std::cout << "[TESTING] [\033[32mSUCCESS\033[0m]" << std::endl;	
					succeded += 1;
				}
			}
			
			std::cout << "[TESTING] " << succeded << " tests out of " << testing_files.size() << " passed." << std::endl;
			if(failed.size() > 0) {
				std::cout << "[TESTING] Failed tests:" << std::endl;
				for(std::string &val : failed) {
					std::cout << "[TESTING] "  << val << std::endl;
				}
			}
			return 0;
		}
		return 0;
	} else {
		std::cout << "Specify a file: ./riscvem <path to binary> -k <path to kernel> -d <path to dtb file>" << std::endl;
		return 1;
	}
}
