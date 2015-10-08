/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 *
 */

#include "benchmark.hpp"
#include "treap-frontier.hpp"

int main(int argc, char** argv) {
  long n;
  Treap<long,long> T;

  auto init = [&] {
    n = (long)pasl::util::cmdline::parse_or_default_long("n", 24);
  };

  auto run = [&] (bool sequential) {
    std::cout << n << std::endl;
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
