#include <string>

struct SkyboxFace {
  // array of floating point nums containing albedo color data
  float* albedo;

  // array of floating point nums containing diffuse lighting data
  float* diffuse;

  // each ind contains specular lighting data for resp mipmap level
  float** spec;
};

struct Skybox {
  // list of skyboxes (starting from GL_TEXTURE_CUBE_MAP_POSITIVE_X)
  SkyboxFace* skyboxes;
  
  // dimensions of each albedo face
  int albedoDim;

  // dimensions of each diffuse face
  int diffuseDim;

  // max dim for specular face
  // each face in the list is half dim of prev
  int specularMaxDim;

  // number of mipmap levels generated for our specular :3
  int specularMipmapCount;
};

/**
 *  Convolves the skybox represented by the passed HDR.
 *  @param path - path to the skybox to be convolved.
 *  @param diffuseSize - dims of the diffuse texture.
 *  @param maxSpecularSize - dims of spec mipmap level 0.
 *  @param maxSpecularLevels - the number of mipmap levels to generate. Capped at floor(log2(<dims of passed HDR>))
 *  @returns generated skybox :3
 */ 
Skybox* ConvolveSkybox(const std::string& path, int diffuseSize, int maxSpecularSize, int maxSpecularLevels);
