#include <iostream>
#include <sstream>

#include <map>
#include <vector>
#include <functional>

#include "opt.h"

using namespace std;

namespace emboarden {
  bool verbose = false;

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
}
