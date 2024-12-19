fun f(x: int): int = x
fun g(x: int): int = -(0, x)

fun pass(func: (int) -> int): (int) -> int = func

fun app(): int =
  let
    x = if (1; 0) then pass(f) else g fi,
  in
    +(if 1 then x else pass(g) fi(1), f(10))
