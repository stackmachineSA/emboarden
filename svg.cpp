#include <fstream>
#include <vector>
#include <string>

#include "svg.h"

using namespace std;
using namespace emboarden;

svg::svg(string filename, int width, int height):
  out(filename), width(width), height(height)
{
  out << "<svg height=\"" << height << "\" width=\"" << width << "\">"
      << endl;
}

svg::~svg() {
  out << "</svg>" << endl;
}

void svg::write_polygon(vector<int> &pts) {
  out << "  <polygon points=\"";
  for (unsigned i = 0; i < pts.size(); ++i) {
    int x = pts[i];

    out << x%(width+1) << ',' << x/(width+1);
    if (i != pts.size()-1) out << ' ';
  }
  out << "\" style=\"fill:magenta;stroke:black;stroke-width:1\" />" << endl;
}
