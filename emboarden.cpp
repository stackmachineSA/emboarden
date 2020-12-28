#include <iostream>
#include <fstream>
#include <iomanip>

#include <map>
#include <vector>
#include <set>
#include <stack>
#include <functional>

#include <cstring>
#include <cmath>

#include <libpcb/poly.h>
#include <libpcb/basic.h>

#include <libpng/png.h>

#include "image-io.h"
#include "svg.h"
#include "color.h"

using namespace std;
using namespace libpcb;
using namespace emboarden;

double scale = 1.0/1000.0;
int simplify_passes = 10;
bool verbose = false;

plane *pcb_poly;
ostream *drill_p = NULL;

void filter_poly(vector<int> &pts, int width, set<int> exclude) {
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

vector<double> drill_holes;

void init_drill(ostream &out) {
  out << "M48" << endl << "INCH,TZ" << endl << "FMAT,2" << endl;
}

void final_drill(ostream &out) {
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

void add_hole(double x, double y, double d) {
  drill_holes.push_back(x*scale);
  drill_holes.push_back(-y*scale);
  drill_holes.push_back(d*scale);
}

void simplify_poly(vector<int> &pts, int width, set<int> exclude) {
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

  add_hole(x_sum, y_sum, 2*r_sum);
  
  double girth = sqrt(pow(x_max - x_min, 2) + pow(y_max - y_min, 2));

  double epsilon = (2*girth - 2)/girth;

  for (unsigned p = 0; p < simplify_passes; ++p) {
    set<int> kill;
    int ip = pts.size() - 1;
    for (unsigned i = 0; i < pts.size(); ++i) {
      int in = (i + 1)%pts.size();
      double x1 = pts[ip]%width, x2 = pts[i]%width, x3 = pts[in]%width,
             y1 = pts[ip]/width, y2 = pts[i]/width, y3 = pts[in]/width;
      double h = abs(cos(atan2(x3 - x1, y3 - y1) - atan2(x2 - x1, y2 - y1)) *
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

void trace_polygon(
  vector<int> &pts, const vector<int> &hcount, const vector<int> &vcount,
  int width, int height
) {
  set<int> cut, visited;
  int prev_dir = 1;

  // Find the starting point, then trace the polygon.
  for (unsigned i = 0; i < (width+1)*(height+1); ++i) {
    int x(i%(width+1)), y(i/(width+1));
    if (hcount[y*width + x]) {
      int i0 = i;
      do {
        pts.push_back(i);
        int t((y-1)*(width+1) + x), b(y*(width+1) + x),
            l(y*width + x - 1), r(y*width + x),
            li(y*(width+1) + x - 1), ri(y*(width+1) + x + 1),
            ti((y - 1)*(width+1) + x), bi((y + 1)*(width+1) + x);

        // Cut-ins are always highest priority
        if (x < width-1 && !cut.count(i) && hcount[r] == 0x80000000) {
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

void polygen(
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

          if (x < width-1 && hcount[y*width + x])
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

struct program_opt {
  program_opt(
    string opt, string short_desc, string long_desc, function<void()> action
  );

  program_opt(
    string opt, string short_desc,
    string long_desc, string arg_name, function<void(string)> action_arg
  );

  string opt, arg_name, short_desc, long_desc;
  bool has_arg;

  function<void()> action;
  function<void(string)> action_arg;

  static int longest_optname;

  static map<string, program_opt *> opts;
  static string usage_args;

  static void print_help_msg();
  static void print_usage_msg(string cmd);
  static string parse(int argc, char **argv);
};

program_opt::program_opt(
  string opt, string short_desc, string long_desc, function<void()> action
): opt(opt), short_desc(short_desc), long_desc(long_desc), has_arg(false),
   action(action)
{
  opts[opt] = this;
  int len = opt.length();

  if (len > longest_optname)
    longest_optname = len;
}

program_opt::program_opt(
  string opt, string short_desc, string long_desc, string arg_name,
  function<void(string)> action_arg
): opt(opt), short_desc(short_desc), long_desc(long_desc), arg_name(arg_name),
   has_arg(true), action_arg(action_arg)
{
  opts[opt] = this;

  int len = opt.length() + arg_name.length() + 3;

  if (len > longest_optname)
    longest_optname = len;
}

void program_opt::print_usage_msg(string cmd) {
  cout << "Usage:" << endl;
  cout << "  " << cmd << ' ' << usage_args << endl;
}

void program_opt::print_help_msg() {
  cout << "Program options:" << endl;

  for (auto &x : opts) {
    cout << "  " << x.first;
    if (x.second->has_arg)
      cout << " <" << x.second->arg_name << '>';

    string opt(x.first), arg_name(x.second->arg_name);
    int len = opt.length() + arg_name.length() + (arg_name.length() ? 3 : 0);
    for (unsigned i = len; i < longest_optname + 4; ++i)
      cout << ' ';
    cout << x.second->short_desc << endl;
    
    //cout << x.second->long_desc << endl << endl;
  }
}

string program_opt::parse(int argc, char **argv) {
  string naked_opt, error_opt;
  bool found_naked_opt = false, unrecognized_opts = false, no_arg = false;

  for (unsigned i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (!opts.count(argv[i])) {
        error_opt = argv[i];
        unrecognized_opts = true;
        break;
      } else {
        if (opts[argv[i]]->has_arg) {
          if (argc == i + 1) {
            no_arg = true;
            error_opt = argv[i];
            break;
          } else {
            opts[argv[i]]->action_arg(argv[++i]);
          }
        } else {
          opts[argv[i]]->action();
        }
      }
    } else if (argv[i][0] != '-') {
      if (found_naked_opt == true) {
	// Program should have one and only one naked opt.
	found_naked_opt = false;
	break;
      } else {
	found_naked_opt = true;
	naked_opt = argv[i];
      }
    }
  }

  if (no_arg) {
    cout << "Option \"" << error_opt << "\" requires an argument." << endl;
  } else if (unrecognized_opts) {
    cout << "Unrecognized command-line option \"" << error_opt << "\"."
         << endl;
  }

  if (!found_naked_opt || unrecognized_opts || no_arg) {
    print_usage_msg(argv[0]);
    exit(1);
  }

  return naked_opt;
}

int program_opt::longest_optname = 0;
map<string, program_opt *> program_opt::opts;
string program_opt::usage_args;

int main(int argc, char **argv) {
  program_opt::usage_args = "[options ...] input_file.png";

  program_opt p_h(
    "-h", "Display help message.",
    "TODO",
    [](){
      program_opt::print_help_msg();
    }
  );

  program_opt p_simp(
    "-simp", "Number of polygon simplification passes.",
    "TODO", "passes",
    [](string passes_str) {
      istringstream iss(passes_str);
      iss >> simplify_passes;
    }
  );

  program_opt p_scale(
    "-scale", "Set scale in pixels/inch.",
    "Set scale in pixels-per-inch.", "pixels/in",
    [](string scale_str) {
      istringstream iss(scale_str);
      double ppi;
      iss >> ppi;
      scale = 1.0/ppi;
    }
  );

  string color_filename;
  program_opt p_c(
    "-c", "Dump debugging image with color-coded components.",
    "TODO", ".png filename",
    [&](string filename) { color_filename = filename; }
  );

  string output_filename;
  program_opt p_o(
    "-o", "Set output filename.", "Set output filename.", "filename",
    [&](string filename) { output_filename = filename; }
  );

  string drl_filename;
  program_opt p_d(
    "-d", "Set output .drl filename.",
    "Set output filename (.drl format).", "filename",
    [&](string filename) {
      drl_filename = filename;
    }
  );

  program_opt p_v(
    "-v", "Enable verbose output.", "TODO",
    [](){ verbose = true; }
  );

  string debug_svg_filename;
  program_opt p_svg(
    "-svg", "Set output .svg filename.", "TODO", ".svg filename",
    [&](string filename) { debug_svg_filename = filename; }
  );
  
  string input_filename = program_opt::parse(argc, argv);

  ofstream gfile;
  if (output_filename == "") {
    size_t pos = input_filename.find_first_of(".");
    string stem = (pos == string::npos) ? stem : input_filename.substr(0, pos);
    gfile.open(stem + ".grb");
    if (!gfile) {
      cout << "Could not open default output file \"" << stem << ".grb\"."
           << endl;
      exit(1);
    }
  } else {
    gfile.open(output_filename);
    if (!gfile) {
      cout << "Could not open output file \"" << output_filename << "\"."
           << endl;
      exit(1);
    }
  }

  if (drl_filename != "") {
    drill_p = new ofstream(drl_filename);
    init_drill(*drill_p);
  }

  // 1. Open input image
  vector<int> img, neg_img, out;
  int width;
  read_img(img, width, input_filename.c_str());
  if (verbose)
    cout << "Image size: " << width << 'x' << img.size()/width << endl;

  int c = 0;
  for (auto &x : img)
    neg_img.push_back(x==-1 ? c++ : -1);

  if (verbose)
    cout << "Input read." << endl;

  // 2. Identify connected components (color img)
  int components = color_img(img, width);
  if (verbose)
    cout << "Found " << components << " glyphs." << endl;

  // DEBUG: write image colored by connected component
  if (color_filename != "")
    write_img(color_filename.c_str(), img, width);

  // 3. Generate polygons with cut-ins
  polygen(img, components, width, debug_svg_filename);

  // 4. Write gerber file for layer
  gerber g(gfile);
  drawable::draw_layer(LAYER_CU0, g);

  if (drill_p) {
    final_drill(*drill_p);
    delete drill_p;
  }

  return 0;
}
