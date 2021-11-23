#include "Convolver.hpp"
#include "Convolver_internal.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
// more or less copy our glsl impl in cpp

static const std::string HDR_MAGIC   = "#?RADIANCE";
static const std::string FORMAT_NAME = "FORMAT";
static const std::string FORMAT_RLE  = "32-bit_rle_rgbe";

static int getResolutionValue(const std::string& dim);
static unsigned char* readHDRData(std::istream& data, int major, int minor);
static void readLineOldRLE(std::istream& file, int len, unsigned char* output);
static void readLineARLE(std::istream& file, int len, unsigned char* output);
static void readLine(std::istream& data, int len, unsigned char* output);
static float* HDRDataToFloat(unsigned char* in, int major, int minor);

HDR LoadHDRImage(const std::string& path) {
  // just open the file
  // check for the right flags and read floats from there
  std::ifstream hdr(path, std::ifstream::in);
  // note: handle failure, EOF
  char drop;
  std::string magic;
  magic.resize(10, '\0');
  // ok in c++11 - string data can be modified iirc
  hdr.read(const_cast<char*>(magic.data()), 10);
  std::cout << magic << std::endl;
  std::cout << HDR_MAGIC << std::endl;
  if (HDR_MAGIC.compare(magic) != 0) {
    std::cout << HDR_MAGIC.compare(magic) << std::endl;
    std::cerr << "INCORRECT MAGIC!!!" << std::endl;
    exit(1);
  } else {
    std::cout << ":D" << std::endl;
  }

  while (hdr.get() == '\n');
  hdr.unget();

  std::string line, key, val;
  std::string format;
  size_t eq_pos;
  for (;;) {
    // read line
    // if empty: double \n indicates that we're done reading
    // else: find the = sign, get pre and post. look for format
    std::getline(hdr, line);
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
    exit(1);
  } else {
    std::cout << "Format: " << format << std::endl;
  }
  
  if (format.compare(FORMAT_RLE) != 0) {
    std::cerr << "CANNOT HANDLE FORMAT " << format << std::endl;
    exit(1);
  }

  // at this point, we read the empty line, so the next line is res data
  
  std::string resolution;

  std::getline(hdr, resolution);
  std::cout << resolution << std::endl;
  size_t space_pos = resolution.find(' ');

  space_pos = resolution.find(' ', space_pos + 1);

  std::string res_major = resolution.substr(0, space_pos);
  std::string res_minor = resolution.substr(space_pos + 1);

  std::cout << res_major << ", " << res_minor << std::endl;
  
  // <+/-><dim> <num> <+/-><other dim> <num>
  int dim_major = getResolutionValue(res_major);
  int dim_minor = getResolutionValue(res_minor);

  std::cout << dim_major << " AND " << dim_minor << std::endl;
  // todo: write a quick test exec which uses convolver
  // write a header to facilitate testing (shouldnt be necessary in final)
  
  unsigned char* data = readHDRData(hdr, dim_major, dim_minor);
  float* col = HDRDataToFloat(data, dim_major, dim_minor);

  for (size_t i = 0; i < dim_major * dim_minor; i++) {
    std::cout << *(col + 3 * i) << ", " << *(col + 3 * i + 1) << ", " << *(col + 3 * i + 2) << std::endl;
  }
  // double check??
  // move this to an HDR exclusive file??
  HDR res;
  res.x = 1;
  res.y = 1;
  res.colorData = NULL;
  return res;
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
  float* res = new float[major * minor * 4];
  float exponent;
  unsigned char* cur = in;
  float* out = res;

  unsigned char* stop = cur + (major * minor * 4);
  unsigned char r, g, b, e;
  while (cur < stop) {
    r = *cur++;
    g = *cur++;
    b = *cur++;
    e = *cur++;

    exponent = pow(2.0F, static_cast<float>(e) - 128.0F);

    *out++ = ((static_cast<float>(r) + 0.5) / 256.0F) * exponent;
    *out++ = ((static_cast<float>(g) + 0.5) / 256.0F) * exponent;
    *out++ = ((static_cast<float>(b) + 0.5) / 256.0F) * exponent;
  }

  return res;
}

/**
 *  Reads data (decompressing if necessary) from HDR file to int array.
 *  @param data - istream pointing at start of data
 *  @param major - number of lines in hdr file
 *  @param minor - number of entries in each line
 */ 
static unsigned char* readHDRData(std::istream& data, int major, int minor) {
  unsigned char* res = new unsigned char[major * minor * 4];
  unsigned char* live = res;
  for (int i = 0; i < major; i++) {
    readLine(data, minor, live);
    live += (minor * 4);
    std::cout << "read line " << i << std::endl;
  }

  return res;  
}

static void readLine(std::istream& data, int len, unsigned char* output) {
  unsigned char* out = output;
  unsigned char r, g, b, e;

  r = data.get();
  g = data.get();
  b = data.get();
  e = data.get();

  if (r != 2 || g != 2 || (b & 128)) {
    // might bug -- 4 ungets would work too :(
    data.seekg(-4, std::ios_base::cur);
    return readLineOldRLE(data, len, output);
  }

  const int len_check = (static_cast<int>(b) << 8 | static_cast<int>(e));
  if (len_check != len) {
    std::cerr << "invalid scanline len on HDR :(" << std::endl;
    std::cerr << "expected " << len << ", actual " << len_check << std::endl;
    exit(1);
  }

  readLineARLE(data, len, output);
}

static void readLineARLE(std::istream& file, int len, unsigned char* output) {
  unsigned char code, repeatCount, temp;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < len; ) {
      code = file.get();
      if (code > 128) {
        repeatCount = code & 127;
        temp = file.get();
        while (repeatCount--) {
          *(output + (4 * j++) + i) = temp;
        }

      } else {
        while (code--) {
          *(output + (4 * j++) + i) = file.get();
        }
      }
    }
  }
  
  // output is definite
  // stream offset is maintained (pass by ref)
  return;
}

static void readLineOldRLE(std::istream& file, int len, unsigned char* output) {
  unsigned char r, g, b, e;
  unsigned char* out = output;
  int repeat_offset = 0;
  for (int i = 0; i < len; i++) {
    r = file.get();
    g = file.get();
    b = file.get();
    e = file.get();

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
