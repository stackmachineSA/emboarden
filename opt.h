#ifndef OPT_H
#define OPT_H

#include <string>
#include <map>
#include <functional>

namespace emboarden {
  extern bool verbose;

  struct program_opt {
    program_opt(
      std::string opt, std::string short_desc, std::string long_desc,
      std::function<void()> action
    );

    program_opt(
      std::string opt, std::string short_desc,
      std::string long_desc, std::string arg_name,
      std::function<void(std::string)> action_arg
    );

    std::string opt, arg_name, short_desc, long_desc;
    bool has_arg;

    std::function<void()> action;
    std::function<void(std::string)> action_arg;

    static int longest_optname;

    static std::map<std::string, program_opt *> opts;
    static std::string usage_args;

    static void print_help_msg();
    static void print_usage_msg(std::string cmd);
    static std::string parse(int argc, char **argv);
  };
}

#endif
