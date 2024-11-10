#include <tuple>
#include <utility>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include "gen.h"
#include "../util_llvm.h"

extern "C" {
#include "../sim.h"
}
#include "../isa.h"

struct gen_full_reader : assembler_reader {
  llvm::IRBuilder<> &builder;
  llvm::GlobalVariable *reg_file;

  struct proxy {
    llvm::Value *read;
    llvm::Value *write;
  };

  using assembler_reader::read;
  template<typename T>
  auto read(imm<T>) {
    T value = assembler_reader::read(imm<T>{});
    auto *val = value_from_int(builder, value);
    return proxy{val, nullptr};
  }
  auto read(reg) {
    auto *val = builder.CreateConstGEP2_64(reg_file->getValueType(), reg_file,
                                           0, assembler_reader::read(reg{}));
    return proxy{
        builder.CreateLoad(llvm_type<sword_t>::get(builder.getContext()), val),
        val
    };
  }
  auto read(reg_ptr) {
    auto *ptr = builder.CreateLoad(llvm_type<void *>::get(builder.getContext()),
                                   read(reg{}).write);
    return proxy{
        builder.CreateLoad(llvm_type<sword_t>::get(builder.getContext()), ptr),
        ptr};
  }
};

void gen::fixup_bb(llvm::User::op_iterator iter, const std::string &name) {
  fixup_labels[iter] = name;
}

std::string gen::split_bb() {
  auto name = builder.GetInsertBlock()->getName().str() + ".c";
  create_bb(name);
  return name;
}

llvm::BasicBlock *gen::create_bb(const std::string &name) {
  llvm::BasicBlock *old_BB = builder.GetInsertBlock(), *BB = nullptr;
  if (old_BB && old_BB->empty()) {
    BB = old_BB;
    old_BB = nullptr;
    BB->setName(name);
  } else {
    BB = llvm::BasicBlock::Create(context, name, main_func);
  }
  BBs[name] = {BBs.size(), BB};
  if (old_BB && !old_BB->getTerminator())
    builder.CreateBr(BB);
  builder.SetInsertPoint(BB);
  return BB;
}

void gen::run(std::istream &i) {
  module = new llvm::Module("top", context);

  module->getOrInsertGlobal("reg_file", llvm_type<sword_t[]>::get(context, ctx_regs_stack::REG_SIZE));
  reg_file = module->getNamedGlobal("reg_file");

  llvm::FunctionType *main_type = llvm_type<void()>::get(context);
  main_func = llvm::Function::Create(main_type, llvm::Function::ExternalLinkage,
                                     "main", module);

  reg_type = builder.getIntNTy(CHAR_BIT * sizeof(sword_t));

  printf = module->getOrInsertFunction(
      "printf", llvm::FunctionType::get(builder.getInt32Ty(),
                                        {
                                            builder.getInt8Ty()->getPointerTo(),
                                        },
                                        true));

  simRand = module->getOrInsertFunction(
      "simRand", llvm_type<decltype(::simRand)>::get(context));
  simFlush = module->getOrInsertFunction(
      "simFlush", llvm_type<decltype(::simFlush)>::get(context));
  simPutPixel = module->getOrInsertFunction(
      "simPutPixel", llvm_type<decltype(::simPutPixel)>::get(context));
  simClear = module->getOrInsertFunction(
      "simClear", llvm_type<decltype(::simClear)>::get(context));

  default_dest = create_bb("__entry");

  std::string inst;
  while (i >> inst) {
    if (inst.empty())
      break;

    std::cout << "inst: " << inst << std::endl;

    if (inst.ends_with(':')) {
      auto name = inst.substr(0, inst.size() - 1);
      create_bb(name);
      continue;
    }

    bool found = find_inst<bool>(
      [&inst]<typename Inst>(Inst) { return Inst::name == inst; },
      [&]<typename Inst>(Inst) {
        gen_full_reader reader{i, builder, reg_file};
        auto arg_values = read_args_storage(reader, typename Inst::args_t{});
        std::apply(
            []<typename... Ts>(Ts &&...args) {
              Inst::gen(std::forward<Ts>(args)...);
            },
            std::tuple_cat(std::tuple<gen &>(*this), arg_values.tuple()));
        return true;
      });

    if (!found) {
      std::cerr << "Unknown instruction: " << inst << std::endl;
      exit(1);
    }
  }

  for (auto &&[iter, label] : fixup_labels)
    iter->set(BBs[label].second);

  llvm::EliminateUnreachableBlocks(*main_func);

  module->print(llvm::outs(), nullptr);

  bool verif = llvm::verifyFunction(*main_func, &llvm::outs());
  std::cout << "[VERIFICATION] " << (!verif ? "OK\n\n" : "FAIL\n\n");
}

void gen::executeIR(ctx_regs &cpu) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  llvm::ExecutionEngine *ee =
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module)).create();
  using namespace std::string_literals;
  ee->InstallLazyFunctionCreator([=](const std::string &fn_name) -> void * {
    if (fn_name == "simFlush")
      return reinterpret_cast<void *>(::simFlush);
    else if (fn_name == "simRand")
      return reinterpret_cast<void *>(::simRand);
    else if (fn_name == "simPutPixel")
      return reinterpret_cast<void *>(::simPutPixel);
    else if (fn_name == "simClear")
      return reinterpret_cast<void *>(::simClear);
    return nullptr;
  });

  ee->addGlobalMapping(reg_file, (void *)cpu.reg_file);
  ee->finalizeObject();

  simInit();

  llvm::ArrayRef<llvm::GenericValue> noargs;
  ee->runFunction(main_func, noargs);

  simExit();
}
