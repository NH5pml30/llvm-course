N_STARS = 200
WORLD_SIZE_X = 100
WORLD_SIZE_Y = 100
WORLD_SIZE_Z = 100
SIM_X_SIZE = 512
SIM_Y_SIZE = 256

fun center(x: int, bound: int) = -(%(x, *(2, bound)), bound)

fun generateStar(s: [int]) =
  set(0, center(simRand(), WORLD_SIZE_X), s);
  set(1, center(simRand(), WORLD_SIZE_Y), s);
  set(2, WORLD_SIZE_Z, s);
  ()

fun generateStars(s: [int], i: int, length: int): () =
  if <(i, length) then
    generateStar(slice(i, 3, s));
    set(+(i, 2), %(simRand(), WORLD_SIZE_Z), s);
    generateStars(s, +(i, 3), length)
  fi

fun drawStar(s: [int]) = let
  z = at(2, s),
  new_z = -(z, 1)
in
  set(2, new_z, s);
  if <=(new_z, 0) then
    generateStar(s)
  fi;
  let
    x = at(0, s),
    y = at(1, s),
    z = at(2, s),
    scr_half_x = /(SIM_X_SIZE, 2),
    scr_half_y = /(SIM_Y_SIZE, 2),
    scr_x = +(scr_half_x, /(*(scr_half_x, x), z)),
    scr_y = +(scr_half_y, /(*(scr_half_y, y), z)),
    scr_x_bounds = &&(>=(scr_x, 0), <(scr_x, SIM_X_SIZE)),
    scr_y_bounds = &&(>=(scr_y, 0), <(scr_y, SIM_Y_SIZE)),
  in
    if &&(scr_x_bounds, scr_y_bounds) then
      simPutPixel(scr_x, scr_y, -1)
    fi

fun drawStars(s: [int], i: int, length: int): () =
  if <(i, length) then
    drawStar(slice(i, 3, s));
    drawStars(s, +(i, 1), length);
  fi

fun drawFrame(s: [int], length: int) =
  simClear(0);
  drawStars(s, 0, length);
  simFlush()

fun drawFrames(s: [int], length: int): () =
  drawFrame(s, length);
  drawFrames(s, length)

fun app() = let
  x = write(0),
  length = *(N_STARS, 3),
  stars = alloca int[length],
in
  generateStars(stars, 0, length);
  drawFrames(stars, length);
  0
