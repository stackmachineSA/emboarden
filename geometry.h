#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <set>
#include <string>

namespace emboarden {
  extern double scale, epsilon;
  extern int simplify_passes;

  void simplify_poly(std::vector<int> &pts, int width, std::set<int> exclude);

  void trace_polygon(
    std::vector<int> &pts, const std::vector<int> &hcount,
    const std::vector<int> &vcount, int width, int height
  );

  void polygen(
    std::vector<int> &img, int components, int width,
    std::string debug_svg_filename
  );
}

#endif
