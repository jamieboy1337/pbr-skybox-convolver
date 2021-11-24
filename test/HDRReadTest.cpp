#include <iostream>
#include "Convolver_internal.hpp"

int main(int argc, char** argv) {
  std::cout << "test open hdr" << std::endl;

  LoadHDRImage("./res/hdr_beach_1k.hdr");
  return 0;
}
