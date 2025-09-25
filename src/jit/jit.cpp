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

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"

using namespace llvm;
using namespace llvm::orc;

std::unique_ptr<Module> module;
LLVMContext context;
llvm::IRBuilder<> builder(context);

int jit_init() {
    // Initialize LLVM JIT targets
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    auto jit = LLJITBuilder().create();
    if (!jit) {
        errs() << "Failed to create JIT\n";
        return 1;
    }

    module = std::make_unique<Module>("jit_module", context);
    return 0;
}
