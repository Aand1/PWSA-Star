/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 *
 */

#include "benchmark.hpp"
#include "treap-frontier.hpp"
#include "weighted-graph.hpp"

// template <class FRONTIER>
// void pwsa(const WeightedGraph& graph, const vertex& source, const vertex& destination) {
//   nat N = graph.number_vertices();
//   std::atomic<bool>* visited =
// }

int main(int argc, char** argv) {
  long n;

  auto init = [&] {
    n = (long)pasl::util::cmdline::parse_or_default_long("n", 24);
  };

  auto run = [&] (bool sequential) {
    std::cout << n << std::endl;

    auto T = Treap<long,Range<int,int>>();
    for (int i = 0; i < n; i++) {
      long x = rand() % 1000;
      T.insert(x, Range<int,int>(x,x,2*x), x);
    }

    T.check();
    //T.display();

    auto M = T.split_at_weight(T.total_weight() / 2);
    T.check();
    M.check();

    //T.display();
    //M.display();
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
