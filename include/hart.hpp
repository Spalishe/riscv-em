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

#include "defines/csr.hpp"
#include <cstdint>

enum class PrivilegeMode
{
	User	   = 0,
	Supervisor = 1,
	Hypervisor = 2,
	Machine	   = 3
};

struct Hart
{
	Hart(uint8_t id) : id(id) {

					   };
	uint8_t id;
	uint64_t GPR[32];
	uint64_t csrs[4096];

	uint64_t pc;
	PrivilegeMode mode;

	status_t status;
	ie_t ie;
	ip_t ip;

	bool WFI = false;

	void init();
	uint64_t csr_read(uint16_t csr);
	void csr_write(uint16_t csr, uint64_t val);
	void trap(uint64_t cause, uint64_t tval, bool interrupt);
	void tick();
	bool int_local_pending();
	void check_ints();
};
