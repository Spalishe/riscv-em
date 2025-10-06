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


void jit_LR_D(HART* hart, CACHE_DecodedOperands* cache, IRBuilder<>* builder, llvm::Function* currentFunc, llvm::Value* hartPtr) {
    llvm::Type* i64Ty = builder->getInt64Ty();
    llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs

    llvm::Value* rs1Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rs1));
    llvm::Value* rdPtr  = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(cache->rd));

    // load values from rs1 and rs2
    llvm::Value* val1 = builder->CreateLoad(i64Ty, rs1Ptr);

    llvm::BasicBlock* ifB = llvm::BasicBlock::Create(context, "lrd_if", currentFunc);
    llvm::BasicBlock* elseB = llvm::BasicBlock::Create(context, "lrd_else", currentFunc);
    llvm::BasicBlock* normalB = llvm::BasicBlock::Create(context, "lrd_end", currentFunc);

    builder->CreateCondBr(builder->CreateICmpNE(builder->CreateURem(val1,builder->getInt64(8)), builder->getInt64(0)), ifB, elseB);

    builder->SetInsertPoint(ifB);

    AllocaInst* structPtr = builder->CreateAlloca(cache->types[3], nullptr);

    Value* field0Ptr = builder->CreateStructGEP(cache->types[3], structPtr, 0);
    builder->CreateStore(builder->getInt64((uint64_t)hart),field0Ptr);
    Value* field1Ptr = builder->CreateStructGEP(cache->types[3], structPtr, 1);
    builder->CreateStore(builder->getInt64(EXC_LOAD_ADDR_MISALIGNED),field1Ptr);
    Value* field2Ptr = builder->CreateStructGEP(cache->types[3], structPtr, 2);
    builder->CreateStore(val1,field2Ptr);

    builder->CreateCall(cache->trapFunc,{structPtr});
    builder->CreateRetVoid();
    
    builder->SetInsertPoint(elseB);
    builder->CreateBr(normalB);
}