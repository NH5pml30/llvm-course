; ModuleID = '/home/nh5/work/llvm/SDL/app.c'
source_filename = "/home/nh5/work/llvm/SDL/app.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: minsize nounwind optsize sspstrong uwtable
define dso_local void @generateStar(ptr nocapture noundef writeonly %0) local_unnamed_addr #0 {
  %2 = tail call i32 @simRand() #5
  %3 = srem i32 %2, 200
  %4 = add nsw i32 %3, -100
  store i32 %4, ptr %0, align 4, !tbaa !5
  %5 = tail call i32 @simRand() #5
  %6 = srem i32 %5, 200
  %7 = add nsw i32 %6, -100
  %8 = getelementptr inbounds i32, ptr %0, i64 1
  store i32 %7, ptr %8, align 4, !tbaa !5
  %9 = getelementptr inbounds i32, ptr %0, i64 2
  store i32 100, ptr %9, align 4, !tbaa !5
  ret void
}

; Function Attrs: minsize optsize
declare i32 @simRand() local_unnamed_addr #1

; Function Attrs: minsize noreturn nounwind optsize sspstrong uwtable
define dso_local void @app() local_unnamed_addr #2 {
  %1 = alloca [200 x [3 x i32]], align 16
  call void @llvm.lifetime.start.p0(i64 2400, ptr nonnull %1) #6
  call void @llvm.memset.p0.i64(ptr noundef nonnull align 16 dereferenceable(2400) %1, i8 0, i64 2400, i1 false)
  br label %2

2:                                                ; preds = %5, %0
  %3 = phi i64 [ %10, %5 ], [ 0, %0 ]
  %4 = icmp eq i64 %3, 200
  br i1 %4, label %11, label %5

5:                                                ; preds = %2
  %6 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %3
  call void @generateStar(ptr noundef nonnull %6) #7
  %7 = tail call i32 @simRand() #5
  %8 = srem i32 %7, 100
  %9 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %3, i64 2
  store i32 %8, ptr %9, align 4, !tbaa !5
  %10 = add nuw nsw i64 %3, 1
  br label %2, !llvm.loop !9

11:                                               ; preds = %2, %15
  tail call void @simClear(i32 noundef 0) #5
  br label %12

12:                                               ; preds = %41, %11
  %13 = phi i64 [ %42, %41 ], [ 0, %11 ]
  %14 = icmp eq i64 %13, 200
  br i1 %14, label %15, label %16

15:                                               ; preds = %12
  tail call void @simFlush() #5
  br label %11

16:                                               ; preds = %12
  %17 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %13
  %18 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %13, i64 2
  %19 = load i32, ptr %18, align 4, !tbaa !5
  %20 = add nsw i32 %19, -1
  store i32 %20, ptr %18, align 4, !tbaa !5
  %21 = icmp slt i32 %19, 2
  br i1 %21, label %22, label %24

22:                                               ; preds = %16
  call void @generateStar(ptr noundef nonnull %17) #7
  %23 = load i32, ptr %18, align 4, !tbaa !5
  br label %24

24:                                               ; preds = %22, %16
  %25 = phi i32 [ %23, %22 ], [ %20, %16 ]
  %26 = load i32, ptr %17, align 4, !tbaa !5
  %27 = shl nsw i32 %26, 8
  %28 = sdiv i32 %27, %25
  %29 = getelementptr inbounds [200 x [3 x i32]], ptr %1, i64 0, i64 %13, i64 1
  %30 = load i32, ptr %29, align 4, !tbaa !5
  %31 = shl nsw i32 %30, 7
  %32 = sdiv i32 %31, %25
  %33 = add i32 %28, 256
  %34 = icmp ult i32 %33, 512
  %35 = icmp sgt i32 %32, -129
  %36 = select i1 %34, i1 %35, i1 false
  %37 = icmp slt i32 %32, 128
  %38 = select i1 %36, i1 %37, i1 false
  br i1 %38, label %39, label %41

39:                                               ; preds = %24
  %40 = add nsw i32 %32, 128
  tail call void @simPutPixel(i32 noundef %33, i32 noundef %40, i32 noundef -1) #5
  br label %41

41:                                               ; preds = %39, %24
  %42 = add nuw nsw i64 %13, 1
  br label %12, !llvm.loop !11
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #3

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #4

; Function Attrs: minsize optsize
declare void @simClear(i32 noundef) local_unnamed_addr #1

; Function Attrs: minsize optsize
declare void @simPutPixel(i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: minsize optsize
declare void @simFlush() local_unnamed_addr #1

attributes #0 = { minsize nounwind optsize sspstrong uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { minsize optsize "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { minsize noreturn nounwind optsize sspstrong uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #4 = { mustprogress nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #5 = { minsize nounwind optsize }
attributes #6 = { nounwind }
attributes #7 = { minsize optsize }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 18.1.8"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = distinct !{!9, !10}
!10 = !{!"llvm.loop.mustprogress"}
!11 = distinct !{!11, !10}
