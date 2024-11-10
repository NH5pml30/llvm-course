#pragma once

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

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
struct llvm_type<int64_t> {
  static llvm::Type *get(llvm::LLVMContext &ctx) {
    return llvm::Type::getInt64Ty(ctx);
  }
};

template<>
struct llvm_type<uint64_t> {
  static llvm::Type *get(llvm::LLVMContext &ctx) {
    return llvm::Type::getInt64Ty(ctx);
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

template<typename T>
static llvm::Constant *value_from_int(llvm::IRBuilder<> &builder, T val) {
  return builder.getIntN(CHAR_BIT * sizeof(T), val);
}
