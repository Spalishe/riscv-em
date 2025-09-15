#include <iostream>
#include "../include/cpu.h"
#include <stdint.h>
#include <string>
#include <fstream>
#include "../include/opcodes.h"
#include "../include/instset.h"
#include "../include/csr.h"
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

void HART::cpu_start(bool debug, uint64_t dtb_path) {
	for(int i=0; i<32; i++) regs[i] = 0;
	for(int i=0; i<4069; i++) csrs[i] = 0;
	regs[0] = 0x00;
	//regs[2] = 0x1000;
	pc      = DRAM_BASE;
	virt_pc = pc;
	dbg_singlestep = false;
	dbg     = debug;

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
int HART::cpu_start_testing() {
	testing = true;
	trap_active = false;
	trap_notify = false;
	block_enabled = false;
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
				if(reg == "" || reg == " " || std::stoi(reg) < 0 || std::stoi(reg) > 1971) {
					std::cout << "Specify a csr from 0 to 1971" << std::endl;
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
				if(reg == "" || reg == " " || std::stoi(reg) < 0 || std::stoi(reg) > 1971) {
					std::cout << "Specify a csr from 0 to 1971" << std::endl;
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

// TODO: OPTIMIZE THIS SHIT!!!!!!!!!!!!!!!!!!!!!!!!!!
void HART::cpu_execute(uint32_t inst) {
	int OP = inst & 3;
	bool increase = true;
	bool junction = false;
	void (*fn)(HART*, uint32_t);

	auto it1 = instr_block_cache.find(pc);

	if(it1 != instr_block_cache.end()) {
		for(auto &in : instr_block_cache[pc]) {
			auto [fn_b, inst_b] = in;
			if(trap_notify) {trap_notify = false;}
			fn_b(this,inst_b);
			if(trap_notify) {trap_notify = false; break;}
			pc += ((inst_b & 3) == 3 ? 4 : 2);
			csrs[CYCLE] += 1;
			regs[0] = 0;
		}
		virt_pc = pc;
	} else {
		auto it = instr_cache.find(inst);

		if(it != instr_cache.end()) {
			auto [fn, incr, j] = instr_cache[inst];
			if(block_enabled) {
				if(!j) {
					instr_block.push_back(CACHE_InstrBl{fn,inst});
					virt_pc += (OP == 3 ? 4 : 2);
				} else {
					if(instr_block.size() > 0) {
						auto cp = instr_block;
						instr_block_cache[pc] = cp;
						for(auto &in : instr_block) {
							auto [fn_b, inst_b] = in;
							if(trap_notify) {trap_notify = false;}
							fn_b(this,inst_b);
							if(trap_notify) {trap_notify = false; break;}
							pc += ((inst_b & 3) == 3 ? 4 : 2);
							csrs[CYCLE] += 1;
							regs[0] = 0;
						}
						instr_block.clear();
					}
					fn(this,inst);
					if(incr) {pc += (OP == 3 ? 4 : 2);}
					virt_pc = pc;
				}
			} else {
				fn(this,inst);
				if(incr) {pc += (OP == 3 ? 4 : 2);}
			}
		} else {
			if(OP == 3) {
				int opcode = inst & 0x7f;
				int funct3 = (inst >> 12) & 0x7;
				int amo_funct5 = (inst >> 27) & 0x1F;
				int funct7 = (inst >> 25) & 0x7f;
				int funct6 = (inst >> 26);
				int imm = (inst >> 20);
				switch(opcode) {
					case FENCE:
						switch(funct3) {
							case 1: fn = exec_FENCE_I; junction = true; break;
							case 0: fn = exec_FENCE; junction = true; break;
						}
					case R_TYPE:
						switch(funct3) {
							case ADDSUB:
								switch(funct7) {
									case ADD: fn = exec_ADD; break;
									case 1: fn = exec_MUL; break;
									case SUB: fn = exec_SUB; break;
								}; break;
							case XOR: 
								switch(funct7) {
									case 0: fn = exec_XOR; break;
									case 1: fn = exec_DIV; break;
								};  break;
							case OR: 
								switch(funct7) {
									case 0: fn = exec_OR; break;
									case 1: fn = exec_REM; break;
								};  break;
							case AND: 
								switch(funct7) {
									case 0: fn = exec_AND; break;
									case 1: fn = exec_REMU; break;
								};  break;
							case SLL: 
								switch(funct7) {
									case 0: fn = exec_SLL; break;
									case 1: fn = exec_MULH; break;
								}; break;
							case SR:
								switch(funct7) {
									case SRL: fn = exec_SRL; break;
									case SRA: fn = exec_SRA; break;
									case 1: fn = exec_DIVU; break;
								}; break;
							case SLT: 
								switch(funct7) {
									case 0: fn = exec_SLT; break;
									case 1: fn = exec_MULHSU; break;
								};  break;
							case SLTU: 
								switch(funct7) {
									case 0: fn = exec_SLTU; break;
									case 1: fn = exec_MULHU; break;
								};  break;
						}; break;
					case R_TYPE64:
						switch(funct3) {
							case ADDSUB:
								switch(funct7) {
									case ADD: fn = exec_ADDW; break;
									case SUB: fn = exec_SUBW; break;
									case 1: fn = exec_MULW; break;
								}; break;
							case SLL: fn = exec_SLLW; break;
							case SR:
								switch(funct7) {
									case SRL: fn = exec_SRLW; break;
									case SRA: fn = exec_SRAW; break;
									case 1: fn = exec_DIVUW; break;
								}; break;
							case XOR: fn = exec_DIVW; break;
							case OR: fn = exec_REMW; break;
							case AND: fn = exec_REMUW; break;
						}; break;
					case AMO:
						switch(funct3) {
							case AMO_W: 
								switch(amo_funct5) {
									case AMOADD: fn = exec_AMOADD_W; break;
									case AMOSWAP: fn = exec_AMOSWAP_W; break;
									case LR: fn = exec_LR_W; break;
									case SC: fn = exec_SC_W; break;
									case AMOXOR: fn = exec_AMOXOR_W; break;
									case AMOOR: fn = exec_AMOOR_W; break;
									case AMOAND: fn = exec_AMOAND_W; break;
									case AMOMIN: fn = exec_AMOMIN_W; break;
									case AMOMAX: fn = exec_AMOMAX_W; break;
									case AMOMINU: fn = exec_AMOMINU_W; break;
									case AMOMAXU: fn = exec_AMOMAXU_W; break;
								}; break;
							case AMO_D: 
								switch(amo_funct5) {
									case AMOADD: fn = exec_AMOADD_D; break;
									case AMOSWAP: fn = exec_AMOSWAP_D; break;
									case LR: fn = exec_LR_D; break;
									case SC: fn = exec_SC_D; break;
									case AMOXOR: fn = exec_AMOXOR_D; break;
									case AMOOR: fn = exec_AMOOR_D; break;
									case AMOAND: fn = exec_AMOAND_D; break;
									case AMOMIN: fn = exec_AMOMIN_D; break;
									case AMOMAX: fn = exec_AMOMAX_D; break;
									case AMOMINU: fn = exec_AMOMINU_D; break;
									case AMOMAXU: fn = exec_AMOMAXU_D; break;
								}; break;
						}; break;
					case I_TYPE:
						switch(funct3) {
							case ADDI: fn = exec_ADDI; break;
							case XORI: fn = exec_XORI; break;
							case ORI: fn = exec_ORI; break;
							case ANDI: fn = exec_ANDI; break;
							case SLLI: fn = exec_SLLI; break;
							case SRI:
								/*switch(funct7) {
									case SRLI: fn = exec_SRLI; break;
									case SRAI: fn = exec_SRAI; break;
								}; break;*/
								switch(funct6) {
									case SRLI: fn = exec_SRLI; break;
									case SRAI: fn = exec_SRAI; break;
								}; break;
							case SLTI: fn = exec_SLTI; break;
							case SLTIU: fn = exec_SLTIU; break;
						}; break;
					case I_TYPE64:
						switch(funct3) {
							case ADDI: fn = exec_ADDIW; break;
							case SLLI: fn = exec_SLLIW; break;
							case SRI:
								switch(funct7) {
									case SRLI: fn = exec_SRLIW; break;
									case SRAIW: fn = exec_SRAIW; break;
								}; break;
						}; break;
					case LOAD_TYPE:
						switch(funct3) {
							case LB: fn = exec_LB; break;
							case LH: fn = exec_LH; break;
							case LW: fn = exec_LW; break;
							case LD: fn = exec_LD; break;
							case LBU: fn = exec_LBU; break;
							case LHU: fn = exec_LHU; break;
							case LWU: fn = exec_LWU; break;
						}; break;
					case S_TYPE:
						switch(funct3) {
							case SB: fn = exec_SB; break;
							case SH: fn = exec_SH; break;
							case SW: fn = exec_SW; break;
							case SD: fn = exec_SD; break;
						}; break;
					case B_TYPE:
						switch(funct3) {
							case BEQ: fn = exec_BEQ; junction = true; break;
							case BNE: fn = exec_BNE; junction = true; break;
							case BLT: fn = exec_BLT; junction = true; break;
							case BGE: fn = exec_BGE; junction = true; break;
							case BLTU: fn = exec_BLTU; junction = true; break;
							case BGEU: fn = exec_BGEU; junction = true; break;
						}; break;
					case JAL: fn = exec_JAL; increase = false; junction = true; break;
					case JALR: fn = exec_JALR; increase = false; junction = true; break;
					case LUI: fn = exec_LUI; break;
					case AUIPC: fn = exec_AUIPC; break;
					case ECALL: 
						switch(funct3) {
							case CSRRW: fn = exec_CSRRW; break;
							case CSRRS: fn = exec_CSRRS; break;
							case CSRRC: fn = exec_CSRRC; break;
							case CSRRWI: fn = exec_CSRRWI; break;
							case CSRRSI: fn = exec_CSRRSI; break;
							case CSRRCI: fn = exec_CSRRCI; break;
							case 0:
								switch(imm) {
									case 0: fn = exec_ECALL; increase = false; junction = true; break;
									case 1: fn = exec_EBREAK; junction = true; break;
									case 261: fn = exec_WFI; junction = true; break;
									case 258: fn = exec_SRET; increase = false; junction = true; break;
									case 288: fn = exec_SFENCE_VMA; break;
									case 770: fn = exec_MRET; increase = false; junction = true; break;
								}; break;
						}; break;

					default: cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false); break;
				}
				//if(increase) pc += 4;
			} else {
				int funct2 = (inst >> 5) & 0x3;
				int funct3 = (inst >> 13) & 0x7;
				int funct4 = (inst >> 12) & 0xF;
				int funct5 = (inst >> 10) & 0x3;
				int funct6 = (inst >> 10) & 0x1F;
				switch(OP) {
					case 0: {
						switch(funct3) {
							case 0: fn = exec_C_ADDI4SPN; break;
							case 2: fn = exec_C_LW; break;
							case 3: fn = exec_C_LD; break;
							case 6: fn = exec_C_SW; break;
							case 7: fn = exec_C_SD; break;
						}
					}; break;
					case 1: {
						switch(funct3) {
							case 0: {
								if(inst == 0x1) {
									fn = exec_C_NOP;
								} else {
									fn = exec_C_ADDI;
								}
							}; break;
							case 1: 
								{
									// fn = exec_C_JAL; increase = false; junction = true;
									// Uncomment this if u planning making a fully working 32-bit arch
									fn = exec_C_ADDIW;
								} break;
							case 2: fn = exec_C_LI; break;
							case 3: {
								uint64_t rd = get_bits(inst,11,7);
								if(rd == 2) {
									fn = exec_C_ADDI16SP;
								} else {
									fn = exec_C_LUI;
								}
							}; break;
							case 4: {
								if(funct6 == 35) {
									switch(funct2) {
										case 0: fn = exec_C_SUB; break;
										case 1: fn = exec_C_XOR; break;
										case 2: fn = exec_C_OR; break;
										case 3: fn = exec_C_AND; break;
									}
								} else if(funct6 == 39) {
									switch(funct2) {
										case 0: fn = exec_C_SUBW; break;
										case 1: fn = exec_C_ADDW; break;
									}
								} else {
									switch(funct5) {
										case 0: fn = exec_C_SRLI; break;
										case 1: fn = exec_C_SRAI; break;
										case 2: fn = exec_C_ANDI; break;
									}
								}
							}; break;
							case 5: fn = exec_C_J; increase = false; junction = true; break;
							case 6: fn = exec_C_BEQZ; break;
							case 7: fn = exec_C_BNEZ; break;
						}
					}; break;
					case 2: {
						switch(funct3) {
							case 0: fn = exec_C_SLLI; break;
							case 2: fn = exec_C_LWSP; break;
							case 3: fn = exec_C_LDSP; break;
							case 4: {
								switch(funct4) {
									case 8: {
										uint64_t i = get_bits(inst,6,2);
										switch(i) {
											case 0: fn = exec_C_JR; increase = false; junction = true; break;
											default: fn = exec_C_MV; break;
										}
									}; break;
									case 9: {
										uint64_t i = get_bits(inst,6,2);
										uint64_t i1 = get_bits(inst,11,7);
										if(i == 0 && i1 == 0) {
											fn = exec_C_EBREAK; junction = true;
										} else if(i == 0 && i1 != 0) {
											fn = exec_C_JALR; increase = false; junction = true;
										} else if(i != 0 && i1 != 0) {
											fn = exec_C_ADD;
										}
									}; break;
								}
							}; break;
							case 6: fn = exec_C_SWSP; break;
							case 7: fn = exec_C_SDSP; break;
						}
					}; break;
					default: cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false); break;
				}
				//if(increase) pc += 2;
			}

			if(fn != NULL) {
				auto it1 = instr_cache.find(inst);
				if(it1 == instr_cache.end()) {
					instr_cache[inst] = CACHE_Instr{fn, increase,junction};
				}

				if(block_enabled) {
					if(!junction) {
						instr_block.push_back(CACHE_InstrBl{fn,inst});
						virt_pc += (OP == 3 ? 4 : 2);
					} else {
						if(instr_block.size() > 0) {
							auto cp = instr_block;
							instr_block_cache[pc] = cp;
							for(auto &in : instr_block) {
								auto [fn_b, inst_b] = in;
								if(trap_notify) {trap_notify = false;}
								fn_b(this,inst_b);
								if(trap_notify) {trap_notify = false; break;}
								pc += ((inst_b & 3) == 3 ? 4 : 2);
								csrs[CYCLE] += 1;
								regs[0] = 0;
							}
							instr_block.clear();
						}
						fn(this,inst);
						if(increase) {pc += (OP == 3 ? 4 : 2);}
						virt_pc = pc;
					}
				} else {
					fn(this,inst);
					if(increase) {pc += (OP == 3 ? 4 : 2);}
				}
			}	
		}

		csrs[CYCLE] += 1;
		regs[0] = 0;
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