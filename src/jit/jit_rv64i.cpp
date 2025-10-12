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
#include "../../include/jit_insts.hpp"
#include "../../include/csr.h"

using namespace llvm;
using namespace llvm::orc;

void jit_RTYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type, bool isW) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    auto int32ptr = llvm::Type::getInt32Ty(*context);
    if(isW) {
        val1 = builder->CreateTrunc(val1, int32ptr);
        val1 = builder->CreateSExt(val1, i64Ty);
        val2 = builder->CreateTrunc(val2, int32ptr);
        val2 = builder->CreateSExt(val2, i64Ty);
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
        sum = builder->CreateTrunc(sum,int32ptr);
        sum = builder->CreateSExt(sum, i64Ty);
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_RTYPE_SHIFT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type, bool isW) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context);
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(*context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    if(isW) {
        val1 = builder->CreateTrunc(val1, i32Ty);
        val2 = builder->CreateTrunc(val2, i32Ty);
    } else {
        val2 = builder->CreateSExt(val2,i64Ty);
    }

    llvm::Value* sum = nullptr;
    switch(type) {
        case 0: sum = builder->CreateShl(val1, val2); break; // SLL
        case 1: sum = builder->CreateLShr(val1, val2); break; // SRL
        case 2: sum = builder->CreateAShr(val1, val2); break; // SRA
    }
    if(isW) {
        sum = builder->CreateTrunc(sum,i32Ty);
        sum = builder->CreateSExt(sum,i64Ty);
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_RTYPE_SLT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

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
void jit_ITYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type, bool isW) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i32Ty = builder->getInt32Ty();

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* val2 = ConstantInt::get(i32Ty, (int32_t)cache->imm,true);
    val2 = builder->CreateSExt(val2, i64Ty);
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);

    llvm::Value* sum = nullptr;
    if(isW) {
        val1 = builder->CreateTrunc(val1, i32Ty);
        val2 = builder->CreateTrunc(val2, i32Ty);
    }
    switch(type) {
        case 0: sum = builder->CreateAdd(val1, val2); break; // ADDI
        case 1: sum = builder->CreateXor(val1, val2); break; // XORI
        case 2: sum = builder->CreateOr(val1, val2); break; // ORI
        case 3: sum = builder->CreateAnd(val1, val2); break; // ANDI
    }
    if(isW) {
        sum = builder->CreateSExt(sum,i64Ty);
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);

}
void jit_ITYPE_SHIFT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type, bool isW) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context);
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(*context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* val2 = ConstantInt::get(i32Ty, (int32_t)cache->imm,true);
    val2 = builder->CreateSExt(val2, i64Ty);
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);

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
        sum = builder->CreateTrunc(sum,i32Ty);
        sum = builder->CreateSExt(sum,i64Ty);
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_ITYPE_SLT(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context);
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(*context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* val2 = ConstantInt::get(i32Ty, (int32_t)cache->imm,true);
    val2 = builder->CreateSExt(val2, i64Ty);
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);

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
void jit_ITYPE_L(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i32Ty = builder->getInt32Ty();
    llvm::Type* i1Ty = builder->getInt1Ty();

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* val2 = ConstantInt::get(i32Ty, (int32_t)cache->imm,true);
    val2 = builder->CreateSExt(val2, i64Ty);
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    auto int8ptr = builder->getInt8Ty();
    auto int16ptr = builder->getInt16Ty();
    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* sum = builder->CreateAdd(val1, val2);

    llvm::Value* res = nullptr;
    llvm::Value* size = llvm::ConstantInt::get(i64Ty, 8, true);
    if(type == 2 || type == 3) size = llvm::ConstantInt::get(i64Ty, 16, true);
    if(type == 4 || type == 5) size = llvm::ConstantInt::get(i64Ty, 32, true);
    if(type == 6) size = llvm::ConstantInt::get(i64Ty, 64, true);

    llvm::Value* loadst = builder->CreateCall(cache->loadFunc, {hartPtr,sum,size});

    llvm::Value* value = builder->CreateExtractValue(loadst, {0});
    llvm::Value* hasValue = builder->CreateExtractValue(loadst, {1});

    switch(type) {
        case 0: res = builder->CreateSExt(builder->CreateTrunc(value,int8ptr),i64Ty); break; // LB
        case 2: res = builder->CreateSExt(builder->CreateTrunc(value,int16ptr),i64Ty); break; // LH
        case 4: res = builder->CreateSExt(builder->CreateTrunc(value,i32Ty),i64Ty); break; // LW
        case 6: res = value; break; // LD
        default: res = value; break;
    }

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "load_has_value", currentFunc);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "load_no_value", currentFunc);
    llvm::BasicBlock* contBB = llvm::BasicBlock::Create(*context, "load_cont", currentFunc);

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
void jit_STORE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i32Ty = builder->getInt32Ty();
    llvm::Type* i1Ty = builder->getInt1Ty();

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* val2 = ConstantInt::get(i32Ty,(int32_t)cache->imm,true);
    val2 = builder->CreateSExt(val2,i64Ty);

    auto int8ptr = builder->getInt8Ty();
    auto int16ptr = builder->getInt16Ty();
    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val3 = builder->CreateLoad(i64Ty, rs2Ptr);
    llvm::Value* sum = builder->CreateAdd(val1, val2);
    sum = builder->CreateIntCast(sum,i64Ty,true);
    llvm::Value* res = builder->getInt1(1);

    llvm::Value* size = builder->getInt64(8);
    switch(type) {
        case 0: size = builder->getInt64(8); break;
        case 1: size = builder->getInt64(16); break;
        case 2: size = builder->getInt64(32); break;
        case 3: size = builder->getInt64(64); break;
    }
    
    res = builder->CreateCall(cache->storeFunc, {hartPtr,sum,size,val3});

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "store_has_value", currentFunc);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "store_no_value", currentFunc);
    llvm::BasicBlock* contBB = llvm::BasicBlock::Create(*context, "store_cont", currentFunc);

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
void jit_UTYPE(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context);
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(*context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 1); // HART->pc

    llvm::Value* val1 = ConstantInt::get(i32Ty, cache->imm,true);
    val1 = builder->CreateSExt(val1, i64Ty);
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    llvm::Value* shl = builder->CreateSExt(builder->CreateShl(val1, builder->getInt32(12)),i64Ty);
    llvm::Value* res = nullptr;
    switch(type) {
        case 0: res = shl; break; // LUI
        case 1: res = builder->CreateAdd(shl, builder->CreateLoad(i64Ty, pcPtr)); break; // AUIPC
    }

    // store in rd
    builder->CreateStore(res, rdPtr);
}
void jit_BR(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context);
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(*context);

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs
    llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 1); // HART->pc

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::Value* imm = ConstantInt::get(i32Ty,(int32_t)cache->imm,true);
    imm = builder->CreateSExt(imm,i64Ty);

    llvm::Value* icmpval;
    switch(type) {
        case 0: icmpval = builder->CreateICmpEQ(val1,val2); break; // BEQ
        case 1: icmpval = builder->CreateICmpNE(val1,val2); break; // BNE
        case 2: icmpval = builder->CreateICmpSLT(val1,val2); break; // BLT
        case 3: icmpval = builder->CreateICmpSGE(val1,val2); break; // BGE
        case 4: icmpval = builder->CreateICmpULT(val1,val2); break; // BLTU
        case 5: icmpval = builder->CreateICmpUGE(val1,val2); break; // BGEU
        case 6: icmpval = builder->getInt1(1); break; // JAL
    }

    uint64_t thisPc = cache->jit_virtpc + (int32_t)cache->imm;
    /*for(auto const [key,val] : *cache->branches) {
        std::cout << key << std::endl;
    }
    std::cout << "end" << std::endl;
    std::cout << cache->jit_virtpc << std::endl;
    std::cout << (int32_t)cache->imm << std::endl;
    std::cout << hart->pc << std::endl;*/
    BasicBlock* bl = cache->branches->at(thisPc);
    
    BasicBlock* els = llvm::BasicBlock::Create(*context, std::format("br_{}_after",thisPc), currentFunc);

    builder->CreateCondBr(icmpval,bl,els);

    builder->SetInsertPoint(els);
}