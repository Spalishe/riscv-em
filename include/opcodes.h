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

#pragma once

#define R_TYPE  0x33
    #define ADDSUB  0x0
	#define ADD	0x00
        #define SUB     0x20
    #define XOR     0x4
    #define OR      0x6
    #define AND     0x7
    #define SLL     0x1
    #define SR      0x5
	#define SRL	0x00
	#define SRA     0x20
    #define SLT     0x2
    #define SLTU    0x3
#define R_TYPE64 0x3B
#define I_TYPE  0x13
    #define ADDI    0x0
    #define SLLI    0x1
    #define SLTI    0x2
    #define SLTIU   0x3
    #define XORI    0x4
    #define SRI     0x5
        #define SRLI    0x00
        #define SRAI    0x10
        #define SRAIW    0x20
    #define ORI     0x6
    #define ANDI    0x7
#define I_TYPE64  0x1B
#define LOAD_TYPE 0x3
	#define LB		0x0
	#define LH		0x1
	#define LW		0x2
	#define LD		0x3
	#define LBU		0x4
	#define LHU		0x5
	#define LWU		0x6
#define S_TYPE    0x23
	#define SB		0x0
	#define SH		0x1
	#define SW		0x2
	#define SD		0x3
#define B_TYPE    0x63
	#define BEQ		0x0
	#define BNE		0x1
	#define BLT     0x4
	#define BGE     0x5
	#define BLTU    0x6
	#define BGEU    0x7
#define JAL 	  0x6F
#define JALR 	  0x67
#define LUI       0x37
#define AUIPC     0x17
#define ECALL     0x73
    #define CSRRW 0x1
    #define CSRRS 0x2
    #define CSRRC 0x3
    #define CSRRWI 0x5
    #define CSRRSI 0x6
    #define CSRRCI 0x7

#define AMO       0x2F
    #define AMO_W 0x2
    #define AMO_D 0x3

    #define AMOADD    0x0
    #define AMOSWAP    0x1
    #define LR    0x2
    #define SC    0x3
    #define AMOXOR    0x4
    #define AMOOR     0x8
    #define AMOAND    0xC
    #define AMOMIN    0x10
    #define AMOMAX    0x14
    #define AMOMINU   0x18
    #define AMOMAXU   0x1C

#define FENCE     0xF