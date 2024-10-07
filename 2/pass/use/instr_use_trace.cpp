#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
using namespace llvm;

static bool instrumentOneFunction(Function &F) {
  llvm::errs() << "instrumentOneFunction " << F.getName() << "\n";
  auto &Ctx = F.getContext();
  auto *M = F.getParent();
  IRBuilder<> Builder(Ctx);
  ArrayRef<Type *> ParamTypes = {
      Builder.getInt8Ty()->getPointerTo(),
  };
  auto PrintF = M->getOrInsertFunction(
      "printf", FunctionType::get(Builder.getInt32Ty(), ParamTypes, true));
  bool Changed = false;
  for (auto &B : F) {
    for (auto &I : B) {
      if (PHINode::classof(&I))
        continue;
      Builder.SetInsertPoint(&I);
      std::string Format = "%s <- ";
      SmallVector<Value *, 0> Args = {
          nullptr,
          Builder.CreateGlobalStringPtr(std::string(I.getOpcodeName())),
      };
      auto PlaceholderStr = Builder.CreateGlobalStringPtr("_");
      for (auto &Op : I.operands()) {
        Format += "%s;";
        auto *OpI = dyn_cast_if_present<Instruction>(Op.get());
        if (OpI && !PHINode::classof(OpI))
          Args.push_back(
              Builder.CreateGlobalStringPtr(std::string(OpI->getOpcodeName())));
        else
          Args.push_back(PlaceholderStr);
      }
      Format += "\n";
      Args[0] = Builder.CreateGlobalStringPtr(Format);
      Builder.CreateCall(PrintF, Args);
      Changed = true;
    }
  }
  return Changed;
}

struct InstrUseTracePass : public PassInfoMixin<InstrUseTracePass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    llvm::errs() << "Run InstrUseTracePass\n";
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
      MPM.addPass(InstrUseTracePass{});
      return true;
    });
  };

  return {LLVM_PLUGIN_API_VERSION, "MyPlugin2", "0.0.1", callback};
};

/* When a plugin is loaded by the driver, it will call this entry point to
obtain information about this plugin and about how to register its passes.
*/
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getPassPluginInfo();
}