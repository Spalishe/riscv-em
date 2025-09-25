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

using namespace llvm;
using namespace llvm::orc;

LLVMContext context;
std::unique_ptr<Module> module = std::make_unique<Module>("jit_module", context);
IRBuilder<> builder(context);
llvm::StructType* hartStructTy;
llvm::StructType* optStructTy;

llvm::FunctionCallee loadFunc;
llvm::FunctionCallee storeFunc;
llvm::FunctionCallee trapFunc;

struct OptUInt64 {
    bool has_value;
    uint64_t value;
};

OptUInt64 dram_jit_load(HART* hart, uint64_t addr, uint64_t size) {
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,size);
    return OptUInt64{val.has_value(), *val};
}
bool dram_jit_store(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
	return hart->mmio->store(hart,addr,size,value);
}

int jit_init() {
    // Initialize LLVM JIT targets
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    auto jit = LLJITBuilder().create();
    if (!jit) {
        errs() << "Failed to create JIT\n";
        return 1;
    }

    // Define HART struct in LLVM

    std::vector<llvm::Type*> hartElements;

    hartElements.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context))); // DRAM*
    hartElements.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context))); // MMIO*

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

    hartElements.push_back(llvm::Type::getInt1Ty(context)); // block_enabled
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // stopexec
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // trap_active
    hartElements.push_back(llvm::Type::getInt1Ty(context)); // trap_notify

    // Reservation
    hartElements.push_back(llvm::Type::getInt64Ty(context)); // reservation_addr
    hartElements.push_back(llvm::Type::getInt64Ty(context)); // reservation_value
    hartElements.push_back(llvm::Type::getInt1Ty(context));  // reservation_valid
    hartElements.push_back(llvm::Type::getInt8Ty(context));  // reservation_size

    hartStructTy = llvm::StructType::create(context, hartElements, "HART");

    llvm::Type* i1Ty = llvm::Type::getInt1Ty(context);
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(context);
    llvm::Type* voidTy = llvm::Type::getVoidTy(context);

    optStructTy = llvm::StructType::create(context, hartElements, "OptUInt64");
        
    // load
    std::vector<llvm::Type*> loadArgs = { hartStructTy, i64Ty, i64Ty };
    llvm::FunctionType* loadTy = llvm::FunctionType::get(optStructTy, loadArgs, false);
    loadFunc = module->getOrInsertFunction("dram_jit_load", loadTy);

    // store
    std::vector<llvm::Type*> storeArgs = { hartStructTy, i64Ty, i64Ty, i64Ty };
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
