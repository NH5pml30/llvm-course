#include <ANTLRInputStream.h>
#include <CharStream.h>
#include <CommonTokenStream.h>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>

#include "AST.h"

#include "HALexer.h"
#include "HAParser.h"
#include "HABaseListener.h"
#include "HABaseVisitor.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

extern "C" {
#include "sim.h"
}

using namespace five;

namespace {

template<typename T>
class ContextVisitor : public AST::Visitor {
public:
  std::vector<std::unordered_map<std::string, T>> Contexts;

  ContextVisitor(std::unordered_map<std::string, T> GlobalCtx) : Contexts({std::move(GlobalCtx)}) {}

  void enterContext() {
    Contexts.emplace_back();
  }

  void exitContext() {
    Contexts.pop_back();
  }

  void registerName(const std::string &Name, T Value) {
    Contexts.back()[Name] = std::move(Value);
  }

  T *resolveName(const std::string &Name) {
    for (auto CtxIt = Contexts.rbegin(); CtxIt != Contexts.rend(); ++CtxIt) {
      auto &Ctx = *CtxIt;
      if (auto it = Ctx.find(Name); it != Ctx.end())
        return &it->second;
    }
    return nullptr;
  }
};

AST::PType getArrayTy(AST::PType T) {
  return std::make_shared<AST::ArrayType>(T);
}
template<typename ...ArgTs>
AST::PProductType getProductTy(ArgTs... Ts) {
  return std::make_shared<AST::ProductType>(std::vector<AST::PType>{Ts...});
}
template<typename ...ArgTs>
AST::PType getFunctionTy(AST::PType RetTy, ArgTs... Ts) {
  return std::make_shared<AST::FunctionType>(RetTy, getProductTy(Ts...));
}

AST::PType IntTy = AST::IntLiteralExprNode::ExprType;
AST::PType IntArrTy = getArrayTy(IntTy);

class TypeChecker : public ContextVisitor<AST::LocalVarDeclNode *> {
public:
  using BaseVisitor = ContextVisitor<AST::LocalVarDeclNode *>;

  TypeChecker(
      std::unordered_map<std::string, AST::LocalVarDeclNode *> GlobalCtx)
      : ContextVisitor<AST::LocalVarDeclNode *>(std::move(GlobalCtx)) {}

  template<typename T>
  void matchTypes(AST::SourceInterval Loc, std::shared_ptr<T> LHS, std::shared_ptr<T> RHS) {
    if (LHS && RHS && !LHS->equals(RHS.get())) {
      std::cerr << Loc << " incompatible inferred types: ";
      LHS->dump(std::cerr);
      std::cerr << " and ";
      RHS->dump(std::cerr);
      std::cerr << std::endl;
      exit(1);
    }
  }

  template<typename T>
  void updateType(AST::SourceInterval Loc, std::shared_ptr<T> &LHS, std::shared_ptr<T> RHS) {
    matchTypes(Loc, LHS, RHS);
    LHS = RHS;
  }

  void visit(AST::FunctionDefNode *Node) override {
    Node->AST::LocalVarDeclNode::accept(*this);
    enterContext();
    BaseVisitor::visit(Node);
    exitContext();
    updateType(Node->Loc, Node->RetType, Node->InitValue->getType());
    AST::PType FunType = Node->computeType();
    if (!FunType) {
      if (!Node->RetType) {
        std::cerr << Node->Loc << " cannot infer return type\n";
        exit(1);
      }
      for (auto &P : Node->Params) {
        auto T = P->getType();
        if (!T) {
          std::cerr << P->Loc << " cannot infer argument type\n";
          exit(1);
        }
      }
      std::cerr << Node->Loc << " cannot infer function type\n";
      exit(1);
    }
    updateType<AST::Type>(Node->Loc, Node->FunType, FunType);
  }
  void visit(AST::LetInExprNode *Node) override {
    enterContext();
    BaseVisitor::visit(Node);
    exitContext();
  }
  void visit(AST::LocalVarDeclNode *Node) override {
    registerName(Node->Name, Node);
  }
  void visit(AST::LocalVarDefNode *Node) override {
    Node->LocalVarDeclNode::accept(*this);
    Node->InitValue->accept(*this);
    updateType(Node->Loc, Node->RetType, Node->InitValue->getType());
  }
  void visit(AST::IfExprNode *Node) override {
    BaseVisitor::visit(Node);
    matchTypes(Node->CondExpr->Loc, Node->CondExpr->getType(), IntTy);
    matchTypes(Node->Loc, Node->ThenExpr->getType(), Node->ElseExpr->getType());
  }
  void visit(AST::CallExprNode *Node) override {
    BaseVisitor::visit(Node);
    AST::PType T = Node->Callee->getType();
    if (auto FT = std::dynamic_pointer_cast<AST::FunctionType>(T)) {
      if (FT->ParamTypes->ElementTypes.size() != Node->Args.size()) {
        std::cerr
            << Node->Loc
            << " incorrect number of arguments passed to function: expected "
            << FT->ParamTypes->ElementTypes.size() << ", got "
            << Node->Args.size() << std::endl;
        exit(1);
      }
      for (int i = 0; i < Node->Args.size(); i++)
        matchTypes(Node->Args[i]->Loc, Node->Args[i]->getType(), FT->ParamTypes->ElementTypes[i]);
      updateType(Node->Loc, Node->RetType, FT->RetType);
    } else {
      std::cerr << Node->Loc << " cannot infer callee type" << std::endl;
      exit(1);
    }
  }
  void visit(AST::ProductExprNode *Node) override {
    BaseVisitor::visit(Node);
    std::vector<AST::PType> ElementTypes;
    ElementTypes.reserve(Node->Elements.size());
    for (auto &E : Node->Elements) {
      auto ET = E->getType();
      if (!ET) {
        std::cerr << Node->Loc << " cannot infer expression type\n";
        exit(1);
      }
      ElementTypes.push_back(ET);
    }
    updateType(Node->Loc, Node->ExprType,
               std::make_shared<AST::ProductType>(std::move(ElementTypes)));
  }

  void visit(AST::IdentExprNode *Node) override {
    auto *Decl = resolveName(Node->Name);
    if (!Decl) {
      std::cerr << Node->Loc << " unknown name '" << Node->Name << "'\n";
      exit(1);
    }
    updateType(Node->Loc, Node->ExprType, (*Decl)->getType());
  }
};

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

class LLVMTypeVisitor : public AST::TypeVisitor {
public:
  llvm::LLVMContext &Ctx;
  llvm::Type *Res{};

  LLVMTypeVisitor(llvm::LLVMContext &Ctx) : Ctx(Ctx) {}

  void visit(AST::BuiltinAtomType *Type) {
    switch (Type->Kind) {
    case AST::BuiltinAtomTypeKind::INT:
      Res = llvm::Type::getInt32Ty(Ctx);
      break;
    default:
      Res = nullptr;
      break;
    }
  }
  void visit(AST::ArrayType *Type) {
    AST::TypeVisitor::visit(Type);
    Res = llvm::PointerType::get(Res, 0);
  }
  void visit(AST::ProductType *Type) {
    std::vector<llvm::Type *> ElementTypes;
    ElementTypes.reserve(Type->ElementTypes.size());
    for (auto &E : Type->ElementTypes) {
      E->accept(*this);
      ElementTypes.push_back(Res);
    }
    Res = llvm::StructType::get(Ctx, ElementTypes.size());
  }
  void visit(AST::FunctionType *Type) {
    std::vector<llvm::Type *> ParamTypes;
    ParamTypes.reserve(Type->ParamTypes->ElementTypes.size());
    for (auto &E : Type->ParamTypes->ElementTypes) {
      E->accept(*this);
      ParamTypes.push_back(Res);
    }
    Type->RetType->accept(*this);
    Res = llvm::FunctionType::get(Res, ParamTypes, false);
  }
};

llvm::Type *toLLVMType(llvm::LLVMContext &Ctx, AST::PType T) {
  LLVMTypeVisitor v(Ctx);
  T->accept(v);
  return v.Res;
}

class LLVMCodeGen : public ContextVisitor<llvm::Value *> {
public:
  using BaseVisitor = ContextVisitor<llvm::Value *>;
  llvm::Function *CurrFunc;
  llvm::LLVMContext &LLVMCtx;
  llvm::Module *Module;
  llvm::IRBuilder<> Builder;
  llvm::Type *Int32Type;
  llvm::Value *Res;

  LLVMCodeGen(llvm::Module *Module,
              std::unordered_map<std::string, llvm::Value *> GlobalCtx)
      : ContextVisitor<llvm::Value *>(std::move(GlobalCtx)),
        LLVMCtx(Module->getContext()), Module(Module), Builder(LLVMCtx) {
    Int32Type = Builder.getInt32Ty();
  }

  void visit(AST::FunctionDefNode *Node) override {
    llvm::FunctionType *FuncType =
        cast<llvm::FunctionType>(toLLVMType(LLVMCtx, Node->getType()));
    CurrFunc = llvm::Function::Create(FuncType, llvm::Function::ExternalLinkage,
                                      Node->Name, Module);
    llvm::BasicBlock *EntryBB = llvm::BasicBlock::Create(LLVMCtx, "entry", CurrFunc);
    Builder.SetInsertPoint(EntryBB);

    registerName(Node->Name, CurrFunc);

    enterContext();

    for (int arg = 0; arg < Node->Params.size(); arg++)
      registerName(Node->Params[arg]->Name, CurrFunc->getArg(arg));

    Node->InitValue->accept(*this);
    Builder.CreateRet(Res);

    exitContext();
  }

  void visit(AST::LocalVarDefNode *Node) override {
    Node->InitValue->accept(*this);
    registerName(Node->Name, Res);
  }
  void visit(AST::LetInExprNode *Node) override {
    enterContext();
    BaseVisitor::visit(Node);
    exitContext();
  }
  void visit(AST::IfExprNode *Node) override {
    Node->CondExpr->accept(*this);
    llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(LLVMCtx, "then", CurrFunc);
    llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(LLVMCtx, "else", CurrFunc);
    llvm::BasicBlock *FiBB = llvm::BasicBlock::Create(LLVMCtx, "fi", CurrFunc);
    Builder.CreateCondBr(Builder.CreateTrunc(Res, Builder.getInt1Ty()), ThenBB,
                         ElseBB);

    Builder.SetInsertPoint(ThenBB);
    Node->ThenExpr->accept(*this);
    auto *ThenValue = Res;
    ThenBB = Builder.GetInsertBlock();
    Builder.CreateBr(FiBB);

    Builder.SetInsertPoint(ElseBB);
    Node->ElseExpr->accept(*this);
    auto *ElseValue = Res;
    ElseBB = Builder.GetInsertBlock();
    Builder.CreateBr(FiBB);

    Builder.SetInsertPoint(FiBB);
    auto *PHI = Builder.CreatePHI(toLLVMType(LLVMCtx, Node->getType()), 2);
    PHI->addIncoming(ThenValue, ThenBB);
    PHI->addIncoming(ElseValue, ElseBB);
    Res = PHI;
  }
  void visit(AST::AllocaExprNode *Node) override {
    Node->AllocaSize->accept(*this);
    Res = Builder.CreateAlloca(toLLVMType(LLVMCtx, Node->AllocaType->ElementType), Res);
  }
  void visit(AST::CallExprNode *Node) override {
    Node->Callee->accept(*this);
    auto Func = cast<llvm::Function>(Res);
    std::vector<llvm::Value *> Args;
    Args.reserve(Node->Args.size());
    for (auto &A : Node->Args) {
      A->accept(*this);
      Args.push_back(Res);
    }
    Res = Builder.CreateCall(Func, Args);
  }
  void visit(AST::ProductExprNode *Node) override {
    auto *Type = toLLVMType(LLVMCtx, Node->getType());
    llvm::Value *Agg = llvm::PoisonValue::get(Type);
    for (unsigned I = 0; I < Node->Elements.size(); I++) {
      Node->Elements[I]->accept(*this);
      Agg = Builder.CreateInsertValue(Agg, Res, {I});
    }
    Res = Agg;
  }
  void visit(AST::IdentExprNode *Node) override {
    Res = *resolveName(Node->Name);
  }
  void visit(AST::IntLiteralExprNode *Node) override {
    Res = Builder.getInt32(Node->Value);
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
        llvm::Function::Create(cast<llvm::FunctionType>(toLLVMType(Ctx, T)),
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
    // set (write at index)
    auto ElT = IntTy;
    auto T = getFunctionTy(ElT, IntTy, ElT, getArrayTy(ElT));
    auto [Node, Func] = StartFunction("set", T);
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

int main() {
  std::ifstream in("test.ha");
  antlr4::ANTLRInputStream input(in);
  HALexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  HAParser parser(&tokens);

  HAParser::FileContext *tree = parser.file();
  // std::cout << tree->toString() << "\n";

  llvm::LLVMContext Ctx;
  llvm::Module *Module = new llvm::Module("app", Ctx);
  llvm::IRBuilder<> Builder(Ctx);

  auto Builtins = getBuiltins(Module);

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
