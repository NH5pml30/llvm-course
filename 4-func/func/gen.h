#pragma once

#include <map>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>

#include "../memory.h"

class gen {
public:
  void run(std::istream &i);
  void executeIR(memory &cpu);

  llvm::LLVMContext context;
  llvm::Module *module{};
  llvm::Function *main_func{};
  llvm::GlobalVariable *runtime_context{};
  llvm::GlobalVariable *reg_file{};
  llvm::GlobalVariable *stack_mem{};
  llvm::GlobalVariable *stack{};
  llvm::IRBuilder<> builder = llvm::IRBuilder<>(context);
  llvm::BasicBlock *default_dest{};

  void fixup_bb(llvm::User::op_iterator iter, const std::string &name);
  std::string split_bb();

private:
  llvm::BasicBlock *create_bb(const std::string &name);

  std::map<llvm::User::op_iterator, std::string> fixup_labels;
  std::map<std::string, std::pair<uint32_t, llvm::BasicBlock *>> BBs;
};