#include <iostream>
#include <fstream>

#include <map>
#include <vector>
#include <sstream>

#include <libpcb/basic.h>

#include "image-io.h"
#include "color.h"
#include "geometry.h"
#include "drl.h"
#include "opt.h"

using namespace std;
using namespace libpcb;
using namespace emboarden;

int main(int argc, char **argv) {
  program_opt::usage_args = "[options ...] input_file.png";

  program_opt p_h(
    "-h", "Display help message.",
    "Display a help message.",
    [](){
      program_opt::print_help_msg();
    }
  );

  program_opt p_simp(
    "-simp", "Number of polygon simplification passes (default 10).",
    "Number of polygon simplification passes (default 10).", "passes",
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
    "Dump debugging image with color-coded components.", ".png filename",
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
    "-v", "Enable verbose output.",
    "Enable printing verbose output.",
    [](){ verbose = true; }
  );

  string debug_svg_filename;
  program_opt p_svg(
    "-svg", "Set output .svg filename.",
    "Set filename for debugging .svg file.", ".svg filename",
    [&](string filename) { debug_svg_filename = filename; }
  );
  
  string input_filename = program_opt::parse(argc, argv);

  ofstream gfile;
  if (output_filename == "") {
    size_t pos = input_filename.find_first_of(".");
    string stem = (pos == string::npos) ?
      input_filename : input_filename.substr(0, pos);
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
