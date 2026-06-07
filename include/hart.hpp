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

#include "decode.hpp"
#include "defines/csr.hpp"
#include "defines/traps.hpp"
#include "memory_map.hpp"
#include "mmio.hpp"
#include <cstdint>
#include <unordered_map>

enum class PrivilegeMode
{
	User	   = 0,
	Supervisor = 1,
	Hypervisor = 2,
	Machine	   = 3
};

struct MMIO;

struct InstructionCache;

struct Reservation
{
	uint64_t vaddr;
	MemorySize size;
	bool valid;
};

struct Hart
{
	Hart(uint8_t id) : id(id) {

					   };
	uint8_t id;
	uint64_t GPR[32];
	uint64_t csrs[4096];
	InstructionDecoder* idec;
	MemoryMap* mmap;
	MMIO* mmio;

	uint64_t pc;
	PrivilegeMode mode;

	status_t status;
	ie_t ie;
	ip_t ip;
	uint64_t stimecmp = UINT64_MAX;
	bool WFI		  = false;

	// Fetch buffer
	uint32_t fetch_buffer[8];
	uint64_t fetch_buffer_pc;

	Reservation reservation;
	inline void amo_check_reservation(uint64_t va)
	{
		if(reservation.valid && reservation.vaddr >= va && va <= reservation.vaddr + (int)reservation.size)
		{
			reservation.valid = false;
		}
	}

	void init(uint64_t dtb_pos_at_memory, uint64_t entry_pc);
	uint64_t csr_read(uint16_t csr);
	void csr_write(uint16_t csr, uint64_t val);
	void trap(uint64_t cause, uint64_t tval, bool interrupt);
	void tick();
	ExecReturn single_inst(uint32_t inst);
	uint32_t fetch();
	bool int_local_pending();
	bool check_ints();
};
