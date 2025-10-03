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


void jit_MUL(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i32Ty = builder->getInt32Ty();
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i128Ty = builder->getInt128Ty();

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    switch(type) {
        case 0: {

        }; break; // MUL
        case 1: {
            val1 = builder->CreateSExt(val1,i128Ty);
            val2 = builder->CreateSExt(val2,i128Ty);
        }; break; // MULH
        case 2: {
            val1 = builder->CreateSExt(val1,i128Ty);
            val2 = builder->CreateZExt(val2,i128Ty);
        }; break; // MULHSU
        case 3: {
            val1 = builder->CreateZExt(val1,i128Ty);
            val2 = builder->CreateZExt(val2,i128Ty);
        }; break; // MULHU
        case 4: {
            val1 = builder->CreateTrunc(val1,i32Ty);
            val2 = builder->CreateTrunc(val2,i32Ty);
        }; break; // MULW
    }
    
    llvm::Value* sum = builder->CreateMul(val1, val2);

    switch(type) {
        case 0: {

        }; break; // MUL
        case 1: 
        case 2:
        case 3:
        {
            llvm::Value* high = builder->CreateLShr(sum,ConstantInt::get(i128Ty,64));
            sum = builder->CreateTrunc(high,i64Ty);
        }; break; // MULH, MULHSU, MULHU
        case 4:
        {
            sum = builder->CreateSExt(sum,i64Ty);
        }; break; // MULW
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_DIV(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i32Ty = builder->getInt32Ty();
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i128Ty = builder->getInt128Ty();

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::BasicBlock* isZero = llvm::BasicBlock::Create(context, "isZero", currentFunc);
    llvm::BasicBlock* SignOverflowCheck = llvm::BasicBlock::Create(context, "SignOverflowCheck", currentFunc);
    llvm::BasicBlock* SignOverflow = llvm::BasicBlock::Create(context, "SignOverflow", currentFunc);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context, "normal", currentFunc);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context, "done", currentFunc);

    llvm::Value* zero = builder->getInt64(0);
    if(type == 2 || type == 3) {
        //W
        val1 = builder->CreateTrunc(val1,i32Ty);
        val2 = builder->CreateTrunc(val2,i32Ty);
        zero = builder->getInt32(0);
    }

    builder->CreateCondBr(builder->CreateICmpEQ(val2,zero), isZero, (type == 1 || type == 3) ? normal : SignOverflowCheck);

    builder->SetInsertPoint(isZero);
    builder->CreateStore(builder->getInt64(-1), rdPtr);
    builder->CreateBr(done);

    builder->SetInsertPoint(SignOverflowCheck);
    llvm::Value* isMin = builder->CreateICmpEQ(val1,(type == 2 || type == 3) ? builder->getInt32(0x80000000) : builder->getInt64(0x8000000000000000));
    llvm::Value* isNeg = builder->CreateICmpEQ(val2,(type == 2 || type == 3) ? builder->getInt32(-1) : builder->getInt64(-1));
    llvm::Value* ovf = builder->CreateAnd(isMin,isNeg);
    builder->CreateCondBr(builder->CreateTrunc(ovf,builder->getInt1Ty()), SignOverflow, normal);

    builder->SetInsertPoint(SignOverflow);
    builder->CreateStore(val1, rdPtr);
    builder->CreateBr(done);

    builder->SetInsertPoint(normal);

    llvm::Value* sum = nullptr;

    switch(type) {
        case 0: {
            sum = builder->CreateSDiv(val1, val2);
        }; break; // DIV
        case 1: {
            sum = builder->CreateUDiv(val1, val2);
        }; break; // DIVU
        case 2: {
            sum = builder->CreateSDiv(val1, val2);
            sum = builder->CreateSExt(sum,i64Ty);
        }; break; // DIVW
        case 3: {
            sum = builder->CreateUDiv(val1, val2);
            sum = builder->CreateSExt(sum,i64Ty);
        }; break; // DIVUW
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
    builder->CreateBr(done);
    
    builder->SetInsertPoint(done);
}

void jit_REM(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i32Ty = builder->getInt32Ty();
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i128Ty = builder->getInt128Ty();

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    // pointer to rs1, rs2 and rd
    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::BasicBlock* isZero = llvm::BasicBlock::Create(context, "isZero", currentFunc);
    llvm::BasicBlock* SignOverflowCheck = llvm::BasicBlock::Create(context, "SignOverflowCheck", currentFunc);
    llvm::BasicBlock* SignOverflow = llvm::BasicBlock::Create(context, "SignOverflow", currentFunc);
    llvm::BasicBlock* normal = llvm::BasicBlock::Create(context, "normal", currentFunc);
    llvm::BasicBlock* done = llvm::BasicBlock::Create(context, "done", currentFunc);

    llvm::Value* zero = builder->getInt64(0);
    if(type == 2 || type == 3) {
        //W
        val1 = builder->CreateTrunc(val1,i32Ty);
        val2 = builder->CreateTrunc(val2,i32Ty);
        zero = builder->getInt32(0);
    }

    builder->CreateCondBr(builder->CreateICmpEQ(val2,zero), isZero, (type == 1 || type == 3) ? normal : SignOverflowCheck);

    builder->SetInsertPoint(isZero);
    builder->CreateStore(builder->getInt64(-1), rdPtr);
    builder->CreateBr(done);

    builder->SetInsertPoint(SignOverflowCheck);
    llvm::Value* isMin = builder->CreateICmpEQ(val1,(type == 2 || type == 3) ? builder->getInt32(0x80000000) : builder->getInt64(0x8000000000000000));
    llvm::Value* isNeg = builder->CreateICmpEQ(val2,(type == 2 || type == 3) ? builder->getInt32(-1) : builder->getInt64(-1));
    llvm::Value* ovf = builder->CreateAnd(isMin,isNeg);
    builder->CreateCondBr(builder->CreateTrunc(ovf,builder->getInt1Ty()), SignOverflow, normal);

    builder->SetInsertPoint(SignOverflow);
    builder->CreateStore(zero, rdPtr);
    builder->CreateBr(done);

    builder->SetInsertPoint(normal);

    llvm::Value* sum = nullptr;

    switch(type) {
        case 0: {
            sum = builder->CreateSRem(val1, val2);
        }; break; // REM
        case 1: {
            sum = builder->CreateURem(val1, val2);
        }; break; // REMU
        case 2: {
            sum = builder->CreateSRem(val1, val2);
            sum = builder->CreateSExt(sum,i64Ty);
        }; break; // REMW
        case 3: {
            sum = builder->CreateURem(val1, val2);
            sum = builder->CreateSExt(sum,i64Ty);
        }; break; // REMUW
    }

    // store in rd
    builder->CreateStore(sum, rdPtr);
    builder->CreateBr(done);
    
    builder->SetInsertPoint(done);
}