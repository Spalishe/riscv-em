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

#pragma once

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"

extern llvm::StructType* hartStructTy;
extern llvm::StructType* optStructTy;
extern llvm::FunctionCallee loadFunc;
extern llvm::FunctionCallee storeFunc;
extern llvm::FunctionCallee trapFunc;
extern llvm::FunctionCallee amo64;

extern llvm::LLVMContext *context;