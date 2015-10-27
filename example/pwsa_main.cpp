/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 */
#include "benchmark.hpp"
#include "pwsa.hpp"
#include "sam_bin_heap.hpp"
#include "weighted_graph.hpp"
//include "bin_heap.hpp"
//include "treap-frontier.hpp"
//include <math.h>

int main(int argc, char** argv) {
  int split_cutoff; // (K)
  int poll_cutoff; // (D)
  std::string fname;
  int srow;
  int scol;
  int drow;
  int dcol;
  double w;
  double exptime;
  bool debug;

  Graph G;
  int src;
  int dst;

  std::atomic<int>* res;

  auto init = [&] {
    split_cutoff = (int)pasl::util::cmdline::parse_or_default_int("K", 10000);
    poll_cutoff = (int)pasl::util::cmdline::parse_or_default_int("D", 10000);
    fname = pasl::util::cmdline::parse_or_default_string("map", "maps/simple_map.map");
    srow = (int)pasl::util::cmdline::parse_or_default_int("sr", 1);
    scol = (int)pasl::util::cmdline::parse_or_default_int("sc", 1);
    drow = (int)pasl::util::cmdline::parse_or_default_int("dr", 1);
    dcol = (int)pasl::util::cmdline::parse_or_default_int("dc", 1);
    w = (double)pasl::util::cmdline::parse_or_default_float("w", 1.0);
    exptime = (double)pasl::util::cmdline::parse_or_default_float("exptime", 0.0000000001);
    debug = pasl::util::cmdline::parse_or_default_bool("debug", false);

    G = Graph(fname.c_str());

    src = G.vertex_at(srow, scol);
    dst = G.vertex_at(drow, dcol);
  };

  auto run = [&] (bool sequential) {
    auto heuristic = [&] (int v) {
      return G.weighted_euclidean(w, v, dst);
    };
    res = pwsa<Graph, Heap<std::pair<int,int>>, decltype(heuristic)>(G, heuristic, src, dst, split_cutoff, poll_cutoff, exptime, debug);
  };

  auto output = [&] {
    int num_expanded = 0;
    for (int i = 0; i < G.number_vertices(); i++) {
      if (res[i].load() != -1) num_expanded++;
    }
    std::cout << "expanded " << num_expanded << std::endl;
    std::cout << "pathlen " << double(res[dst].load())/10000.0 << std::endl;
  };

  auto destroy = [&] {;};

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
