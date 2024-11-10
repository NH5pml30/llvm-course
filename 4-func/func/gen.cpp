
#include <limits>
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

extern "C" {
#include "../sim.h"
}
#include "../isa.h"

template<typename Inst>
struct InstWithMappedArgs {
  template<size_t I>
  using arg_i_t = arg_repr_t<ram_io, std::tuple_element_t<I, typename Inst::args_t>>;

  using FuncT = decltype(read_args_storage(std::declval<ram_io &>(),
                                           typename Inst::args_t{}))::template FuncT<memory>;

  template<size_t I>
  static decltype(auto) map_arg(memory &ctx, arg_i_t<I> arg) {
    using tag_t = std::tuple_element_t<I, typename Inst::args_t>;
    memory_reader reader{(unsigned char *)&arg, ctx};
    return reader.read(tag_t{});
  }

  template<typename ...Ts>
  static void exec(memory &ctx, Ts... args) {
    []<size_t ...Is>(memory &ctx, Ts... args, std::index_sequence<Is...>) {
      Inst::exec(ctx, map_arg<Is>(ctx, args)...);
    }(ctx, args..., std::make_index_sequence<sizeof...(args)>());
  }

  static auto get_exec() {
    return []<size_t ...Is>(std::index_sequence<Is...>) {
      return &InstWithMappedArgs::exec<arg_i_t<Is>...>;
    }(std::make_index_sequence<std::tuple_size_v<typename Inst::args_t>>());
  }
};

template<typename X>
struct llvm_type;

template<>
struct llvm_type<unsigned char> {
  static llvm::Type *get(llvm::LLVMContext &ctx) {
    return llvm::Type::getInt8Ty(ctx);
  }
};

template<>
struct llvm_type<int32_t> {
  static llvm::Type *get(llvm::LLVMContext &ctx) {
    return llvm::Type::getInt32Ty(ctx);
  }
};

template<>
struct llvm_type<uint32_t> {
  static llvm::Type *get(llvm::LLVMContext &ctx) {
    return llvm::Type::getInt32Ty(ctx);
  }
};

template<>
struct llvm_type<void> {
  static llvm::Type *get(llvm::LLVMContext &ctx) {
    return llvm::Type::getVoidTy(ctx);
  }
};

template<typename T>
struct llvm_type<T *> {
  static llvm::Type *get(llvm::LLVMContext &ctx) {
    return llvm::PointerType::get(ctx, 0);
  }
};

template<typename T>
struct llvm_type<T &> {
  static llvm::Type *get(llvm::LLVMContext &ctx) {
    return llvm::PointerType::get(ctx, 0);
  }
};

template<typename R, typename ...As>
struct llvm_type<R (As...)> {
  static llvm::FunctionType *get(llvm::LLVMContext &ctx) {
    return llvm::FunctionType::get(llvm_type<R>::get(ctx),
                                   {llvm_type<As>::get(ctx)...}, false);
  }
};

template<typename T>
struct llvm_type<T[]> {
  static llvm::ArrayType *get(llvm::LLVMContext &ctx, size_t N) {
    return llvm::ArrayType::get(llvm_type<T>::get(ctx), N);
  }
};

struct gen_func_writer {
  llvm::IRBuilder<> &builder;
  std::vector<llvm::Value *> args;
  std::map<std::string, std::pair<uint32_t, llvm::BasicBlock *>> BBs;

  template<typename T>
  void write(T val) {
    args.push_back(builder.getIntN(CHAR_BIT * sizeof(T), val));
  }

  template<typename T>
  void write(T val, imm<T>) {
    write<T>(val);
  }

  void write(uint8_t val, reg) {
    write<uint8_t>(val);
  }

  /*void write(const std::string &val, label) {
    uint32_t id{};
    if (auto place = BBs.find(val); place != BBs.end()) {
      id = place->second.first;
    } else {
      id = (uint32_t)BBs.size();
      BBs[val] = {id, nullptr};
    }
    write<uint32_t>(id);
  }*/
};

struct gen_full_reader : assembler_reader {
  llvm::IRBuilder<> &builder;
  llvm::GlobalVariable *reg_file;
  llvm::GlobalVariable *stack_mem;
  llvm::GlobalVariable *stack;

  struct proxy {
    llvm::Value *read;
    llvm::Value *write;
  };

  using assembler_reader::read;
  template<typename T>
  auto read(imm<T>) {
    auto *val = builder.getIntN(CHAR_BIT * sizeof(T), assembler_reader::read(imm<T>{}));
    return proxy{val, nullptr};
  }
  auto read(reg) {
    auto *val = builder.CreateConstGEP2_32(reg_file->getValueType(), reg_file, 0,
                                      assembler_reader::read(reg{}));
    return proxy{builder.CreateLoad(builder.getInt32Ty(), val), val};
  }
  auto read(reg_ptr) {
    auto *val = builder.CreateGEP(stack_mem->getValueType(), stack_mem,
                                  {builder.getInt32(0), read(reg{}).read});
    return proxy{builder.CreateLoad(builder.getInt32Ty(), val), val};
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
  BBs[name] = {(uint32_t)BBs.size(), BB};
  if (old_BB && !old_BB->getTerminator())
    builder.CreateBr(BB);
  builder.SetInsertPoint(BB);
  return BB;
}

void gen::run(std::istream &i) {
  module = new llvm::Module("top", context);

  module->getOrInsertGlobal("runtime_context", llvm_type<void *>::get(context));
  runtime_context = module->getNamedGlobal("runtime_context");

  module->getOrInsertGlobal("reg_file", llvm_type<int32_t[]>::get(context, memory::REG_SIZE));
  reg_file = module->getNamedGlobal("reg_file");

  module->getOrInsertGlobal("stack_mem", llvm_type<unsigned char[]>::get(context, memory::STACK_SIZE));
  stack_mem = module->getNamedGlobal("stack_mem");

  module->getOrInsertGlobal("stack", llvm_type<uint32_t>::get(context));
  stack = module->getNamedGlobal("stack");
  stack->setInitializer(builder.getInt32(memory::STACK_SIZE));

  llvm::FunctionType *main_type = llvm_type<void()>::get(context);
  main_func = llvm::Function::Create(main_type, llvm::Function::ExternalLinkage,
                                     "main", module);

  default_dest = create_bb("__entry");
  auto *ctx_arg =
      builder.CreateLoad(llvm_type<void *>::get(context), runtime_context);

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
        if constexpr (is_control_flow(Inst{})) {
          gen_full_reader reader{i, builder, reg_file, stack_mem, stack};
          auto arg_values = read_args_storage(reader, typename Inst::args_t{});
          std::apply(
              []<typename... Ts>(Ts &&...args) {
                Inst::gen(std::forward<Ts>(args)...);
              },
              std::tuple_cat(std::tuple<gen &>(*this), arg_values.tuple()));
          return true;
        } else {
          assembler_reader reader{i};
          gen_func_writer writer{builder};
          writer.args.push_back(ctx_arg);
          auto arg_values =
              read_args_storage(reader, typename Inst::args_t{});
          arg_values.write(writer);
          using namespace std::string_literals;
          llvm::FunctionCallee callee = module->getOrInsertFunction(
              "do_"s + Inst::name,
              llvm_type<typename InstWithMappedArgs<Inst>::FuncT>::get(context));
          builder.CreateCall(callee, writer.args);
          return true;
        }
        return false;
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

void gen::executeIR(memory &cpu) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  llvm::ExecutionEngine *ee =
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module)).create();
  using namespace std::string_literals;
  ee->InstallLazyFunctionCreator([=](const std::string &fn_name) -> void * {
    return find_inst<void *>(
        [&]<typename Inst>(Inst) { return fn_name == "do_"s + Inst::name; },
        [&]<typename Inst>(Inst) {
          if constexpr (!is_control_flow(Inst{})) {
            return reinterpret_cast<void *>(InstWithMappedArgs<Inst>::get_exec());
          } else {
            std::cerr << "Invalid state\n";
            exit(1);
            return nullptr;
          }
        });
  });

  void *runtime_context_value = (void *)&cpu;

  ee->addGlobalMapping(runtime_context, (void *)&runtime_context_value);
  ee->addGlobalMapping(reg_file, (void *)cpu.reg_file);
  ee->addGlobalMapping(stack_mem, (void *)cpu.stack_mem);
  ee->addGlobalMapping(stack, (void *)&cpu.stack);
  ee->finalizeObject();

  simInit();

  llvm::ArrayRef<llvm::GenericValue> noargs;
  ee->runFunction(main_func, noargs);

  simExit();
}