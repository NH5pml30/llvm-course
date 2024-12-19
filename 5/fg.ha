fun f(x: int, y: int): int
  = +(x, y)

fun g(x: [int]): int
  = at(0, x)

fun app()
  = let
    x = 1,
    y = 2,
    z = alloca int[10],
  in
    f(1, 2);
    f(x, y);
    (let w = 3 in
      g(slice(3, 4, z)));
    30
