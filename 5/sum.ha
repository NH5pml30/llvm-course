fun zero(arr: [int], i: int, length: int): ()
  =
    if <(i, length) then
      set(i, 0, arr);
      zero(arr, +(i, 1), length);
      ()
    fi;

fun sum(arr: [int], i: int, length: int): int
  =
    if <(i, length) then
      +(at(i, arr), sum(arr, +(i, 1), length))
    else
      0
    fi;

fun app() =
  let
    arr = alloca int[10]
  in
    zero(arr, 0, 10);
    set(0, 30, arr);
    set(1, 40, arr);
    sum(arr, 0, 10);
