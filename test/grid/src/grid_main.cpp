/* COPYRIGHT (c) 2016 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 */
#include "benchmark.hpp"
#include "pwsa.hpp"
#include "astar.hpp"
#include "wpanre.hpp"
#include "pase.hpp"
#include "bin_heap.hpp"
#include "result.hpp"
#include "weighted_graph.hpp"

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
  bool pathcorrect;
  double opt;
  std::string visualize;
  std::string algo;

  bool pebble;

  Graph G;
  int src;
  int dst;

  SearchResult* result;
  std::function<SearchResult*()> search;
  std::function<int(int)> heuristic;
  std::function<int(int,int)> pase_heuristic;

  auto init = [&] {
    split_cutoff = (int)pasl::util::cmdline::parse_or_default_int("K", 8);
    poll_cutoff = (int)pasl::util::cmdline::parse_or_default_int("D", 1);
    fname = pasl::util::cmdline::parse_or_default_string("map", "maps/simple_map.map");
    srow = (int)pasl::util::cmdline::parse_or_default_int("sr", 1);
    scol = (int)pasl::util::cmdline::parse_or_default_int("sc", 1);
    drow = (int)pasl::util::cmdline::parse_or_default_int("dr", 1);
    dcol = (int)pasl::util::cmdline::parse_or_default_int("dc", 1);
    w = (double)pasl::util::cmdline::parse_or_default_float("w", 1.0);
    exptime = (double)pasl::util::cmdline::parse_or_default_float("exptime", 0.0);
    pathcorrect = pasl::util::cmdline::parse_or_default_bool("pathcorrect", false);
    opt = (double)pasl::util::cmdline::parse_or_default_float("opt", 1.0);
    visualize = pasl::util::cmdline::parse_or_default_string("visualize", "");

    G = Graph(fname.c_str());

    src = G.vertex_at(srow, scol);
    dst = G.vertex_at(drow, dcol);

    pebble = !visualize.empty();

    heuristic = [&] (int v) { return G.weighted_euclidean(w, v, dst); };
    pase_heuristic = [&] (int u, int v) { return G.weighted_euclidean(w, u, v); };

    pasl::util::cmdline::argmap<std::function<SearchResult*()>> m;
    m.add("wA*",           [&] { return astar<Graph, Heap<int>, decltype(heuristic)>(G, heuristic, src, dst, exptime, pebble); });
    m.add("wPA*NRE",       [&] { return wpanre<Graph, Heap<int>, decltype(heuristic)>(G, heuristic, src, dst, exptime, pebble); });
    m.add("wPA*SE",        [&] { return pase<Graph, decltype(pase_heuristic)>(G, pase_heuristic, src, dst, exptime, pebble); });
    m.add("wPWSA*",        [&] { return pwsa_pc<Graph, Heap<int>, decltype(heuristic)>(G, heuristic, src, dst, split_cutoff, poll_cutoff, exptime, pebble); });
    m.add("simple_wPWSA*", [&] { return pwsa<Graph, Heap<int>, decltype(heuristic)>(G, heuristic, src, dst, split_cutoff, poll_cutoff, exptime, pebble); });
    search = m.find_by_arg("algo");
  };

  auto run = [&] (bool sequential) {
    result = search();
  };

  auto output = [&] {
    int num_expanded = 0;
    for (int i = 0; i < G.number_vertices(); i++)
      if (result->is_expanded(i)) num_expanded++;
    double pathlen = double(G.pathlen(result, src, dst)) / 10000.0;

    std::cout << "expanded " << num_expanded << std::endl;
    std::cout << "pathlen " << pathlen << std::endl;
    std::cout << "deviation " << pathlen / opt << std::endl;

    if (pebble) G.pebble_dump(result, src, dst, visualize.c_str());
  };

  auto destroy = [&] {;};

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
