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

llvm::MaybeAlign align(8);

void jit_LR_D(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);

    llvm::Value* loadst = builder->CreateCall(cache->loadFunc, {hartPtr,val1,builder->getInt64(64)});

    llvm::Value* hasValue = builder->CreateExtractValue(loadst, {0});
    llvm::Value* value = builder->CreateExtractValue(loadst, {1});

    llvm::BasicBlock* ifB = llvm::BasicBlock::Create(*context, "lrd_if", currentFunc);
    llvm::BasicBlock* elseB = llvm::BasicBlock::Create(*context, "lrd_else", currentFunc);
    llvm::BasicBlock* normalB = llvm::BasicBlock::Create(*context, "lrd_end", currentFunc);

    builder->CreateCondBr(hasValue, ifB, elseB);

    // then
    builder->SetInsertPoint(ifB);
    builder->CreateStore(value, rdPtr);
    llvm::Value* res_addr_ptr = builder->CreateStructGEP(hartStructTy, hartPtr, 5,"reservation_addr");
    llvm::Value* res_val_ptr = builder->CreateStructGEP(hartStructTy, hartPtr, 6,"reservation_value");
    llvm::Value* res_valid_ptr = builder->CreateStructGEP(hartStructTy, hartPtr, 7,"reservation_valid");
    llvm::Value* res_size_ptr = builder->CreateStructGEP(hartStructTy, hartPtr, 8,"reservation_size");
    builder->CreateStore(val1,res_addr_ptr);
    builder->CreateStore(value,res_val_ptr);
    builder->CreateStore(builder->getInt1(1),res_valid_ptr);
    builder->CreateStore(builder->getInt64(64),res_size_ptr);

    // continue
    builder->CreateBr(normalB);

    // else
    builder->SetInsertPoint(elseB);
    builder->CreateRetVoid();

    // continue
    builder->SetInsertPoint(normalB);
}
void jit_SC_D(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i1Ty = builder->getInt1Ty();
    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::BasicBlock* ifB = llvm::BasicBlock::Create(*context, "scd_if", currentFunc);
    llvm::BasicBlock* elseB = llvm::BasicBlock::Create(*context, "scd_else", currentFunc);
    llvm::BasicBlock* normalB = llvm::BasicBlock::Create(*context, "scd_end", currentFunc);

    llvm::Value* res_addr_ptr = builder->CreateStructGEP(hartStructTy, hartPtr, 5,"reservation_addr");
    llvm::Value* res_val_ptr = builder->CreateStructGEP(hartStructTy, hartPtr, 6,"reservation_value");
    llvm::Value* res_valid_ptr = builder->CreateStructGEP(hartStructTy, hartPtr, 7,"reservation_valid");
    llvm::Value* res_size_ptr = builder->CreateStructGEP(hartStructTy, hartPtr, 8,"reservation_size");

    llvm::Value* isValid = builder->CreateLoad(i1Ty,res_valid_ptr);
    builder->CreateCondBr(
        builder->CreateAnd(
            builder->CreateAnd(
                isValid,
                builder->CreateICmpEQ(builder->CreateLoad(i64Ty,res_size_ptr),builder->getInt64(64))
            ),
            builder->CreateICmpEQ(builder->CreateLoad(i64Ty,res_addr_ptr),val1)
        ), ifB, elseB);

    // then
    builder->SetInsertPoint(ifB);
    builder->CreateCall(cache->storeFunc, {hartPtr,val1,builder->getInt64(64),val2});
    builder->CreateStore(builder->getInt64(0),rdPtr);

    // continue
    builder->CreateBr(normalB);

    // else
    builder->SetInsertPoint(elseB);
    builder->CreateStore(builder->getInt64(1),rdPtr);

    // continue
    builder->SetInsertPoint(normalB);
    builder->CreateStore(builder->getInt1(0),res_valid_ptr);
}

void jit_AMOSWAP_D(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i1Ty = builder->getInt1Ty();
    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::BasicBlock* ifB = llvm::BasicBlock::Create(*context, "amo_if", currentFunc);
    llvm::BasicBlock* elseB = llvm::BasicBlock::Create(*context, "amo_else", currentFunc);
    llvm::BasicBlock* normalB = llvm::BasicBlock::Create(*context, "amo_end", currentFunc);

    llvm::Value* loadst = builder->CreateCall(cache->amo64Func,{hartPtr,builder->getInt64(0),val1,val2});

    llvm::Value* hasValue = builder->CreateExtractValue(loadst, {0});
    llvm::Value* value = builder->CreateExtractValue(loadst, {1});

    builder->CreateCondBr(hasValue, ifB, elseB);

    // then
    builder->SetInsertPoint(ifB);
    // store in rd
    builder->CreateStore(value, rdPtr);
    // continue
    builder->CreateBr(normalB);

    // else
    builder->SetInsertPoint(elseB);
    builder->CreateRetVoid();

    // continue
    builder->SetInsertPoint(normalB);
}

void jit_AMOADD_D(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Type* i1Ty = builder->getInt1Ty();
    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rs2Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs2));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);
    llvm::Value* val2 = builder->CreateLoad(i64Ty, rs2Ptr);

    llvm::BasicBlock* ifB = llvm::BasicBlock::Create(*context, "amo_if", currentFunc);
    llvm::BasicBlock* elseB = llvm::BasicBlock::Create(*context, "amo_else", currentFunc);
    llvm::BasicBlock* normalB = llvm::BasicBlock::Create(*context, "amo_end", currentFunc);

    llvm::Value* loadst = builder->CreateCall(cache->amo64Func,{hartPtr,builder->getInt64(1),val1,val2});

    llvm::Value* hasValue = builder->CreateExtractValue(loadst, {0});
    llvm::Value* value = builder->CreateExtractValue(loadst, {1});

    builder->CreateCondBr(hasValue, ifB, elseB);

    // then
    builder->SetInsertPoint(ifB);
    // store in rd
    builder->CreateStore(value, rdPtr);
    // continue
    builder->CreateBr(normalB);

    // else
    builder->SetInsertPoint(elseB);
    builder->CreateRetVoid();

    // continue
    builder->SetInsertPoint(normalB);
}
