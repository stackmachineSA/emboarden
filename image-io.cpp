#include <iostream>
#include <fstream>
#include <iomanip>

#include <map>
#include <vector>
#include <cstring>

#include <libpng/png.h>

#include "image-io.h"

using namespace std;

const double SCALE = 1.0/1000.0;

void emboarden::read_img(vector<int> &img, int &width, const char *filename) {
  png_image p;
  p.version = PNG_IMAGE_VERSION;
  p.opaque = NULL;

  png_image_begin_read_from_file(&p, filename);
  if (PNG_IMAGE_FAILED(p)) {
    cerr << "Image read failed." << endl;
    abort();
  }

  p.format = PNG_FORMAT_RGBA;

  vector<unsigned> bitmap(p.width*p.height);
  png_image_finish_read(&p, 0, (void*)&bitmap[0], 0, 0);

  img.resize(p.width*p.height);
  int c = 0;
  for (unsigned i = 0; i < bitmap.size(); ++i)
    if (bitmap[i] == 0xff000000)
      img[i] = c++;
    else
      img[i] = -1;

  width = p.width;
}

void emboarden::write_img(const char *filename, vector<int> &img, int width) {
  png_image p;
  memset((void*)&p, 0, sizeof(p));

  p.version = PNG_IMAGE_VERSION;
  p.opaque = NULL;

  p.width = width;
  p.height = img.size()/width;
  p.format = PNG_FORMAT_RGBA;

  vector<unsigned> bitmap(p.width*p.height, 0xffffffff);

  map<int, unsigned> cmap;
  srand(0);
  for (unsigned i = 0; i  < img.size(); ++i) {
    if (img[i] == -1) continue;

    if (!cmap.count(img[i]))
      cmap[img[i]] = 0xff000000 | (rand() & 0xffffff);

    bitmap[i] = cmap[img[i]];
  }

  png_image_write_to_file(&p, filename, 0, (void*)&bitmap[0], 0, 0);
}
