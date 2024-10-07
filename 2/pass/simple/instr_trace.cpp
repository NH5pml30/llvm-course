#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
using namespace llvm;

static bool instrumentOneFunction(Function &F) {
  llvm::errs() << "instrumentOneFunction " << F.getName() << "\n";
  auto &Ctx = F.getContext();
  auto *M = F.getParent();
  IRBuilder<> Builder(Ctx);
  ArrayRef<Type *> ParamTypes = {
      Builder.getInt8Ty()->getPointerTo(),
  };
  auto Puts = M->getOrInsertFunction(
      "puts", FunctionType::get(Builder.getInt32Ty(), ParamTypes, false));
  bool Changed = false;
  for (auto &B : F) {
    for (auto &I : B) {
      if (PHINode::classof(&I))
        continue;
      Builder.SetInsertPoint(&I);
      ArrayRef<Value *> Args = {
          Builder.CreateGlobalStringPtr(std::string(I.getOpcodeName()))};
      Builder.CreateCall(Puts, Args);
      Changed = true;
    }
  }
  return Changed;
}

struct InstrTracePass : public PassInfoMixin<InstrTracePass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    llvm::errs() << "Run InstrTracePass\n";
    bool Changed = false;
    for (auto &F : M) {
      if (F.isDeclaration())
        continue;
      Changed |= instrumentOneFunction(F);
    }
    if (Changed)
      return PreservedAnalyses::none();
    return PreservedAnalyses::all();
  };
};

PassPluginLibraryInfo getPassPluginInfo() {
  const auto callback = [](PassBuilder &PB) {
    PB.registerOptimizerLastEPCallback([&](ModulePassManager &MPM, auto &&) {
      MPM.addPass(InstrTracePass{});
      return true;
    });
  };

  return {LLVM_PLUGIN_API_VERSION, "MyPlugin1", "0.0.1", callback};
};

/* When a plugin is loaded by the driver, it will call this entry point to
obtain information about this plugin and about how to register its passes.
*/
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getPassPluginInfo();
}