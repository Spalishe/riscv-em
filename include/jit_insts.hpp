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

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "cpu.h"

extern void jit_RTYPE(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type, bool isW);
extern void jit_RTYPE_SHIFT(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type, bool isW);
extern void jit_RTYPE_SLT(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);
extern void jit_ITYPE(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type, bool isW);
extern void jit_ITYPE_SHIFT(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type, bool isW);
extern void jit_ITYPE_SLT(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);
extern void jit_ITYPE_L(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);
extern void jit_STORE(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);
extern void jit_UTYPE(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);

extern void jit_ZICSR_R(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);
extern void jit_ZICSR_I(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);

extern void jit_MUL(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);
extern void jit_DIV(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);
extern void jit_REM(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type);

extern void jit_LR_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_SC_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOSWAP_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOADD_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOXOR_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOAND_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOOR_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOMIN_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOMAX_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOUMIN_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOUMAX_D(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);

extern void jit_LR_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_SC_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOSWAP_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOADD_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOXOR_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOAND_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOOR_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOMIN_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOMAX_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOUMIN_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);
extern void jit_AMOUMAX_W(HART* hart, CACHE_DecodedOperands* cache, llvm::IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr);