#include <string>
#include <glm/glm.hpp>

struct HDR {
  // x/y dims of the passed HDR
  int x;
  int y;

  // RGB order -- lower left to right, upper top to bottom.
  float* colorData;
};

HDR LoadHDRImage(const std::string& path);

/**
 *  Generates albedo data for a single cube face.
 */ 

float* GetAlbedoData(const HDR* data, int dim, glm::vec3 center, glm::vec3 up);

/**
 *  Generates diffuse lighting data for a single cube face.
 *  @param data - the HDR we wish to generate data from.
 *  @param dim - dims of the result face, in pixels.
 *  @param center - cartesian vector representing the center of the face being drawn.
 *  @param up - cartesian vector representing upwards direction for face being drawn.
 */ 
float* GetDiffuseData(const HDR* data, int dim, glm::vec3 center, glm::vec3 up);

/**
 *  Generates specular lighting data for a single cube face.
 */ 
float** GetSpecularData(const HDR* data, int maxDim, glm::vec3 center, glm::vec3 up);

