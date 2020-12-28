#include <iostream>
#include <fstream>
#include <iomanip>

#include <map>
#include <vector>
#include <set>
#include <stack>

#include <cmath>

#include <libpcb/poly.h>
#include <libpcb/basic.h>

#include "geometry.h"
#include "drl.h"
#include "svg.h"
#include "opt.h"

using namespace std;
using namespace libpcb;
using namespace emboarden;

double emboarden::scale = 1.0/1000.0;
int emboarden::simplify_passes = 10;

static plane *pcb_poly;

static void filter_poly(vector<int> &pts, int width, set<int> exclude) {
  vector<int> out;

  const double THRESH = 20, RATIO = 0.0;
  int interval;
  if (pts.size() > 50000) interval = 50;
  if (pts.size() > 10000) interval = 20;
  else if (pts.size() > 1000) interval = 10;
  else if (pts.size() > 100) interval = 5;
  else interval = 2;
  
  double prev_x = pts[0]%width, prev_y = pts[0]/width;
  for (unsigned i = 0; i < pts.size(); ++i) {
    bool include = true;//(i % interval == 0);
    int this_x = pts[i]%width, this_y = pts[i]/width;
    if (exclude.count(pts[i]) ||
	sqrt(pow(this_x - prev_x, 2) + pow(this_y - prev_y, 2)) > THRESH)
    {
      prev_x = this_x;
      prev_y = this_y;
      include = true;
    } else {
      prev_x = RATIO * prev_x + (1.0 - RATIO) * this_x;
      prev_y = RATIO * prev_y + (1.0 - RATIO) * this_y;
    }

    int x = round(prev_x), y = round(prev_y);
    if (x >= width) x = width - 1;
    if (include) out.push_back(y*width + x);
  }

  if (verbose)
    cout << "Shrank from " << pts.size() << " to " << out.size() << endl;
  
  pts.clear();
  for (unsigned i = 0; i < out.size(); ++i) {
    pts.push_back(out[i]);
  }
}

void emboarden::simplify_poly(vector<int> &pts, int width, set<int> exclude) {
  filter_poly(pts, width, exclude);

  vector<int> out;

  double x_max = 0, x_min = 1e100, y_max = 0, y_min = 1e100,
         x_sum = 0, y_sum = 0, r_sum = 0;

  for (auto &p : pts) {
    int x(p%width), y(p/width);
    if (x > x_max) x_max = x;
    if (y > y_max) y_max = y;
    if (x < x_min) x_min = x;
    if (y < y_min) y_min = y;

    x_sum += x;
    y_sum += y;
  }

  x_sum /= pts.size();
  y_sum /= pts.size();
  for (auto &p : pts) {
    int x(p%width), y(p/width);
    r_sum += sqrt(pow(x - x_sum, 2) + pow(y - y_sum, 2));
  }
  r_sum /= pts.size();

  add_hole(x_sum, y_sum, 2*r_sum, scale);
  
  double girth = sqrt(pow(x_max - x_min, 2) + pow(y_max - y_min, 2));

  double epsilon = (2*girth - 2)/girth;

  for (unsigned p = 0; p < simplify_passes; ++p) {
    set<int> kill;
    int ip = pts.size() - 1;
    for (unsigned i = 0; i < pts.size(); ++i) {
      int in = (i + 1)%pts.size();
      double x1 = pts[ip]%width, x2 = pts[i]%width, x3 = pts[in]%width,
             y1 = pts[ip]/width, y2 = pts[i]/width, y3 = pts[in]/width;
      double h = abs(sin(atan2(x3 - x1, y3 - y1) - atan2(x2 - x1, y2 - y1)) *
        sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)));
      double limit = epsilon*(p + 1)/double(simplify_passes);
      if (h <= limit && !exclude.count(pts[i])){
        kill.insert(i);
      } else {
        out.push_back(pts[i]);
        ip = i;
      }
    }
    if (verbose)
      cout << pts.size() << " => " << out.size() << endl;
    pts = out;
    out.clear();
  }

  for (auto &p : pts) {
    int x = p % width, y = p / width;
    if (!pcb_poly) pcb_poly = new plane(LAYER_CU0);
    pcb_poly->add_point(point(x*scale, -y*scale));
  }
}

void emboarden::trace_polygon(
  vector<int> &pts, const vector<int> &hcount, const vector<int> &vcount,
  int width, int height
) {
  set<int> cut, visited;
  int prev_dir = 1;

  // Find the starting point, then trace the polygon.
  for (unsigned i = 0; i < (width+1)*(height+1); ++i) {
    int x(i%(width+1)), y(i/(width+1));
    if (x == width) continue;

    if (hcount[y*width + x]) {
      int i0 = i;
      do {
        pts.push_back(i);
        int t((y-1)*(width + 1) + x), b(y*(width + 1) + x),
            l(y*width + x - 1), r(y*width + x),
            li(y*(width+1) + x - 1), ri(y*(width+1) + x + 1),
            ti((y - 1)*(width+1) + x), bi((y + 1)*(width+1) + x);

        // Cut-ins are always highest priority
        if (x < width - 1 && !cut.count(i) && hcount[r] == 0x80000000) {
          cut.insert(i);
          i = ri;
          prev_dir = 1;
        } else if (x > 0 && visited.count(i) && hcount[l] == 0x80000000) {
          i = li;
          cut.insert(i);
          visited.insert(i);
          prev_dir = 3;
        } else {
          visited.insert(i);
          i = -1;
          int count = 0;
          for (int j = (prev_dir+1)%4; count != 4; j = (j+1)%4) {
            if (j == 0 && y < height && vcount[b] == 1)
              { i = bi; prev_dir = j; break; }
            if (j == 1 && x < width && hcount[r] == -1)
              { i = ri; prev_dir = j; break; }
            if (j == 2 && y > 0 && vcount[t] == -1)
              { i = ti; prev_dir = j; break; }
            if (j == 3 && x > 0 && hcount[l] == 1)
              { i = li; prev_dir = j; break; }
            count++;
          }
          if (i == -1) { cout << "TRAPPED!" << endl; break; }
        }
        x = i%(width+1);
        y = i/(width+1);
      } while (i != i0);
      break;
    }
  }

  pcb_poly = NULL;
  simplify_poly(pts, width+1, cut);
}

void emboarden::polygen(
  vector<int> &img, int components, int width,
  string debug_svg_filename
) {
  int height = img.size()/width;

  svg *debug_svg = NULL;

  if (debug_svg_filename != "")
    debug_svg = new svg(debug_svg_filename.c_str(), width, img.size()/width);
  
  for (unsigned c = 0; c < components; ++c) {
    vector<int> hcount((height+1)*width, 0),
                vcount((width+1)*height, 0),
                h(hcount), v(vcount);

    for (unsigned i = 0; i < img.size(); ++i) {
      int y = i/width, x = i%width;
      if (img[i] == c) {
	++hcount[y*width + x];
        --hcount[(y + 1)*(width) + x];
        ++vcount[y*(width+1) + x];
	--vcount[y*(width+1) + x + 1];
       }
    }

    int hole_count = 0;
    bool outer_shell = true;
    set<int> visited;
    for (unsigned i = 0; i < hcount.size(); ++i) {
      if (visited.count(i)) continue;

      if (hcount[i]) {
        stack<int> s;
	s.push(i);
	while (!s.empty()) {
	  int x(s.top()%width), y(s.top()/width);
	  s.pop();
	  if (visited.count(y*width + x)) continue;
	  visited.insert(y*width + x);

          if (x < width - 1 && hcount[y*width + x])
	    s.push(y*width + x + 1);
	  if (x > 0 && hcount[y*width + x - 1])
	    s.push(y*width + x - 1);
	  if (y < height && vcount[y*(width + 1) + x])
	    s.push((y + 1)*width + x);
	  if (y > 0 && vcount[(y - 1)*(width + 1) + x])
	    s.push((y - 1)*width + x);
	}

        if (outer_shell) {
          outer_shell = false;
        } else {
	  ++hole_count;
          if (verbose)
            cout << "New hole at (" << i%width << ',' << i/width << ')'
                 << endl;
          // Insert the cut-in
          int count = 0;
          for (int j = i-1; j == i-1 || !visited.count(j + 1); --j) {
            hcount[j] = 0x80000000;
            ++count;
          }
	  if (verbose)
            cout << "  " << count << " cut-in pixels." << endl;
        }
      }
    }
    if (verbose)
      cout << "Component " << c << " has " << hole_count << " holes." << endl;

    vector<int> pts;
    trace_polygon(pts, hcount, vcount, width, height);
    
    if (debug_svg)
      debug_svg->write_polygon(pts);
  }

  if (debug_svg)
    delete debug_svg;
}
