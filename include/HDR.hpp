#include <string>

struct HDR {
  // x/y dims of the passed HDR
  int x;
  int y;

  // RGB order -- lower left to right, upper top to bottom.
  float* colorData;
};

HDR LoadHDRImage(const std::string& path);
