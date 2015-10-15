/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 */
#include "pwsa.hpp"

int main(int argc, char** argv) {
  int split_cutoff; // (K)
  int poll_cutoff; // (D)
  std::string fname;
  int src;
  int dst;

  auto init = [&] {
    split_cutoff = (int)pasl::util::cmdline::parse_or_default_int("K", 100);
    poll_cutoff = (int)pasl::util::cmdline::parse_or_default_int("D", 100);
    fname = pasl::util::cmdline::parse_or_default_string("graph", "graphs/simple_weighted.txt");
    src = (int)pasl::util::cmdline::parse_or_default_int("src", 0);
    dst = (int)pasl::util::cmdline::parse_or_default_int("dst", 0);
  };

  auto run = [&] (bool sequential) {

    bool isSym = false;
    graph<asymmetricVertex> g = readGraphFromFile<asymmetricVertex>(fname.c_str(), isSym);

    // g.printGraph();
    auto heuristic = [] (intT vtx) { return 0; };
    std::atomic<long>* res = pwsa<Treap<intT, VertexPackage>, 
                                  decltype(heuristic), 
                                  asymmetricVertex>(g, heuristic, src, dst, 
                                                    split_cutoff, poll_cutoff);

    int numExpanded = 0;
    for (int i = 0; i < g.n; i++) {
      if (res[i].load() != -1) {
        numExpanded++;
      }
    }
    std::cout << "expanded : " << numExpanded << " many nodes out of " << g.n << std::endl;
    std::cout << "path lengh is : " << res[dst].load() <<std::endl; 

//    for (int i = 0; i < g.n; i++) {
//      std::cout << "res[" << i << "] = " << res[i].load() << std::endl;
//    }
  };

  auto output = [&] {;};

  auto destroy = [&] {;};

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
