#include <ANTLRInputStream.h>
#include <CharStream.h>
#include <CommonTokenStream.h>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>

#include "AST/AST.h"
#include "AST/TypeChecker.h"
#include "AST/LLVMCodeGen.h"

#include "HALexer.h"
#include "HAParser.h"
#include "HABaseListener.h"
#include "HABaseVisitor.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "sim/sim.h"

using namespace five;

namespace {

template<typename Visitor>
class VisitorRunner : public HABaseVisitor {
public:
  Visitor &Checker;
  VisitorRunner(Visitor &Checker) : Checker(Checker) {}

  std::any visitFunDef(HAParser::FunDefContext *ctx) override {
    ctx->Result->accept(Checker);
    return {};
  }

  std::any visitVarDef(HAParser::VarDefContext *ctx) override {
    ctx->Result->accept(Checker);
    return {};
  }
};

struct Builtin {
  AST::LocalVarDeclNode Node;
  llvm::Value *Value;
};

const std::unordered_map<std::string, llvm::Instruction::BinaryOps> BinOps = {
  {"+", llvm::Instruction::BinaryOps::Add},
  {"-", llvm::Instruction::BinaryOps::Sub},
  {"*", llvm::Instruction::BinaryOps::Mul},
  {"/", llvm::Instruction::BinaryOps::SDiv},
  {"%", llvm::Instruction::BinaryOps::SRem},
  {"&&", llvm::Instruction::BinaryOps::And},
  {"||", llvm::Instruction::BinaryOps::Or},
};

const std::unordered_map<std::string, llvm::CmpInst::Predicate> CmpOps = {
  {"==", llvm::CmpInst::ICMP_EQ},
  {"!=", llvm::CmpInst::ICMP_NE},
  {"<", llvm::CmpInst::ICMP_SLT},
  {"<=", llvm::CmpInst::ICMP_SLE},
  {">", llvm::CmpInst::ICMP_SGT},
  {">=", llvm::CmpInst::ICMP_SGE},
};

const std::unordered_map<std::string, std::pair<AST::PType, void *>> SimFuncs = {
  {"simFlush", {getFunctionTy(getProductTy()), reinterpret_cast<void *>(simFlush)}},
  {"simPutPixel", {getFunctionTy(getProductTy(), IntTy, IntTy, IntTy), reinterpret_cast<void *>(simPutPixel)}},
  {"simClear", {getFunctionTy(getProductTy(), IntTy), reinterpret_cast<void *>(simClear)}},
  {"simRand", {getFunctionTy(IntTy), reinterpret_cast<void *>(simRand)}},
};

std::vector<Builtin> getBuiltins(llvm::Module *Module) {
  std::vector<Builtin> Res;
  auto &Ctx = Module->getContext();
  llvm::IRBuilder<> Builder(Ctx);

  auto StartStub = [&](std::string Name, AST::PType T) {
    AST::LocalVarDeclNode Node = {AST::SourceInterval{}, Name, T};
    auto *Func =
        llvm::Function::Create(cast<llvm::FunctionType>(toRawLLVMType(Ctx, T)),
                               llvm::Function::ExternalLinkage, Name, Module);
    return std::make_pair(Node, Func);
  };
  auto StartFunction = [&](std::string Name, AST::PType T) {
    auto [Node, Func] = StartStub(Name, T);
    llvm::BasicBlock *EntryBB = llvm::BasicBlock::Create(Ctx, "entry", Func);
    Builder.SetInsertPoint(EntryBB);
    return std::make_pair(Node, Func);
  };

  // integer binary operations
  for (auto &&[Name, BinOp] : BinOps) {
    auto T = getFunctionTy(IntTy, IntTy, IntTy);
    auto [Node, Func] = StartFunction(Name, T);
    Builder.CreateRet(
        Builder.CreateBinOp(BinOp, Func->getArg(0), Func->getArg(1)));
    Res.push_back({Node, Func});
  }
  for (auto &&[Name, CmpOp] : CmpOps) {
    auto T = getFunctionTy(IntTy, IntTy, IntTy);
    auto [Node, Func] = StartFunction(Name, T);
    Builder.CreateRet(Builder.CreateZExt(
        Builder.CreateICmp(CmpOp, Func->getArg(0), Func->getArg(1)),
        toLLVMType(Ctx, IntTy)));
    Res.push_back({Node, Func});
  }

  // array operations
  {
    // at (access at index)
    auto ElT = IntTy;
    auto T = getFunctionTy(ElT, IntTy, getArrayTy(ElT));
    auto [Node, Func] = StartFunction("at", T);
    Builder.CreateRet(Builder.CreateLoad(toLLVMType(Ctx, ElT),
                                         Builder.CreateGEP(toLLVMType(Ctx, ElT),
                                                           Func->getArg(1),
                                                           Func->getArg(0))));
    Res.push_back({Node, Func});
  }
  {
    // set_at (write at index)
    auto ElT = IntTy;
    auto T = getFunctionTy(ElT, IntTy, ElT, getArrayTy(ElT));
    auto [Node, Func] = StartFunction("set_at", T);
    Builder.CreateStore(Func->getArg(1),
                        Builder.CreateGEP(toLLVMType(Ctx, ElT), Func->getArg(2),
                                          Func->getArg(0)));
    Builder.CreateRet(Func->getArg(1));
    Res.push_back({Node, Func});
  }
  {
    // slice (array slice)
    auto ElT = IntTy;
    auto T = getFunctionTy(getArrayTy(ElT), IntTy, IntTy, getArrayTy(ElT));
    auto [Node, Func] = StartFunction("slice", T);
    Builder.CreateRet(Builder.CreateGEP(toLLVMType(Ctx, ElT), Func->getArg(2),
                                        Func->getArg(0)));
    Res.push_back({Node, Func});
  }

  {
    // write int to screen
    auto ElT = IntTy;
    auto T = getFunctionTy(IntTy, IntTy);
    auto [Node, Func] = StartFunction("write", T);
    auto Printf = Module->getOrInsertFunction(
        "printf",
        llvm::FunctionType::get(Builder.getInt32Ty(),
                                {
                                    Builder.getInt8Ty()->getPointerTo(),
                                },
                                true));
    Builder.CreateCall(
        Printf, {Builder.CreateGlobalStringPtr("%d\n"), Func->getArg(0)});
    Builder.CreateRet(Func->getArg(0));
    Res.push_back({Node, Func});
  }

  // sim functions
  for (auto &&[Name, TV] : SimFuncs) {
    auto [Node, Func] = StartStub(Name, TV.first);
    Res.push_back({Node, Func});
  }

  return Res;
}

}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <source file>" << std::endl;
    return 1;
  }

  std::ifstream in(argv[1]);
  antlr4::ANTLRInputStream input(in);
  HALexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  HAParser parser(&tokens);

  HAParser::FileContext *tree = parser.file();

  llvm::LLVMContext Ctx;
  llvm::Module *Module = new llvm::Module("app", Ctx);
  llvm::IRBuilder<> Builder(Ctx);

  auto Builtins = getBuiltins(Module);

  try {
    {
      std::unordered_map<std::string, AST::LocalVarDeclNode *> GlobalCtx;
      for (auto &&[Node, Value] : Builtins)
        GlobalCtx[Node.Name] = &Node;

      TypeChecker c{GlobalCtx};
      VisitorRunner<TypeChecker> r{c};
      r.visit(tree);
    }

    {
      std::unordered_map<std::string, llvm::Value *> GlobalCtx;
      for (auto &&[Node, Value] : Builtins)
        GlobalCtx[Node.Name] = Value;

      LLVMCodeGen c{Module, GlobalCtx};
      VisitorRunner<LLVMCodeGen> r{c};
      r.visit(tree);
    }
  } catch (FrontendError &E) {
    std::cerr << "FrontendError: " << E.what() << std::endl;
    return 1;
  }

  llvm::outs() << "[LLVM IR]\n";
  Module->print(llvm::outs(), nullptr);
  llvm::outs() << "\n";
  bool Verif = verifyModule(*Module, &llvm::outs());
  llvm::outs() << "[VERIFICATION] " << (!Verif ? "OK\n\n" : "FAIL\n\n");

  llvm::Function *appFunc = Module->getFunction("app");
  if (appFunc == nullptr) {
    llvm::outs() << "Can't find app function\n";
    return -1;
  }

  // LLVM IR Interpreter
  llvm::outs() << "[EE] Run\n";
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  llvm::ExecutionEngine *ee = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(Module)).create();
  ee->InstallLazyFunctionCreator([=](const std::string &FnName) -> void * {
    if (auto it = SimFuncs.find(FnName); it != SimFuncs.end())
      return it->second.second;
    return nullptr;
  });
  ee->finalizeObject();

  simInit();

  llvm::ArrayRef<llvm::GenericValue> noargs;
  llvm::GenericValue v = ee->runFunction(appFunc, noargs);
  llvm::outs() << "[EE] Result: " << v.IntVal << "\n";

  simExit();

  return 0;
}
