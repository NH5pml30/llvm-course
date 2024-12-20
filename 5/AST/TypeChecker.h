#pragma once

#include <iostream>

#include "AST/AST.h"
#include "ContextVisitor.h"

class TypeChecker : public ContextVisitor<Instantiator<AST::LocalVarDeclNode *>> {
public:
  using BaseVisitor = ContextVisitor<Instantiator<AST::LocalVarDeclNode *>>;

  TypeChecker(
      std::unordered_map<std::string, Instantiator<AST::LocalVarDeclNode *>>
          GlobalCtx)
      : BaseVisitor(std::move(GlobalCtx)) {}

  template<typename T>
  void matchTypes(AST::SourceInterval Loc, std::shared_ptr<T> LHS, std::shared_ptr<T> RHS) {
    if (LHS && RHS && !LHS->equals(RHS.get()))
      throw FrontendError(Loc, "incompatible inferred types: ", *LHS, " and ", *RHS);
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
      if (!Node->RetType)
        throw FrontendError(Node->Loc, "cannot infer return type");
      for (auto &P : Node->Params) {
        auto T = P->getType();
        if (!T)
          throw FrontendError(P->Loc, "cannot infer argument type");
      }
      throw FrontendError(Node->Loc, "cannot infer function type");
    }
    updateType<AST::Type>(Node->Loc, Node->FunType, FunType);
  }
  void visit(AST::LetInExprNode *Node) override {
    enterContext();
    BaseVisitor::visit(Node);
    exitContext();
  }
  void visit(AST::LocalVarDeclNode *Node) override {
    registerName(Node->Name, [Node](auto &&) { return Node; });
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
    {
      std::vector<AST::PType> ArgTypes;
      for (auto &A : Node->Args) {
        A->accept(*this);
        ArgTypes.push_back(A->getType());
      }
      if (auto *Ident = dynamic_cast<AST::IdentExprNode *>(Node->Callee.get())) {
        resolveIdent(Ident, ArgTypes);
      } else {
        Node->Callee->accept(*this);
      }
    }

    AST::PType T = Node->Callee->getType();
    if (auto FT = std::dynamic_pointer_cast<AST::FunctionType>(T)) {
      if (FT->ParamTypes->ElementTypes.size() != Node->Args.size())
        throw FrontendError(
            Node->Loc,
            "incorrect number of arguments passed to function: expected ",
            FT->ParamTypes->ElementTypes.size(), ", got ", Node->Args.size());
      for (int i = 0; i < Node->Args.size(); i++)
        matchTypes(Node->Args[i]->Loc, Node->Args[i]->getType(),
                   FT->ParamTypes->ElementTypes[i]);
      updateType(Node->Loc, Node->RetType, FT->RetType);
    } else {
      throw FrontendError(Node->Loc, "cannot infer callee type");
    }
  }
  void visit(AST::ProductExprNode *Node) override {
    BaseVisitor::visit(Node);
    std::vector<AST::PType> ElementTypes;
    ElementTypes.reserve(Node->Elements.size());
    for (auto &E : Node->Elements) {
      auto ET = E->getType();
      if (!ET)
        throw FrontendError(Node->Loc, "cannot infer expression type");
      ElementTypes.push_back(ET);
    }
    updateType(Node->Loc, Node->ExprType,
               std::make_shared<AST::ProductType>(std::move(ElementTypes)));
  }

  void resolveIdent(AST::IdentExprNode *Node, const std::vector<AST::PType> &ArgTs) {
    auto *R = resolveName(Node->Name);
    if (!R)
      throw FrontendError(Node->Loc, "unknown name '", Node->Name, "'");
    auto *Decl = (*R)(ArgTs);
    if (!Decl)
      throw FrontendError(Node->Loc, "cannot instantiate '", Node->Name,
                          "' with arguments of types: ",
                          std::make_shared<AST::ProductType>(ArgTs));
    updateType(Node->Loc, Node->ExprType, Decl->getType());
  }

  void visit(AST::IdentExprNode *Node) override {
    resolveIdent(Node, {});
  }
};
