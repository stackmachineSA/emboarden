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
#include "color.h"
#include "geometry.h"
#include "drl.h"

using namespace std;
using namespace libpcb;
using namespace emboarden;

bool verbose = false;

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
