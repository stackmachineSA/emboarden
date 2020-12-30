#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>

#include "drl.h"

using namespace std;
using namespace emboarden;

ostream *emboarden::drill_p = NULL;

static vector<double> drill_holes;

void emboarden::init_drill(ostream &out) {
  out << "M48" << endl << "INCH,TZ" << endl << "FMAT,2" << endl;
}

void emboarden::final_drill(ostream &out) {
  map<int, string> tools;

  unsigned int next_id(1);
  out << setprecision(4);
  for (unsigned i = 0; i < drill_holes.size(); i += 3) {
    double d = drill_holes[i + 2];
    if (!tools.count(d*1000)) {
      ostringstream oss;
      oss << setw(2) << setfill('0') << next_id;
      out << 'T' << oss.str() << 'C' << d << endl;
      tools[d*1000] = oss.str();
      ++next_id;
    }
  }
  out << '%' << endl << "G90" << endl << "G05" << endl;

  for (auto &t : tools) {
    out << 'T' << t.second << endl;
    for (unsigned i = 0; i < drill_holes.size(); i += 3) {
      double x = drill_holes[i];
      double y = drill_holes[i + 1];
      double d = drill_holes[i + 2];
      if (t.first != int(d*1000)) continue;
      out << 'X' << x << 'Y' << y << endl;
    }
  }

  out << "T0" << endl << "M30" << endl;
}

void emboarden::add_hole(double x, double y, double d, double scale) {
  drill_holes.push_back(x*scale);
  drill_holes.push_back(-y*scale);
  drill_holes.push_back(d*scale);
}
