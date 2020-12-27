#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include <vector>

namespace emboarden {
  void read_img(std::vector<int> &img, int &width, const char *filename);
  void write_img(const char *filename, std::vector<int> &img, int width);
}

#endif
