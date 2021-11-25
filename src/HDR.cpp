#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <glm/glm.hpp>

#include "HDR.hpp"

#define RGB_ADD(col, t, out) {\
  out[0] += col[0] * t;\
  out[1] += col[1] * t;\
  out[2] += col[2] * t;\
}

struct hdr_file_info {
  std::ifstream* stream;
  std::streampos streamsize;
};

static const std::string HDR_MAGIC   = "#?RADIANCE";
static const std::string FORMAT_NAME = "FORMAT";
static const std::string FORMAT_RLE  = "32-bit_rle_rgbe";
static const std::string EOF_ERROR   = "reached eof before file was done parsing";

static int getResolutionValue(const std::string& dim);
static unsigned char* readHDRData(const hdr_file_info& data, int major, int minor);
static void readLineOldRLE(const hdr_file_info& file, int len, unsigned char* output);
static void readLineARLE(const hdr_file_info& file, int len, unsigned char* output);
static void readLine(const hdr_file_info& data, int len, unsigned char* output);
static float* HDRDataToFloat(unsigned char* in, int major, int minor);

HDR::HDR(int width, int height, float* data) 
  : x(width), y(height), colorData(data), res_step_x(1.0F / static_cast<float>(width)), res_step_y(1.0F / static_cast<float>(height)) {}

void HDR::nearestRGBLookup(float tex_x, float tex_y, float rgb_out[3]) {
  float tex_x_actual = glm::fract(tex_x);
  float tex_y_actual = glm::fract(tex_y);

  int pixel_x = static_cast<int>(tex_x_actual * static_cast<float>(x) + 0.5F) % x;
  int pixel_y = static_cast<int>((1.0F - tex_y_actual) * static_cast<float>(y) + 0.5F) % y;

  const float* colresult = (colorData + 3 * (x * pixel_y + pixel_x));
  memcpy(rgb_out, colresult, 3 * sizeof(float));
}

void HDR::linearRGBLookup(float tex_x, float tex_y, float rgb_out[3]) {
  float pix_x = tex_x * x;
  float pix_y = glm::fract(1.0F - tex_y) * y;

  float f_x = floor(pix_x);
  float f_y = floor(pix_y);

  float t_x = pix_x - f_x;
  float t_y = pix_y - f_y;

  float l_x = f_x * res_step_x;
  float l_y = f_y * res_step_y;

  // do our lookups now
  rgb_out[0] = 0;
  rgb_out[1] = 0;
  rgb_out[2] = 0;
  
  float temp[3];
  float fac_temp;

  this->nearestRGBLookup(l_x, l_y, temp);
  fac_temp = (1.0 - t_x) * (1.0 - t_y);
  RGB_ADD(temp, fac_temp, rgb_out);

  this->nearestRGBLookup(l_x + res_step_x, l_y, temp);
  fac_temp = t_x * (1.0 - t_y);
  RGB_ADD(temp, fac_temp, rgb_out);

  this->nearestRGBLookup(l_x, l_y + res_step_y, temp);
  fac_temp = (1.0 - t_x) * t_y;
  RGB_ADD(temp, fac_temp, rgb_out);

  this->nearestRGBLookup(l_x + res_step_x, l_y + res_step_y, temp);
  fac_temp = t_x * t_y;
  RGB_ADD(temp, fac_temp, rgb_out);
}

// return ptr to handle invalid?
HDR LoadHDRImage(const std::string& path) {
  HDR res(-1, -1, NULL);

  hdr_file_info hdr;
  // just open the file
  // check for the right flags and read floats from there
  hdr.stream = new std::ifstream(path, std::ifstream::in | std::ifstream::binary);
  hdr.stream->seekg(0, hdr.stream->end);
  hdr.streamsize = hdr.stream->tellg();
  hdr.stream->seekg(0);
  if (hdr.stream->fail()) {
    std::cerr << "could not open desired file :(" << std::endl; 
    return res;
  }
  // note: handle failure, EOF
  std::string magic;
  magic.resize(10, '\0');
  // ok in c++11 - string data can be modified iirc
  hdr.stream->read(const_cast<char*>(magic.data()), 10);

  if (HDR_MAGIC.compare(magic) != 0) {
    std::cout << HDR_MAGIC.compare(magic) << std::endl;
    std::cerr << "INCORRECT MAGIC!!!" << std::endl;
    return res;
  }

  while (hdr.stream->get() == '\n');
  hdr.stream->unget();

  std::string line, key, val;
  std::string format;
  size_t eq_pos;
  for (;;) {
    // read line
    // if empty: double \n indicates that we're done reading
    // else: find the = sign, get pre and post. look for format

    // if for some reason we reach the end of the file due to a bug,
    // just return.
    if (hdr.stream->eof()) {
      return res;
    }

    std::getline(*hdr.stream, line);
    std::cout << line << std::endl;
    
    // final line :)
    if (line.size() == 0) {
      break;
    }

    if (line.at(0) == '#') {
      continue;
    }

    eq_pos = line.find('=');

    // only if no eql (newline?)
    if (eq_pos == line.npos) {
      break;
    }

    key = line.substr(0, eq_pos);
    val = line.substr(eq_pos + 1);

    if (key.compare(FORMAT_NAME) == 0) {
      format = val;
    }
  }

  if (format.size() == 0) {
    // format string not found
    std::cerr << "FORMAT STRING NOT FOUND" << std::endl;
    return res;
  }
  
  if (format.compare(FORMAT_RLE) != 0) {
    std::cerr << "CANNOT HANDLE FORMAT " << format << std::endl;
    return res;
  }

  // at this point, we read the empty line, so the next line is res data
  if (hdr.stream->eof()) {
    // early eof
    return res;
  }
  
  std::string resolution;

  std::getline(*hdr.stream, resolution);
  size_t space_pos = resolution.find(' ');
  if (space_pos == resolution.npos) {
    return res;
  }

  space_pos = resolution.find(' ', space_pos + 1);
  if (space_pos == resolution.npos) {
    return res;
  }

  std::string res_major = resolution.substr(0, space_pos);
  std::string res_minor = resolution.substr(space_pos + 1);
  
  // <+/-><dim> <num> <+/-><other dim> <num>
  int dim_major = getResolutionValue(res_major);
  int dim_minor = getResolutionValue(res_minor);
  // todo: write a quick test exec which uses convolver
  // write a header to facilitate testing (shouldnt be necessary in final)
  
  if (dim_major > 65536 || dim_minor > 65536 || dim_major <= 0 || dim_minor <= 0) {
    // invalid resolution field
    return res;
  }

  unsigned char* data = readHDRData(hdr, dim_major, dim_minor);
  float* col = HDRDataToFloat(data, dim_major, dim_minor);
  size_t pixels = static_cast<size_t>(dim_major) * static_cast<size_t>(dim_minor);
  return HDR(dim_minor, dim_major, col);
}
/**
 *  Extracts dimensionality from resolution tuple
 *  @param dim - resolution tuple string
 *  @returns size of specified res tuple, or -1 if not found
 */ 
static int getResolutionValue(const std::string& dim) {
  size_t space_pos = dim.find(' ');
  if (space_pos == dim.npos) {
    return -1;
  }

  int res = atoi(dim.c_str() + space_pos + 1);
  return res;
}

static float* HDRDataToFloat(unsigned char* in, int major, int minor) {
  size_t bytes = static_cast<size_t>(major) * static_cast<size_t>(minor) * 3;
  float* res = new float[bytes];
  if (res == NULL) {
    return res;
  }

  float exponent;
  unsigned char* cur = in;
  float* out = res;

  unsigned char* stop = cur + (bytes);
  unsigned char r, g, b, e;
  while (cur < stop) {
    r = *cur++;
    g = *cur++;
    b = *cur++;
    e = *cur++;

    exponent = pow(2.0F, static_cast<float>(e) - 128.0F);

    *out++ = ((static_cast<float>(r) + 0.5F) / 256.0F) * exponent;
    *out++ = ((static_cast<float>(g) + 0.5F) / 256.0F) * exponent;
    *out++ = ((static_cast<float>(b) + 0.5F) / 256.0F) * exponent;
  }

  return res;
}

/**
 *  Reads data (decompressing if necessary) from HDR file to int array.
 *  @param data - istream pointing at start of data
 *  @param major - number of lines in hdr file
 *  @param minor - number of entries in each line
 */ 
static unsigned char* readHDRData(const hdr_file_info& data, int major, int minor) {
  size_t bytes = static_cast<size_t>(major) * static_cast<size_t>(minor) * 4;
  unsigned char* res = new unsigned char[bytes];
  if (res == NULL) {
    return res;
  }

  unsigned char* live = res;
  for (int i = 0; i < major; i++) {
    readLine(data, minor, live);
    live += (minor * 4);
    if (data.stream->eof()) {
      return NULL;
    }
  }

  return res;  
}

static void readLine(const hdr_file_info& data, int len, unsigned char* output) {
  // ensure we have 4 bytes avail to read
  unsigned char r, g, b, e;

  r = data.stream->get();
  g = data.stream->get();
  b = data.stream->get();
  e = data.stream->get();

  if (data.stream->eof()) {
    return;
  }

  if (r != 2 || g != 2 || (b & 128)) {
    // might bug -- 4 ungets would work too :(
    data.stream->seekg(-4, std::ios_base::cur);
    return readLineOldRLE(data, len, output);
  }

  const int len_check = (static_cast<int>(b) << 8 | static_cast<int>(e));
  if (len_check != len) {
    std::cerr << "invalid scanline len on HDR :(" << std::endl;
    std::cerr << "expected " << len << ", actual " << len_check << std::endl;
    exit(1);
  }

  return readLineARLE(data, len, output);
}

static void readLineARLE(const hdr_file_info& data, int len, unsigned char* output) {
  int code, repeatCount, temp;
  char c;
  for (int i = 0; i < 4; i++) {
    int j;
    for (j = 0; j < len; ) {

      if (data.stream->eof()) {
        std::cerr << EOF_ERROR << std::endl;
      }

      data.stream->get(c);
      code = *reinterpret_cast<unsigned char*>(&c);
      if (code > 128) {
        repeatCount = code & 127;
        temp = data.stream->get();
        while (repeatCount--) {
          *(output + (4 * j++) + i) = static_cast<unsigned char>(temp);
        }
      } else {
        while (code--) {
          *(output + (4 * j++) + i) = static_cast<unsigned char>(data.stream->get());
        }
      }
    }
  }
  
  // output is definite
  // stream offset is maintained (pass by ref)
  return;
}

static void readLineOldRLE(const hdr_file_info& data, int len, unsigned char* output) {
  unsigned char r, g, b, e;
  unsigned char* out = output;
  int repeat_offset = 0;
  for (int i = 0; i < len; i++) {
    if (data.stream->eof()) {
      std::cerr << "reached eof before file was done parsing" << std::endl;
      return;
    }

    r = data.stream->get();
    g = data.stream->get();
    b = data.stream->get();
    e = data.stream->get();

    if (r == 1 && g == 1 && b == 1) {
      const unsigned char* repeater = output - 4;
      size_t repeat_count = (static_cast<size_t>(e)) << repeat_offset;
      while (repeat_count--) {
        for (int j = 0; j < 4; j++) {
          *(out++) = *(repeater + j);
        }

        i++;
    
      }

      repeat_offset += 8;
    } else {
      *out++ = r;
      *out++ = g;
      *out++ = b;
      *out++ = e;
      repeat_offset = 0;
    }
  }
}
