#ifndef BITMAP_H_
#define BITMAP_H_

#include <string>
#include "HDR.hpp"

/**
 * @brief Saves the HDR as a bitmap file, for testing purposes.
 * 
 * @param data - HDR file data
 * @param output - name of output file
 */
void SaveHDRAsBitmap(HDR& data, std::string output);

#endif // BITMAP_H_