/* COPYRIGHT (c) 2014 Umut Acar, Arthur Chargueraud, and Michael
 * Rainey
 * All rights reserved.
 *
 * \file dfs.hpp
 *
 */

#include "benchmark.hpp"



int main(int argc, char** argv) {
  long n;

  auto init = [&] {
    n = (long)pasl::util::cmdline::parse_or_default_long("n", 24);
  };

  auto run = [&] (bool sequential) {
    std::cout << n << std::endl;
    hi();
  };

  auto output = [&] {
    ;
  };

  auto destroy = [&] {
    ;
  };

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
