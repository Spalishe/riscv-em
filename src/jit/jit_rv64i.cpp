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

void jit_RTYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type, bool isW) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    auto int32ptr = llvm::Type::getInt32Ty(context);
    if(isW) {
        val1 = builder->CreateSExt(val1,int32ptr);    
        val2 = builder->CreateSExt(val2,int32ptr);    
    }
    llvm::Value* sum = nullptr;
    switch(type) {
        case 0: sum = builder->CreateAdd(val1, val2); break; // ADD
        case 1: sum = builder->CreateSub(val1, val2); break; // SUB
        case 2: sum = builder->CreateXor(val1, val2); break; // XOR
        case 3: sum = builder->CreateOr(val1, val2); break; // OR
        case 4: sum = builder->CreateAnd(val1, val2); break; // AND
    }
    if(isW) {
        sum = builder->CreateSExt(sum,int32ptr);
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));

}
void jit_RTYPE_SHIFT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type, bool isW) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));
    
    auto int32ptr = llvm::Type::getInt32Ty(context);

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    if(isW) {
        val1 = builder->CreateTrunc(val1, int32ptr);
        val2 = builder->CreateTrunc(val2, int32ptr);
        val2 = builder->CreateAnd(val2,(Value*)0x1F);
    } else {
        if(type == 0) {
            val2 = builder->CreateSExt(val2,i64Ty);
        } else if(type == 2) {
            val1 = builder->CreateTrunc(val1,int32ptr);
            val2 = builder->CreateSExt(val2,i64Ty);
        }
    }

    llvm::Value* sum = nullptr;
    switch(type) {
        case 0: sum = builder->CreateShl(val1, val2); break; // SLL
        case 1: sum = builder->CreateLShr(val1, val2); break; // SRL
        case 2: sum = builder->CreateAShr(val1, val2); break; // SRA
    }
    if(isW) {
        if(type == 0 || type == 1) {
            sum = builder->CreateSExt(sum,int32ptr);
        } else {
            sum = builder->CreateSExt(sum,i64Ty);
        }
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));
}
void jit_RTYPE_SLT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);

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

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));
}
void jit_ITYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type, bool isW) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, immPtr);

    llvm::Value* sum = nullptr;
    if(isW) {
        val1 = builder->CreateTrunc(val1, i32Ty);
        val2 = builder->CreateTrunc(val2, i32Ty);
    }
    switch(type) {
        case 0: sum = builder->CreateAdd(val1, val2); break; // ADDI
        case 2: sum = builder->CreateXor(val1, val2); break; // XORI
        case 3: sum = builder->CreateOr(val1, val2); break; // ORI
        case 4: sum = builder->CreateAnd(val1, val2); break; // ANDI
    }
    if(isW) {
        sum = builder->CreateTrunc(sum,i64Ty);
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));
}
void jit_ITYPE_SHIFT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type, bool isW) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, immPtr);

    if(isW) {
        val1 = builder->CreateTrunc(val1, i32Ty);
        val2 = builder->CreateTrunc(val2, i32Ty);
    }

    llvm::Value* sum = nullptr;
    switch(type) {
        case 0: sum = builder->CreateShl(val1, val2); break; // SLLI
        case 1: sum = builder->CreateLShr(val1, val2); break; // SRLI
        case 2: sum = builder->CreateAShr(val1, val2); break; // SRAI
    }
    if(isW) {
        if(type == 0 || type == 1) {
            sum = builder->CreateSExt(sum,i32Ty);
        } else {
            sum = builder->CreateSExt(sum,i64Ty);
        }
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));
}
void jit_ITYPE_SLT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);

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

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));
}
void jit_ITYPE_L(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    auto int8ptr = llvm::Type::getInt8Ty(context);
    auto int16ptr = llvm::Type::getInt16Ty(context);
    auto int32ptr = llvm::Type::getInt32Ty(context);
    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateSExt(builder->CreateLoad(i64Ty, immPtr),i64Ty);
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
        case 0: res = builder->CreateSExt(builder->CreateSExt(value,int8ptr),i64Ty); break; // LB
        case 2: res = builder->CreateSExt(builder->CreateSExt(value,int16ptr),i64Ty); break; // LH
        case 4: res = builder->CreateSExt(builder->CreateSExt(value,int32ptr),i64Ty); break; // LW
        case 6: res = builder->CreateSExt(value,i64Ty); break; // LD
        default: res = builder->CreateZExt(value,i64Ty);
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

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));
}
void jit_STORE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));

    auto int8ptr = llvm::Type::getInt8Ty(context);
    auto int16ptr = llvm::Type::getInt16Ty(context);
    auto int32ptr = llvm::Type::getInt32Ty(context);
    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateSExt(builder->CreateLoad(i64Ty, immPtr),i64Ty);
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

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));
}
void jit_UTYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 2); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, (Value*)hart, 3); // HART->pc

    llvm::Value* immPtr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->imm));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    llvm::Value* val1 = builder->CreateLoad(i64Ty, immPtr);
    auto int32ptr = llvm::Type::getInt32Ty(context);

    llvm::Value* shl = builder->CreateSExt(builder->CreateSExt(builder->CreateShl(val1, (Value*)12),int32ptr),i64Ty);
    llvm::Value* res = nullptr;
    switch(type) {
        case 0: res = shl; break; // LUI
        case 1: res = builder->CreateAdd(shl, pcPtr); break; // AUIPC
    }

    // store in rd
    builder->CreateStore(res, rdPtr);

    builder->CreateStore(pcPtr,builder->CreateAdd(pcPtr,(Value*)4));
}