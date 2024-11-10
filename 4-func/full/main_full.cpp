#include <fstream>
#include <sstream>

#include "gen.h"

int main() {
  std::ifstream in("../app.s");
  /*std::string source = R"__delim(
entry:
  ALLOCA r0 4
start:
  LOAD r1 r0
  ADDi r1 r1 1
  STORE r0 r1
  WRITE r0
  WRITE r1
  JMPLTi r1 255 start
  EXIT
)__delim";
  std::stringstream in(source);*/

  gen g;
  g.run(in);

  memory mem;
  g.executeIR(mem);

  return 0;
}