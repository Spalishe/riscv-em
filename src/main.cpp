#include <iostream>
#include "../include/cpu.h"

#include "../include/devices/mmio.h"
#include "../include/devices/clint.hpp"
#include "../include/devices/rom.hpp"
#include "../include/devices/plic.hpp"
#include "../include/devices/uart.hpp"
#include "../include/devices/virtio.hpp"
#include "../include/devices/dtb.hpp"
#include "../include/devices/syscon.hpp"
#include "../include/devices/pci.hpp"
#include "../include/devices/i2c_oc.hpp"
#include "../include/devices/i2c_hid.hpp"
#include "../include/devices/hid_keyboard.hpp"

#include <linux/input.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include "../include/memory_map.h"
#include "../include/libfdt.hpp"
#include "../include/main.hpp"
#include <string>
#include <vector>
#include <random>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <filesystem>

/*
	What should i add to functionality if i want to add 2 harts:
		- FENCE
		- CLINT Timer Update

	The Broken Insts:
		none
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

MemoryMap memmap;


std::string file;
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

HART* hart;
MMIO* mmio;
CLINT* clint;

bool kb_running = true;
std::thread kb_t;

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
		fdt_node_add_prop_str(chosen, "bootargs", "console=ttyS0");
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

	memmap.add_region(0x00000000, 1024*1024*12); // Do not make it round 16 mb, or it will override syscon
	ROM* rom = new ROM(0,1024*1024*12,hart->dram);
	mmio->add(rom);

	memmap.add_region(0x0C000000, 0x400000);
	PLIC* plic = new PLIC(0x0C000000,0x400000,hart->dram,64,(dtb_has ? NULL : fdt),1);
	mmio->add(plic);

	memmap.add_region(0x1000000, 0x1000);
	SYSCON* syscon = new SYSCON(0x1000000,0x1000,hart->dram,(dtb_has ? NULL : fdt));
	mmio->add(syscon);

	memmap.add_region(0x30000000, 0x10000000);
	PCI* pci = new PCI(0x30000000,0x10000000,hart->dram,(dtb_has ? NULL : fdt));
	mmio->add(pci);
	
	memmap.add_region(0x10000000, 0x100);
	UART* uart = new UART(0x10000000,hart->dram,plic,irq_num,(dtb_has ? NULL : fdt),1);
	mmio->add(uart);
	irq_num ++;

	memmap.add_region(0x10030000, 0x1000);
	i2c_bus_t* i2c_os = new i2c_bus_t(0x10030000,hart->dram,plic,irq_num,(dtb_has ? NULL : fdt));
	mmio->add(i2c_os);
	irq_num ++;

	hid_keyboard* kb = hid_keyboard_init(1,hart->dram,plic,irq_num,(dtb_has ? NULL : fdt),i2c_os);
	irq_num ++;
	kb_running = true;
	kb_t = std::thread([&uart]() {
		struct termios oldt, newt;
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);

		while (kb_running) {
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(STDIN_FILENO, &fds);
			
			struct timeval tv = {0, 100000}; // 100ms timeout
			
			if (select(1, &fds, NULL, NULL, &tv) > 0) {
				char c;
				if (read(STDIN_FILENO, &c, 1) == 1) {
					uart->receive_byte(c);
				}
			}
		}
		
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	});
	

	memmap.add_region(0x02000000, 0x10000);
	clint = new CLINT(0x02000000,hart->dram,1,(dtb_has ? NULL : fdt));
	uint64_t clint_freq = 10'000'0;
	if(!dtb_has) fdt_node_add_prop_u32(fdt_node_find(fdt,"cpus"), "timebase-frequency", clint_freq);
	clint->start_timer(clint_freq,hart);
	mmio->add(clint);

	if(image_has) {
		memmap.add_region(0x10001000, 0x1000);
		VirtioBlkDevice* virtio_blk = new VirtioBlkDevice(0x10001000,0x1000,hart->dram,plic,(dtb_has ? NULL : fdt),irq_num,image_path);
		mmio->add(virtio_blk);
		irq_num ++;
	}
}

void reset() {
	kb_running = false;
	if (kb_t.joinable())
        kb_t.join();
	clint->stop_timer_thread();
	
	fdt_node_free(fdt);

	delete hart;
	delete mmio;
	delete clint;
	
	hart = new HART();
	hart->dram.mmap = &memmap;
	mmio = new MMIO(hart->dram);
	hart->mmio = mmio;

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

	hart->cpu_start(debug,dtb_path_in_memory);
}

int main(int argc, char* argv[]) {
	if(argc > 1) {
		std::vector<std::string> filtered_args;

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
				if (dp == NULL) {
					std::cerr << "[TESTING] Couldn't open the \"tests\" directory" << std::endl;
					return 1;
				}
				
				using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
				for (const auto& dirEntry : recursive_directory_iterator("./tests")) {
					testing_files.push_back(dirEntry.path());
					i += 1;
				}

				std::cout << "[TESTING] CPU will iterate over " << i << " files" << std::endl;
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
		
		if (!filtered_args.empty()) {
			file = filtered_args[0];
			std::cout << "Opening bootloader: " << file << std::endl;
		}

		hart = new HART();
		hart->dram.mmap = &memmap;
		mmio = new MMIO(hart->dram);
		hart->mmio = mmio;

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

			hart->cpu_readfile(dtb_path, dtb_path_in_memory, false);
		}

		if (kernel_has) {
			std::cout << "Loading kernel: " << kernel_path << std::endl;
			hart->cpu_readfile(kernel_path, DRAM_BASE + 0x200000,false);
		}

		static struct termios old_tio;
		if(!debug && false) { // disable for rn
			struct termios new_tio;

			if (tcgetattr(STDIN_FILENO, &old_tio) < 0) {
				perror("tcgetattr");
				exit(1);
			}

			new_tio = old_tio;
			
			new_tio.c_lflag &= ~(ICANON | ECHO);

			if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tio) < 0) {
				perror("tcsetattr");
				exit(1);
			}
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
					std::cout << "[\033[31mFAIL\033[0m] A0 = " << out << std::endl;	
					failed.push_back(val);
				} else {
					std::cout << "[\033[32mSUCCESS\033[0m]" << std::endl;	
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
