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
#include "../../include/defines/csr.hpp"
#include "../../include/hart.hpp"

// ZiCSR

bool compatible_mode(PrivilegeMode csr_level, PrivilegeMode cur_mode)
{
	if(csr_level == PrivilegeMode::Machine)
	{
		return cur_mode == PrivilegeMode::Machine;
	}
	else if(csr_level == PrivilegeMode::Supervisor)
	{
		return (cur_mode == PrivilegeMode::Machine) || (cur_mode == PrivilegeMode::Supervisor);
	}
	else if(csr_level == PrivilegeMode::User)
	{
		return (cur_mode == PrivilegeMode::Machine) || (cur_mode == PrivilegeMode::Supervisor) || (cur_mode == PrivilegeMode::User);
	}
	return true;
}

bool csr_accessible(uint16_t csr_addr, PrivilegeMode current_priv, bool write)
{
	PrivilegeMode csr_level;

	if(csr_addr >= 0x000 && csr_addr <= 0x0FF)
		csr_level = PrivilegeMode::User; // User
	else if(csr_addr >= 0x100 && csr_addr <= 0x1FF)
		csr_level = PrivilegeMode::Supervisor; // Supervisor
	else if(csr_addr >= 0x200 && csr_addr <= 0x2FF)
		csr_level = PrivilegeMode::Hypervisor; // Hypervisor
	else if(csr_addr >= 0x300 && csr_addr <= 0x3FF)
		csr_level = PrivilegeMode::Machine; // Machine
	else if(csr_addr >= 0xB00 && csr_addr <= 0xBFF)
		csr_level = PrivilegeMode::Machine; // Machine counters
	else if(csr_addr >= 0xC00 && csr_addr <= 0xCFF)
		csr_level = PrivilegeMode::User; // User counters (cycle/time)
	else if(csr_addr >= 0xF00 && csr_addr <= 0xFFF)
		csr_level = PrivilegeMode::Machine; // Machine info
	else
		csr_level = PrivilegeMode::User;

	if(!compatible_mode(csr_level, current_priv))
	{
		return false;
	}

	if(csr_addr == 0xF11
	   || csr_addr == 0xF12
	   || csr_addr == 0xF13
	   || csr_addr == 0xF14
	   || csr_addr == 0xF15
	   || csr_addr == 0xC00
	   || csr_addr == 0xC01
	   || csr_addr == 0xC02
	   || (csr_addr >= 0xC03 && csr_addr <= 0xC1F)
	   || csr_addr == 0xC80
	   || csr_addr == 0xC81
	   || csr_addr == 0xC82
	   || (csr_addr >= 0xC83 && csr_addr <= 0xC9F))
	{
		return !write; // RO
	}

	return true;
}
ExecReturn exec_CSRRW(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, true))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		hart.csr_write(inst.imm, hart.GPR[inst.rs1]);
	}
	if(inst.rd != 0)
	{
		hart.GPR[inst.rd] = init_val;
	}
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CSRRS(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, (inst.rs1 != 0)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		uint64_t mask = hart.GPR[inst.rs1];
		hart.csr_write(inst.imm, init_val | mask);
	}
	hart.GPR[inst.rd] = init_val;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CSRRC(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, (inst.rs1 != 0)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		uint64_t mask = hart.GPR[inst.rs1];
		hart.csr_write(inst.imm, init_val & ~mask);
	}
	hart.GPR[inst.rd] = init_val;
	return { true, false, 4, 0, 0 };
}

ExecReturn exec_CSRRWI(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, inst.rs1 != 0))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	if(inst.rd != 0)
	{
		hart.GPR[inst.rd] = hart.csr_read(inst.imm);
	}
	hart.csr_write(inst.imm, inst.rs1);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CSRRSI(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, (inst.rs1 != 0)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		uint64_t mask = inst.rs1;
		hart.csr_write(inst.imm, init_val | mask);
	}
	hart.GPR[inst.rd] = init_val;
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_CSRRCI(Hart& hart, InstructionData& inst)
{
	bool tvm = hart.status.fields.TVM;
	if(hart.mode == PrivilegeMode::Supervisor && tvm && inst.imm == CSR_SATP)
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}
	if(!csr_accessible(inst.imm, hart.mode, (inst.rs1 != 0)))
	{
		return { false, false, 0, EXC_ILLEGAL_INSTRUCTION, inst.inst };
	}

	uint64_t init_val = hart.csr_read(inst.imm);
	if(inst.rs1 != 0)
	{
		uint64_t mask = inst.rs1;
		hart.csr_write(inst.imm, init_val & ~mask);
	}
	hart.GPR[inst.rd] = init_val;
	return { true, false, 4, 0, 0 };
}

void InstructionDecoder::init_zicsr()
{
	register_instr("*****************001*****1110011", exec_CSRRW);
	register_instr("*****************010*****1110011", exec_CSRRS);
	register_instr("*****************011*****1110011", exec_CSRRC);
	register_instr("*****************101*****1110011", exec_CSRRWI);
	register_instr("*****************110*****1110011", exec_CSRRSI);
	register_instr("*****************111*****1110011", exec_CSRRCI);
}
