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

#ifdef USE_FPU
#include "../../include/decode.hpp"
#include "../../include/hart.hpp"
#include <cfenv>
#include <cmath>

/*
 *   FOR FUTURE:
 *       In RV64D run sequence of each instruction must be changed:
 *           Firstly runs rm_func(without val)
 *           Then FLOAT_START
 *           then operation(without any f32_in)
 *           then FLOAT_END
 *           and finally write without any f32_out
 *
 */
inline double rm_func(double val, uint8_t rm, Hart& hart, bool& is_error)
{
	uint8_t cmp = 0;
	if(rm == 0b111) [[likely]]
		cmp = hart.fcsr.fields.frm;
	else
	{
		cmp = rm;
	}
	if(cmp == 0b101 || cmp == 0b110)
	{
		// Reserved
		is_error = true;
		return val;
	}
	if(cmp == 0b100)
	{
		return std::round(val);
	}
	else
	{
		switch(cmp)
		{
			case 0b000:
				std::fesetround(FE_TONEAREST);
				break; // RNE
			case 0b001:
				std::fesetround(FE_TOWARDZERO);
				break; // RTZ
			case 0b010:
				std::fesetround(FE_DOWNWARD);
				break; // RDN
			case 0b011:
				std::fesetround(FE_UPWARD);
				break; // RUP
		}
		return val;
	}
	return val;
}
inline float f32_in(double reg)
{
	uint32_t bits = static_cast<uint32_t>(std::bit_cast<uint64_t>(reg));
	return std::bit_cast<float>(bits);
}

inline double f32_out(float val)
{
	uint64_t bits = 0xFFFFFFFF00000000ULL | std::bit_cast<uint32_t>(val);
	return std::bit_cast<double>(bits);
}
#define RM_HANDLE(val, inst, rm, hart)                                    \
	{                                                                     \
		do                                                                \
		{                                                                 \
			bool error_flag = false;                                      \
			double inter	= rm_func((val), (rm), (hart), (error_flag)); \
			if(error_flag)                                                \
			{                                                             \
				return { false, false, 4, 2, inst };                      \
			}                                                             \
			(val) = static_cast<float>(inter);                            \
		} while(0);                                                       \
	}

#define FLOAT_START std::feclearexcept(FE_ALL_EXCEPT);
#define FLOAT_END(fflags)                                        \
	{                                                            \
		do                                                       \
		{                                                        \
			int host_except = std::fetestexcept(FE_ALL_EXCEPT);  \
			if(host_except & FE_INVALID) (fflags) |= (1 << 4);   \
			if(host_except & FE_DIVBYZERO) (fflags) |= (1 << 3); \
			if(host_except & FE_OVERFLOW) (fflags) |= (1 << 2);  \
			if(host_except & FE_UNDERFLOW) (fflags) |= (1 << 1); \
			if(host_except & FE_INEXACT) (fflags) |= (1 << 0);   \
		} while(0);                                              \
	}

ExecReturn exec_FMADD_S(Hart& hart, InstructionData& inst)
{
	double res = (double)f32_in(hart.FPR[inst.rs1] * hart.FPR[inst.rs2] + hart.FPR[inst.rs3]);
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(res);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FMSUB_S(Hart& hart, InstructionData& inst)
{
	double res = (double)f32_in(hart.FPR[inst.rs1] * hart.FPR[inst.rs2] - hart.FPR[inst.rs3]);
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(res);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FNMADD_S(Hart& hart, InstructionData& inst)
{
	double res = (double)f32_in(-hart.FPR[inst.rs1] * hart.FPR[inst.rs2] - hart.FPR[inst.rs3]);
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(res);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FNMSUB_S(Hart& hart, InstructionData& inst)
{
	double res = (double)f32_in(-hart.FPR[inst.rs1] * hart.FPR[inst.rs2] + hart.FPR[inst.rs3]);
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(res);
	return { true, false, 4, 0, 0 };
}
ExecReturn exec_FADD_S(Hart& hart, InstructionData& inst)
{
	double res = (double)f32_in(hart.FPR[inst.rs1] + hart.FPR[inst.rs2]);
	FLOAT_START
	RM_HANDLE(res, inst.inst, inst.rm, hart);
	FLOAT_END(hart.fcsr.fields.fflags);
	hart.FPR[inst.rd] = f32_out(res);
	return { true, false, 4, 0, 0 };
}
void InstructionDecoder::init_rv64f()
{
	register_instr("*****00******************1000011", exec_FMADD_S);
	register_instr("*****00******************1000111", exec_FMSUB_S);
	register_instr("*****00******************1001111", exec_FNMADD_S);
	register_instr("*****00******************1001011", exec_FNMSUB_S);
	register_instr("0000000******************1010011", exec_FADD_S);
	register_instr("0000100******************1010011", exec_FSUB_S);
	register_instr("0001000******************1010011", exec_FMUL_S);
	register_instr("0001100******************1010011", exec_FDIV_S);
	register_instr("010110000000*************1010011", exec_FSQRT_S);
	register_instr("0010000**********000*****1010011", exec_FSGNJ_S);
	register_instr("0010000**********001*****1010011", exec_FSGNJN_S);
	register_instr("0010000**********010*****1010011", exec_FSGNJX_S);
	register_instr("0010100**********000*****1010011", exec_FMIN_S);
	register_instr("0010100**********001*****1010011", exec_FMAX_S);
	register_instr("110000000000*************1010011", exec_FCVT_W_S);
	register_instr("110000000001*************1010011", exec_FCVT_WU_S);
	register_instr("110100000000*************1010011", exec_FCVT_S_W);
	register_instr("110100000001*************1010011", exec_FCVT_S_WU);
	register_instr("111000000000*****000*****1010011", exec_FMV_X_W);
	register_instr("111100000000*****000*****1010011", exec_FMV_W_X);
	register_instr("1010000**********010*****1010011", exec_FEQ_S);
	register_instr("1010000**********001*****1010011", exec_FLT_S);
	register_instr("1010000**********000*****1010011", exec_FLE_S);
	register_instr("111000000000*****001*****1010011", exec_FCLASS_S);
}
#endif
