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

#include "../../include/devices/aclint.hpp"
#include "../../include/libfdt.hpp"
#include "../../include/machine.hpp"

ACLINT::ACLINT(uint64_t start, uint64_t size, Machine& cpu, fdt_node* fdt)
	: Device(start, size, fdt, cpu.mmap),
	  msip(cpu.harts_count, 0),
	  mtimecmp(cpu.harts_count, 0),
	  cpu(cpu)
{
	cpu.mmap->add_region(start, size);
	if(fdt != NULL)
	{
		struct fdt_node* cpus		  = fdt_node_find(fdt, "cpus");
		std::vector<uint32_t> irq_ext = {};
		for(int i = 0; i < cpu.harts_count; i++)
		{
			struct fdt_node* cpu	 = fdt_node_find_reg(cpus, "cpu", i);
			struct fdt_node* cpu_irq = fdt_node_find(cpu, "interrupt-controller");
			uint32_t irq_phandle	 = fdt_node_get_phandle(cpu_irq);

			if(irq_phandle)
			{
				irq_ext.push_back(irq_phandle);
				irq_ext.push_back(0x3); // RISCV_INTERRUPT_MSOFTWARE
				irq_ext.push_back(irq_phandle);
				irq_ext.push_back(0x7); // RISCV_INTERRUPT_MTIMER
			}
			else
			{
				// missing interrupt controller in hart i
			}
		}

		struct fdt_node* clint_fdt = fdt_node_create_reg("clint", start);
		fdt_node_add_prop_reg(clint_fdt, "reg", start, size);
		fdt_node_add_prop(clint_fdt, "compatible", "sifive,clint0\0riscv,clint0\0", 27);
		fdt_node_add_prop_cells(clint_fdt, "interrupts-extended", irq_ext, irq_ext.size());

		fdt_node_add_child(fdt_node_find(fdt, "soc"), clint_fdt);
	}
}

std::shared_ptr<ACLINT> ACLINT::init_auto(Machine& cpu)
{
	return std::make_shared<ACLINT>(0x02000000, 0x10000, cpu, cpu.fdt);
}

void ACLINT::tick()
{
	mtime += 1;
	update_mip();
}

uint64_t ACLINT::read(uint64_t addr, MemorySize size)
{
	uint64_t off = addr - start;

	if(off < ACLINT_MSWI_SIZE)
	{
		return read_mswi(off);
	}
	else if(off >= ACLINT_MSWI_SIZE && off < ACLINT_MSWI_SIZE + ACLINT_MTIMER_SIZE)
	{
		return read_mtimer(off - ACLINT_MSWI_SIZE);
	}

	return 0;
}

void ACLINT::write(uint64_t addr, MemorySize size, uint64_t value)
{
	uint64_t off = addr - start;

	if(off < ACLINT_MSWI_SIZE)
	{
		write_mswi(off, value);
	}
	else if(off >= ACLINT_MSWI_SIZE && off < ACLINT_MSWI_SIZE + ACLINT_MTIMER_SIZE)
	{
		write_mtimer(off - ACLINT_MSWI_SIZE, value);
	}
}

uint64_t ACLINT::read_mswi(uint64_t offset)
{
	uint64_t hart_id = offset >> 2;
	return msip[hart_id];
}
uint64_t ACLINT::read_mtimer(uint64_t offset)
{
	uint64_t hart_id = offset >> 3;
	if(offset == 0x7FF8)
	{
		// return timer_get(&mtime);
		return mtime;
	}
	// return timecmp_get(&mtimecmp[hart_id]);
	return mtimecmp[hart_id];
}
void ACLINT::write_mswi(uint64_t offset, uint64_t value)
{
	uint64_t hart_id = offset >> 2;
	msip[hart_id]	 = value;
	update_mip();
}
void ACLINT::write_mtimer(uint64_t offset, uint64_t value)
{
	uint64_t hart_id = offset >> 3;
	if(offset == 0x7FF8)
	{
		// timer_set(&mtime,value);
		mtime = value;
		update_mip();
		return;
	}

	// timecmp_set(&mtimecmp[hart_id],value);
	mtimecmp[hart_id] = value;
	update_mip();
}

void ACLINT::update_mip()
{
	for(Hart& hart : cpu.harts)
	{
		hart.csrs[CSR_TIME] = mtime;
		uint32_t hart_id	= hart.id;

		if(msip[hart_id] & 1)
			hart.ip.fields.MSIP = 1;
		else
			hart.ip.fields.MSIP = 0;

		if(mtime >= mtimecmp[hart_id])
			hart.ip.fields.MTIP = 1;
		else
			hart.ip.fields.MTIP = 0;

		if(mtime >= hart.stimecmp && hart.stimecmp != UINT64_MAX)
			hart.ip.fields.STIP = 1;
		else
			hart.ip.fields.STIP = 0;
	}
}
