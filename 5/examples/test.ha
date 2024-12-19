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

#- ok -#
# fibs
fun fibs(x: int, max: int): () =
  if <(x, max) then
    write(fib(x));
    fibs(+(x, 1), max);
  fi

fun app() =
  fibs(0, 12);
  0
