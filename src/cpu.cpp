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
#include "../include/jit_h.hpp"
#include "../include/devices/plic.hpp"
#include <cstdio>
#include <cstdarg>
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

void HART::cpu_start(bool debug, uint64_t dtb_path, bool nojit) {
	for(int i=0; i<32; i++) regs[i] = 0;
	for(int i=0; i<4069; i++) csrs[i] = 0;
	regs[0] = 0x00;
	//regs[2] = 0x1000;
	pc      = DRAM_BASE;
	virt_pc = pc;
	dbg_singlestep = false;
	dbg     = debug;

	id = 0;

	jit_enabled = !nojit;

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
	csrs[MSTATUS] = 0xA00000000;

	mode = 3;
	/*
		Just for reminder the ids:
			0 - User
			1 - Superuser
			2 - Hyperuser (who even needs that??)
			3 - Machine
	*/

	cpu_loop();
}
int HART::cpu_start_testing(bool nojit) {
	testing = true;
	trap_active = false;
	trap_notify = false;
	//block_enabled = false;
	instr_block_cache_jit.clear();
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

	jit_enabled = !nojit;

	reservation_valid = false;
	reservation_value = 0;
	reservation_addr = 0;
	reservation_size = 32;

	csrs[MISA] = riscv_mkmisa("imasu");
	csrs[MVENDORID] = 0; 
	csrs[MARCHID] = 0; 
	csrs[MIMPID] = 0;
	csrs[MHARTID] = 0;
	csrs[MSTATUS] = 0xA00000000;

	mode = 3;
	cpu_loop();
	return regs[10];
}
uint32_t HART::cpu_fetch() {
	uint64_t upc = (block_enabled ? virt_pc : pc);
	if (upc % 2 != 0) {
		cpu_trap(EXC_INST_ADDR_MISALIGNED,upc,false);
	}
	uint32_t inst = 0;
	std::optional<uint64_t> val = mmio->load(this,upc,32);
	if(val.has_value()) {
		inst = (uint32_t) *val;
	}
	if(dbg && dbg_showinst) {
		std::cout << "Next instruction: 0x";
		printf("%.8X",inst);
		std::cout << std::endl;
	}
	return inst;
}
void HART::cpu_loop() {
	while(true) {
		if(trap_active && testing) break;
		if(stopexec) continue;

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
					HART::cpu_execute(inst);
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
						uint32_t inst = cpu_fetch();
						cpu_execute(inst);
						if(pc == breakpoint && breakpoint != 0) break;
					}
				} else {
					while(true) {
						if(stopexec) continue;
						uint32_t inst = cpu_fetch();
						if(inst == 0) break;
						cpu_execute(inst);
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
			uint32_t inst = cpu_fetch();
			if(inst == 0) {
				cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
			} else
				cpu_execute(inst);
		}
	}
}

void HART::cpu_execute(uint32_t inst) {
	int OP = inst & 3;
	bool increase = true;
	bool junction = false;
	void (*fn)(HART*, uint32_t);

	auto it1 = instr_block_cache.find(pc);
	auto it2 = instr_block_cache_jit.find(pc);
	if(jit_enabled && it2 != instr_block_cache_jit.end()) {
		instr_block_cache_jit[pc](this);
		virt_pc = pc;
	} else if(it1 != instr_block_cache.end()) {
		uint64_t pc_v = pc;
		if(instr_block_cache_count_executed[pc_v] > BLOCK_EXECUTE_COUNT_TO_JIT && jit_enabled){
			BlockFn jfn = jit_create_block(this,instr_block_cache[pc_v]);
			instr_block_cache_jit[pc_v] = jfn;
			jfn(this);
		} else {
			for(auto &in : instr_block_cache[pc]) {
				auto [fn_b, __1,__2,inst_b,isBr,immopt,oprs] = in;
				(void)__1; (void)__2; // unused
				if(trap_notify) {trap_notify = false;}
				fn_b(this,inst_b,&oprs,NULL);
				if(trap_notify) {trap_notify = false; break;}
				if(!isBr) pc += ((inst_b & 3) == 3 ? 4 : 2);
				csrs[CYCLE] += 1;
				regs[0] = 0;
				if(isBr) break; 
			}
		}
		instr_block_cache_count_executed[pc_v] += 1;
		virt_pc = pc;
	} else {
		CACHE_Instr instr = parse_instruction(this,inst);
		auto [fn, incr, j, _, isBr, immopt, oprs] = instr;
		if(block_enabled) {
			if(!j) {
				instr_block.push_back(instr);
				virt_pc += (OP == 3 ? 4 : 2);
			} else {
				// TODO: Two-Pass IR translation
				//		 First time store all block instrs until junction(Branches and JAL included in list)
				//		 Then iterate for each instruction and check branches, if it poiniting further then stop iterating and create list, else continue
				//		 It will possibly increase block creation time due to O(2), but it grands is possibility to create branches that can point to +imm values
				bool can = true;
				if(isBr) {
					if((pc + immopt) >= pc && (pc + immopt) <= (instr_block.size()*4)) {
						instr_block.push_back(instr);
						virt_pc += (OP == 3 ? 4 : 2);
						can = false;
					}
				}
				if(can) {
					if(instr_block.size() > 0) {
						auto cp = instr_block;
						instr_block_cache[pc] = cp;
						instr_block_cache_count_executed[pc] = 1;
						for(auto &in : instr_block) {
							auto [fn_b, __1,__2,inst_b,isBr,immopt,oprs] = in;
							(void)__1; (void)__2; // unused
							if(trap_notify) {trap_notify = false;}
							fn_b(this,inst_b,&oprs,NULL);
							if(trap_notify) {trap_notify = false; break;}
							if(!isBr) pc += ((inst_b & 3) == 3 ? 4 : 2);
							csrs[CYCLE] += 1;
							regs[0] = 0;
							if(isBr) break;
						}
						instr_block.clear();
					}
					fn(this,inst,&oprs,NULL);
					if(incr) {pc += (OP == 3 ? 4 : 2);}
					virt_pc = pc;

					csrs[CYCLE] += 1;
					regs[0] = 0;
				}
			}
		} else {
			fn(this,inst,&oprs,NULL);
			if(incr) {pc += (OP == 3 ? 4 : 2);}

			csrs[CYCLE] += 1;
			regs[0] = 0;
		}
	}
}
uint64_t HART::cpu_readfile(std::string path, uint64_t addr, bool bigendian) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "File open error: " << path << std::endl;
        return 0;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    try {
        for (std::streamsize i = 0; i < size; ++i) {
            uint8_t byte;
            file.read(reinterpret_cast<char*>(&byte), 1);
            dram.mmap->store(addr + i, 8, byte);
        }
		if (bigendian) {
			for(uint64_t i = 0; i < size; i += 4) {
				uint32_t val = dram.mmap->load(addr+i,32);
				dram.mmap->store(addr+i,32,switch_endian(val));
			}	
		}
    } catch (const std::out_of_range&) {
        std::cout << "File too big or address out of memory map range" << std::endl;
        std::cout << size << "   " << addr << std::endl;
        file.close();
        return 0;
    }

    file.close();
    return static_cast<uint64_t>(size);
}

// convenience macro to set mcause interrupt bit for XLEN=64
static inline uint64_t mcause_encode(bool interrupt, uint64_t code) {
    if (sizeof(uintptr_t) == 8)
        return (interrupt ? (1ULL << 63) : 0ULL) | (code & ~(1ULL<<63));
    else
        return (interrupt ? (1U << 31) : 0U) | (code & ~(1U<<31));
}

// Helper: map interrupt cause code -> corresponding MIP bit index
static inline int cause_to_mipbit(int cause) {
    switch(cause) {
        case IRQ_MEXT: return MIP_MEIP;
        case IRQ_SEXT: return MIP_SEIP;
        case IRQ_UEXT: return MIP_UEIP;
        case IRQ_MTIMER: return MIP_MTIP;
        case IRQ_STIMER: return MIP_STIP;
        case IRQ_UTIMER: return MIP_UTIP;
        case IRQ_MSW: return MIP_MSIP;
        case IRQ_SSW: return MIP_SSIP;
        case IRQ_USW: return MIP_USIP;
        default: 
            // For platform interrupts >=16 you would map differently
            return -1;
    }
}

void HART::cpu_trap(uint64_t cause, uint64_t tval, bool is_interrupt) {
	trap_active = true;
	trap_notify = true;
	instr_block.clear(); // Trap changes PC so yea
	if(dbg) {
		std::cout << (is_interrupt ? "INTERRUPT" : "EXCEPTION") << " " << cause << "   " << tval << std::endl;
	}

    // current mode
    uint64_t cur_mode = mode; // 0=U,1=S,2=H,3=M

	if(is_interrupt && stopexec) {
		uint64_t tw = (csrs[MSTATUS] & (1<<21)) >> 21;
		if(cur_mode == 0) {
			stopexec = false;
			cpu_trap(EXC_ILLEGAL_INSTRUCTION,cur_mode,false);
		} else {
			if(tw == 1 && cur_mode == 1) {
				stopexec = false;
				cpu_trap(EXC_ILLEGAL_INSTRUCTION,cur_mode,false);
			}
		}

		return;
	}

    // Read delegation registers
    uint64_t medeleg = csrs[MEDELEG];
    uint64_t mideleg = csrs[MIDELEG];

    // Decide whether to delegate to S-mode:
    bool can_delegate_to_s = (cur_mode != 3); // only traps from U or S can be delegated
    bool delegate_to_s = false;
    if (can_delegate_to_s) {
        if (is_interrupt) {
            if ( (mideleg >> cause) & 1ULL ) delegate_to_s = true;
        } else {
            if ( (medeleg >> cause) & 1ULL ) delegate_to_s = true;
        }
    }

    if (delegate_to_s) {
        // Supervisor trap handling (write sepc/scause/stval, update sstatus, jump to stvec)
        csrs[SEPC] = pc;
        csrs[SCAUSE] = mcause_encode(is_interrupt, cause);
        csrs[STVAL] = tval;

        // Update sstatus: SPIE <- SIE; SIE <- 0; SPP <- old privilege (0 for U, 1 for S)
        uint64_t sstatus = csrs[SSTATUS];
        uint64_t old_sie = (sstatus >> SSTATUS_SIE_BIT) & 1ULL;
        // clear SPIE, SPP, SIE
        sstatus &= ~((1ULL<<SSTATUS_SPIE_BIT) | (1ULL<<SSTATUS_SPP_BIT) | (1ULL<<SSTATUS_SIE_BIT));
        if (old_sie) sstatus |= (1ULL<<SSTATUS_SPIE_BIT);
        if (cur_mode == 1) sstatus |= (1ULL<<SSTATUS_SPP_BIT); // set SPP if came from S; else leave 0 for U
        csrs[SSTATUS] = sstatus;

        // Switch to S-mode
        mode = 1;

        // Compute target PC from stvec
        uint64_t stvec = csrs[STVEC];
        uint64_t mode = stvec & TVEC_MODE_MASK;
        uint64_t base = stvec & TVEC_BASE_MASK;
        if (mode == 0) {
            pc = base;
        } else if (mode == 1 && is_interrupt) {
            // vectored only for interrupts
            pc = base + 4 * cause;
        } else {
            pc = base; // fallback
        }

    } else {
        // Machine trap handling (write mepc/mcause/mtval, update mstatus, jump to mtvec)
        csrs[MEPC] = pc;
        csrs[MCAUSE] = mcause_encode(is_interrupt, cause);
        csrs[MTVAL] = tval;

        // Update mstatus: MPIE <- MIE; MIE <- 0; MPP <- cur_mode
        uint64_t mstatus = csrs[MSTATUS];
        uint64_t old_mie = (mstatus >> MSTATUS_MIE_BIT) & 1ULL;
        // clear MPIE, MPP, MIE
        mstatus &= ~((1ULL<<MSTATUS_MPIE_BIT) | MSTATUS_MPP_MASK | (1ULL<<MSTATUS_MIE_BIT));
        if (old_mie) mstatus |= (1ULL<<MSTATUS_MPIE_BIT);
        uint64_t mpp = 0ULL;
        if (cur_mode == 0) mpp = 0ULL;
        else if (cur_mode == 1) mpp = 1ULL;
        else if (cur_mode == 3) mpp = 3ULL;
        mstatus |= ((mpp << MSTATUS_MPP_SHIFT) & MSTATUS_MPP_MASK);
        csrs[MSTATUS] = mstatus;

        // Switch to M-mode
        mode = 3;

        // Compute target PC from mtvec
        uint64_t mtvec = csrs[MTVEC];
        uint64_t mode = mtvec & TVEC_MODE_MASK;
        uint64_t base = mtvec & TVEC_BASE_MASK;
        if (mode == 0) {
            pc = base;
        } else if (mode == 1 && is_interrupt) {
            pc = base + 4 * cause;
        } else {
            pc = base;
        }
    }
}


// Those functions existing just for header files btw
uint64_t HART::h_cpu_csr_read(uint64_t addr) {
	return csrs[addr];
}

void HART::h_cpu_csr_write(uint64_t addr, uint64_t value) {
	csrs[addr] = value;
}

uint8_t HART::h_cpu_id() {
	return id;
}