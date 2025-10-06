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

void jit_ZICSR_R(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i32Ty = builder->getInt32Ty();

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs
    llvm::Value* csrsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 3); // HART->csrs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* val2 = ConstantInt::get(i64Ty, cache->imm);
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);

    llvm::Value* sum = nullptr;
    llvm::Value* csrP = builder->CreateGEP(i64Ty, csrsPtr, val2);
    switch(type) {
        case 0: {
            sum = builder->CreateLoad(i64Ty,csrP);
            builder->CreateStore(val1,csrP);
        }; break; // RW
        case 1: {
            sum = builder->CreateLoad(i64Ty,csrP);
            builder->CreateStore(val1,csrP);
            llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "has_value", currentFunc);
            llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "no_value", currentFunc);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(*context, "cont", currentFunc);
            llvm::Value* cond = builder->CreateICmpNE(val1, llvm::ConstantInt::get(i64Ty, 0));
            builder->CreateCondBr(cond, thenBB, elseBB);

            // then
            builder->SetInsertPoint(thenBB);
            builder->CreateStore(builder->CreateOr(sum,val1),csrP);
            builder->CreateBr(contBB);

            builder->SetInsertPoint(elseBB);
            builder->CreateBr(contBB);

            // continue
            builder->SetInsertPoint(contBB);
        }; break; // RS
        case 2: {
            sum = builder->CreateLoad(i64Ty,csrP);
            builder->CreateStore(val1,csrP);
            llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "has_value", currentFunc);
            llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "no_value", currentFunc);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(*context, "cont", currentFunc);
            llvm::Value* cond = builder->CreateICmpNE(val1, llvm::ConstantInt::get(i64Ty, 0));
            builder->CreateCondBr(cond, thenBB, elseBB);

            // then
            builder->SetInsertPoint(thenBB);
            builder->CreateStore(builder->CreateAnd(sum,builder->CreateNot(val1)),csrP);
            builder->CreateBr(contBB);

            builder->SetInsertPoint(elseBB);
            builder->CreateBr(contBB);

            // continue
            builder->SetInsertPoint(contBB);  
        }; break; // RC
    }
    // store in rd
    builder->CreateStore(sum, rdPtr);
}
void jit_ZICSR_I(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr, uint8_t type) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i32Ty = builder->getInt32Ty();

    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs
    llvm::Value* csrsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 3); // HART->csrs

    llvm::Value* val1 = builder->getInt32(cache->rs1);
    llvm::Value* val2 = ConstantInt::get(i64Ty, cache->imm);
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    llvm::Value* sum = nullptr;
    llvm::Value* csrP = builder->CreateGEP(i64Ty, csrsPtr, val2);
    switch(type) {
        case 0: {
            sum = builder->CreateLoad(i64Ty,csrP);
            builder->CreateStore(val1,csrP);
        }; break; // RW
        case 1: {
            sum = builder->CreateLoad(i64Ty,csrP);
            builder->CreateStore(val1,csrP);
            llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "has_value", currentFunc);
            llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "no_value", currentFunc);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(*context, "cont", currentFunc);
            llvm::Value* cond = builder->CreateICmpNE(val1, llvm::ConstantInt::get(i64Ty, 0));
            builder->CreateCondBr(cond, thenBB, elseBB);

            // then
            builder->SetInsertPoint(thenBB);
            builder->CreateStore(builder->CreateOr(sum,val1),csrP);
            builder->CreateBr(contBB);

            builder->SetInsertPoint(elseBB);
            builder->CreateBr(contBB);

            // continue
            builder->SetInsertPoint(contBB);
        }; break; // RS
        case 2: {
            sum = builder->CreateLoad(i64Ty,csrP);
            builder->CreateStore(val1,csrP);
            llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "has_value", currentFunc);
            llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "no_value", currentFunc);
            llvm::BasicBlock* contBB = llvm::BasicBlock::Create(*context, "cont", currentFunc);
            llvm::Value* cond = builder->CreateICmpNE(val1, llvm::ConstantInt::get(i64Ty, 0));
            builder->CreateCondBr(cond, thenBB, elseBB);

            // then
            builder->SetInsertPoint(thenBB);
            builder->CreateStore(builder->CreateAnd(sum,builder->CreateNot(val1)),csrP);
            builder->CreateBr(contBB);

            builder->SetInsertPoint(elseBB);
            builder->CreateBr(contBB);

            // continue
            builder->SetInsertPoint(contBB);  
        }; break; // RC
    }
    // store in rd
    builder->CreateStore(sum, rdPtr);
}