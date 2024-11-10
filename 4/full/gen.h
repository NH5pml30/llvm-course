#pragma once

#include <llvm/IR/GlobalVariable.h>
#include <map>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>

#include "../contexts.h"

// class to generate full LLVM IR with stack while simulating reg_file
class gen {
public:
  void run(std::istream &i);
  void executeIR(ctx_regs &cpu);

  llvm::LLVMContext context;
  llvm::Module *module{};
  llvm::Function *main_func{};
  llvm::GlobalVariable *reg_file{};
  llvm::IRBuilder<> builder = llvm::IRBuilder<>(context);
  llvm::BasicBlock *default_dest{}; // to reserve branch inst operands

  llvm::Type *reg_type{};

  llvm::FunctionCallee simRand;
  llvm::FunctionCallee simFlush;
  llvm::FunctionCallee simPutPixel;
  llvm::FunctionCallee simClear;
  llvm::FunctionCallee printf;

  void fixup_bb(llvm::User::op_iterator iter, const std::string &name);
  std::string split_bb();

private:
  llvm::BasicBlock *create_bb(const std::string &name);

  std::map<llvm::User::op_iterator, std::string> fixup_labels;
  std::map<std::string, std::pair<word_t, llvm::BasicBlock *>> BBs;
};