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

#include <iostream>
#include "../include/cpu.h"
#include <stdint.h>
#include <string>
#include <fstream>
#include "../include/opcodes.h"
#include "../include/instset.h"
#include "../include/csr.h"
#include "../include/main.hpp"
#include "../include/devices/plic.hpp"
#include <cstdio>
#include <cstdarg>
#include <format>
#include <sstream>
#include <iomanip>
#include <functional>

std::string uint8_to_hex(uint8_t value) {
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value);
    return ss.str();
}
std::string uint32_to_hex(uint32_t value) {
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << static_cast<int>(value);
    return ss.str();
}
std::string uint_to_hex(uint64_t value) {
    std::stringstream stream;
	stream << std::hex << value;
	std::string result( stream.str() );
	return result;
}

void HART::print_d(const std::string& fmt, ...) {
	if(dbg && dbg_showinst) {
		va_list args;
    	va_start(args, fmt);
		
    	vprintf(fmt.c_str(), args);
		va_end(args);
		printf("\n");
	}
}

uint32_t HART::cpu_fetch(uint64_t _pc) {
	uint64_t fetch_buffer_end = fetch_pc + 28;
	if(_pc < fetch_pc || _pc > fetch_buffer_end) {
		// Refetch
		bool out = memmap.copy_mem_safe(_pc,32,&fetch_buffer);
		if(out == false) return 0; 
		fetch_pc = _pc;
	}
	uint8_t fetch_indx = (_pc - fetch_pc) / 4;
	return fetch_buffer[fetch_indx];
}

void HART::cpu_start(bool debug, uint64_t dtb_path, bool gdbstub) {
	for(int i=0; i<32; i++) regs[i] = 0;
	for(int i=0; i<4069; i++) csrs[i] = 0;
	regs[0] = 0x00;
	//regs[2] = 0x1000;
	pc      = DRAM_BASE;
	virt_pc = pc;
	dbg_singlestep = false;
	dbg     = debug;
	this->gdbstub = gdbstub;

	id = 0;

	stopexec = false;
	regs[10] = id;

	regs[11] = dtb_path;

	reservation_valid = false;
	reservation_value = 0;
	reservation_addr = 0;
	reservation_size = 32;

	csrs[MISA] = riscv_mkmisa("imasu");
	csrs[MVENDORID] = 0; 
	csrs[MARCHID] = 0; 
	csrs[MIMPID] = 0;
	csrs[MHARTID] = 0;
	csrs[MIDELEG] = 5188;
	csrs[MIP] = 0x80;
	csrs[MSTATUS] = (0xA00000000);
	csrs[SSTATUS] = (0x200000000);

    uint32_t fetch_buffer[8];
    uint64_t fetch_pc; 
    uint8_t fetch_buffer_i; 

	mode = 3;
	/*
		Just for reminder the ids:
			0 - User
			1 - Supervisor
			2 - Hypervisor (who even needs that??)
			3 - Machine
	*/

	cpu_loop();
}
int HART::cpu_start_testing() {
	testing = true;
	trap_active = false;
	trap_notify = false;
	//block_enabled = false;
	instr_block.clear();
	for(int i=0; i<32; i++) regs[i] = 0;
	for(int i=0; i<4069; i++) csrs[i] = 0;
	regs[0] = 0x00;
	//regs[2] = 0x1000;
	pc      = DRAM_BASE;
	virt_pc = pc;

	id = 0;

	stopexec = false;
	regs[10] = id;

	reservation_valid = false;
	reservation_value = 0;
	reservation_addr = 0;
	reservation_size = 32;

	csrs[MISA] = riscv_mkmisa("imasu");
	csrs[MVENDORID] = 0; 
	csrs[MARCHID] = 0; 
	csrs[MIMPID] = 0;
	csrs[MHARTID] = 0;
	csrs[MIDELEG] = 5188;
	csrs[MIP] = 0x80;
	csrs[MSTATUS] = (0xA00000000);
	csrs[SSTATUS] = (0x200000000);

    uint32_t fetch_buffer[8];
    uint64_t fetch_pc; 
    uint8_t fetch_buffer_i; 

	mode = 3;
	cpu_loop();
	return regs[10];
}
void HART::cpu_loop() {
	while(true) {
		if(god_said_to_destroy_this_thread) break;
		if(trap_active && testing && regs[17] == 93) break;
		if(stopexec) {continue;}
		if(gdbstub) continue;

		if(reservation_valid) {
			uint64_t val = dram_load(&(dram),reservation_addr,reservation_size);
			if(val != reservation_value) {
				reservation_valid = false;
			}
		}

		if(dbg) {
			Device* dev = mmio->devices[1];
			dynamic_cast<PLIC*>(dev)->plic_service(this);

			std::cout << "[dbg] ";
			std::string line;
			if(!std::getline(std::cin,line))
				continue;

			if(line == "exit" || line == "quit") {
				fastexit();
				break;
			}
			else if(line == "singlestep") {
				dbg_singlestep = !dbg_singlestep;
				std::cout << dbg_singlestep << std::endl;
			}
			else if(line == "showinst") {
				dbg_showinst = !dbg_showinst;
				std::cout << dbg_showinst << std::endl;
			}
			else if(line == "getall") {
				std::cout << "--------------------------------------------------------------------------------------------------" << std::endl;
				for(int i=0; i < 32; i++) {
					std::cout << "x" << i << ": 0x" << uint32_to_hex(regs[i]) << std::endl;
				}
				std::cout << "--------------------------------------------------------------------------------------------------" << std::endl;
				std::cout << "mstatus: 0x" << uint32_to_hex(csrs[MSTATUS]) << std::endl;
				std::cout << "mcause: 0x" << uint32_to_hex(csrs[MCAUSE]) << std::endl;
				std::cout << "sstatus: 0x" << uint32_to_hex(csrs[SSTATUS]) << std::endl;
				std::cout << "scause: 0x" << uint32_to_hex(csrs[SCAUSE]) << std::endl;
				std::cout << "mtvec: 0x" << uint32_to_hex(csrs[MTVEC]) << std::endl;
				std::cout << "medeleg: 0x" << uint32_to_hex(csrs[MEDELEG]) << std::endl;
				std::cout << "stvec: 0x" << uint32_to_hex(csrs[STVEC]) << std::endl;
				std::cout << "sedeleg: 0x" << uint32_to_hex(csrs[SEDELEG]) << std::endl;
				std::cout << "mepc: 0x" << uint32_to_hex(csrs[MEPC]) << std::endl;
				std::cout << "mideleg: 0x" << uint32_to_hex(csrs[MIDELEG]) << std::endl;
				std::cout << "sepc: 0x" << uint32_to_hex(csrs[SEPC]) << std::endl;
				std::cout << "sideleg: 0x" << uint32_to_hex(csrs[SIDELEG]) << std::endl;
				std::cout << "--------------------------------------------------------------------------------------------------" << std::endl;
			}
			else if(line == "getpc") {
				std::cout << pc << std::endl;
			}
			else if(line.rfind("bp",0) == 0) {
				std::string ignore;
				std::string count;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, count, ' ');
				if(count == "" || count == " ") {
					std::cout << "Specify a instruction address" << std::endl;
					continue;
				} else {
					breakpoint = std::stoul(count);
				}
			}
			else if(line.rfind("memd",0) == 0) {
				std::string ignore;
				std::string count;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, count, ' ');
				if(count == "" || count == " ")
					count = std::to_string(pc);
				for(uint64_t i=std::stoul(count); i < std::stoul(count) + 1024; i+=1) {
					std::cout << " 0x" << uint8_to_hex(dram.mmap->load(i,8));
					if(i%4 == 0) {
						std::cout << std::endl;
					}
				}
			}
			else if(line.rfind("runinst",0) == 0) {
				std::string ignore;
				std::string ins;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, ins, ' ');
				uint32_t inst = std::stoul(ins);
				if(ins == "" || ins == " " || inst == 0) {
					std::cout << "Specify a instruction number" << std::endl;
					continue;
				} else {
					HART::cpu_execute_inst(inst);
				}
			}
			else if(line.rfind("test",0) == 0) {
				mmio->store(this,0x10000000, 8, 'A');
			}
			else if(line.rfind("run", 0) == 0) {
				if(dbg_singlestep) {
					std::string ignore;
					std::string count;
					std::istringstream iss(line);
					std::getline(iss, ignore, ' ');
					std::getline(iss, count, ' ');
					if(count == "" || count == " ")
						count = "1";
					for(int i = 0; i < std::stoi(count); ++i) {
						if(stopexec) continue;
						cpu_execute();
						if(pc == breakpoint && breakpoint != 0) break;
					}
				} else {
					while(true) {
						if(stopexec) continue;
						cpu_execute();
						if(pc == breakpoint && breakpoint != 0) break;
					}
				}
			}
			else if(line.rfind("getmem8",0) == 0) {
				std::string ignore;
				std::string reg;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, reg, ' ');
				if(reg == "" || reg == " " || std::stoul(reg) < 0) {
					std::cout << "Specify a memory address" << std::endl;
					continue;
				} else {
					std::cout << dram.mmap->load(std::stoul(reg),8) << std::endl;
				}
			}
			else if(line.rfind("getmem16",0) == 0) {
				std::string ignore;
				std::string reg;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, reg, ' ');
				if(reg == "" || reg == " " || std::stoul(reg) < 0) {
					std::cout << "Specify a memory address" << std::endl;
					continue;
				} else {
					std::cout << dram.mmap->load(std::stoul(reg),16) << std::endl;
				}
			}
			else if(line.rfind("getmem32",0) == 0) {
				std::string ignore;
				std::string reg;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, reg, ' ');
				if(reg == "" || reg == " " || std::stoul(reg) < 0) {
					std::cout << "Specify a memory address" << std::endl;
					continue;
				} else {
					std::cout << dram.mmap->load(std::stoul(reg),32) << std::endl;
				}
			}
			else if(line.rfind("getmem64",0) == 0) {
				std::string ignore;
				std::string reg;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, reg, ' ');
				if(reg == "" || reg == " " || std::stoul(reg) < 0) {
					std::cout << "Specify a memory address" << std::endl;
					continue;
				} else {
					std::cout << dram.mmap->load(std::stoul(reg),64) << std::endl;
				}
			}
			else if(line.rfind("getreg",0) == 0) {
				std::string ignore;
				std::string reg;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, reg, ' ');
				if(reg == "" || reg == " " || std::stoi(reg) < 0 || std::stoi(reg) > 31) {
					std::cout << "Specify a register from 0 to 31" << std::endl;
					continue;
				} else {
					std::cout << "Register x" << std::stoi(reg) << ": " << regs[std::stoi(reg)] << "; " << (int64_t)regs[std::stoi(reg)] << "; 0x" << uint_to_hex(regs[std::stoi(reg)]) << std::endl;
				}
			}
			else if(line.rfind("setreg",0) == 0) {
				std::string ignore;
				std::string reg;
				std::string value;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, reg, ' ');
				std::getline(iss, value, ' ');
				if(reg == "" || reg == " " || std::stoi(reg) < 0 || std::stoi(reg) > 31) {
					std::cout << "Specify a register from 0 to 31" << std::endl;
					continue;
				} else {
					regs[std::stoi(reg)] = std::stoi(value);
				}
			}
			else if(line.rfind("getmode",0) == 0) {
				std::cout << (mode == 0 ? "User" : (mode == 1 ? "Supervisor" : (mode == 2 ? "Hypervisor" : "Machine"))) << std::endl;
			}
			else if(line.rfind("getcsr",0) == 0) {
				std::string ignore;
				std::string reg;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, reg, ' ');
				if(reg == "" || reg == " " || std::stoi(reg) < 0 || std::stoi(reg) > 4096) {
					std::cout << "Specify a csr from 0 to 4096" << std::endl;
					continue;
				} else {
					std::cout << "CSR " << std::stoi(reg) << ": " << csrs[std::stoi(reg)] << std::endl;
				}
			}
			else if(line.rfind("setcsr",0) == 0) {
				std::string ignore;
				std::string reg;
				std::string value;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, reg, ' ');
				std::getline(iss, value, ' ');
				if(reg == "" || reg == " " || std::stoi(reg) < 0 || std::stoi(reg) > 4096) {
					std::cout << "Specify a csr from 0 to 4096" << std::endl;
					continue;
				} else {
					csrs[std::stoi(reg)] = std::stoi(value);
				}
			}
			else {
				std::cout << "No such command." << std::endl;
			}
		} else {
			Device* dev = mmio->devices[1];
			dynamic_cast<PLIC*>(dev)->plic_service(this);
			cpu_execute();
		}
	}
}

void HART::cpu_execute() {
	cpu_check_interrupts();
	if(trap_notify) {trap_notify = false; return;}
	bool increase = true;
	bool junction = false;
	void (*fn)(HART*, uint32_t);

	auto it1 = instr_block_cache.find(pc);
	if(it1 != instr_block_cache.end()) {
		uint64_t pc_v = pc;
		bool brb = false;
		for(auto &in : instr_block_cache[pc]) {
			auto [fn_b, incr_b,__2,inst_b,isBr,immopt,oprs,__3,__4] = in;
			(void)__2; (void)__3; (void)__2;  // unused
			if(trap_notify) {trap_notify = false; brb = true; break;}
			fn_b(this,inst_b,&oprs);
			brb = oprs.brb;
			if(trap_notify) {trap_notify = false; brb = true; break;}
			if(!isBr && !brb) {pc += ((inst_b & 3) == 3 ? 4 : 2);}
			csrs[INSTRET] += 1;
			csrs[MINSTRET] += 1;
			regs[0] = 0;
			if(isBr && brb) break; 
		}
		csrs[CYCLE] += 1;
		csrs[MCYCLE] += 1;
		instr_block_cache_count_executed[pc_v] += 1;
		virt_pc = pc;
	} else {
		instr_block_cache_count_executed[pc] += 1;
		uint64_t upc = ((block_enabled && instr_block_cache_count_executed[pc] >= (gdbstub ? UINT64_MAX : PC_EXECUTE_COUNT_TO_BLOCK)) ? virt_pc : pc);
		pc_trace_full.push_back(upc);
		if (upc % 4 != 0) {
			cpu_trap(EXC_INST_ADDR_MISALIGNED,upc,false);
		}
		uint32_t inst = 0;
    	CACHE_Instr instr = instr_cache[(upc >> 2) & 0x1FFF];
		if(instr.valid && instr.pc == upc) {
			inst = instr.inst;
		} else {
			inst = cpu_fetch(upc);
			if(inst == 0) {
				std::cout << "[RISCVEM] ILLEGAL INSTR at PC 0x" << std::hex << upc << " from PC 0x" << debug_last_pc << std::dec << std::endl;
				std::cout << "[RISCVEM] PC Trace:" << std::endl;
				for(uint64_t i = pc_trace_full.size()-1; i > pc_trace_full.size()-9; i--) {
					std::cout << std::format("    [{:}] 0x{:08x}",i,pc_trace_full[i]) << std::endl;
				}
				cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
			}
			instr = parse_instruction(this,inst,upc);
			if(instr.valid == false) {
				std::cout << "[RISCVEM] ILLEGAL INSTR at PC 0x" << std::hex << upc << " from PC 0x" << debug_last_pc << std::dec << std::endl;
				std::cout << "[RISCVEM] PC Trace:" << std::endl;
				for(uint64_t i = pc_trace_full.size()-1; i > pc_trace_full.size()-9; i--) {
					std::cout << std::format("    [{:}] 0x{:08x}",i,pc_trace_full[i]) << std::endl;
				}
				cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
			}
		}
		debug_last_pc = upc;
		if(dbg && dbg_showinst) {
			std::cout << "New instruction: 0x";
			printf("%.8X",inst);
			std::cout << std::endl;
		}
		
		auto [fn, incr, j, _, isBr, immopt, oprs, __1, __2] = instr;
		(void)__1;
		(void)__2;
		int32_t immo = (int32_t)immopt;
		int OP = inst & 3;
		if(block_enabled && instr_block_cache_count_executed[pc] >= (gdbstub ? UINT64_MAX : UINT64_MAX)) {
			if(!j) {
				instr_block.push_back(instr);
				virt_pc += (OP == 3 ? 4 : 2);
			} else {
				// TODO: Two-Pass IR construction
				//		 First time store all block instrs until junction(Branches and JAL included in list)
				//		 Then iterate for each instruction and check branches, if it poiniting further then stop iterating and create list, else continue
				//		 It will possibly increase block creation time due to O(2), but it grands is possibility to create branches that can point to +imm values
				bool can = true;
				if(isBr) {
					/*std::cout << virt_pc << std::endl;
					std::cout << immo << std::endl;
					std::cout << pc << std::endl;
					std::cout << instr_block.size()*4 << std::endl;

					std::cout << ((virt_pc + immo) >= pc) << std::endl;
					std::cout << ((virt_pc + immo) <= (pc + instr_block.size()*4)) << std::endl;*/
					if((virt_pc + immo) >= pc && (virt_pc + immo) <= (pc + instr_block.size()*4)) {
						instr_block.push_back(instr);
						virt_pc += (OP == 3 ? 4 : 2);
						//can = false;
					}
				}
				if(can) {
					bool brb = false;
					if(instr_block.size() > 0) {
						auto cp = instr_block;
						instr_block_cache[pc] = cp;
						instr_block_cache_count_executed[pc] = 1;
						for(auto &in : instr_block) {
							auto [fn_b, incr_b,__2,inst_b,isBr,immopt,oprs,__3,__4] = in;
							(void)__2; (void)__3; (void)__2;  // unused
							if(trap_notify) {trap_notify = false; brb = true; break;}
							fn_b(this,inst_b,&oprs);
							brb = oprs.brb;
							if(trap_notify) {trap_notify = false; brb = true; break;}
							if(!isBr && !brb) {pc += ((inst_b & 3) == 3 ? 4 : 2);}
							csrs[INSTRET] += 1;
							csrs[MINSTRET] += 1;
							regs[0] = 0;
							if(isBr && brb) break;
						}
						instr_block.clear();
					}
					if(!brb) {
						fn(this,inst,&oprs);
						if(trap_notify) {brb = true; trap_notify = false;}
						if(incr && !brb) {pc += (OP == 3 ? 4 : 2);}
					}
					virt_pc = pc;

					csrs[CYCLE] += 1;
					csrs[MCYCLE] += 1;
					csrs[INSTRET] += 1;
					csrs[MINSTRET] += 1;
					regs[0] = 0;
				}
			}
		} else {
			regs[0] = 0;
			if(!trap_notify) {
				fn(this,inst,&oprs);
				if(incr && !trap_notify) {pc += (OP == 3 ? 4 : 2);}
				virt_pc = pc;
				csrs[CYCLE] += 1;
				csrs[MCYCLE] += 1;
				csrs[INSTRET] += 1;
				csrs[MINSTRET] += 1;
			}
			if(trap_notify) {trap_notify = false;}
			regs[0] = 0;
		}
	}
}
void HART::cpu_execute_inst(uint32_t ins) {
	int OP = ins & 3;
	CACHE_Instr instr = parse_instruction(this,ins,0);
	auto [fn, incr, j, _, isBr, immopt, oprs,__1,__2] = instr;
	(void)__1;
	(void)__2;
	int32_t immo = (int32_t)immopt;
	fn(this,ins,&oprs);
	if(incr) {pc += (OP == 3 ? 4 : 2);}

	csrs[CYCLE] += 1;
	csrs[MCYCLE] += 1;
	csrs[INSTRET] += 1;
	csrs[MINSTRET] += 1;
	regs[0] = 0;
}

uint64_t HART::cpu_readfile(std::string path, uint64_t addr, bool bigendian) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "File open error: " << path << std::endl;
        return 0;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
	std::vector<char> buffer(size);
    file.read(buffer.data(), size);

    try {
        /*for (std::streamsize i = 0; i < size; ++i) {
            uint8_t byte;
            file.read(reinterpret_cast<char*>(&byte), 1);
            dram.mmap->store(addr + i, 8, byte);
        }
		if (bigendian) {
			for(uint64_t i = 0; i < size; i += 4) {
				uint32_t val = dram.mmap->load(addr+i,32);
				dram.mmap->store(addr+i,32,switch_endian(val));
			}	
		}*/
		auto region = memmap.find_region(addr);
		uint8_t* ptr = region->ptr(addr);
		memcpy(ptr, buffer.data(), size);
		
    } catch (const std::out_of_range&) {
        std::cout << "File too big or address out of memory map range" << std::endl;
        std::cout << size << "   " << addr << std::endl;
        file.close();
        return 0;
    }

    file.close();
    return static_cast<uint64_t>(size);
}


void HART::cpu_trap(uint64_t cause, uint64_t tval, bool is_interrupt)
{
    if (trap_active)
        return;

	if (mode == 3)
		csrs[MSTATUS] &= ~(1ULL << MSTATUS_MIE_BIT);
	else if (mode == 1)
		csrs[MSTATUS] &= ~(1ULL << SSTATUS_SIE_BIT);

    trap_active = true;
    trap_notify = true;
    instr_block.clear();

    if (dbg) {
        std::cout << (is_interrupt ? "INTERRUPT" : "EXCEPTION")
                  << " cause=" << cause
                  << " tval=" << tval << std::endl;
    }

    uint64_t cur_mode = mode; // 0=U, 1=S, 3=M

    if (is_interrupt && stopexec)
        stopexec = false;

    uint64_t medeleg = csrs[MEDELEG];
    uint64_t mideleg = csrs[MIDELEG];

    bool delegate_to_s = false;
    if (cur_mode != 3) {
        if (is_interrupt)
            delegate_to_s = (mideleg >> cause) & 1ULL;
        else
            delegate_to_s = (medeleg >> cause) & 1ULL;
    }

    if (delegate_to_s) {

        csr_write(SEPC, pc);
        csr_write(SCAUSE, ((uint64_t)is_interrupt << 63 | cause));
        csr_write(STVAL, tval);

        uint64_t sstatus = csr_read(SSTATUS);

        uint64_t sie = (sstatus >> SSTATUS_SIE_BIT) & 1ULL;

        sstatus &= ~(
            (1ULL << SSTATUS_SIE_BIT)  |
            (1ULL << SSTATUS_SPIE_BIT) |
            (1ULL << SSTATUS_SPP_BIT)
        );

        if (sie)
            sstatus |= (1ULL << SSTATUS_SPIE_BIT);

        if (cur_mode == 1)
            sstatus |= (1ULL << SSTATUS_SPP_BIT);

        csr_write(SSTATUS, sstatus);

        mode = 1;

        uint64_t stvec = csr_read(STVEC);
        uint64_t base  = stvec & TVEC_BASE_MASK;
        uint64_t vmode = stvec & TVEC_MODE_MASK;

        if (vmode == 1 && is_interrupt)
            pc = base + 4 * cause;
        else
            pc = base;
		

        virt_pc = pc;
        return;
    }

    csrs[MEPC]   = pc;
    csrs[MCAUSE] = ((uint64_t)is_interrupt << 63 | cause);
    csrs[MTVAL]  = tval;

    uint64_t mstatus = csrs[MSTATUS];
    uint64_t mie = (mstatus >> MSTATUS_MIE_BIT) & 1ULL;

    mstatus &= ~(
        (1ULL << MSTATUS_MIE_BIT)  |
        (1ULL << MSTATUS_MPIE_BIT) |
        MSTATUS_MPP_MASK
    );

    if (mie)
        mstatus |= (1ULL << MSTATUS_MPIE_BIT);

    mstatus |= (cur_mode << MSTATUS_MPP_SHIFT) & MSTATUS_MPP_MASK;

    csrs[MSTATUS] = mstatus;

    mode = 3;

    uint64_t mtvec = csrs[MTVEC];
    uint64_t base  = mtvec & TVEC_BASE_MASK;
    uint64_t vmode = mtvec & TVEC_MODE_MASK;

    if (vmode == 1 && is_interrupt)
        pc = base + 4 * cause;
    else
        pc = base;

    virt_pc = pc;
}

void HART::cpu_check_interrupts() {
    uint64_t mip     = csrs[MIP];
    uint64_t mie     = csrs[MIE];
    uint64_t mstatus = csrs[MSTATUS];
    uint64_t sstatus = csr_read(SSTATUS);
    uint64_t mideleg = csrs[MIDELEG];

    bool m_ie = (mstatus >> MSTATUS_MIE_BIT) & 1;
    bool s_ie = (sstatus >> SSTATUS_SIE_BIT) & 1;

    uint64_t pending = mip & mie;
	if(trap_active) return;
    if (!pending)
        return;

    if (mode == 3) {
        if (!m_ie)
            return;

        if (pending & (1ULL << MIP_MEIP)) {
            cpu_trap(IRQ_MEXT, 0, true);
            return;
        }
        if (pending & (1ULL << MIP_MSIP)) {
            cpu_trap(IRQ_MSW, 0, true);
            return;
        }
        if (pending & (1ULL << MIP_MTIP)) {
            cpu_trap(IRQ_MTIMER, 0, true);
            return;
        }
        return;
    }

    /* ---- external ---- */
    if (pending & (1ULL << MIP_SEIP)) {
        if ((mideleg & (1ULL << IRQ_SEXT)) && s_ie) {
            cpu_trap(IRQ_SEXT, 0, true);
        } else if (m_ie) {
            cpu_trap(IRQ_SEXT, 0, true);
        }
        return;
    }

    /* ---- software ---- */
    if (pending & (1ULL << MIP_SSIP)) {
        if ((mideleg & (1ULL << IRQ_SSW)) && s_ie) {
            cpu_trap(IRQ_SSW, 0, true);
        } else if (m_ie) {
            cpu_trap(IRQ_SSW, 0, true);
        }
        return;
    }

    /* ---- timer ---- */
    if (pending & (1ULL << MIP_STIP)) {
        if ((mideleg & (1ULL << IRQ_STIMER)) && s_ie) {
            cpu_trap(IRQ_STIMER, 0, true);
        } else if (m_ie) {
            cpu_trap(IRQ_STIMER, 0, true);
        }
        return;
    }
}

#define SSTATUS_MASK 0x80000003000DE762

#define SIE_MASK ((1ULL << MIP_SSIP) | (1ULL << MIP_STIP) | (1ULL << MIP_SEIP))

#define SIP_WMASK (1ULL << MIP_SSIP)

uint64_t HART::csr_read(uint64_t addr) {
	switch(addr) {
		case SSTATUS:
			return csrs[MSTATUS] & SSTATUS_MASK; // Bit mask for S-bits
		case SIE:
			return csrs[MIE] & SIE_MASK;
		case SIP:
			return csrs[MIP] & SIE_MASK;
		default:
			return csrs[addr];
	}
}
void HART::csr_write(uint64_t addr,uint64_t val) {
	switch(addr) {
		case MTVEC:
			csrs_old_mtvec = csrs[MTVEC];
			csrs[MTVEC] = val;
			break;
		case STVEC:
			csrs_old_stvec = csrs[STVEC];
			csrs[STVEC] = val;
			break;
		case SSTATUS:
			csrs[MSTATUS] = (csrs[MSTATUS] & ~SSTATUS_MASK) | (val & SSTATUS_MASK); // Bit mask for S-bits
			break;
		case SIE:
			csrs[MIE] = (csrs[MIE] & ~SIE_MASK) | (val & SIE_MASK);
			break;
		case SIP:
			csrs[MIP] = (csrs[MIP] & ~SIP_WMASK) | (val & SIP_WMASK);
			break;
		default:
			csrs[addr] = val;
			break;
	}
}

// Those functions existing just for header files btw
uint64_t HART::h_cpu_csr_read(uint64_t addr) {
	return csr_read(addr);
}

void HART::h_cpu_csr_write(uint64_t addr, uint64_t value) {
	csr_write(addr,value);
}

uint8_t HART::h_cpu_id() {
	return id;
}