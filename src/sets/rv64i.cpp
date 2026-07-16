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

#include "../../include/decode.hpp"
#include "../../include/hart.hpp"

// R-Type

ExecReturn exec_ADDW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] + (uint32_t)hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SUBW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] - (uint32_t)hart.GPR[inst.rs2]);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLLW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] << ((uint32_t)hart.GPR[inst.rs2] & 0x1F));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRLW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] >> ((uint32_t)hart.GPR[inst.rs2] & 0x1F));
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRAW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)(((int32_t)hart.GPR[inst.rs1]) >> ((uint32_t)hart.GPR[inst.rs2] & 0x1F));
	return { true, false, 4, 0, 0 };
}

// I-Type
ExecReturn exec_ADDIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] + inst.imm);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLLIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] << inst.imm);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRLIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(int32_t)((uint32_t)hart.GPR[inst.rs1] >> inst.imm);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRAIW(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)(((int32_t)hart.GPR[inst.rs1]) >> inst.imm);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_LD(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint64_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Long, &val);

	if(success.is_success)
	{
		hart.GPR[inst.rd] = val;
	}

	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LWU(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint32_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Int, &val);

	if(success.is_success)
	{
		hart.GPR[inst.rd] = val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}

// S-Type
ExecReturn exec_SD(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	MemoryReturn out = hart.mmio->write(hart, addr, MemorySize::Long, hart.GPR[inst.rs2]);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}

/// RV32I

// R-Type

ExecReturn exec_ADD(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] + hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SUB(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] - hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_XOR(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] ^ hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_OR(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] | hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_AND(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] & hart.GPR[inst.rs2];
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLL(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] << (hart.GPR[inst.rs2] & 0x3f);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRL(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] >> (hart.GPR[inst.rs2] & 0x3f);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRA(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((int64_t)hart.GPR[inst.rs1]) >> (hart.GPR[inst.rs2] & 0x3f);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLT(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((int64_t)hart.GPR[inst.rs1] < (int64_t)hart.GPR[inst.rs2]) ? 1 : 0;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLTU(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (hart.GPR[inst.rs1] < hart.GPR[inst.rs2]) ? 1 : 0;
	return { true, false, 4, 0, 0 };
}

// I-Type
ExecReturn exec_ADDI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_XORI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] ^ inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_ORI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] | inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_ANDI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] & inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLLI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] << inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRLI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.GPR[inst.rs1] >> inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SRAI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((int64_t)hart.GPR[inst.rs1]) >> inst.imm;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLTI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = ((int64_t)hart.GPR[inst.rs1] < (int64_t)inst.imm) ? 1 : 0;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_SLTIU(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (hart.GPR[inst.rs1] < inst.imm) ? 1 : 0;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_LB(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	int8_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Byte, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LH(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	int16_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Short, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LW(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	int32_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Int, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LBU(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint8_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Byte, &val);
	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)(uint8_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}
ExecReturn exec_LHU(Hart& hart, InstructionData& inst)
{
	uint64_t addr = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	uint16_t val;
	MemoryReturn success = hart.mmio->read(hart, addr, MemorySize::Short, &val);

	if(success.is_success)
	{
		hart.GPR[inst.rd] = (uint64_t)val;
	}
	return {
		success.is_success,
		false,
		4,
		success.exc_code,
		success.tval,
	};
}

// S-Type
ExecReturn exec_SB(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	MemoryReturn out = hart.mmio->write(hart, addr, MemorySize::Byte, hart.GPR[inst.rs2]);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}
ExecReturn exec_SH(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	MemoryReturn out = hart.mmio->write(hart, addr, MemorySize::Short, hart.GPR[inst.rs2]);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}
ExecReturn exec_SW(Hart& hart, InstructionData& inst)
{
	uint64_t addr	 = hart.GPR[inst.rs1] + (int64_t)inst.imm;
	MemoryReturn out = hart.mmio->write(hart, addr, MemorySize::Int, hart.GPR[inst.rs2]);
	return {
		out.is_success,
		false,
		4,
		out.exc_code,
		out.tval
	};
}

// B-Type
ExecReturn exec_BEQ(Hart& hart, InstructionData& inst)
{
	if((int64_t)hart.GPR[inst.rs1] == (int64_t)hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BNE(Hart& hart, InstructionData& inst)
{
	if((int64_t)hart.GPR[inst.rs1] != (int64_t)hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BLT(Hart& hart, InstructionData& inst)
{
	if((int64_t)hart.GPR[inst.rs1] < (int64_t)hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
		{
			hart.pc = hart.pc + (int64_t)inst.imm;
		}
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BGE(Hart& hart, InstructionData& inst)
{
	if((int64_t)hart.GPR[inst.rs1] >= (int64_t)hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;
		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BLTU(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[inst.rs1] < hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;

		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_BGEU(Hart& hart, InstructionData& inst)
{
	if(hart.GPR[inst.rs1] >= hart.GPR[inst.rs2])
	{
		if((hart.pc + (int64_t)inst.imm) % 2 != 0)
		{
			return { false, false, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
		}
		else
			hart.pc = hart.pc + (int64_t)inst.imm;

		return { true, true, 0, 0, 0 };
	}
	return { true, false, 4, 0, 0 };
}

// JUMP
ExecReturn exec_JAL(Hart& hart, InstructionData& inst)
{
	uint64_t tmp = hart.pc;
	if((hart.pc + (int64_t)inst.imm) % 2 != 0)
	{
		return { false, true, 0, EXC_INST_ADDR_MISALIGNED, hart.pc + (int64_t)inst.imm };
	}
	else
	{
		hart.pc			  = hart.pc + (int64_t)inst.imm;
		hart.GPR[inst.rd] = tmp + 4;
	}
	return { true, true, 0, 0, 0 };
}
ExecReturn exec_JALR(Hart& hart, InstructionData& inst)
{
	uint64_t tmp	= hart.pc;
	uint64_t target = (hart.GPR[inst.rs1] + (int64_t)inst.imm) & ~1;
	if(target % 2 != 0)
	{
		return { false, true, 0, EXC_INST_ADDR_MISALIGNED, target };
	}
	else
	{
		hart.pc			  = target;
		hart.GPR[inst.rd] = tmp + 4;
	}
	return { true, true, 0, 0, 0 };
}

// what

ExecReturn exec_LUI(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = (int64_t)((uint64_t)inst.imm);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_AUIPC(Hart& hart, InstructionData& inst)
{
	hart.GPR[inst.rd] = hart.pc + (int64_t)((uint64_t)inst.imm);
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_ECALL(Hart& hart, InstructionData& inst)
{

	switch(hart.mode)
	{
		case PrivilegeMode::Machine:
			return { false, true, 0, EXC_ENV_CALL_FROM_M, 0 };
		case PrivilegeMode::Supervisor:
			return { false, true, 0, EXC_ENV_CALL_FROM_S, 0 };
		case PrivilegeMode::User:
			return { false, true, 0, EXC_ENV_CALL_FROM_U, 0 };
		default:
			// How did we get here?
			return { true, false, 4, 0, 0 };
	}
}
ExecReturn exec_EBREAK(Hart& hart, InstructionData& inst)
{
	return { false, true, 0, EXC_BREAKPOINT, hart.pc };
}

ExecReturn exec_FENCE(Hart& hart, InstructionData& inst)
{
	// nop
	// If you planning adding some memory write/read buffer, you have to implement this instruction then
	// FENCE guaranties that all cores will see all changes that have done by 1 specific core before FENCE instruction

	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_rv64i()
{
	// R-Type
	register_instr("0000000**********000*****0110011", exec_ADD);
	register_instr("0000000**********000*****0111011", exec_ADDW);
	register_instr("0100000**********000*****0110011", exec_SUB);
	register_instr("0100000**********000*****0111011", exec_SUBW);
	register_instr("0000000**********100*****0110011", exec_XOR);
	register_instr("0000000**********110*****0110011", exec_OR);
	register_instr("0000000**********111*****0110011", exec_AND);
	register_instr("0000000**********001*****0110011", exec_SLL);
	register_instr("0000000**********001*****0111011", exec_SLLW);
	register_instr("0000000**********101*****0110011", exec_SRL);
	register_instr("0000000**********101*****0111011", exec_SRLW);
	register_instr("0100000**********101*****0110011", exec_SRA);
	register_instr("0100000**********101*****0111011", exec_SRAW);
	register_instr("0000000**********010*****0110011", exec_SLT);
	register_instr("0000000**********011*****0110011", exec_SLTU);

	// I-Type
	register_instr("*****************000*****0010011", exec_ADDI, imm_I);
	register_instr("*****************000*****0011011", exec_ADDIW, imm_I);
	register_instr("*****************100*****0010011", exec_XORI, imm_I);
	register_instr("*****************110*****0010011", exec_ORI, imm_I);
	register_instr("*****************111*****0010011", exec_ANDI, imm_I);
	register_instr("000000***********001*****0010011", exec_SLLI, shamt64);
	register_instr("0000000**********001*****0011011", exec_SLLIW, shamt);
	register_instr("0000000**********101*****0011011", exec_SRLIW, shamt);
	register_instr("0100000**********101*****0011011", exec_SRAIW, shamt);
	register_instr("000000***********101*****0010011", exec_SRLI, shamt64);
	register_instr("010000***********101*****0010011", exec_SRAI, shamt64);
	register_instr("*****************010*****0010011", exec_SLTI, imm_I);
	register_instr("*****************011*****0010011", exec_SLTIU, imm_I);
	register_instr("*****************000*****0000011", exec_LB, imm_I);
	register_instr("*****************001*****0000011", exec_LH, imm_I);
	register_instr("*****************010*****0000011", exec_LW, imm_I);
	register_instr("*****************011*****0000011", exec_LD, imm_I);
	register_instr("*****************100*****0000011", exec_LBU, imm_I);
	register_instr("*****************101*****0000011", exec_LHU, imm_I);
	register_instr("*****************110*****0000011", exec_LWU, imm_I);

	// S-Type
	register_instr("*****************000*****0100011", exec_SB, imm_S);
	register_instr("*****************001*****0100011", exec_SH, imm_S);
	register_instr("*****************010*****0100011", exec_SW, imm_S);
	register_instr("*****************011*****0100011", exec_SD, imm_S);

	// B-Type
	register_instr("*****************000*****1100011", exec_BEQ, imm_B);
	register_instr("*****************001*****1100011", exec_BNE, imm_B);
	register_instr("*****************100*****1100011", exec_BLT, imm_B);
	register_instr("*****************101*****1100011", exec_BGE, imm_B);
	register_instr("*****************110*****1100011", exec_BLTU, imm_B);
	register_instr("*****************111*****1100011", exec_BGEU, imm_B);

	// what
	register_instr("*************************1101111", exec_JAL, imm_J);
	register_instr("*****************000*****1100111", exec_JALR, imm_I);
	register_instr("*************************0110111", exec_LUI, imm_U);
	register_instr("*************************0010111", exec_AUIPC, imm_U);

	register_instr("00000000000000000000000001110011", exec_ECALL);
	register_instr("00000000000100000000000001110011", exec_EBREAK);

	register_instr("0000********00000000000000001111", exec_FENCE, imm_I);
}
#ifdef USE_JIT
#include "../../include/rvjit/rvjit_x86_64.hpp"
void execjit_ADD(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg && rd.vreg != rs2.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			add_rr(blk, rd.host_reg, rs2.host_reg);
		}
		else if(rd.vreg == rs1.vreg)
		{
			add_rr(blk, rd.host_reg, rs2.host_reg);
		}
		else
		{
			add_rr(blk, rd.host_reg, rs1.host_reg);
		}
	}, blk.pc + blk.size);
}
void execjit_ADDW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg && rd.vreg != rs2.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			add_rr32(blk, rd.host_reg, rs2.host_reg);
		}
		else if(rd.vreg == rs1.vreg)
		{
			add_rr32(blk, rd.host_reg, rs2.host_reg);
		}
		else
		{
			add_rr32(blk, rd.host_reg, rs1.host_reg);
		}

		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SUB(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg && rd.vreg != rs2.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			sub_rr(blk, rd.host_reg, rs2.host_reg);
		}
		else if(rd.vreg == rs1.vreg)
		{
			sub_rr(blk, rd.host_reg, rs2.host_reg);
		}
		else
		{
			mov(blk, REG_RCX, rs2.host_reg);
			mov(blk, rd.host_reg, rs1.host_reg);
			sub_rr(blk, rd.host_reg, REG_RCX);
		}
	}, blk.pc + blk.size);
}
void execjit_SUBW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg && rd.vreg != rs2.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
			sub_rr32(blk, rd.host_reg, rs2.host_reg);
		}
		else if(rd.vreg == rs1.vreg)
		{
			sub_rr32(blk, rd.host_reg, rs2.host_reg);
		}
		else
		{
			mov(blk, REG_RCX, rs2.host_reg);
			mov(blk, rd.host_reg, rs1.host_reg);
			sub_rr32(blk, rd.host_reg, REG_RCX);
		}
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_XOR(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			xor_rr(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		xor_rr(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
}
void execjit_OR(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			or_rr(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		or_rr(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
}
void execjit_AND(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rs1.is_zero || rs2.is_zero)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			and_rr(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		and_rr(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SLL(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			shl_rc(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shl_rc(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SLLW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			shl_rc32(blk, rd.host_reg, REG_RCX);
			movsxd(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shl_rc32(blk, rd.host_reg, rs2.host_reg);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SRL(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			shr_rc(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shr_rc(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SRLW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			shr_rc32(blk, rd.host_reg, REG_RCX);
			movsxd(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shr_rc32(blk, rd.host_reg, rs2.host_reg);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SRA(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			sar_rc(blk, rd.host_reg, REG_RCX);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		sar_rc(blk, rd.host_reg, rs2.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SRAW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		if(rd.vreg == rs2.vreg)
		{
			mov(blk, REG_RCX, rs2.host_reg);
			if(rd.vreg != rs1.vreg) mov(blk, rd.host_reg, rs1.host_reg);
			sar_rc32(blk, rd.host_reg, REG_RCX);
			movsxd(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		sar_rc32(blk, rd.host_reg, rs2.host_reg);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SLT(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, false,
							 [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		cmp(blk, rs1.host_reg, rs2.host_reg);
		setl(blk, rd.host_reg);
		movzx(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SLTU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_r_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, VReg& rs2, uint64_t pc, void* tmp)
	{
		cmp(blk, rs1.host_reg, rs2.host_reg);
		setb(blk, rd.host_reg);
		movzx(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}

void execjit_ADDI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}
		add_rimm32(blk, rd.host_reg, imm);
	}, blk.pc + blk.size);
}
void execjit_ADDIW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		add_r32imm32(blk, rd.host_reg, imm);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_XORI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}
		xor_rimm32(blk, rd.host_reg, imm);
	}, blk.pc + blk.size);
}
void execjit_ORI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		or_rimm32(blk, rd.host_reg, imm);
	}, blk.pc + blk.size);
}
void execjit_ANDI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rs1.is_zero || imm == 0)
		{
			xor_rr(blk, rd.host_reg, rd.host_reg);
			return;
		}
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		and_rimm32(blk, rd.host_reg, imm);
	}, blk.pc + blk.size);
}
void execjit_SLLI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shl_rimm8(blk, rd.host_reg, imm & 0x3F);
	}, blk.pc + blk.size);
}
void execjit_SLLIW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shl_r32imm8(blk, rd.host_reg, imm & 0x1F);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SRLI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shr_rimm8(blk, rd.host_reg, imm & 0x3F);
	}, blk.pc + blk.size);
}
void execjit_SRLIW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		shr_r32imm8(blk, rd.host_reg, imm & 0x1F);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}
void execjit_SRAI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		sar_rimm8(blk, rd.host_reg, imm & 0x3F);
	}, blk.pc + blk.size);
}
void execjit_SRAIW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, true, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		if(rd.vreg != rs1.vreg)
		{
			mov(blk, rd.host_reg, rs1.host_reg);
		}

		sar_r32imm8(blk, rd.host_reg, imm & 0x1F);
		movsxd(blk, rd.host_reg, rd.host_reg);
	}, blk.pc + blk.size);
}

void jit_load(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter, void* func)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		mov(blk, REG_RCX, rs1.host_reg);
		add_rimm32(blk, REG_RCX, imm);
		sub_rimm32(blk, REG_RCX, 0x40000000); //
		sub_rimm32(blk, REG_RCX, 0x40000000); // This does sum of 0x80000000, which is beyond the int32_t limit
		cmp_rm(blk, REG_RCX, REG_R12, NO_INDEX, 0, 24);

		blk.jmp_labels.push_back({ "fast_path", blk.byte_pos, false, 1 });
		jbe8(blk, 0);

		{
			// Slow path, make interpreter work instead
			mov_imm64(blk, REG_RCX, pc);
			mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, 32);
			blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
			jmp32(blk, 0);
		}

		em.realize_label(blk, "fast_path");

		auto function_ptr = reinterpret_cast<MovSignature>(tmp);
		function_ptr(blk, rd.host_reg, REG_R14, REG_RCX, 0, 0);
	}, blk.pc + blk.size, func);
}
void execjit_LB(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movsx_r64m8));
}
void execjit_LBU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movzx_r32m8));
}
void execjit_LH(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movsx_r64m16));
}
void execjit_LHU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movzx_r32m16));
}
void execjit_LW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&movsxd_r64m32));
}
void execjit_LWU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_r32m));
}
void execjit_LD(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_rm));
}
void jit_store(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter, void* func)
{
	emitter.inst_emit_s_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rs1, VReg& rs2, uint64_t imm, uint64_t pc, void* tmp)
	{
		mov(blk, REG_RCX, rs1.host_reg);
		add_rimm32(blk, REG_RCX, imm);
		sub_rimm32(blk, REG_RCX, 0x40000000); //
		sub_rimm32(blk, REG_RCX, 0x40000000); // This does sum of 0x80000000, which is beyond the int32_t limit
		cmp_rm(blk, REG_RCX, REG_R12, NO_INDEX, 0, 24);

		blk.jmp_labels.push_back({ "fast_path", blk.byte_pos, false, 1 });
		jbe8(blk, 0);

		{
			// Slow path, make interpreter work instead
			mov_imm64(blk, REG_RCX, pc);
			mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, 32);
			blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
			jmp32(blk, 0);
		}

		em.realize_label(blk, "fast_path");

		auto function_ptr = reinterpret_cast<MovSignature>(tmp);
		function_ptr(blk, rs2.host_reg, REG_R14, REG_RCX, 0, 0);
	}, blk.pc + blk.size, func);
}
void execjit_SB(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_m8r8));
}
void execjit_SH(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_m16r16));
}
void execjit_SW(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_m32r32));
}
void execjit_SD(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_load(hart, inst, blk, emitter, reinterpret_cast<void*>(&mov_mr));
}

void jit_branch(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter, void* func)
{
	emitter.inst_emit_b_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rs1, VReg& rs2, uint64_t imm, uint64_t pc, void* tmp)
	{
		cmp(blk, rs1.host_reg, rs2.host_reg);

		blk.jmp_labels.push_back({ "taken",
								   blk.byte_pos,
								   false,
								   1 });

		auto function_ptr = reinterpret_cast<Jmp8Signature>(tmp);
		function_ptr(blk, 0);

		// not taken
		blk.jmp_labels.push_back({ "end",
								   blk.byte_pos,
								   false,
								   1 });
		jmp8(blk, 0);

		// taken handler
		em.realize_label(blk, "taken");
		blk.jmp_labels.push_back({ "branch",
								   blk.byte_pos,
								   false,
								   4,
								   (int64_t)blk.size + (int64_t)imm });
		jmp32(blk, 0);
		{
			// Slow path, make interpreter work instead
			mov_imm64(blk, REG_RCX, pc);
			mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, 32);
			blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
			jmp32(blk, 0);
		}

		em.realize_label(blk, "end");
	}, blk.pc + blk.size, func);
}

void execjit_BEQ(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_branch(hart, inst, blk, emitter, reinterpret_cast<void*>(&je8));
}
void execjit_BNE(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_branch(hart, inst, blk, emitter, reinterpret_cast<void*>(&jne8));
}
void execjit_BLT(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_branch(hart, inst, blk, emitter, reinterpret_cast<void*>(&jl8));
}
void execjit_BGE(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_branch(hart, inst, blk, emitter, reinterpret_cast<void*>(&jge8));
}
void execjit_BLTU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_branch(hart, inst, blk, emitter, reinterpret_cast<void*>(&jb8));
}
void execjit_BGEU(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	jit_branch(hart, inst, blk, emitter, reinterpret_cast<void*>(&jae8));
}
void execjit_JAL(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_j_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, uint64_t imm, uint64_t pc, void* tmp)
	{
		// Move PC+4 to RD
		mov_imm64(blk, rd.host_reg, imm);
		add_r64imm8(blk, rd.host_reg, 4);
		blk.jmp_labels.push_back({ "branch",
								   blk.byte_pos,
								   false,
								   4,
								   (int64_t)blk.size + (int64_t)imm });
		jmp32(blk, 0);
		{
			// Slow path, make interpreter work instead
			mov_imm64(blk, REG_RCX, pc);
			mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, 32);
			blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
			jmp32(blk, 0);
		}
	}, blk.pc + blk.size);
}
void execjit_JALR(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_i_type(hart, inst, blk, false, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, VReg& rs1, uint64_t imm, uint64_t pc, void* tmp)
	{
		// Move PC+4 to RD
		mov_imm64(blk, rd.host_reg, imm);
		add_r64imm8(blk, rd.host_reg, 4);

		{
			// Slow path, make interpreter work instead
			mov_imm64(blk, REG_RCX, pc);
			mov_mr(blk, REG_RCX, REG_R12, NO_INDEX, 0, 32);
			blk.jmp_labels.push_back({ "epilogue", blk.byte_pos, false });
			jmp32(blk, 0);
		}
	}, blk.pc + blk.size);
}
void execjit_LUI(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_u_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, uint64_t imm, uint64_t pc, void* tmp)
	{
		// RD = IMM << 12
		mov_imm64(blk, rd.host_reg, imm);
		shl_rimm8(blk, rd.host_reg, 12);
	}, blk.pc + blk.size);
}
void execjit_AUIPC(Hart& hart, InstructionData& inst, JIT_Block& blk, JIT_Emitter& emitter)
{
	emitter.inst_emit_u_type(hart, inst, blk, [](JIT_Emitter& em, JIT_Block& blk, VReg& rd, uint64_t imm, uint64_t pc, void* tmp)
	{
		// RD = IMM << 12
		mov_imm64(blk, rd.host_reg, imm);
		shl_rimm8(blk, rd.host_reg, 12);
		mov_imm64(blk, REG_RCX, pc);
		add_rr(blk, rd.host_reg, REG_RCX);
	}, blk.pc + blk.size);
}

void JIT_InstructionDecoder::init_rv64i()
{
	conversion_tbl[&exec_ADD]	= &execjit_ADD;
	conversion_tbl[&exec_ADDW]	= &execjit_ADDW;
	conversion_tbl[&exec_SUB]	= &execjit_SUB;
	conversion_tbl[&exec_SUBW]	= &execjit_SUBW;
	conversion_tbl[&exec_XOR]	= &execjit_XOR;
	conversion_tbl[&exec_OR]	= &execjit_OR;
	conversion_tbl[&exec_AND]	= &execjit_AND;
	conversion_tbl[&exec_SLL]	= &execjit_SLL;
	conversion_tbl[&exec_SLLW]	= &execjit_SLLW;
	conversion_tbl[&exec_SRL]	= &execjit_SRL;
	conversion_tbl[&exec_SRLW]	= &execjit_SRLW;
	conversion_tbl[&exec_SRA]	= &execjit_SRA;
	conversion_tbl[&exec_SRAW]	= &execjit_SRAW;
	conversion_tbl[&exec_SLT]	= &execjit_SLT;
	conversion_tbl[&exec_SLTU]	= &execjit_SLTU;
	conversion_tbl[&exec_ADDI]	= &execjit_ADDI;
	conversion_tbl[&exec_ADDIW] = &execjit_ADDIW;
	conversion_tbl[&exec_XORI]	= &execjit_XORI;
	conversion_tbl[&exec_ORI]	= &execjit_ORI;
	conversion_tbl[&exec_ANDI]	= &execjit_ANDI;
	conversion_tbl[&exec_SLLI]	= &execjit_SLLI;
	conversion_tbl[&exec_SLLIW] = &execjit_SLLIW;
	conversion_tbl[&exec_SRLI]	= &execjit_SRLI;
	conversion_tbl[&exec_SRLIW] = &execjit_SRLIW;
	conversion_tbl[&exec_SRAI]	= &execjit_SRAI;
	conversion_tbl[&exec_SRAIW] = &execjit_SRAIW;
	conversion_tbl[&exec_LB]	= &execjit_LB;
	conversion_tbl[&exec_LBU]	= &execjit_LBU;
	conversion_tbl[&exec_LH]	= &execjit_LH;
	conversion_tbl[&exec_LHU]	= &execjit_LHU;
	conversion_tbl[&exec_LW]	= &execjit_LW;
	conversion_tbl[&exec_LWU]	= &execjit_LWU;
	conversion_tbl[&exec_LD]	= &execjit_LD;
	conversion_tbl[&exec_SB]	= &execjit_SB;
	conversion_tbl[&exec_SH]	= &execjit_SH;
	conversion_tbl[&exec_SW]	= &execjit_SW;
	conversion_tbl[&exec_SD]	= &execjit_SD;
	conversion_tbl[&exec_BEQ]	= &execjit_BEQ;
	conversion_tbl[&exec_BNE]	= &execjit_BNE;
	conversion_tbl[&exec_BLT]	= &execjit_BLT;
	conversion_tbl[&exec_BGE]	= &execjit_BGE;
	conversion_tbl[&exec_BLTU]	= &execjit_BLTU;
	conversion_tbl[&exec_BGEU]	= &execjit_BGEU;
	conversion_tbl[&exec_JAL]	= &execjit_JAL;
	conversion_tbl[&exec_JALR]	= &execjit_JALR;
	conversion_tbl[&exec_LUI]	= &execjit_LUI;
	conversion_tbl[&exec_AUIPC] = &execjit_AUIPC;
}
#endif
