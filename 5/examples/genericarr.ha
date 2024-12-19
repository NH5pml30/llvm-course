fun f(x: int): int = x
fun g(x: int): int = -(0, x)

fun app() =
  let
    arr = alloca int -> int [10]
  in
    set_at(0, f, arr);
    set_at(1, g, arr);
    write(at(0, arr)(10));
    write(at(1, arr)(30))
