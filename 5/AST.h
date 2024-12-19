#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <numeric>

namespace AST {
struct SourceLocation {
  size_t Line, Col;

  friend std::ostream &operator<<(std::ostream &o, const SourceLocation &I) {
    return o << "at " << I.Line << ":" << I.Col << ":";
  }
};

struct SourceInterval {
  SourceLocation Start, End;

  friend std::ostream &operator<<(std::ostream &o, const SourceInterval &I) {
    return o << I.Start;
  }
};

class LocalVarDeclNode;
class FunctionDefNode;
class BlockExprNode;
class LocalVarDefNode;
class LetInExprNode;
class IfExprNode;
class AllocaExprNode;
class CallExprNode;
class ProductExprNode;
class IdentExprNode;
class IntLiteralExprNode;

class Visitor {
public:
  inline virtual void visit(FunctionDefNode *Node);
  inline virtual void visit(LocalVarDeclNode *Node);
  inline virtual void visit(LocalVarDefNode *Node);
  inline virtual void visit(BlockExprNode *Node);
  inline virtual void visit(LetInExprNode *Node);
  inline virtual void visit(IfExprNode *Node);
  inline virtual void visit(AllocaExprNode *Node);
  inline virtual void visit(CallExprNode *Node);
  inline virtual void visit(ProductExprNode *Node);
  inline virtual void visit(IdentExprNode *Node);
  inline virtual void visit(IntLiteralExprNode *Node);
};

class Node {
public:
  SourceInterval Loc;

  Node(SourceInterval Loc) : Loc(Loc) {}

  virtual void accept(Visitor &Visitor) = 0;
  virtual void dump(std::ostream &o) = 0;
  virtual ~Node() = default;
};

template<typename Derived, typename Base = Node>
class NodeBase : public Base {
public:
  using Base::Base;

  void accept(Visitor &visitor) override {
    visitor.visit(static_cast<Derived *>(this));
  }
};

enum class BuiltinAtomTypeKind {
  INT
};

class BuiltinAtomType;
class ArrayType;
class ProductType;
class FunctionType;

class TypeVisitor {
public:
  inline virtual void visit(BuiltinAtomType *Type);
  inline virtual void visit(ArrayType *Type);
  inline virtual void visit(ProductType *Type);
  inline virtual void visit(FunctionType *Type);
};

class Type {
public:
  virtual void accept(TypeVisitor &V) = 0;
  virtual void dump(std::ostream &o) = 0;
  virtual bool equals(Type *Other) = 0;
  virtual ~Type() = default;
};

template<typename Derived>
class TypeBase : public Type {
public:
  void accept(TypeVisitor &V) override {
    V.visit(static_cast<Derived *>(this));
  }

  bool equals(Type *Other) override {
    auto *T = dynamic_cast<Derived *>(Other);
    if (!T)
      return false;
    return static_cast<Derived *>(this)->equalsTo(*T);
  }
};

class BuiltinAtomType : public TypeBase<BuiltinAtomType> {
public:
  BuiltinAtomTypeKind Kind;

  BuiltinAtomType(BuiltinAtomTypeKind Kind) : Kind(Kind) {}

  bool equalsTo(const BuiltinAtomType &Other) const {
    return Kind == Other.Kind;
  }

  void dump(std::ostream &o) override {
    switch (Kind) {
    case BuiltinAtomTypeKind::INT:
      o << "int";
      break;
    default:
      o << "<unknown type>";
      break;
    }
  }
};

using PType = std::shared_ptr<Type>;

class ArrayType : public TypeBase<ArrayType> {
public:
  PType ElementType;

  ArrayType(PType ElementType) : ElementType(std::move(ElementType)) {}

  bool equalsTo(const ArrayType &Other) const {
    return ElementType->equals(Other.ElementType.get());
  }

  void dump(std::ostream &o) override {
    o << '[';
    ElementType->dump(o);
    o << ']';
  }
};

using PArrayType = std::shared_ptr<ArrayType>;

class ProductType : public TypeBase<ProductType> {
public:
  std::vector<PType> ElementTypes;

  ProductType(std::vector<PType> ElementTypes) : ElementTypes(std::move(ElementTypes)) {}

  bool equalsTo(const ProductType &Other) const {
    if (ElementTypes.size() != Other.ElementTypes.size())
      return false;
    return std::transform_reduce(
        ElementTypes.begin(), ElementTypes.end(), Other.ElementTypes.begin(),
        true, std::logical_and<>{},
        [](auto &Lhs, auto &Rhs) { return Lhs->equals(Rhs.get()); });
  }

  void dump(std::ostream &o) override {
    o << '(';
    for (auto &T : ElementTypes) {
      T->dump(o);
      o << ',';
    }
    o << ')';
  }
};

using PProductType = std::shared_ptr<ProductType>;

class FunctionType : public TypeBase<FunctionType> {
public:
  PType RetType;
  PProductType ParamTypes;

  FunctionType(PType RetType, PProductType ParamTypes)
      : RetType(std::move(RetType)), ParamTypes(std::move(ParamTypes)) {}

  bool equalsTo(const FunctionType &Other) const {
    return RetType->equals(Other.RetType.get()) &&
           ParamTypes->equals(Other.ParamTypes.get());
  }

  void dump(std::ostream &o) override {
    ParamTypes->dump(o);
    o << " -> ";
    RetType->dump(o);
  }
};

void TypeVisitor::visit(BuiltinAtomType *Type) {}
void TypeVisitor::visit(ArrayType *Type) {
  Type->ElementType->accept(*this);
}
void TypeVisitor::visit(ProductType *Type) {
  for (auto &E : Type->ElementTypes)
    E->accept(*this);
}
void TypeVisitor::visit(FunctionType *Type) {
  Type->RetType->accept(*this);
  Type->ParamTypes->accept(*this);
}

class ExprNode : public Node {
public:
  using Node::Node;

  virtual PType getType() = 0;
};

using PExprNode = std::unique_ptr<ExprNode>;

class LocalVarDeclNode : public NodeBase<LocalVarDeclNode> {
public:
  std::string Name;
  PType RetType;

  LocalVarDeclNode(SourceInterval Loc, std::string Name, PType VarType)
      : NodeBase<LocalVarDeclNode>(Loc), Name(std::move(Name)),
        RetType(std::move(VarType)) {}

  virtual PType getType() {
    return RetType;
  }

  void dump(std::ostream &o) override {
    o << Name << ": ";
    if (RetType)
      RetType->dump(o);
  }
};

class LocalVarDefNode : public NodeBase<LocalVarDefNode, LocalVarDeclNode> {
public:
  PExprNode InitValue;

  LocalVarDefNode(SourceInterval Loc, std::string Name, PType VarType,
                  PExprNode InitValue)
      : NodeBase<LocalVarDefNode, LocalVarDeclNode>(Loc, std::move(Name),
                                                    std::move(VarType)),
        InitValue(std::move(InitValue)) {}

  void dump(std::ostream &o) override {
    LocalVarDeclNode::dump(o);
    o << " = ";
    InitValue->dump(o);
  }
};

using PLocalVarDefNode = std::unique_ptr<LocalVarDefNode>;

class FunctionDefNode : public NodeBase<FunctionDefNode, LocalVarDefNode> {
public:
  std::vector<std::unique_ptr<LocalVarDeclNode>> Params;
  PType FunType;

  FunctionDefNode(SourceInterval Loc, std::string Name,
                  std::vector<std::unique_ptr<LocalVarDeclNode>> Params,
                  PType RetType, PExprNode Body)
      : NodeBase<FunctionDefNode, LocalVarDefNode>(Loc, Name, RetType,
                                                   std::move(Body)),
        Params(std::move(Params)), FunType(computeType()) {}

  AST::PType computeType() const {
    if (!RetType)
      return nullptr;
    std::vector<AST::PType> ParamTypes;
    ParamTypes.reserve(Params.size());
    for (auto &P : Params) {
      auto T = P->getType();
      if (!T) {
        return nullptr;
      }
      ParamTypes.push_back(T);
    }
    return std::make_shared<FunctionType>(
        RetType, std::make_shared<ProductType>(std::move(ParamTypes)));
  }

  virtual PType getType() override {
    return FunType;
  }

  void dump(std::ostream &o) override {
    o << "fun " << Name << '(';
    for (auto &P : Params) {
      P->dump(o);
      if (&P != &Params.back())
        o << ", ";
    }
    o << ')';
    if (RetType) {
      o << ": ";
      RetType->dump(o);
    }
    o << " = ";
    InitValue->dump(o);
  }
};

template<typename Derived>
using ExprNodeBase = NodeBase<Derived, ExprNode>;

class BlockExprNode : public ExprNodeBase<BlockExprNode> {
public:
  std::vector<PExprNode> Exprs;

  BlockExprNode(SourceInterval Loc, std::vector<PExprNode> Exprs)
      : ExprNodeBase<BlockExprNode>(Loc), Exprs(std::move(Exprs)) {}

  PType getType() override {
    return Exprs.back()->getType();
  }

  void dump(std::ostream &o) override {
    o << '(';
    for (auto &E : Exprs) {
      E->dump(o);
      if (&E != &Exprs.back())
        o << "; ";
    }
    o << ')';
  }
};

class LetInExprNode : public ExprNodeBase<LetInExprNode> {
public:
  std::vector<PLocalVarDefNode> LocalVars;
  PExprNode InnerExpr;

  LetInExprNode(SourceInterval Loc, std::vector<PLocalVarDefNode> LocalVars,
                PExprNode InnerExpr)
      : ExprNodeBase<LetInExprNode>(Loc), LocalVars(std::move(LocalVars)),
        InnerExpr(std::move(InnerExpr)) {}

  PType getType() override {
    return InnerExpr->getType();
  }

  void dump(std::ostream &o) override {
    o << "let ";
    for (auto &L : LocalVars) {
      L->dump(o);
      if (&L != &LocalVars.back())
        o << ", ";
    }
    o << " in ";
    InnerExpr->dump(o);
  }
};

using PLetInExprNode = std::unique_ptr<LetInExprNode>;

class IfExprNode : public ExprNodeBase<IfExprNode> {
public:
  PExprNode CondExpr;
  PExprNode ThenExpr;
  PExprNode ElseExpr;

  IfExprNode(SourceInterval Loc, PExprNode CondExpr, PExprNode ThenExpr,
             PExprNode ElseExpr)
      : ExprNodeBase<IfExprNode>(Loc), CondExpr(std::move(CondExpr)),
        ThenExpr(std::move(ThenExpr)), ElseExpr(std::move(ElseExpr)) {}

  PType getType() override { return ThenExpr->getType(); }

  void dump(std::ostream &o) override {
    o << "if ";
    CondExpr->dump(o);
    o << " then ";
    ThenExpr->dump(o);
    o << " else ";
    ElseExpr->dump(o);
    o << " fi";
  }
};

using PIfExprNode = std::unique_ptr<IfExprNode>;

class AllocaExprNode : public ExprNodeBase<AllocaExprNode> {
public:
  PArrayType AllocaType;
  PExprNode AllocaSize;

  AllocaExprNode(SourceInterval Loc, PType AllocaType, PExprNode AllocaSize)
      : ExprNodeBase<AllocaExprNode>(Loc),
        AllocaType(std::make_shared<ArrayType>(AllocaType)),
        AllocaSize(std::move(AllocaSize)) {}

  PType getType() override { return AllocaType; }

  void dump(std::ostream &o) override {
    o << "alloca ";
    AllocaType->ElementType->dump(o);
    o << '[';
    AllocaSize->dump(o);
    o << ']';
  }
};

using PAllocaExprNode = std::unique_ptr<AllocaExprNode>;

class CallExprNode : public ExprNodeBase<CallExprNode> {
public:
  PExprNode Callee;
  std::vector<PExprNode> Args;
  PType RetType;

  CallExprNode(SourceInterval Loc, PExprNode Callee,
               std::vector<PExprNode> Args)
      : ExprNodeBase<CallExprNode>(Loc), Callee(std::move(Callee)),
        Args(std::move(Args)) {}

  PType getType() override { return RetType; }

  void dump(std::ostream &o) override {
    Callee->dump(o);
    o << '(';
    for (auto &A : Args) {
      A->dump(o);
      if (&A != &Args.back())
        o << ", ";
    }
    o << ')';
  }
};

class ProductExprNode : public ExprNodeBase<ProductExprNode> {
public:
  std::vector<PExprNode> Elements;
  PProductType ExprType;

  ProductExprNode(SourceInterval Loc, std::vector<PExprNode> Elements)
      : ExprNodeBase<ProductExprNode>(Loc), Elements(std::move(Elements)) {}

  PType getType() override { return ExprType; }

  void dump(std::ostream &o) override {
    o << '(';
    for (auto &E : Elements) {
      E->dump(o);
      o << ',';
    }
    o << ')';
  }
};

class IdentExprNode : public ExprNodeBase<IdentExprNode>  {
public:
  std::string Name;
  PType ExprType;

  IdentExprNode(SourceInterval Loc, std::string Name)
      : ExprNodeBase<IdentExprNode>(Loc), Name(std::move(Name)) {}

  PType getType() override { return ExprType; }

  void dump(std::ostream &o) override { o << Name; }
};

class IntLiteralExprNode : public ExprNodeBase<IntLiteralExprNode> {
public:
  int Value;
  static inline PType ExprType = std::make_shared<BuiltinAtomType>(BuiltinAtomTypeKind::INT);

  IntLiteralExprNode(SourceInterval Loc, int Value)
      : ExprNodeBase<IntLiteralExprNode>(Loc), Value(Value) {}

  PType getType() override { return ExprType; }

  void dump(std::ostream &o) override { o << Value; }
};

using PIntLiteralExprNode = std::unique_ptr<IntLiteralExprNode>;

void Visitor::visit(LocalVarDeclNode *Node) {}
void Visitor::visit(FunctionDefNode *Node) {
  for (auto &P: Node->Params)
    P->accept(*this);
  Node->InitValue->accept(*this);
}
void Visitor::visit(BlockExprNode *Node) {
  for (auto &E : Node->Exprs)
    E->accept(*this);
}
void Visitor::visit(LocalVarDefNode *Node) {
  Node->InitValue->accept(*this);
}
void Visitor::visit(LetInExprNode *Node) {
  for (auto &L : Node->LocalVars)
    L->accept(*this);
  Node->InnerExpr->accept(*this);
}
void Visitor::visit(IfExprNode *Node) {
  Node->CondExpr->accept(*this);
  Node->ThenExpr->accept(*this);
  Node->ElseExpr->accept(*this);
}
void Visitor::visit(AllocaExprNode *Node) {
  Node->AllocaSize->accept(*this);
}
void Visitor::visit(CallExprNode *Node) {
  Node->Callee->accept(*this);
  for (auto &A : Node->Args)
    A->accept(*this);
}
void Visitor::visit(ProductExprNode *Node) {
  for (auto &E : Node->Elements)
    E->accept(*this);
}
void Visitor::visit(IdentExprNode *Node) {}
void Visitor::visit(IntLiteralExprNode *Node) {}

} // namespace AST
