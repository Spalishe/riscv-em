#include <iostream>
#include "../include/cpu.h"
#include <stdint.h>
#include <string>
#include <fstream>
#include "../include/opcodes.h"
#include "../include/instset.h"
#include "../include/csr.h"
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

void print_d(struct HART *hart, const std::string& fmt, ...) {
	if(hart->dbg) {
		va_list args;
    	va_start(args, fmt);
		
    	vprintf(fmt.c_str(), args);
		va_end(args);
		printf("\n");
	}
}

void cpu_start(struct HART *hart, bool debug, uint64_t dtb_path) {
	for(int i=0; i<32; i++) hart->regs[i] = 0;
	for(int i=0; i<4069; i++) hart->csrs[i] = 0;
	hart->regs[0] = 0x00;
	//hart->regs[2] = 0x1000;
	//hart->pc      = DRAM_BASE;
	hart->pc      = 0;
	hart->dbg_singlestep = false;
	hart->dbg     = debug;

	hart->id = 0;

	hart->stopexec = false;
	hart->regs[10] = hart->id;

	hart->regs[11] = dtb_path;

	hart->reservation_valid = false;
	hart->reservation_value = 0;
	hart->reservation_addr = 0;
	hart->reservation_size = 32;

	hart->csrs[MISA] = riscv_mkmisa("imasu");
	hart->csrs[MVENDORID] = 0; 
	hart->csrs[MARCHID] = 0; 
	hart->csrs[MIMPID] = 0;
	hart->csrs[MHARTID] = 0;
	hart->csrs[MSTATUS] = 0;

	hart->mode = 3;
	/*
		Just for reminder the ids:
			0 - User
			1 - Superuser
			2 - Hyperuser (who even needs that??)
			3 - Machine
	*/

	cpu_loop(hart,debug);
}
uint32_t cpu_fetch(struct HART *hart) {
	if (hart->pc % 2 != 0) {
		cpu_trap(hart,EXC_INST_ADDR_MISALIGNED,hart->pc,false);
	}
	uint32_t inst = 0;
	std::optional<uint64_t> val = hart->mmio->load(hart,hart->pc,32);
	if(val.has_value()) {
		inst = (uint32_t) *val;
	}
	if(hart->dbg) {
		std::cout << "Next instruction: 0x";
		printf("%.8X",inst);
		std::cout << std::endl;
	}
	return inst;
}
void cpu_loop(struct HART *hart,bool debug) {
	while(true) {
		if(hart->stopexec) continue;

		if(hart->reservation_valid) {
			uint64_t val = dram_load(&(hart->dram),hart->reservation_addr,hart->reservation_size);
			if(val != hart->reservation_value) {
				hart->reservation_valid = false;
			}
		}

		if(debug) {
			Device* dev = hart->mmio->devices[1];
			dynamic_cast<PLIC*>(dev)->plic_service(hart);

			std::cout << "[dbg] ";
			std::string line;
			if(!std::getline(std::cin,line))
				continue;

			if(line == "exit" || line == "quit") {
				break;
			}
			else if(line == "singlestep") {
				hart->dbg_singlestep = !hart->dbg_singlestep;
				std::cout << hart->dbg_singlestep << std::endl;
			}
			else if(line == "getall") {
				std::cout << "--------------------------------------------------------------------------------------------------" << std::endl;
				for(int i=0; i < 32; i++) {
					std::cout << "x" << i << ": 0x" << uint32_to_hex(hart->regs[i]) << std::endl;
				}
				std::cout << "--------------------------------------------------------------------------------------------------" << std::endl;
				std::cout << "mstatus: 0x" << uint32_to_hex(hart->csrs[MSTATUS]) << std::endl;
				std::cout << "mcause: 0x" << uint32_to_hex(hart->csrs[MCAUSE]) << std::endl;
				std::cout << "sstatus: 0x" << uint32_to_hex(hart->csrs[SSTATUS]) << std::endl;
				std::cout << "scause: 0x" << uint32_to_hex(hart->csrs[SCAUSE]) << std::endl;
				std::cout << "mtvec: 0x" << uint32_to_hex(hart->csrs[MTVEC]) << std::endl;
				std::cout << "medeleg: 0x" << uint32_to_hex(hart->csrs[MEDELEG]) << std::endl;
				std::cout << "stvec: 0x" << uint32_to_hex(hart->csrs[STVEC]) << std::endl;
				std::cout << "sedeleg: 0x" << uint32_to_hex(hart->csrs[SEDELEG]) << std::endl;
				std::cout << "mepc: 0x" << uint32_to_hex(hart->csrs[MEPC]) << std::endl;
				std::cout << "mideleg: 0x" << uint32_to_hex(hart->csrs[MIDELEG]) << std::endl;
				std::cout << "sepc: 0x" << uint32_to_hex(hart->csrs[SEPC]) << std::endl;
				std::cout << "sideleg: 0x" << uint32_to_hex(hart->csrs[SIDELEG]) << std::endl;
				std::cout << "--------------------------------------------------------------------------------------------------" << std::endl;
			}
			else if(line == "getpc") {
				std::cout << hart->pc << std::endl;
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
					hart->breakpoint = std::stoul(count);
				}
			}
			else if(line.rfind("memd",0) == 0) {
				std::string ignore;
				std::string count;
				std::istringstream iss(line);
				std::getline(iss, ignore, ' ');
				std::getline(iss, count, ' ');
				if(count == "" || count == " ")
					count = std::to_string(hart->pc);
				for(uint64_t i=std::stoul(count); i < std::stoul(count) + 1024; i+=1) {
					std::cout << " 0x" << uint8_to_hex(hart->dram.mmap->load(i,8));
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
            		cpu_execute(hart,inst);
				}
			}
			else if(line.rfind("test",0) == 0) {
				hart->mmio->store(hart,0x10000000, 8, 'A');
			}
			else if(line.rfind("run", 0) == 0) {
				if(hart->dbg_singlestep) {
					std::string ignore;
					std::string count;
					std::istringstream iss(line);
					std::getline(iss, ignore, ' ');
					std::getline(iss, count, ' ');
					if(count == "" || count == " ")
						count = "1";
					for(int i = 0; i < std::stoi(count); ++i) {
						if(hart->stopexec) continue;
						uint32_t inst = cpu_fetch(hart);
            			cpu_execute(hart,inst);
						if(hart->pc == hart->breakpoint && hart->breakpoint != 0) break;
					}
				} else {
					while(true) {
						if(hart->stopexec) continue;
						uint32_t inst = cpu_fetch(hart);
            			if(inst == 0) break;
            			cpu_execute(hart,inst);
						if(hart->pc == hart->breakpoint && hart->breakpoint != 0) break;
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
					std::cout << hart->dram.mmap->load(std::stoul(reg),8) << std::endl;
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
					std::cout << hart->dram.mmap->load(std::stoul(reg),16) << std::endl;
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
					std::cout << hart->dram.mmap->load(std::stoul(reg),32) << std::endl;
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
					std::cout << hart->dram.mmap->load(std::stoul(reg),64) << std::endl;
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
					std::cout << "Register x" << std::stoi(reg) << ": " << hart->regs[std::stoi(reg)] << "; " << (int64_t)hart->regs[std::stoi(reg)] << std::endl;
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
					hart->regs[std::stoi(reg)] = std::stoi(value);
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
					std::cout << "CSR " << std::stoi(reg) << ": " << hart->csrs[std::stoi(reg)] << std::endl;
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
					hart->csrs[std::stoi(reg)] = std::stoi(value);
				}
			}
			else {
				std::cout << "No such command." << std::endl;
			}
		} else {
			Device* dev = hart->mmio->devices[1];
			dynamic_cast<PLIC*>(dev)->plic_service(hart);
			uint32_t inst = cpu_fetch(hart);
			if(inst == 0) {
				cpu_trap(hart,EXC_ILLEGAL_INSTRUCTION,inst,false);
			} else
				cpu_execute(hart,inst);
		}
	}
}
void cpu_execute(struct HART *hart, uint32_t inst) {
	int OP = inst & 3;
	bool increase = true;

	if(OP == 3) {
		int opcode = inst & 0x7f;
		int funct3 = (inst >> 12) & 0x7;
		int amo_funct5 = (inst >> 27) & 0x1F;
		int funct7 = (inst >> 25) & 0x7f;
		int funct6 = (inst >> 26);
		int imm = (inst >> 20);
		switch(opcode) {
			case FENCE:
				exec_FENCE_I(hart,inst); break;
			case R_TYPE:
				switch(funct3) {
					case ADDSUB:
						switch(funct7) {
							case ADD: exec_ADD(hart,inst); break;
							case 1: exec_MUL(hart,inst); break;
							case SUB: exec_SUB(hart,inst); break;
						}; break;
					case XOR: 
						switch(funct7) {
							case 0: exec_XOR(hart,inst); break;
							case 1: exec_DIV(hart,inst); break;
						};  break;
					case OR: 
						switch(funct7) {
							case 0: exec_OR(hart,inst); break;
							case 1: exec_REM(hart,inst); break;
						};  break;
					case AND: 
						switch(funct7) {
							case 0: exec_AND(hart,inst); break;
							case 1: exec_REMU(hart,inst); break;
						};  break;
					case SLL: 
						switch(funct7) {
							case 0: exec_SLL(hart,inst); break;
							case 1: exec_MULH(hart,inst); break;
						}; break;
					case SR:
						switch(funct7) {
							case SRL: exec_SRL(hart,inst); break;
							case SRA: exec_SRA(hart,inst); break;
							case 1: exec_DIVU(hart,inst); break;
						}; break;
					case SLT: 
						switch(funct7) {
							case 0: exec_SLT(hart,inst); break;
							case 1: exec_MULHSU(hart,inst); break;
						};  break;
					case SLTU: 
						switch(funct7) {
							case 0: exec_SLTU(hart,inst); break;
							case 1: exec_MULHU(hart,inst); break;
						};  break;
				}; break;
			case R_TYPE64:
				switch(funct3) {
					case ADDSUB:
						switch(funct7) {
							case ADD: exec_ADDW(hart,inst); break;
							case SUB: exec_SUBW(hart,inst); break;
							case 1: exec_MULW(hart,inst); break;
						}; break;
					case SLL: exec_SLLW(hart,inst); break;
					case SR:
						switch(funct7) {
							case SRL: exec_SRLW(hart,inst); break;
							case SRA: exec_SRAW(hart,inst); break;
							case 1: exec_DIVUW(hart,inst); break;
						}; break;
					case XOR: exec_DIVW(hart,inst); break;
					case OR: exec_REMW(hart,inst); break;
					case AND: exec_REMUW(hart,inst); break;
				}; break;
			case AMO:
				switch(funct3) {
					case AMO_W: 
						switch(amo_funct5) {
							case AMOADD: exec_AMOADD_W(hart,inst); break;
							case AMOSWAP: exec_AMOSWAP_W(hart,inst); break;
							case LR: exec_LR_W(hart,inst); break;
							case SC: exec_SC_W(hart,inst); break;
							case AMOXOR: exec_AMOXOR_W(hart,inst); break;
							case AMOOR: exec_AMOOR_W(hart,inst); break;
							case AMOAND: exec_AMOAND_W(hart,inst); break;
							case AMOMIN: exec_AMOMIN_W(hart,inst); break;
							case AMOMAX: exec_AMOMAX_W(hart,inst); break;
							case AMOMINU: exec_AMOMINU_W(hart,inst); break;
							case AMOMAXU: exec_AMOMAXU_W(hart,inst); break;
						}; break;
					case AMO_D: 
						switch(amo_funct5) {
							case AMOADD: exec_AMOADD_D(hart,inst); break;
							case AMOSWAP: exec_AMOSWAP_D(hart,inst); break;
							case LR: exec_LR_D(hart,inst); break;
							case SC: exec_SC_D(hart,inst); break;
							case AMOXOR: exec_AMOXOR_D(hart,inst); break;
							case AMOOR: exec_AMOOR_D(hart,inst); break;
							case AMOAND: exec_AMOAND_D(hart,inst); break;
							case AMOMIN: exec_AMOMIN_D(hart,inst); break;
							case AMOMAX: exec_AMOMAX_D(hart,inst); break;
							case AMOMINU: exec_AMOMINU_D(hart,inst); break;
							case AMOMAXU: exec_AMOMAXU_D(hart,inst); break;
						}; break;
				}; break;
			case I_TYPE:
				switch(funct3) {
					case ADDI: exec_ADDI(hart,inst); break;
					case XORI: exec_XORI(hart,inst); break;
					case ORI: exec_ORI(hart,inst); break;
					case ANDI: exec_ANDI(hart,inst); break;
					case SLLI: exec_SLLI(hart,inst); break;
					case SRI:
						/*switch(funct7) {
							case SRLI: exec_SRLI(hart,inst); break;
							case SRAI: exec_SRAI(hart,inst); break;
						}; break;*/
						switch(funct6) {
							case SRLI: exec_SRLI(hart,inst); break;
							case SRAI: exec_SRAI(hart,inst); break;
						}; break;
					case SLTI: exec_SLTI(hart,inst); break;
					case SLTIU: exec_SLTIU(hart,inst); break;
				}; break;
			case I_TYPE64:
				switch(funct3) {
					case ADDI: exec_ADDIW(hart,inst); break;
					case SLLI: exec_SLLIW(hart,inst); break;
					case SRI:
						switch(funct7) {
							case SRLI: exec_SRLIW(hart,inst); break;
							case SRAIW: exec_SRAIW(hart,inst); break;
						}; break;
				}; break;
			case LOAD_TYPE:
				switch(funct3) {
					case LB: exec_LB(hart,inst); break;
					case LH: exec_LH(hart,inst); break;
					case LW: exec_LW(hart,inst); break;
					case LD: exec_LD(hart,inst); break;
					case LBU: exec_LBU(hart,inst); break;
					case LHU: exec_LHU(hart,inst); break;
					case LWU: exec_LWU(hart,inst); break;
				}; break;
			case S_TYPE:
				switch(funct3) {
					case SB: exec_SB(hart,inst); break;
					case SH: exec_SH(hart,inst); break;
					case SW: exec_SW(hart,inst); break;
					case SD: exec_SD(hart,inst); break;
				}; break;
			case B_TYPE:
				switch(funct3) {
					case BEQ: exec_BEQ(hart,inst); break;
					case BNE: exec_BNE(hart,inst); break;
					case BLT: exec_BLT(hart,inst); break;
					case BGE: exec_BGE(hart,inst); break;
					case BLTU: exec_BLTU(hart,inst); break;
					case BGEU: exec_BGEU(hart,inst); break;
				}; break;
			case JAL: exec_JAL(hart,inst); increase = false; break;
			case JALR: exec_JALR(hart,inst); increase = false; break;
			case LUI: exec_LUI(hart,inst); break;
			case AUIPC: exec_AUIPC(hart,inst); break;
			case ECALL: 
				switch(funct3) {
					case CSRRW: exec_CSRRW(hart,inst); break;
					case CSRRS: exec_CSRRS(hart,inst); break;
					case CSRRC: exec_CSRRC(hart,inst); break;
					case CSRRWI: exec_CSRRWI(hart,inst); break;
					case CSRRSI: exec_CSRRSI(hart,inst); break;
					case CSRRCI: exec_CSRRCI(hart,inst); break;
					case 0:
						switch(imm) {
							case 0: exec_ECALL(hart,inst); break;
							case 1: exec_EBREAK(hart,inst); break;
							case 261: exec_WFI(hart,inst); break;
							case 258: exec_SRET(hart,inst); increase = false; break;
							case 770: exec_MRET(hart,inst); increase = false; break;
						}; break;
				}; break;

			default: cpu_trap(hart,EXC_ILLEGAL_INSTRUCTION,inst,false); break;
		}
		if(increase) hart->pc += 4;
	} else {
		int funct2 = (inst >> 5) & 0x3;
		int funct3 = (inst >> 13) & 0x7;
		int funct4 = (inst >> 12) & 0xF;
		int funct5 = (inst >> 10) & 0x3;
		int funct6 = (inst >> 10) & 0x1F;
		switch(OP) {
			case 0: {
				switch(funct3) {
					case 0: exec_C_ADDI4SPN(hart,inst); break;
					case 2: exec_C_LW(hart,inst); break;
					case 3: exec_C_LD(hart,inst); break;
					case 6: exec_C_SW(hart,inst); break;
					case 7: exec_C_SD(hart,inst); break;
				}
			}; break;
			case 1: {
				switch(funct3) {
					case 0: {
						if(inst == 0x1) {
							exec_C_NOP(hart,inst);
						} else {
							exec_C_ADDI(hart,inst);
						}
					}; break;
					case 1: 
						{
							// exec_C_JAL(hart,inst); increase = false;
							// Uncomment this if u planning making a fully working 32-bit arch
							exec_C_ADDIW(hart,inst);
						} break;
					case 2: exec_C_LI(hart,inst); break;
					case 3: {
						uint64_t rd = get_bits(inst,11,7);
						if(rd == 2) {
							exec_C_ADDI16SP(hart,inst);
						} else {
							exec_C_LUI(hart,inst);
						}
					}; break;
					case 4: {
						if(funct6 == 35) {
							switch(funct2) {
								case 0: exec_C_SUB(hart,inst); break;
								case 1: exec_C_XOR(hart,inst); break;
								case 2: exec_C_OR(hart,inst); break;
								case 3: exec_C_AND(hart,inst); break;
							}
						} else if(funct6 == 39) {
							switch(funct2) {
								case 0: exec_C_SUBW(hart,inst); break;
								case 1: exec_C_ADDW(hart,inst); break;
							}
						} else {
							switch(funct5) {
								case 0: exec_C_SRLI(hart,inst); break;
								case 1: exec_C_SRAI(hart,inst); break;
								case 2: exec_C_ANDI(hart,inst); break;
							}
						}
					}; break;
					case 5: exec_C_J(hart,inst); increase = false; break;
					case 6: exec_C_BEQZ(hart,inst); break;
					case 7: exec_C_BNEZ(hart,inst); break;
				}
			}; break;
			case 2: {
				switch(funct3) {
					case 0: exec_C_SLLI(hart,inst); break;
					case 2: exec_C_LWSP(hart,inst); break;
					case 3: exec_C_LDSP(hart,inst); break;
					case 4: {
						switch(funct4) {
							case 8: {
								uint64_t i = get_bits(inst,6,2);
								switch(i) {
									case 0: exec_C_JR(hart,inst); increase = false; break;
									default: exec_C_MV(hart,inst); break;
								}
							}; break;
							case 9: {
								uint64_t i = get_bits(inst,6,2);
								uint64_t i1 = get_bits(inst,11,7);
								if(i == 0 && i1 == 0) {
									exec_C_EBREAK(hart,inst);
								} else if(i == 0 && i1 != 0) {
									exec_C_JALR(hart,inst); increase = false;
								} else if(i != 0 && i1 != 0) {
									exec_C_ADD(hart,inst);
								}
							}; break;
						}
					}; break;
					case 6: exec_C_SWSP(hart,inst); break;
					case 7: exec_C_SDSP(hart,inst); break;
				}
			}; break;
			default: cpu_trap(hart,EXC_ILLEGAL_INSTRUCTION,inst,false); break;
		}
		if(increase) hart->pc += 2;
	}

	hart->csrs[CYCLE] += 1;
	hart->regs[0] = 0;
}
uint64_t cpu_readfile(struct HART *hart,std::string path, uint64_t addr, bool bigendian) {
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
            hart->dram.mmap->store(addr + i, 8, byte);
        }
		if (bigendian) {
			for(uint64_t i = 0; i < size; i += 4) {
				uint32_t val = hart->dram.mmap->load(addr+i,32);
				hart->dram.mmap->store(addr+i,32,switch_endian(val));
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

// Primary trap routine: performs full CSR updates and vector jump.
// - hart: pointer to hart state
// - cause: exception/interrupt code (Exception Code field, without interrupt bit)
// - tval: trap value (bad address, instruction bits, etc.)
// - is_interrupt: true if this is an asynchronous interrupt
//
void cpu_trap(struct HART *hart, uint64_t cause, uint64_t tval, bool is_interrupt) {
	if(hart->dbg) {
		std::cout << (is_interrupt ? "INTERRUPT" : "EXCEPTION") << " " << cause << "   " << tval << std::endl;
	}

    // current mode
    uint64_t cur_mode = hart->mode; // 0=U,1=S,2=H,3=M

	if(is_interrupt && hart->stopexec) {
		uint64_t tw = (hart->csrs[MSTATUS] & (1<<21)) >> 21;
		if(cur_mode == 0) {
			hart->stopexec = false;
			cpu_trap(hart,EXC_ILLEGAL_INSTRUCTION,cur_mode,false);
		} else {
			if(tw == 1 && cur_mode == 1) {
				hart->stopexec = false;
				cpu_trap(hart,EXC_ILLEGAL_INSTRUCTION,cur_mode,false);
			}
		}

		return;
	}

    // Read delegation registers
    uint64_t medeleg = hart->csrs[MEDELEG];
    uint64_t mideleg = hart->csrs[MIDELEG];

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
        hart->csrs[SEPC] = hart->pc;
        hart->csrs[SCAUSE] = mcause_encode(is_interrupt, cause);
        hart->csrs[STVAL] = tval;

        // Update sstatus: SPIE <- SIE; SIE <- 0; SPP <- old privilege (0 for U, 1 for S)
        uint64_t sstatus = hart->csrs[SSTATUS];
        uint64_t old_sie = (sstatus >> SSTATUS_SIE_BIT) & 1ULL;
        // clear SPIE, SPP, SIE
        sstatus &= ~((1ULL<<SSTATUS_SPIE_BIT) | (1ULL<<SSTATUS_SPP_BIT) | (1ULL<<SSTATUS_SIE_BIT));
        if (old_sie) sstatus |= (1ULL<<SSTATUS_SPIE_BIT);
        if (cur_mode == 1) sstatus |= (1ULL<<SSTATUS_SPP_BIT); // set SPP if came from S; else leave 0 for U
        hart->csrs[SSTATUS] = sstatus;

        // Switch to S-mode
        hart->mode = 1;

        // Compute target PC from stvec
        uint64_t stvec = hart->csrs[STVEC];
        uint64_t mode = stvec & TVEC_MODE_MASK;
        uint64_t base = stvec & TVEC_BASE_MASK;
        if (mode == 0) {
            hart->pc = base;
        } else if (mode == 1 && is_interrupt) {
            // vectored only for interrupts
            hart->pc = base + 4 * cause;
        } else {
            hart->pc = base; // fallback
        }

    } else {
        // Machine trap handling (write mepc/mcause/mtval, update mstatus, jump to mtvec)
        hart->csrs[MEPC] = hart->pc;
        hart->csrs[MCAUSE] = mcause_encode(is_interrupt, cause);
        hart->csrs[MTVAL] = tval;

        // Update mstatus: MPIE <- MIE; MIE <- 0; MPP <- cur_mode
        uint64_t mstatus = hart->csrs[MSTATUS];
        uint64_t old_mie = (mstatus >> MSTATUS_MIE_BIT) & 1ULL;
        // clear MPIE, MPP, MIE
        mstatus &= ~((1ULL<<MSTATUS_MPIE_BIT) | MSTATUS_MPP_MASK | (1ULL<<MSTATUS_MIE_BIT));
        if (old_mie) mstatus |= (1ULL<<MSTATUS_MPIE_BIT);
        uint64_t mpp = 0ULL;
        if (cur_mode == 0) mpp = 0ULL;
        else if (cur_mode == 1) mpp = 1ULL;
        else if (cur_mode == 3) mpp = 3ULL;
        mstatus |= ((mpp << MSTATUS_MPP_SHIFT) & MSTATUS_MPP_MASK);
        hart->csrs[MSTATUS] = mstatus;

        // Switch to M-mode
        hart->mode = 3;

        // Compute target PC from mtvec
        uint64_t mtvec = hart->csrs[MTVEC];
        uint64_t mode = mtvec & TVEC_MODE_MASK;
        uint64_t base = mtvec & TVEC_BASE_MASK;
        if (mode == 0) {
            hart->pc = base;
        } else if (mode == 1 && is_interrupt) {
            hart->pc = base + 4 * cause;
        } else {
            hart->pc = base;
        }
    }
}


// Those functions existing just for header files btw
uint64_t h_cpu_csr_read(struct HART *hart, uint64_t addr) {
	return hart->csrs[addr];
}

void h_cpu_csr_write(struct HART *hart, uint64_t addr, uint64_t value) {
	hart->csrs[addr] = value;
}

uint8_t h_cpu_id(struct HART *hart) {
	return hart->id;
}