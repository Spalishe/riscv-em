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

#include "../../include/cpu.hpp"
#include "../../include/decode.h"

// SYSTEM

bool exec_MRET(HART *hart, inst_data& inst) {
    hart->pc = hart->csrs[MEPC] - 4;
    switch(csr_read_mstatus(hart,MSTATUS_MPP_LOW,MSTATUS_MPP_HIGH)) {
        case 0b00:
            // If MPP != M-mode, MRET also sets MPRV=0.
            csr_write_mstatus(hart,MSTATUS_MPRV,MSTATUS_MPRV,0);
            hart->mode = PrivilegeMode::User;
            break;
        case 0b01:
            csr_write_mstatus(hart,MSTATUS_MPRV,MSTATUS_MPRV,0);
            hart->mode = PrivilegeMode::Supervisor;
            break;
        case 0b11:
            hart->mode = PrivilegeMode::Machine;
            break;
    }
    // Read a previous interrupt-enable bit for machine mode (MPIE, 7), and set a global interrupt-enable bit for machine mode (MIE, 3) to it.
    csr_write_mstatus(hart,MSTATUS_MIE,MSTATUS_MIE,csr_read_mstatus(hart,MSTATUS_MPIE,MSTATUS_MPIE));
    csr_write_mstatus(hart,MSTATUS_MPIE,MSTATUS_MPIE,1);
    csr_write_mstatus(hart,MSTATUS_MPP_LOW,MSTATUS_MPP_HIGH,(char)PrivilegeMode::User);
    return true;
}
bool exec_SRET(HART *hart, inst_data& inst) {
    if (hart->mode == PrivilegeMode::User ||
        (hart->mode == PrivilegeMode::Supervisor && (csr_read_mstatus(hart,MSTATUS_TSR,MSTATUS_TSR) & (1ULL << 22)))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION, inst.inst, false);
        return false;
    }
    hart->pc = hart->csrs[SEPC] - 4;
    switch(csr_read_mstatus(hart,MSTATUS_SPP,MSTATUS_SPP)) {
        case 0b0:
            hart->mode = PrivilegeMode::User;
            break;
        case 0b1:
            // If SPP != M-mode, SRET also sets MPRV=0.
            csr_write_mstatus(hart,MSTATUS_MPRV,MSTATUS_MPRV,0);
            hart->mode = PrivilegeMode::Supervisor;
            break;
    }
    // Read a previous interrupt-enable bit for supervisor mode (SPIE,5), and set a global interrupt-enable bit for supervisor mode (SIE, 1) to it.
    csr_write_mstatus(hart,MSTATUS_SIE,MSTATUS_SIE,csr_read_mstatus(hart,MSTATUS_SPIE,MSTATUS_SPIE));
    csr_write_mstatus(hart,MSTATUS_SPIE,MSTATUS_SPIE,1);
    csr_write_mstatus(hart,MSTATUS_SPP,MSTATUS_SPP,(char)PrivilegeMode::User);
    return true;
}