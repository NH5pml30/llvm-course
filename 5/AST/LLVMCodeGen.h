#pragma once

#include <iostream>

#include "ContextVisitor.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

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

inline llvm::Type *toLLVMType(llvm::LLVMContext &Ctx, AST::PType T) {
  LLVMTypeVisitor v(Ctx);
  T->accept(v);
  return v.Res;
}

class LLVMCodeGen : public ContextVisitor<llvm::Value *> {
public:
  using BaseVisitor = ContextVisitor<llvm::Value *>;
  llvm::Function *CurrFunc{};
  llvm::LLVMContext &LLVMCtx;
  llvm::Module *Module;
  llvm::IRBuilder<> Builder;
  llvm::Type *Int32Type;
  llvm::Value *Res{};

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
    CurrFunc = nullptr;
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
    if (!CurrFunc)
      throw FrontendError(Node->Loc, "alloca in global scope not supported");
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
