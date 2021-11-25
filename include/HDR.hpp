#ifndef HDR_H_
#define HDR_H_

#include <string>

struct HDR {
 private:
  const int x;
  const int y;

  const float res_step_x;
  const float res_step_y;

  // RGB order -- lower left to right, upper top to bottom.
  const float* colorData;
  
  void nearestRGBLookup(float tex_x, float tex_y, float rgb_out[3]);

  // look up color values stored in this HDR with linear filtering.
  // x/y dims of the passed HDR
 public:
  HDR(int x, int y, float* data);
  void linearRGBLookup(float tex_x, float tex_y, float rgb_out[3]);
};

HDR LoadHDRImage(const std::string& path);

#endif // HDR_H_