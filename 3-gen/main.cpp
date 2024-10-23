#include <ANTLRInputStream.h>
#include <CharStream.h>
#include <CommonTokenStream.h>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>

#include "IRLexer.h"
#include "IRBaseVisitor.h"

using namespace three;

namespace {

void printType(std::ostream &o, IRParser::TypeContext *ctx) {
  if (ctx->leafType()) {
    if (ctx->leafType()->i1)
      o << "Type::getInt1Ty(ctx)";
    else if (ctx->leafType()->i8)
      o << "Type::getInt8Ty(ctx)";
    else if (ctx->leafType()->i32)
      o << "Type::getInt32Ty(ctx)";
    else if (ctx->leafType()->i64)
      o << "Type::getInt64Ty(ctx)";
    else if (ctx->leafType()->p)
      o << "PointerType::getUnqual(ctx)";
    else if (ctx->leafType()->v)
      o << "Type::getVoidTy(ctx)";
    else
      assert(false && "unreachable");
    return;
  }

  o << "ArrayType::get(";
  printType(o, ctx->type());
  o << ", " << ctx->DECIMAL()->getText() << ")";
}

std::string token2name(antlr4::tree::TerminalNode *tok) {
  return tok->getText().substr(1);
}

std::string token2name(antlr4::Token *tok) {
  return tok->getText().substr(1);
}

std::string escapeIdent(std::string res) {
  std::replace(res.begin(), res.end(), '.', '_');
  return res;
}

std::string token2ident(antlr4::tree::TerminalNode *tok) {
  return escapeIdent(token2name(tok));
}

std::string token2ident(antlr4::Token *tok) {
  return escapeIdent(token2name(tok));
}

void printValue(std::ostream &o, IRParser::TypeContext *t, IRParser::ValueContext *ctx) {
  if (ctx->REG()) {
    o << "r_" << token2ident(ctx->REG());
  } else if (auto *c = ctx->const_()) {
    if (c->DECIMAL()) {
      o << "ConstantInt::get(";
      printType(o, t);
      o << ", " << c->DECIMAL()->getText() << ", 10)";
    } else if (c->NEG_DECIMAL()) {
      o << "ConstantInt::get(";
      printType(o, t);
      o << ", " << c->NEG_DECIMAL()->getText() << ", 10)";
    } else if (c->fls)
      o << "ConstantInt::getFalse(ctx)";
    else if (c->tru)
      o << "ConstantInt::getTrue(ctx)";
    else
      assert(false && "unreachable");
  } else {
    assert(false && "unreachable");
  }
}

void printSource(std::ostream &o, int indent, antlr4::ParserRuleContext *ctx) {
  for (int i = 0; i < indent; i++)
    o << "  ";
  o << "// "
    << ctx->getStart()->getInputStream()->getText(antlr4::misc::Interval(
           ctx->getStart()->getStartIndex(), ctx->getStop()->getStopIndex()))
    << "\n";
}

class FuncVisitor : public IRBaseVisitor {
  std::ostream &o;
  IRParser::FuncDefContext *funcCtx;
  std::string funcName;
  int uniqueId;
  std::stringstream deferred;

  std::string getBBName(IRParser::RegNameContext *bbCtx) {
    if (bbCtx == nullptr)
      return std::to_string(funcCtx->params.size());
    if (bbCtx->DECIMAL())
      return bbCtx->DECIMAL()->getText();
    return bbCtx->IDENT()->getText();
  }

public:
  FuncVisitor(std::ostream &o, IRParser::FuncDefContext *funcCtx, int uniqueId)
      : o(o), funcCtx(funcCtx),
        funcName(token2ident(funcCtx->GLOBAL())), uniqueId(uniqueId) {}

  int run() {
    o << "  // " << "function '" << funcName << "'\n";
    o << "  Type *f_" << funcName << "RetType = ";
    printType(o, funcCtx->type());
    o << ";\n";
    if (!funcCtx->params.empty()) {
      o << "  Type *f_" << funcName << "ParamTypes[] = {\n";
      for (auto *p : funcCtx->params) {
        o << "    ";
        printType(o, p->type());
        o << ",\n";
      }
      o << "  };\n";
      o << "  FunctionType *f_" << funcName << "Type = FunctionType::get(f_"
        << funcName << "RetType, " << "f_" << funcName << "ParamTypes, false);\n";
    } else {
      o << "  FunctionType *f_" << funcName << "Type = FunctionType::get(f_"
        << funcName << "RetType, false);\n";
    }
    o << "  Function *f_" << funcName << " = Function::Create(f_" << funcName
      << "Type, Function::ExternalLinkage, \"" << token2name(funcCtx->GLOBAL())
      << "\", module);\n";
    o << "  {\n";
    for (size_t i = 0; i < funcCtx->params.size(); i++) {
      auto *p = funcCtx->params[i];
      o << "    Value *r_" << token2ident(p->REG()) << " = f_" << funcName << "->getArg(" << i << ");\n";
    }
    for (auto *bb : funcCtx->basicBlocks) {
      auto bbName = getBBName(bb->regName());
      o << "    BasicBlock *b_" << bbName << " = BasicBlock::Create(ctx, \"\", f_"
        << funcName << ");\n";
    }

    for (auto *bb : funcCtx->basicBlocks)
      visitBasicBlock(bb);

    o << deferred.str();
    o << "  }\n\n";
    return uniqueId;
  }

  std::string getUniqueName() {
    return "u" + std::to_string(uniqueId++);
  }

  std::any visitInnerInst(IRParser::InnerInstContext *ctx) override {
    std::string lhs = "    Value *r_";
    if (ctx->lhs)
      lhs += token2ident(ctx->lhs);
    else
      lhs += getUniqueName();
    lhs += " = ";
    printSource(o, 2, ctx);
    if (ctx->phi) {
      std::string phiName = "p_" + getUniqueName();
      o << "    PHINode *" << phiName << " = " << "builder.CreatePHI(";
      printType(o, ctx->phiType);
      o << ", " << ctx->values.size() << ");\n";
      for (auto [v, l] : std::views::zip(ctx->values, ctx->labels)) {
        deferred << "    " << phiName << "->addIncoming(";
        printValue(deferred, ctx->phiType, v);
        deferred << ", b_" << token2ident(l) << ");\n";
      }
      o << lhs << phiName << ";\n";
    } else if (ctx->alloca) {
      std::string allocaName = "r_" + token2ident(ctx->lhs);
      o << "    AllocaInst *" << allocaName << " = "
        << "builder.CreateAlloca(";
      printType(o, ctx->allocaType);
      o << ");\n";
      o << "    " << allocaName << "->setAlignment(Align("
        << ctx->allocaAlign->getText() << "));\n";
    } else if (auto *bop = ctx->binaryOp()) {
      bool hasNUW = false, hasNSW = false;
      for (auto &attr : ctx->binaryOpsAttrs) {
        if (attr->nuw)
          hasNUW = true;
        if (attr->nsw)
          hasNSW = true;
      }
      o << lhs << "builder.Create";
      if (bop->srem)
        o << "SRem";
      else if (bop->add)
        o << "Add";
      else if (bop->sdiv)
        o << "SDiv";
      else if (bop->shl)
        o << "Shl";
      o << "(";
      printValue(o, ctx->bt, ctx->blhs);
      o << ", ";
      printValue(o, ctx->bt, ctx->brhs);
      if (hasNUW || hasNSW)
        o << ", \"\", " << std::boolalpha << hasNUW << ", " << hasNSW;
      o << ");\n";
    } else if (ctx->store) {
      o << lhs << "builder.CreateAlignedStore(";
      printValue(o, ctx->storeType, ctx->storeValue);
      o << ", r_" << token2ident(ctx->storeReg) << ", MaybeAlign("
        << ctx->storeAlign->getText() << "));\n";
    } else if (ctx->load) {
      o << lhs << "builder.CreateAlignedLoad(";
      printType(o, ctx->loadType);
      o << ", r_" << token2ident(ctx->loadReg) << ", MaybeAlign("
        << ctx->loadAlign->getText() << "));\n";
    } else if (ctx->gep) {
      std::string idxsName = "i_" + getUniqueName();
      o << "    Value *" << idxsName << "[] = {\n";
      for (auto [t, i] : std::views::zip(ctx->gepTypes, ctx->gepIdxs)) {
        o << "      ";
        printValue(o, t, i);
        o << ",\n";
      }
      o << "    };\n";
      o << lhs << "builder.Create";
      if (ctx->inbounds)
        o << "InBounds";
      o << "GEP(";
      printType(o, ctx->gepType);
      o << ", r_" << token2ident(ctx->gepReg) << ", " << idxsName << ");\n";
    } else if (ctx->icmp) {
      std::string predName;
      if (ctx->icmpCond()->ult)
        predName = "ULT";
      else if (ctx->icmpCond()->slt)
        predName = "SLT";
      else if (ctx->icmpCond()->sgt)
        predName = "SGT";
      else if (ctx->icmpCond()->eq)
        predName = "EQ";
      o << lhs << "builder.CreateICmp" << predName << "(";
      printValue(o, ctx->icmpType, ctx->icmpLhs);
      o << ", ";
      printValue(o, ctx->icmpType, ctx->icmpRhs);
      o << ");\n";
    } else if (ctx->select) {
      o << lhs << "builder.CreateSelect(";
      printValue(o, ctx->selectType, ctx->selectValue);
      o << ", ";
      printValue(o, ctx->selLhsType, ctx->selLhs);
      o << ", ";
      printValue(o, ctx->selRhsType, ctx->selRhs);
      o << ");\n";
    } else if (ctx->call) {
      std::string argsName = "a_" + getUniqueName();
      if (!ctx->argValues.empty()) {
        o << "    Value *" << argsName << "[] = {\n";
        for (auto [t, v] : std::views::zip(ctx->argTypes, ctx->argValues)) {
          o << "      ";
          printValue(o, t, v);
          o << ",\n";
        }
        o << "    };\n";
      }
      o << lhs << "builder.CreateCall(f_" << token2ident(ctx->GLOBAL());
      if (!ctx->argValues.empty())
        o << ", " << argsName;
      o << ");\n";
    } else {
      assert(false && "unreachable");
    }

    return {};
  }

  std::any visitTermInst(IRParser::TermInstContext *ctx) override {
    printSource(o, 2, ctx);
    if (ctx->ret) {
      if (auto *v = ctx->rv) {
        o << "    builder.CreateRet(";
        printValue(o, ctx->rt, v);
        o << ");\n";
      } else {
        o << "    builder.CreateRetVoid();\n";
      }
    } else if (ctx->bl) {
      o << "    builder.CreateBr(b_" << token2ident(ctx->br) << ");\n";
    } else {
      o << "    builder.CreateCondBr(";
      printValue(o, ctx->bt, ctx->bv);
      o << ", b_" << token2ident(ctx->brt) << ", b_"
        << token2ident(ctx->brf) << ");\n";
    }

    return {};
  }

  std::any visitBasicBlock(IRParser::BasicBlockContext *ctx) override {
    o << "    // " << "basic block '" << getBBName(ctx->regName()) << "'\n";
    o << "    builder.SetInsertPoint(b_" << getBBName(ctx->regName()) << ");\n";
    for (auto *i : ctx->innerInsts)
      visitInnerInst(i);
    visitTermInst(ctx->termInst());

    return {};
  }
};

class IRGenVisitor : public IRBaseVisitor {
  std::ostream &o;
  int uniqueId = 0;

public:
  IRGenVisitor(std::ostream &o) : o(o) {}

  std::any visitFuncDef(IRParser::FuncDefContext *ctx) override {
    FuncVisitor vis(o, ctx, uniqueId);
    uniqueId = vis.run();

    return {};
  }
};

class DeclVisitor : public IRBaseVisitor {
  std::ostream &o;

public:
  DeclVisitor(std::ostream &o) : o(o) {}

  std::any visitSourceFilename(IRParser::SourceFilenameContext *ctx) override {
    printSource(o, 1, ctx);
    o << "  Module *module = new Module(" << ctx->STRING()->getText()
      << ", ctx);\n";
    o << "  IRBuilder<> builder(ctx);\n";

    return {};
  }

  std::any visitFuncDecl(IRParser::FuncDeclContext *ctx) override {
    std::string funcName = token2ident(ctx->GLOBAL());
    o << "  // " << "function '" << funcName << "'\n";
    o << "  Type *f_" << funcName << "RetType = ";
    printType(o, ctx->type());
    o << ";\n";
    if (!ctx->params.empty()) {
      o << "  Type *f_" << funcName << "ParamTypes[] = {\n";
      for (auto *p : ctx->params) {
        o << "    ";
        printType(o, p->type());
        o << ",\n";
      }
      o << "  };\n";
      o << "  FunctionType *f_" << funcName << "Type = FunctionType::get(f_"
        << funcName << "RetType, " << "f_" << funcName << "ParamTypes, false);\n";
    } else {
      o << "  FunctionType *f_" << funcName << "Type = FunctionType::get(f_"
        << funcName << "RetType, false);\n";
    }
    o << "  FunctionCallee f_" << funcName
      << " = module->getOrInsertFunction(\"" << token2name(ctx->GLOBAL())
      << "\", f_" << funcName << "Type);\n\n";

    return {};
  }
};

}

int main() {
  std::ifstream in("test.ll");
  antlr4::ANTLRInputStream input(in);
  IRLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  IRParser parser(&tokens);

  IRParser::FileContext *tree = parser.file();

  std::cout << R"__delim(
extern "C" {
#include "sim.h"
}

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

int main() {
  LLVMContext ctx;
)__delim";

  {
    DeclVisitor v(std::cout);
    v.visitFile(tree);
  }

  {
    IRGenVisitor v(std::cout);
    v.visitFile(tree);
  }

  std::cout << R"__delim(
  // Dump LLVM IR
  module->print(outs(), nullptr);

  // LLVM IR Interpreter
  outs() << "[EE] Run\n";
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();

  ExecutionEngine *ee = EngineBuilder(std::unique_ptr<Module>(module)).create();
  ee->InstallLazyFunctionCreator([&](const std::string &fnName) -> void * {
    if (fnName == "simPutPixel") {
      return reinterpret_cast<void *>(simPutPixel);
    }
    if (fnName == "simFlush") {
      return reinterpret_cast<void *>(simFlush);
    }
    if (fnName == "simRand") {
      return reinterpret_cast<void *>(simRand);
    }
    if (fnName == "simClear") {
      return reinterpret_cast<void *>(simClear);
    }
    return nullptr;
  });
  ee->finalizeObject();

  simInit();

  ArrayRef<GenericValue> noargs;
  GenericValue v = ee->runFunction(f_app, noargs);
  outs() << "[EE] Result: " << v.IntVal << "\n";

  simExit();
  return EXIT_SUCCESS;
}
)__delim";

  return 0;
}