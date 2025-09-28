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
#include "../../include/jit.hpp"
#include "../../include/jit_h.hpp"
#include "../../include/cpu.h"
#include "../../include/main.hpp"
#include <tuple>

using namespace llvm;
using namespace llvm::orc;

llvm::LLVMContext context;
std::unique_ptr<llvm::Module> module = std::make_unique<llvm::Module>("jit_module", context);
std::unique_ptr<llvm::orc::LLJIT> jit;
llvm::IRBuilder<> builder(context);
llvm::StructType* hartStructTy;
llvm::StructType* optStructTy;

llvm::FunctionCallee loadFunc;
llvm::FunctionCallee storeFunc;
llvm::FunctionCallee trapFunc;

OptUInt64 dram_jit_load(HART* hart, uint64_t addr, uint64_t size) {
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,size);
    return OptUInt64{val.has_value(), *val};
}
bool dram_jit_store(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
	return hart->mmio->store(hart,addr,size,value);
}

int jit_init() {
    // Initialize LLVM JIT targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    auto jit1 = LLJITBuilder().create();
    if (!jit1) {
        llvm::errs() << "Failed to create JIT\n";
        return 1;
    }
    jit = std::move(*jit1);

    // Define HART struct in LLVM

    std::vector<llvm::Type*> hartElements;

    hartElements.push_back(llvm::ArrayType::get(llvm::Type::getInt64Ty(context), 32));   // regs[32]
    hartElements.push_back(llvm::Type::getInt64Ty(context)); // pc
    hartElements.push_back(llvm::Type::getInt64Ty(context)); // virt_pc
    hartElements.push_back(llvm::ArrayType::get(llvm::Type::getInt64Ty(context), 4069)); // csrs[4069]
    hartElements.push_back(llvm::Type::getInt8Ty(context));  // mode

    hartElements.push_back(llvm::Type::getInt1Ty(context)); // testing
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // dbg
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // dbg_showinst
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // dbg_singlestep
    hartElements.push_back(llvm::Type::getInt64Ty(context)); // breakpoint
    hartElements.push_back(llvm::Type::getInt8Ty(context));  // id

    hartElements.push_back(llvm::Type::getInt1Ty(context)); // jit_enabled
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // block_enabled
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // stopexec
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // trap_active
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // trap_notify

    // Reservation
    hartElements.push_back(llvm::Type::getInt64Ty(context)); // reservation_addr
    hartElements.push_back(llvm::Type::getInt64Ty(context)); // reservation_value
    hartElements.push_back(llvm::Type::getInt1Ty(context));  // reservation_valid
    hartElements.push_back(llvm::Type::getInt8Ty(context));  // reservation_size

    hartElements.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context))); // DRAM*
    hartElements.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context))); // MMIO*

    hartStructTy = llvm::StructType::create(context, hartElements, "HART");

    llvm::Type* i1Ty = llvm::Type::getInt1Ty(context);
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);
    llvm::Type* voidTy = llvm::Type::getVoidTy(context);

    optStructTy = llvm::StructType::create(context, {i64Ty,i1Ty}, "OptUInt64");
        
    // load
    std::vector<llvm::Type*> loadArgs = { llvm::PointerType::getUnqual(hartStructTy), i64Ty, i64Ty };
    llvm::FunctionType* loadTy = llvm::FunctionType::get(optStructTy, loadArgs, false);
    loadFunc = module->getOrInsertFunction("dram_jit_load", loadTy);

    // store
    std::vector<llvm::Type*> storeArgs = { llvm::PointerType::getUnqual(hartStructTy), i64Ty, i64Ty, i64Ty };
    llvm::FunctionType* storeTy = llvm::FunctionType::get(i1Ty, storeArgs, false);
    storeFunc = module->getOrInsertFunction("dram_jit_store", storeTy);

    llvm::FunctionCallee trapFunc = module->getOrInsertFunction(
        "cpu_trap", llvm::Type::getVoidTy(context),
        llvm::PointerType::getUnqual(hartStructTy), // HART*
        i64Ty, // cause
        i64Ty, // tval
        llvm::Type::getInt1Ty(context) // is_interrupt
    );

    return 0;
}

using BlockFn = void(*)(HART*);

BlockFn jit_create_block(HART* hart, std::vector<CACHE_Instr>& instrs) {
    try {
        auto ctx = std::make_unique<llvm::LLVMContext>();
        auto module = std::make_unique<llvm::Module>("jit_module", *ctx);
        
        auto* funcType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*ctx),
            { llvm::PointerType::get(hartStructTy,0) },
            false
        );

        std::string funcName = "block_" + std::to_string(hart->pc);
        llvm::Function* func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            funcName,
            module.get()
        );

        llvm::Type* i1Ty = llvm::Type::getInt1Ty(*ctx);
        llvm::Type* i64Ty = llvm::Type::getInt64Ty(*ctx);
        llvm::Type* voidTy = llvm::Type::getVoidTy(*ctx);
        auto l_optStructTy = llvm::StructType::create(*ctx, {i64Ty,i1Ty}, "OptUInt64");

        std::vector<llvm::Type*> loadArgs = { llvm::PointerType::getUnqual(hartStructTy), i64Ty, i64Ty };
        llvm::FunctionType* loadTy = llvm::FunctionType::get(l_optStructTy, loadArgs, false);
        llvm::FunctionCallee l_loadFunc = module->getOrInsertFunction("dram_jit_load",loadTy);

        std::vector<llvm::Type*> storeArgs = { llvm::PointerType::getUnqual(hartStructTy), i64Ty, i64Ty, i64Ty };
        llvm::FunctionType* storeTy = llvm::FunctionType::get(i1Ty, storeArgs, false);
        llvm::FunctionCallee l_storeFunc = module->getOrInsertFunction("dram_jit_store",storeTy);

        auto* entryBB = llvm::BasicBlock::Create(*ctx, "entry", func);
        llvm::IRBuilder<> builder(entryBB);

        llvm::Value* hartPtr = ConstantInt::get(Type::getInt64Ty(context), (uint64_t)hart);
        hartPtr = builder.CreateIntToPtr(hartPtr, llvm::PointerType::get(hartStructTy, 0));
        llvm::Value* regsPtr = builder.CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs
        llvm::Value* csrsPtr = builder.CreateStructGEP(hartStructTy, hartPtr, 3); // HART->csrs
        llvm::Value* pcPtr = builder.CreateStructGEP(hartStructTy, hartPtr, 1); // HART->pc
        llvm::Value* x0Ptr = builder.CreateGEP(i64Ty, regsPtr, builder.getInt32(0));
        llvm::Value* cyclePtr = builder.CreateGEP(i64Ty, csrsPtr, builder.getInt32(CYCLE));

        std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*> tpl{&builder, func, hartPtr};
        uint64_t i = 0;
        for (auto& instr : instrs) {
            i++;
            instr.oprs.loadFunc = l_loadFunc;
            instr.oprs.storeFunc = l_storeFunc;
            instr.fn(hart, instr.inst, &instr.oprs, &tpl);

            builder.CreateStore(builder.getInt64(hart->pc + 4*i),pcPtr);
            builder.CreateStore(builder.getInt64(0),x0Ptr);
            builder.CreateStore(builder.getInt64(hart->csrs[CYCLE] + 1*i),cyclePtr);
        }

        builder.CreateRetVoid();

        if (llvm::verifyFunction(*func, &llvm::errs())) {
            llvm::errs() << "Function verification failed\n";
            return nullptr;
        }

        if (llvm::verifyModule(*module, &llvm::errs())) {
            llvm::errs() << "Module verification failed\n";
            return nullptr;
        }

        auto TSM = llvm::orc::ThreadSafeModule(std::move(module), std::move(ctx));
        
        if (!jit) {
            llvm::errs() << "JIT engine is not initialized\n";
            return nullptr;
        }

        if (auto err = jit->addIRModule(std::move(TSM))) {
            llvm::logAllUnhandledErrors(std::move(err), llvm::errs(), "Failed to add module: ");
            return nullptr;
        }

        auto sym = jit->lookup(funcName);
        if (!sym) {
            llvm::logAllUnhandledErrors(sym.takeError(), llvm::errs(), "Failed to lookup function: ");
            return nullptr;
        }

        auto fnPtr = sym->toPtr<BlockFn>();
        return fnPtr;

    } catch (const std::exception& e) {
        llvm::errs() << "Exception in jit_create_block: " << e.what() << "\n";
        return nullptr;
    }
}