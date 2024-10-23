
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
  // source_filename = "/home/nh5/work/llvm/SDL/app.c"
  Module *module = new Module("/home/nh5/work/llvm/SDL/app.c", ctx);
  IRBuilder<> builder(ctx);
  // function 'simRand'
  Type *f_simRandRetType = Type::getInt32Ty(ctx);
  FunctionType *f_simRandType = FunctionType::get(f_simRandRetType, false);
  FunctionCallee f_simRand = module->getOrInsertFunction("simRand", f_simRandType);

  // function 'llvm_lifetime_start_p0'
  Type *f_llvm_lifetime_start_p0RetType = Type::getVoidTy(ctx);
  Type *f_llvm_lifetime_start_p0ParamTypes[] = {
    Type::getInt64Ty(ctx),
    PointerType::getUnqual(ctx),
  };
  FunctionType *f_llvm_lifetime_start_p0Type = FunctionType::get(f_llvm_lifetime_start_p0RetType, f_llvm_lifetime_start_p0ParamTypes, false);
  FunctionCallee f_llvm_lifetime_start_p0 = module->getOrInsertFunction("llvm.lifetime.start.p0", f_llvm_lifetime_start_p0Type);

  // function 'llvm_memset_p0_i64'
  Type *f_llvm_memset_p0_i64RetType = Type::getVoidTy(ctx);
  Type *f_llvm_memset_p0_i64ParamTypes[] = {
    PointerType::getUnqual(ctx),
    Type::getInt8Ty(ctx),
    Type::getInt64Ty(ctx),
    Type::getInt1Ty(ctx),
  };
  FunctionType *f_llvm_memset_p0_i64Type = FunctionType::get(f_llvm_memset_p0_i64RetType, f_llvm_memset_p0_i64ParamTypes, false);
  FunctionCallee f_llvm_memset_p0_i64 = module->getOrInsertFunction("llvm.memset.p0.i64", f_llvm_memset_p0_i64Type);

  // function 'simClear'
  Type *f_simClearRetType = Type::getVoidTy(ctx);
  Type *f_simClearParamTypes[] = {
    Type::getInt32Ty(ctx),
  };
  FunctionType *f_simClearType = FunctionType::get(f_simClearRetType, f_simClearParamTypes, false);
  FunctionCallee f_simClear = module->getOrInsertFunction("simClear", f_simClearType);

  // function 'simPutPixel'
  Type *f_simPutPixelRetType = Type::getVoidTy(ctx);
  Type *f_simPutPixelParamTypes[] = {
    Type::getInt32Ty(ctx),
    Type::getInt32Ty(ctx),
    Type::getInt32Ty(ctx),
  };
  FunctionType *f_simPutPixelType = FunctionType::get(f_simPutPixelRetType, f_simPutPixelParamTypes, false);
  FunctionCallee f_simPutPixel = module->getOrInsertFunction("simPutPixel", f_simPutPixelType);

  // function 'simFlush'
  Type *f_simFlushRetType = Type::getVoidTy(ctx);
  FunctionType *f_simFlushType = FunctionType::get(f_simFlushRetType, false);
  FunctionCallee f_simFlush = module->getOrInsertFunction("simFlush", f_simFlushType);

  // function 'generateStar'
  Type *f_generateStarRetType = Type::getVoidTy(ctx);
  Type *f_generateStarParamTypes[] = {
    PointerType::getUnqual(ctx),
  };
  FunctionType *f_generateStarType = FunctionType::get(f_generateStarRetType, f_generateStarParamTypes, false);
  Function *f_generateStar = Function::Create(f_generateStarType, Function::ExternalLinkage, "generateStar", module);
  {
    Value *r_0 = f_generateStar->getArg(0);
    BasicBlock *b_1 = BasicBlock::Create(ctx, "", f_generateStar);
    // basic block '1'
    builder.SetInsertPoint(b_1);
    // %2 = tail call i32 @simRand() #5
    Value *r_2 = builder.CreateCall(f_simRand);
    // %3 = srem i32 %2, 200
    Value *r_3 = builder.CreateSRem(r_2, ConstantInt::get(Type::getInt32Ty(ctx), 200, 10));
    // %4 = add nsw i32 %3, -100
    Value *r_4 = builder.CreateAdd(r_3, ConstantInt::get(Type::getInt32Ty(ctx), -100, 10), "", false, true);
    // store i32 %4, ptr %0, align 4, !tbaa !5
    Value *r_u1 = builder.CreateAlignedStore(r_4, r_0, MaybeAlign(4));
    // %5 = tail call i32 @simRand() #5
    Value *r_5 = builder.CreateCall(f_simRand);
    // %6 = srem i32 %5, 200
    Value *r_6 = builder.CreateSRem(r_5, ConstantInt::get(Type::getInt32Ty(ctx), 200, 10));
    // %7 = add nsw i32 %6, -100
    Value *r_7 = builder.CreateAdd(r_6, ConstantInt::get(Type::getInt32Ty(ctx), -100, 10), "", false, true);
    // %8 = getelementptr inbounds i32, ptr %0, i64 1
    Value *i_u3[] = {
      ConstantInt::get(Type::getInt64Ty(ctx), 1, 10),
    };
    Value *r_8 = builder.CreateInBoundsGEP(Type::getInt32Ty(ctx), r_0, i_u3);
    // store i32 %7, ptr %8, align 4, !tbaa !5
    Value *r_u4 = builder.CreateAlignedStore(r_7, r_8, MaybeAlign(4));
    // %9 = getelementptr inbounds i32, ptr %0, i64 2
    Value *i_u5[] = {
      ConstantInt::get(Type::getInt64Ty(ctx), 2, 10),
    };
    Value *r_9 = builder.CreateInBoundsGEP(Type::getInt32Ty(ctx), r_0, i_u5);
    // store i32 100, ptr %9, align 4, !tbaa !5
    Value *r_u6 = builder.CreateAlignedStore(ConstantInt::get(Type::getInt32Ty(ctx), 100, 10), r_9, MaybeAlign(4));
    // ret void
    builder.CreateRetVoid();
  }

  // function 'app'
  Type *f_appRetType = Type::getVoidTy(ctx);
  FunctionType *f_appType = FunctionType::get(f_appRetType, false);
  Function *f_app = Function::Create(f_appType, Function::ExternalLinkage, "app", module);
  {
    BasicBlock *b_0 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_2 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_5 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_11 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_12 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_15 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_16 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_22 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_24 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_39 = BasicBlock::Create(ctx, "", f_app);
    BasicBlock *b_41 = BasicBlock::Create(ctx, "", f_app);
    // basic block '0'
    builder.SetInsertPoint(b_0);
    // %1 = alloca [200 x [3 x i32]], align 16
    AllocaInst *r_1 = builder.CreateAlloca(ArrayType::get(ArrayType::get(Type::getInt32Ty(ctx), 3), 200));
    r_1->setAlignment(Align(16));
    // call void @llvm.lifetime.start.p0(i64 2400, ptr nonnull %1) #6
    Value *a_u8[] = {
      ConstantInt::get(Type::getInt64Ty(ctx), 2400, 10),
      r_1,
    };
    Value *r_u7 = builder.CreateCall(f_llvm_lifetime_start_p0, a_u8);
    // call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(2400) %1, i8 0, i64 2400, i1 false)
    Value *a_u10[] = {
      r_1,
      ConstantInt::get(Type::getInt8Ty(ctx), 0, 10),
      ConstantInt::get(Type::getInt64Ty(ctx), 2400, 10),
      ConstantInt::getFalse(ctx),
    };
    Value *r_u9 = builder.CreateCall(f_llvm_memset_p0_i64, a_u10);
    // br label %2
    builder.CreateBr(b_2);
    // basic block '2'
    builder.SetInsertPoint(b_2);
    // %3 = phi i64 [ %10, %5 ], [ 0, %0 ]
    PHINode *p_u11 = builder.CreatePHI(Type::getInt64Ty(ctx), 2);
    Value *r_3 = p_u11;
    // %4 = icmp eq i64 %3, 200
    Value *r_4 = builder.CreateICmpEQ(r_3, ConstantInt::get(Type::getInt64Ty(ctx), 200, 10));
    // br i1 %4, label %11, label %5
    builder.CreateCondBr(r_4, b_11, b_5);
    // basic block '5'
    builder.SetInsertPoint(b_5);
    // %6 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %3
    Value *i_u12[] = {
      ConstantInt::get(Type::getInt64Ty(ctx), 0, 10),
      r_3,
    };
    Value *r_6 = builder.CreateInBoundsGEP(ArrayType::get(ArrayType::get(Type::getInt32Ty(ctx), 3), 200), r_1, i_u12);
    // call void @generateStar(ptr noundef nonnull %6) #7
    Value *a_u14[] = {
      r_6,
    };
    Value *r_u13 = builder.CreateCall(f_generateStar, a_u14);
    // %7 = tail call i32 @simRand() #5
    Value *r_7 = builder.CreateCall(f_simRand);
    // %8 = srem i32 %7, 100
    Value *r_8 = builder.CreateSRem(r_7, ConstantInt::get(Type::getInt32Ty(ctx), 100, 10));
    // %9 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %3, i64 2
    Value *i_u16[] = {
      ConstantInt::get(Type::getInt64Ty(ctx), 0, 10),
      r_3,
      ConstantInt::get(Type::getInt64Ty(ctx), 2, 10),
    };
    Value *r_9 = builder.CreateInBoundsGEP(ArrayType::get(ArrayType::get(Type::getInt32Ty(ctx), 3), 200), r_1, i_u16);
    // store i32 %8, ptr %9, align 4, !tbaa !5
    Value *r_u17 = builder.CreateAlignedStore(r_8, r_9, MaybeAlign(4));
    // %10 = add nuw nsw i64 %3, 1
    Value *r_10 = builder.CreateAdd(r_3, ConstantInt::get(Type::getInt64Ty(ctx), 1, 10), "", true, true);
    // br label %2, !llvm.loop !9
    builder.CreateBr(b_2);
    // basic block '11'
    builder.SetInsertPoint(b_11);
    // tail call void @simClear(i32 noundef 0) #5
    Value *a_u19[] = {
      ConstantInt::get(Type::getInt32Ty(ctx), 0, 10),
    };
    Value *r_u18 = builder.CreateCall(f_simClear, a_u19);
    // br label %12
    builder.CreateBr(b_12);
    // basic block '12'
    builder.SetInsertPoint(b_12);
    // %13 = phi i64 [ %42, %41 ], [ 0, %11 ]
    PHINode *p_u20 = builder.CreatePHI(Type::getInt64Ty(ctx), 2);
    Value *r_13 = p_u20;
    // %14 = icmp eq i64 %13, 200
    Value *r_14 = builder.CreateICmpEQ(r_13, ConstantInt::get(Type::getInt64Ty(ctx), 200, 10));
    // br i1 %14, label %15, label %16
    builder.CreateCondBr(r_14, b_15, b_16);
    // basic block '15'
    builder.SetInsertPoint(b_15);
    // tail call void @simFlush() #5
    Value *r_u21 = builder.CreateCall(f_simFlush);
    // br label %11
    builder.CreateBr(b_11);
    // basic block '16'
    builder.SetInsertPoint(b_16);
    // %17 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %13
    Value *i_u23[] = {
      ConstantInt::get(Type::getInt64Ty(ctx), 0, 10),
      r_13,
    };
    Value *r_17 = builder.CreateInBoundsGEP(ArrayType::get(ArrayType::get(Type::getInt32Ty(ctx), 3), 200), r_1, i_u23);
    // %18 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %13, i64 2
    Value *i_u24[] = {
      ConstantInt::get(Type::getInt64Ty(ctx), 0, 10),
      r_13,
      ConstantInt::get(Type::getInt64Ty(ctx), 2, 10),
    };
    Value *r_18 = builder.CreateInBoundsGEP(ArrayType::get(ArrayType::get(Type::getInt32Ty(ctx), 3), 200), r_1, i_u24);
    // %19 = load i32, ptr %18, align 4, !tbaa !5
    Value *r_19 = builder.CreateAlignedLoad(Type::getInt32Ty(ctx), r_18, MaybeAlign(4));
    // %20 = add nsw i32 %19, -1
    Value *r_20 = builder.CreateAdd(r_19, ConstantInt::get(Type::getInt32Ty(ctx), -1, 10), "", false, true);
    // store i32 %20, ptr %18, align 4, !tbaa !5
    Value *r_u25 = builder.CreateAlignedStore(r_20, r_18, MaybeAlign(4));
    // %21 = icmp slt i32 %19, 2
    Value *r_21 = builder.CreateICmpSLT(r_19, ConstantInt::get(Type::getInt32Ty(ctx), 2, 10));
    // br i1 %21, label %22, label %24
    builder.CreateCondBr(r_21, b_22, b_24);
    // basic block '22'
    builder.SetInsertPoint(b_22);
    // call void @generateStar(ptr noundef nonnull %17) #7
    Value *a_u27[] = {
      r_17,
    };
    Value *r_u26 = builder.CreateCall(f_generateStar, a_u27);
    // %23 = load i32, ptr %18, align 4, !tbaa !5
    Value *r_23 = builder.CreateAlignedLoad(Type::getInt32Ty(ctx), r_18, MaybeAlign(4));
    // br label %24
    builder.CreateBr(b_24);
    // basic block '24'
    builder.SetInsertPoint(b_24);
    // %25 = phi i32 [ %23, %22 ], [ %20, %16 ]
    PHINode *p_u28 = builder.CreatePHI(Type::getInt32Ty(ctx), 2);
    Value *r_25 = p_u28;
    // %26 = load i32, ptr %17, align 4, !tbaa !5
    Value *r_26 = builder.CreateAlignedLoad(Type::getInt32Ty(ctx), r_17, MaybeAlign(4));
    // %27 = shl nsw i32 %26, 8
    Value *r_27 = builder.CreateShl(r_26, ConstantInt::get(Type::getInt32Ty(ctx), 8, 10), "", false, true);
    // %28 = sdiv i32 %27, %25
    Value *r_28 = builder.CreateSDiv(r_27, r_25);
    // %29 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %13, i64 1
    Value *i_u29[] = {
      ConstantInt::get(Type::getInt64Ty(ctx), 0, 10),
      r_13,
      ConstantInt::get(Type::getInt64Ty(ctx), 1, 10),
    };
    Value *r_29 = builder.CreateInBoundsGEP(ArrayType::get(ArrayType::get(Type::getInt32Ty(ctx), 3), 200), r_1, i_u29);
    // %30 = load i32, ptr %29, align 4, !tbaa !5
    Value *r_30 = builder.CreateAlignedLoad(Type::getInt32Ty(ctx), r_29, MaybeAlign(4));
    // %31 = shl nsw i32 %30, 7
    Value *r_31 = builder.CreateShl(r_30, ConstantInt::get(Type::getInt32Ty(ctx), 7, 10), "", false, true);
    // %32 = sdiv i32 %31, %25
    Value *r_32 = builder.CreateSDiv(r_31, r_25);
    // %33 = add i32 %28, 256
    Value *r_33 = builder.CreateAdd(r_28, ConstantInt::get(Type::getInt32Ty(ctx), 256, 10));
    // %34 = icmp ult i32 %33, 512
    Value *r_34 = builder.CreateICmpULT(r_33, ConstantInt::get(Type::getInt32Ty(ctx), 512, 10));
    // %35 = icmp sgt i32 %32, -129
    Value *r_35 = builder.CreateICmpSGT(r_32, ConstantInt::get(Type::getInt32Ty(ctx), -129, 10));
    // %36 = select i1 %34, i1 %35, i1 false
    Value *r_36 = builder.CreateSelect(r_34, r_35, ConstantInt::getFalse(ctx));
    // %37 = icmp slt i32 %32, 128
    Value *r_37 = builder.CreateICmpSLT(r_32, ConstantInt::get(Type::getInt32Ty(ctx), 128, 10));
    // %38 = select i1 %36, i1 %37, i1 false
    Value *r_38 = builder.CreateSelect(r_36, r_37, ConstantInt::getFalse(ctx));
    // br i1 %38, label %39, label %41
    builder.CreateCondBr(r_38, b_39, b_41);
    // basic block '39'
    builder.SetInsertPoint(b_39);
    // %40 = add nsw i32 %32, 128
    Value *r_40 = builder.CreateAdd(r_32, ConstantInt::get(Type::getInt32Ty(ctx), 128, 10), "", false, true);
    // tail call void @simPutPixel(i32 noundef %33, i32 noundef %40, i32 noundef -1) #5
    Value *a_u31[] = {
      r_33,
      r_40,
      ConstantInt::get(Type::getInt32Ty(ctx), -1, 10),
    };
    Value *r_u30 = builder.CreateCall(f_simPutPixel, a_u31);
    // br label %41
    builder.CreateBr(b_41);
    // basic block '41'
    builder.SetInsertPoint(b_41);
    // %42 = add nuw nsw i64 %13, 1
    Value *r_42 = builder.CreateAdd(r_13, ConstantInt::get(Type::getInt64Ty(ctx), 1, 10), "", true, true);
    // br label %12, !llvm.loop !11
    builder.CreateBr(b_12);
    p_u11->addIncoming(r_10, b_5);
    p_u11->addIncoming(ConstantInt::get(Type::getInt64Ty(ctx), 0, 10), b_0);
    p_u20->addIncoming(r_42, b_41);
    p_u20->addIncoming(ConstantInt::get(Type::getInt64Ty(ctx), 0, 10), b_11);
    p_u28->addIncoming(r_23, b_22);
    p_u28->addIncoming(r_20, b_16);
  }


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
