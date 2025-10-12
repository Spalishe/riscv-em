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
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <tuple>

using namespace llvm;
using namespace llvm::orc;

auto TSC = std::make_unique<llvm::orc::ThreadSafeContext>(
    std::make_unique<llvm::LLVMContext>()
);
llvm::LLVMContext *context = TSC->getContext();
std::unique_ptr<llvm::Module> module = std::make_unique<llvm::Module>("jit_module", *context);
std::unique_ptr<llvm::orc::LLJIT> jit;
std::unique_ptr<llvm::IRBuilder<>> builder = std::make_unique<llvm::IRBuilder<>>(*context);
llvm::StructType* hartStructTy;
llvm::StructType* optStructTy;

llvm::FunctionCallee loadFunc;
llvm::FunctionCallee storeFunc;
llvm::FunctionCallee trapFunc;
llvm::Function* amo64;
llvm::Function* amo32;

extern "C" void jit_trap(HART* hart, uint64_t cause, uint64_t tval) {
	hart->cpu_trap(cause,tval,false);
}
extern "C" OptUInt64 dram_jit_load(HART* hart, uint64_t addr, uint64_t size) {
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,size);
    return OptUInt64{*val,val.has_value()};
}
extern "C" bool dram_jit_store(HART* hart, uint64_t addr, uint64_t size,uint64_t value) {
	return hart->mmio->store(hart,addr,size,value);
}
extern "C" void jit_printval(uint64_t val) {
    std::cout << val << std::endl;
}

llvm::Function* createAmo64Func(IRBuilder<> *bd, std::unique_ptr<llvm::Module> *md, llvm::StructType* l_optStructTy, llvm::FunctionCallee loadFunc, llvm::FunctionCallee storeFunc, llvm::FunctionCallee trapFunc,uint64_t pc) {
    llvm::Type* i64Ty = bd->getInt64Ty();
    llvm::Type* args[] = {
        llvm::PointerType::getUnqual(hartStructTy), // Hart
        i64Ty, // Type
        i64Ty, // Addr
        i64Ty, // RS2
    };
    llvm::FunctionType* funcTy = llvm::FunctionType::get(l_optStructTy, args, false);
    llvm::Function* func = llvm::Function::Create(
        funcTy,
        llvm::Function::ExternalLinkage,
        "amo64_" + std::to_string(pc),
        md->get()
    );
    auto argsIter = func->arg_begin();
    argsIter->setName("hartPtr");
    (++argsIter)->setName("type");
    (++argsIter)->setName("addr");
    (++argsIter)->setName("rs2");

    llvm::GlobalVariable* memCell = new llvm::GlobalVariable(
        **md,
        i64Ty,
        false,                          // isConstant
        llvm::GlobalValue::ExternalLinkage,
        builder->getInt64(0),
        "amo64_memCell_" + std::to_string(pc)
    );

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(bd->getContext(), "entry", func);
    bd->SetInsertPoint(entry);
    llvm::Value* hartPtr = func->getArg(0);
    llvm::Value* type = func->getArg(1);
    llvm::Value* addr = func->getArg(2);
    llvm::Value* rs2 = func->getArg(3);

    llvm::Value* retStruct = llvm::UndefValue::get(l_optStructTy);
    retStruct = bd->CreateInsertValue(retStruct, bd->getInt64(0), {0});
    retStruct = bd->CreateInsertValue(retStruct, bd->getInt1(0), {1});

    llvm::BasicBlock* misalignedBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_misaligned_addr", func);
    llvm::BasicBlock* normalBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_normal", func);
    llvm::BasicBlock* accessBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_access_fault", func);
    llvm::BasicBlock* normal1BB = llvm::BasicBlock::Create(bd->getContext(), "amo64_normal1", func);

    bd->CreateCondBr(bd->CreateICmpNE(bd->CreateURem(addr,bd->getInt64(8)),bd->getInt64(0)),misalignedBB,normalBB);

    bd->SetInsertPoint(normalBB);
    llvm::Value* loadRet = bd->CreateCall(loadFunc,{hartPtr,addr,bd->getInt64(64)});

    llvm::Value* value = bd->CreateExtractValue(loadRet, {0});
    llvm::Value* hasValue = bd->CreateExtractValue(loadRet, {1});

    bd->CreateCondBr(hasValue,normal1BB,accessBB);

    bd->SetInsertPoint(misalignedBB);
    bd->CreateCall(trapFunc,{hartPtr,bd->getInt64(EXC_LOAD_ADDR_MISALIGNED),addr});
    bd->CreateRet(retStruct);

    bd->SetInsertPoint(accessBB);
    bd->CreateCall(trapFunc,{hartPtr,bd->getInt64(EXC_LOAD_ACCESS_FAULT),addr});
    bd->CreateRet(retStruct);

    bd->SetInsertPoint(normal1BB);
    retStruct = bd->CreateInsertValue(retStruct, value, {0});
    retStruct = bd->CreateInsertValue(retStruct, bd->getInt1(1), {1});
    builder->CreateStore(value,memCell);

    llvm::BasicBlock* amoswapBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amoswap", func);
    llvm::BasicBlock* amoaddBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amoadd", func);
    llvm::BasicBlock* amoxorBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amoxor", func);
    llvm::BasicBlock* amoandBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amoand", func);
    llvm::BasicBlock* amoorBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amoor", func);
    llvm::BasicBlock* amominBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amomin", func);
    llvm::BasicBlock* amomaxBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amomax", func);
    llvm::BasicBlock* amouminBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amoumin", func);
    llvm::BasicBlock* amoumaxBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amoumax", func);
    llvm::BasicBlock* amoafterBB = llvm::BasicBlock::Create(bd->getContext(), "amo64_amoafter", func);

    llvm::SwitchInst* sw = bd->CreateSwitch(type,amoafterBB);

    sw->addCase(bd->getInt64(0),amoswapBB);
    sw->addCase(bd->getInt64(1),amoaddBB);
    sw->addCase(bd->getInt64(2),amoxorBB);
    sw->addCase(bd->getInt64(3),amoandBB);
    sw->addCase(bd->getInt64(4),amoorBB);
    sw->addCase(bd->getInt64(5),amominBB);
    sw->addCase(bd->getInt64(6),amomaxBB);
    sw->addCase(bd->getInt64(7),amouminBB);
    sw->addCase(bd->getInt64(8),amoumaxBB);

    llvm::MaybeAlign align(8);
    llvm::Value* prev;

    // Replace Monotonic with Release, Acquire if required

    // AMOSWAP
    bd->SetInsertPoint(amoswapBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Xchg,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOADD
    bd->SetInsertPoint(amoaddBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Add,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOXOR
    bd->SetInsertPoint(amoxorBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Xor,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOAND
    bd->SetInsertPoint(amoandBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::And,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOOR
    bd->SetInsertPoint(amoorBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Or,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOMIN
    bd->SetInsertPoint(amominBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Min,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOMAX
    bd->SetInsertPoint(amomaxBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Max,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOUMIN
    bd->SetInsertPoint(amouminBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::UMin,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOUMAX
    bd->SetInsertPoint(amoumaxBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::UMax,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);


    // After

    bd->SetInsertPoint(amoafterBB);

    bd->CreateCall(storeFunc,{hartPtr,addr,bd->getInt64(64),builder->CreateLoad(i64Ty,memCell)});
    bd->CreateRet(retStruct);
    return func;
}
llvm::Function* createAmo32Func(IRBuilder<> *bd, std::unique_ptr<llvm::Module> *md, llvm::StructType* l_optStructTy, llvm::FunctionCallee loadFunc, llvm::FunctionCallee storeFunc, llvm::FunctionCallee trapFunc,uint64_t pc) {
    llvm::Type* i64Ty = bd->getInt64Ty();
    llvm::Type* i32Ty = bd->getInt32Ty();
    llvm::Type* args[] = {
        llvm::PointerType::getUnqual(hartStructTy), // Hart
        i64Ty, // Type
        i64Ty, // Addr
        i64Ty, // RS2
    };
    llvm::FunctionType* funcTy = llvm::FunctionType::get(l_optStructTy, args, false);
    llvm::Function* func = llvm::Function::Create(
        funcTy,
        llvm::Function::ExternalLinkage,
        "amo32_" + std::to_string(pc),
        md->get()
    );
    auto argsIter = func->arg_begin();
    argsIter->setName("hartPtr");
    (++argsIter)->setName("type");
    (++argsIter)->setName("addr");
    (++argsIter)->setName("rs2");

    llvm::GlobalVariable* memCell = new llvm::GlobalVariable(
        **md,
        i32Ty,
        false,                          // isConstant
        llvm::GlobalValue::ExternalLinkage,
        builder->getInt32(0),
        "amo32_memCell_" + std::to_string(pc)
    );

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(bd->getContext(), "entry", func);
    bd->SetInsertPoint(entry);
    llvm::Value* hartPtr = func->getArg(0);
    llvm::Value* type = func->getArg(1);
    llvm::Value* addr = func->getArg(2);
    llvm::Value* rs2 = func->getArg(3);

    llvm::Value* retStruct = llvm::UndefValue::get(l_optStructTy);
    retStruct = bd->CreateInsertValue(retStruct, bd->getInt64(0), {0});
    retStruct = bd->CreateInsertValue(retStruct, bd->getInt1(0), {1});

    llvm::BasicBlock* misalignedBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_misaligned_addr", func);
    llvm::BasicBlock* normalBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_normal", func);
    llvm::BasicBlock* accessBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_access_fault", func);
    llvm::BasicBlock* normal1BB = llvm::BasicBlock::Create(bd->getContext(), "amo32_normal1", func);

    bd->CreateCondBr(bd->CreateICmpNE(bd->CreateURem(addr,bd->getInt64(4)),bd->getInt64(0)),misalignedBB,normalBB);

    bd->SetInsertPoint(normalBB);
    llvm::Value* loadRet = bd->CreateCall(loadFunc,{hartPtr,addr,bd->getInt64(32)});

    llvm::Value* value = bd->CreateExtractValue(loadRet, {0});
    llvm::Value* hasValue = bd->CreateExtractValue(loadRet, {1});

    bd->CreateCondBr(hasValue,normal1BB,accessBB);

    bd->SetInsertPoint(misalignedBB);
    bd->CreateCall(trapFunc,{hartPtr,bd->getInt64(EXC_LOAD_ADDR_MISALIGNED),addr});
    bd->CreateRet(retStruct);

    bd->SetInsertPoint(accessBB);
    bd->CreateCall(trapFunc,{hartPtr,bd->getInt64(EXC_LOAD_ACCESS_FAULT),addr});
    bd->CreateRet(retStruct);

    bd->SetInsertPoint(normal1BB);
    retStruct = bd->CreateInsertValue(retStruct, value, {0});
    retStruct = bd->CreateInsertValue(retStruct, bd->getInt1(1), {1});
    builder->CreateStore(value,memCell);

    llvm::BasicBlock* amoswapBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amoswap", func);
    llvm::BasicBlock* amoaddBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amoadd", func);
    llvm::BasicBlock* amoxorBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amoxor", func);
    llvm::BasicBlock* amoandBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amoand", func);
    llvm::BasicBlock* amoorBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amoor", func);
    llvm::BasicBlock* amominBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amomin", func);
    llvm::BasicBlock* amomaxBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amomax", func);
    llvm::BasicBlock* amouminBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amoumin", func);
    llvm::BasicBlock* amoumaxBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amoumax", func);
    llvm::BasicBlock* amoafterBB = llvm::BasicBlock::Create(bd->getContext(), "amo32_amoafter", func);

    llvm::SwitchInst* sw = bd->CreateSwitch(type,amoafterBB);

    sw->addCase(bd->getInt64(0),amoswapBB);
    sw->addCase(bd->getInt64(1),amoaddBB);
    sw->addCase(bd->getInt64(2),amoxorBB);
    sw->addCase(bd->getInt64(3),amoandBB);
    sw->addCase(bd->getInt64(4),amoorBB);
    sw->addCase(bd->getInt64(5),amominBB);
    sw->addCase(bd->getInt64(6),amomaxBB);
    sw->addCase(bd->getInt64(7),amouminBB);
    sw->addCase(bd->getInt64(8),amoumaxBB);

    llvm::MaybeAlign align(4);
    llvm::Value* prev;

    // Replace Monotonic with Release, Acquire if required

    // AMOSWAP
    bd->SetInsertPoint(amoswapBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Xchg,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOADD
    bd->SetInsertPoint(amoaddBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Add,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOXOR
    bd->SetInsertPoint(amoxorBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Xor,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOAND
    bd->SetInsertPoint(amoandBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::And,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOOR
    bd->SetInsertPoint(amoorBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Or,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOMIN
    bd->SetInsertPoint(amominBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Min,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOMAX
    bd->SetInsertPoint(amomaxBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::Max,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOUMIN
    bd->SetInsertPoint(amouminBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::UMin,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);

    // AMOUMAX
    bd->SetInsertPoint(amoumaxBB);
    prev = bd->CreateAtomicRMW(
        llvm::AtomicRMWInst::UMax,
        memCell,
        rs2,
        align,
        llvm::AtomicOrdering::Monotonic
    );
    bd->CreateBr(amoafterBB);


    // After

    bd->SetInsertPoint(amoafterBB);

    bd->CreateCall(storeFunc,{hartPtr,addr,bd->getInt64(32),builder->CreateLoad(i64Ty,memCell)});
    bd->CreateRet(retStruct);
    return func;
}

void jit_reset() {
    cantFail(jit->getMainJITDylib().clear());

    jit.reset();
    builder.reset();
    TSC.reset();
    context = nullptr;

    jit_init();
}

int jit_init() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    TSC = std::make_unique<llvm::orc::ThreadSafeContext>(
        std::make_unique<llvm::LLVMContext>()
    );
    context = TSC->getContext();

    builder = std::make_unique<llvm::IRBuilder<>>(*context);

    auto jitExpected = llvm::orc::LLJITBuilder().create();
    if (!jitExpected) {
        llvm::logAllUnhandledErrors(jitExpected.takeError(), llvm::errs(), "JIT init error: ");
        std::abort();
    }
    jit = std::move(*jitExpected);
    JITDylib &jd = jit->getMainJITDylib();
    ExecutionSession &es = jit->getExecutionSession();


    MangleAndInterner mangle(es, jit->getDataLayout());

    // SymbolMap: SymbolStringPtr -> ExecutorSymbolDef
    SymbolMap symbols;

    ExecutorAddr addr_store(reinterpret_cast<JITTargetAddress>(&dram_jit_store));
    symbols[mangle("dram_jit_store")] = ExecutorSymbolDef(addr_store, JITSymbolFlags::Exported);
    ExecutorAddr addr_load(reinterpret_cast<JITTargetAddress>(&dram_jit_load));
    symbols[mangle("dram_jit_load")] = ExecutorSymbolDef(addr_load, JITSymbolFlags::Exported);
    ExecutorAddr addr_trap(reinterpret_cast<JITTargetAddress>(&jit_trap));
    symbols[mangle("jit_trap")] = ExecutorSymbolDef(addr_trap, JITSymbolFlags::Exported);
    ExecutorAddr addr_print(reinterpret_cast<JITTargetAddress>(&jit_printval));
    symbols[mangle("jit_printval")] = ExecutorSymbolDef(addr_print, JITSymbolFlags::Exported);

    cantFail(jd.define(absoluteSymbols(std::move(symbols))));

    // Define HART struct in LLVM

    std::vector<llvm::Type*> hartElements;

    hartElements.push_back(llvm::ArrayType::get(llvm::Type::getInt64Ty(*context), 32));   // regs[32]
    hartElements.push_back(llvm::Type::getInt64Ty(*context)); // pc
    hartElements.push_back(llvm::Type::getInt64Ty(*context)); // virt_pc
    hartElements.push_back(llvm::ArrayType::get(llvm::Type::getInt64Ty(*context), 4069)); // csrs[4069]
    hartElements.push_back(llvm::Type::getInt8Ty(*context));  // mode

    // Reservation
    hartElements.push_back(llvm::Type::getInt64Ty(*context)); // reservation_addr
    hartElements.push_back(llvm::Type::getInt64Ty(*context)); // reservation_value
    hartElements.push_back(llvm::Type::getInt1Ty(*context));  // reservation_valid
    hartElements.push_back(llvm::Type::getInt8Ty(*context));  // reservation_size

    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // testing
    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // dbg
    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // dbg_showinst
    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // dbg_singlestep
    hartElements.push_back(llvm::Type::getInt64Ty(*context)); // breakpoint
    hartElements.push_back(llvm::Type::getInt8Ty(*context));  // id

    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // jit_enabled
    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // block_enabled
    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // stopexec
    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // trap_active
    hartElements.push_back(llvm::Type::getInt1Ty(*context)); // trap_notify

    hartElements.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context))); // DRAM*
    hartElements.push_back(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context))); // MMIO*

    hartStructTy = llvm::StructType::create(*context, hartElements, "HART");

    llvm::Type* i1Ty = llvm::Type::getInt1Ty(*context);
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context);
    llvm::Type* voidTy = llvm::Type::getVoidTy(*context);

    optStructTy = llvm::StructType::create(*context, {i64Ty,i1Ty}, "OptUInt64");

    return 0;
}

using BlockFn = void(*)(HART*);

BlockFn jit_create_block(HART* hart, std::vector<CACHE_Instr>& instrs) {
    try {
        //auto ctx = std::make_unique<llvm::LLVM*context>();
        //auto module = std::make_unique<llvm::Module>("jit_module", *ctx);
        //llvm::IRBuilder<> builder(*ctx);

        llvm::Type* i1Ty = builder->getInt1Ty();
        llvm::Type* i64Ty = builder->getInt64Ty();
        llvm::Type* voidTy = builder->getVoidTy();

        auto* funcType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context),
            { llvm::PointerType::get(hartStructTy,0) },
            false
        );

        std::string funcName = "block_" + std::to_string(hart->pc);
        auto module = std::make_unique<llvm::Module>(funcName, *context);

    
        std::vector<llvm::Type*> loadArgs = { llvm::PointerType::getUnqual(hartStructTy), i64Ty, i64Ty };
        llvm::FunctionType* loadTy = llvm::FunctionType::get(optStructTy, loadArgs, false);
        llvm::FunctionCallee l_loadFunc = module->getOrInsertFunction("dram_jit_load", loadTy);

        std::vector<llvm::Type*> storeArgs = { llvm::PointerType::getUnqual(hartStructTy), i64Ty, i64Ty, i64Ty };
        llvm::FunctionType* storeTy = llvm::FunctionType::get(i1Ty, storeArgs, false);
        llvm::FunctionCallee l_storeFunc = module->getOrInsertFunction("dram_jit_store", storeTy);

        std::vector<llvm::Type*> trapArgs = { llvm::PointerType::getUnqual(hartStructTy), i64Ty, i64Ty };
        llvm::FunctionType* trapTy = llvm::FunctionType::get(voidTy, trapArgs, false);
        llvm::FunctionCallee l_trapFunc = module->getOrInsertFunction("jit_trap", trapTy);
        
        std::vector<llvm::Type*> printArgs = { i64Ty };
        llvm::FunctionType* printTy = llvm::FunctionType::get(voidTy, printArgs, false);
        llvm::FunctionCallee l_printFunc = module->getOrInsertFunction("jit_printval", printTy);

        amo64 = createAmo64Func(builder.get(),&module,optStructTy,l_loadFunc,l_storeFunc,l_trapFunc,hart->pc);

        amo32 = createAmo32Func(builder.get(),&module,optStructTy,l_loadFunc,l_storeFunc,l_trapFunc,hart->pc);

        llvm::Function* func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            funcName,
            module.get()
        );

        auto* entryBB = llvm::BasicBlock::Create(*context, "entry", func);
        builder->SetInsertPoint(entryBB);

        //llvm::Value* hartPtr = ConstantInt::get(Type::getInt64Ty(*context), (uint64_t)hart);
        //hartPtr = builder->CreateIntToPtr(hartPtr, llvm::PointerType::get(hartStructTy, 0));
        llvm::Argument* hartPtr = func->getArg(0);
        hartPtr->setName("hart");

        llvm::Value* regsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 0); // HART->regs
        llvm::Value* csrsPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 3); // HART->csrs
        llvm::Value* pcPtr = builder->CreateStructGEP(hartStructTy, hartPtr, 1); // HART->pc
        llvm::Value* x0Ptr = builder->CreateGEP(i64Ty, regsPtr, builder->getInt32(0));
        llvm::Value* cyclePtr = builder->CreateGEP(i64Ty, csrsPtr, builder->getInt32(CYCLE));

        std::unordered_map<uint64_t,llvm::BasicBlock*> br_blocks;
        uint64_t base_pc = hart->pc;
        uint64_t end_pc = base_pc + instrs.size() * 4;
        for (uint64_t pc = base_pc; pc < end_pc; pc += 4) {
            llvm::BasicBlock* bl = llvm::BasicBlock::Create(*context,"br_" + std::to_string(pc), func);
            br_blocks[pc] = bl;
        }

        std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*> tpl{builder.get(), func, hartPtr};
        uint64_t i = 0;
        for (auto& instr : instrs) {
            // We doing a thing: linearized block CFG with intra-block branch embedding
            i++;
            instr.oprs.loadFunc = l_loadFunc;
            instr.oprs.storeFunc = l_storeFunc;
            instr.oprs.trapFunc = l_trapFunc;
            instr.oprs.amo64Func = amo64;
            instr.oprs.amo32Func = amo32;
            instr.oprs.printFunc = l_printFunc;
            instr.oprs.branches = &br_blocks; 
            instr.oprs.jit_virtpc = hart->pc+(i-1)*4; 

            auto find = br_blocks.find(hart->pc+(i-1)*4);
            llvm::BasicBlock* br_b = NULL;
            if(find != br_blocks.end()) {
                br_b = br_blocks[hart->pc+(i-1)*4];
            }
            if(br_b != NULL) {
                builder->CreateBr(br_b);
                builder->SetInsertPoint(br_b);
            }

            instr.fn(hart, instr.inst, &instr.oprs, &tpl);

            builder->CreateStore(builder->getInt64(hart->pc + 4*i),pcPtr);
            builder->CreateStore(builder->getInt64(0),x0Ptr);
            builder->CreateStore(builder->getInt64(hart->csrs[CYCLE] + 1*i),cyclePtr);
        }
        builder->CreateRetVoid();

        if (llvm::verifyFunction(*func, &llvm::errs())) {
            llvm::errs() << "Function verification failed\n";
            return nullptr;
        }

        if (llvm::verifyModule(*module, &llvm::errs())) {
            llvm::errs() << "Module verification failed\n";
            return nullptr;
        }

        auto TSM = llvm::orc::ThreadSafeModule(std::move(module), *TSC);
        
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