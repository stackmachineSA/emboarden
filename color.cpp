#include <vector>
#include <stack>
#include <iostream>

#include "color.h"

using namespace std;
using namespace emboarden;

int emboarden::color_img(vector<int> &img, int width) {
  int next_color = 0;
  int height = img.size()/width;

  for (unsigned i = 0; i < img.size(); ++i)
    if (img[i] != -1) img[i] = -2;

  for (unsigned i = 0; i < img.size(); ++i) {
    if (img[i] == -2) {
      stack<int> s;
      s.push(i);
      while (!s.empty()) {
        int pt = s.top(), x = pt%width, y = pt/width;
        s.pop();
        if (pt < 0 || pt >= img.size()) {
          cout << "Invalid index (" << x << ',' << y << ')' << endl;
          break;
        }
        if (img[pt] == -2) {
          if (x > 0) s.push(pt - 1);
          if (x < width - 1) s.push(pt + 1);
          if (y > 0) s.push(pt - width);
          if (y < height - 1) s.push(pt + width);

          img[pt] = next_color;
        }
      }
      ++next_color;
    }
  }

  return next_color;
}
