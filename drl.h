#ifndef DRL_H
#define DRL_H

#include <iostream>
#include <vector>

namespace emboarden {
  extern std::ostream *drill_p;

  void init_drill(std::ostream &out);
  void final_drill(std::ostream &out);
  void add_hole(double x, double y, double d, double scale);
}

#endif
