fun fib(x: int): int
  =
    if ==(x, 0) then
      1
    else
      if ==(x, 1) then
        1
      else
        +(fib(-(x, 1)), fib(-(x, 2)));
      fi
    fi

fun app() = fib(11);
