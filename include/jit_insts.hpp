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