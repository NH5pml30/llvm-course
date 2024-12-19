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
  std::string Name;
  Instantiator<std::pair<AST::LocalVarDeclNode, llvm::Value *> &> I;
};

template<typename Key, typename Value>
struct CachingInstantiator {
  std::vector<std::pair<Key, Value>> Cache;
  std::string Name;
  std::function<Value(const std::string &Name, const Key &K)> Eval;
  std::function<Key(const std::vector<AST::PType> &ArgTs)> ToKey;
  std::function<bool(const Key &K1, const Key &K2)> Equals;

  CachingInstantiator(
      std::string Name,
      const std::function<Value(const std::string &Name, const Key &K)> &Eval,
      const std::function<Key(const std::vector<AST::PType> &ArgTs)> &ToKey,
      const std::function<bool(const Key &K1, const Key &K2)> &Equals)
      : Name(std::move(Name)), Eval(Eval), ToKey(ToKey), Equals(Equals) {}

  Value &operator()(const std::vector<AST::PType> &ArgTs) {
    Key K = ToKey(ArgTs);
    for (auto &[CK, CV] : Cache)
      if (Equals(CK, K))
        return CV;
    using namespace StreamOperators;
    std::stringstream Ss;
    Ss << Name << '<' << K << '>';
    return Cache.emplace_back(std::move(K), Eval(std::move(Ss).str(), K))
        .second;
  }

  operator Builtin() {
    return Builtin{Name, *this};
  }
};

template<typename Value>
struct SingleTypeCachingInstantiator : CachingInstantiator<AST::PType, Value> {
  SingleTypeCachingInstantiator(
      std::string Name,
      const std::function<Value(const std::string &Name, const AST::PType &K)>
          &Eval,
      const std::function<AST::PType(const std::vector<AST::PType> &ArgTs)>
          &ToKey)
      : CachingInstantiator<AST::PType, Value>(
            std::move(Name), Eval, ToKey,
            [](auto &K1, auto &K2) { return K1->equals(K2.get()); }) {}

  operator Builtin() {
    return Builtin{this->Name, *this};
  }
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

  auto StartStub = [Module](std::string Name, AST::PType T) {
    AST::LocalVarDeclNode Node = {AST::SourceInterval{}, Name, T};
    auto *Func =
        llvm::Function::Create(cast<llvm::FunctionType>(toRawLLVMType(Module->getContext(), T)),
                               llvm::Function::ExternalLinkage, Name, Module);
    return std::make_pair(Node, Func);
  };
  auto StartFunction = [StartStub, Module](std::string Name, AST::PType T, llvm::IRBuilder<> &Builder) {
    auto [Node, Func] = StartStub(Name, T);
    llvm::BasicBlock *EntryBB = llvm::BasicBlock::Create(Module->getContext(), "entry", Func);
    Builder.SetInsertPoint(EntryBB);
    return std::make_pair(Node, Func);
  };

  auto GetSimpleBuiltin = [&](auto Node, llvm::Value *Func) {
    return Builtin{
        Node.Name,
        [R = std::make_pair(Node, Func)](auto &&...) mutable
            -> std::pair<AST::LocalVarDeclNode, llvm::Value *> & { return R; }};
  };

  // integer binary operations
  for (auto &&[Name, BinOp] : BinOps) {
    auto T = getFunctionTy(IntTy, IntTy, IntTy);
    auto [Node, Func] = StartFunction(Name, T, Builder);
    Builder.CreateRet(
        Builder.CreateBinOp(BinOp, Func->getArg(0), Func->getArg(1)));
    Res.push_back(GetSimpleBuiltin(Node, Func));
  }
  for (auto &&[Name, CmpOp] : CmpOps) {
    auto T = getFunctionTy(IntTy, IntTy, IntTy);
    auto [Node, Func] = StartFunction(Name, T, Builder);
    Builder.CreateRet(Builder.CreateZExt(
        Builder.CreateICmp(CmpOp, Func->getArg(0), Func->getArg(1)),
        toLLVMType(Ctx, IntTy)));
    Res.push_back(GetSimpleBuiltin(Node, Func));
  }

  auto GetInstantiationName = [](std::string Name, AST::PType ElT) {
    std::stringstream Ss;
    Ss << Name << '<';
    ElT->dump(Ss);
    Ss << '>';
    return std::move(Ss).str();
  };

  using InstT = SingleTypeCachingInstantiator<std::pair<AST::LocalVarDeclNode, llvm::Value *>>;

  // array operations
  auto GetArrToKey = [](size_t Idx) {
    return [Idx](const std::vector<AST::PType> &ArgTs) -> AST::PType {
      if (Idx >= ArgTs.size())
        return nullptr;
      if (auto ArrTy = std::dynamic_pointer_cast<AST::ArrayType>(ArgTs[Idx]))
        return ArrTy->ElementType;
      return nullptr;
    };
  };

  {
    // at (access at index)
    auto Eval = [Module, StartFunction](const std::string &Name, AST::PType ElT) {
      auto &Ctx = Module->getContext();
      llvm::IRBuilder<> Builder(Ctx);
      auto T = getFunctionTy(ElT, IntTy, getArrayTy(ElT));
      auto [Node, Func] = StartFunction(Name, T, Builder);
      Builder.CreateRet(Builder.CreateLoad(toLLVMType(Ctx, ElT),
                                          Builder.CreateGEP(toLLVMType(Ctx, ElT),
                                                            Func->getArg(1),
                                                            Func->getArg(0))));
      return std::make_pair(Node, Func);
    };
    Res.push_back(InstT("at", Eval, GetArrToKey(1)));
  }
  {
    // set_at (write at index)
    auto Eval = [Module, StartFunction](const std::string &Name, AST::PType ElT) {
      auto &Ctx = Module->getContext();
      llvm::IRBuilder<> Builder(Ctx);
      auto T = getFunctionTy(ElT, IntTy, ElT, getArrayTy(ElT));
      auto [Node, Func] = StartFunction(Name, T, Builder);
      Builder.CreateStore(Func->getArg(1),
                          Builder.CreateGEP(toLLVMType(Ctx, ElT), Func->getArg(2),
                                            Func->getArg(0)));
      Builder.CreateRet(Func->getArg(1));
      return std::make_pair(Node, Func);
    };
    Res.push_back(InstT("set_at", Eval, GetArrToKey(2)));
  }
  {
    // slice (array slice)
    auto Eval = [Module, StartFunction](const std::string &Name, AST::PType ElT) {
      auto &Ctx = Module->getContext();
      llvm::IRBuilder<> Builder(Ctx);
      auto T = getFunctionTy(getArrayTy(ElT), IntTy, IntTy, getArrayTy(ElT));
      auto [Node, Func] = StartFunction(Name, T, Builder);
      Builder.CreateRet(Builder.CreateGEP(toLLVMType(Ctx, ElT), Func->getArg(2),
                                          Func->getArg(0)));
      return std::make_pair(Node, Func);
    };
    Res.push_back(InstT("slice", Eval, GetArrToKey(2)));
  }

  {
    // write int to screen
    std::string Name = "write";
    auto ElT = IntTy;
    auto T = getFunctionTy(IntTy, IntTy);
    auto [Node, Func] = StartFunction(Name, T, Builder);
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
    Res.push_back(GetSimpleBuiltin(Node, Func));
  }

  // sim functions
  for (auto &&[Name, TV] : SimFuncs) {
    auto [Node, Func] = StartStub(Name, TV.first);
    Res.push_back(GetSimpleBuiltin(Node, Func));
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
      std::unordered_map<std::string, Instantiator<AST::LocalVarDeclNode *>> GlobalCtx;
      for (auto &I : Builtins)
        GlobalCtx[I.Name] = [&](auto &&Arg) { return &I.I(std::forward<decltype(Arg)>(Arg)).first; };

      TypeChecker c{GlobalCtx};
      VisitorRunner<TypeChecker> r{c};
      r.visit(tree);
    }

    {
      std::unordered_map<std::string, Instantiator<llvm::Value *>> GlobalCtx;
      for (auto &I : Builtins)
        GlobalCtx[I.Name] = [&](auto &&Arg) { return I.I(std::forward<decltype(Arg)>(Arg)).second; };

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
