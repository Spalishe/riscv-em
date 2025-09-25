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

#include "../../include/cpu.h"
#include "../../include/jit.hpp"
#include "../../include/jit_h.hpp"

using namespace llvm;
using namespace llvm::orc;

void jit_RTYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::Value* sum = nullptr;
    switch(type) {
        case 0: sum = builder->CreateAdd(val1, val2); break; // ADD
        case 1: sum = builder->CreateSub(val1, val2); break; // SUB
        case 2: sum = builder->CreateXor(val1, val2); break; // XOR
        case 3: sum = builder->CreateOr(val1, val2); break; // OR
        case 4: sum = builder->CreateAnd(val1, val2); break; // AND
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_RTYPE_SHIFT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::Value* sum = nullptr;
    switch(type) {
        case 0: sum = builder->CreateShl(val1, val2); break; // SLL
        case 1: sum = builder->CreateLShr(val1, val2); break; // SRL
        case 2: sum = builder->CreateAShr(val1, val2); break; // SRA
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_RTYPE_SLT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::Value* cmp = nullptr;
    switch(type) {
        case 0: cmp = builder->CreateICmpSLT(val1, val2); break; // SLT
        case 1: cmp = builder->CreateICmpULT(val1, val2); break; // SLTU
    }

    // extend to i64
    llvm::Value* res = builder->CreateZExt(cmp, i64Ty); 

    // store in rd
    builder->CreateStore(res, rdPtr);
}
void jit_ITYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, immPtr);

    llvm::Value* sum = nullptr;
    switch(type) {
        case 0: sum = builder->CreateAdd(val1, val2); break; // ADDI
        case 2: sum = builder->CreateXor(val1, val2); break; // XORI
        case 3: sum = builder->CreateOr(val1, val2); break; // ORI
        case 4: sum = builder->CreateAnd(val1, val2); break; // ANDI
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_ITYPE_SHIFT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, immPtr);

    llvm::Value* sum = nullptr;
    switch(type) {
        case 0: sum = builder->CreateShl(val1, val2); break; // SLLI
        case 1: sum = builder->CreateLShr(val1, val2); break; // SRLI
        case 2: sum = builder->CreateAShr(val1, val2); break; // SRAI
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_ITYPE_SLT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, immPtr);

    llvm::Value* cmp = nullptr;
    switch(type) {
        case 0: cmp = builder->CreateICmpSLT(val1, val2); break; // SLTI
        case 1: cmp = builder->CreateICmpULT(val1, val2); break; // SLTIU
    }

    // extend to i64
    llvm::Value* res = builder->CreateZExt(cmp, i64Ty); 

    // store in rd
    builder->CreateStore(res, rdPtr);
}
void jit_ITYPE_L(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    auto int8ptr = llvm::Type::getInt8Ty(context);
    auto int16ptr = llvm::Type::getInt16Ty(context);
    auto int32ptr = llvm::Type::getInt32Ty(context);
    auto int64ptr = llvm::Type::getInt64Ty(context);
    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateSExt(builder->CreateLoad(i64Ty, immPtr),int64ptr);
    llvm::Value* sum = builder->CreateAdd(val1, val2);

    llvm::Value* res = nullptr;
    llvm::Value* size = (Value*)8;
    if(type == 2 || type == 3) size = (Value*)16;
    if(type == 4 || type == 5) size = (Value*)32;
    if(type == 6) size = (Value*)64;
    llvm::Value* loadst = builder->CreateCall(loadFunc, {(Value*)hart, sum, size});

    llvm::Value* hasValuePtr = builder->CreateStructGEP(optStructTy, loadst, 0);
    llvm::Value* hasValue = builder->CreateLoad(builder->getInt1Ty(), hasValuePtr);

    llvm::Value* valuePtr = builder->CreateStructGEP(optStructTy, loadst, 1);
    llvm::Value* value = builder->CreateLoad(builder->getInt64Ty(), valuePtr);

    switch(type) {
        case 0: res = builder->CreateSExt(builder->CreateSExt(value,int8ptr),int64ptr); break; // LB
        case 2: res = builder->CreateSExt(builder->CreateSExt(value,int16ptr),int64ptr); break; // LH
        case 4: res = builder->CreateSExt(builder->CreateSExt(value,int32ptr),int64ptr); break; // LW
        case 6: res = builder->CreateSExt(value,int64ptr); break; // LD
        default: res = value;
    }

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(context, "has_value", currentFunc);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(context, "no_value", currentFunc);
    llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "cont", currentFunc);

    builder->CreateCondBr(hasValue, thenBB, elseBB);

    // then
    builder->SetInsertPoint(thenBB);
    // continue
    builder->CreateBr(contBB);

    // else
    builder->SetInsertPoint(elseBB);
    builder->CreateRetVoid();

    // continue
    builder->SetInsertPoint(contBB);
    // store in rd
    builder->CreateStore(res, rdPtr);
}
void jit_STORE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));

    auto int8ptr = llvm::Type::getInt8Ty(context);
    auto int16ptr = llvm::Type::getInt16Ty(context);
    auto int32ptr = llvm::Type::getInt32Ty(context);
    auto int64ptr = llvm::Type::getInt64Ty(context);
    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateSExt(builder->CreateLoad(i64Ty, immPtr),int64ptr);
    llvm::Value* val3 = builder->CreateLoad(i64Ty, rs2Ptr);
    llvm::Value* sum = builder->CreateAdd(val1, val2);

    llvm::Value* res = nullptr;
    switch(type) {
        case 0: res = builder->CreateCall(storeFunc, {(Value*)hart, sum, (Value*)8,val3}); break; // SB
        case 1: res = builder->CreateCall(storeFunc, {(Value*)hart, sum, (Value*)16,val3}); break; // SH
        case 2: res = builder->CreateCall(storeFunc, {(Value*)hart, sum, (Value*)32,val3}); break; // SW
        case 3: res = builder->CreateCall(storeFunc, {(Value*)hart, sum, (Value*)64,val3}); break; // SD
    }

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(context, "has_value", currentFunc);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(context, "no_value", currentFunc);
    llvm::BasicBlock* contBB = llvm::BasicBlock::Create(context, "cont", currentFunc);

    builder->CreateCondBr(res, thenBB, elseBB);

    // then
    builder->SetInsertPoint(thenBB);
    // continue
    builder->CreateBr(contBB);

    // else
    builder->SetInsertPoint(elseBB);
    builder->CreateRetVoid();

    // continue
    builder->SetInsertPoint(contBB);
}
void jit_UTYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(builder->getContext());

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    llvm::Value* val1 = builder->CreateLoad(i64Ty, immPtr);
    auto int32ptr = llvm::Type::getInt32Ty(context);
    auto int64ptr = llvm::Type::getInt64Ty(context);

    llvm::Value* shl = builder->CreateSExt(builder->CreateSExt(builder->CreateShl(val1, (Value*)12),int32ptr),int64ptr);
    llvm::Value* res = nullptr;
    switch(type) {
        case 0: res = shl; break; // LUI
        case 1: res = builder->CreateAdd(shl, pcPtr); break; // AUIPC
    }

    // store in rd
    builder->CreateStore(res, rdPtr);
}