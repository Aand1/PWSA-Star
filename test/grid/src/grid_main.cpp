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
#include "weighted_graph.hpp"
#include <iomanip>

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
  std::string cost_type;

  bool pebble;

  Graph G;
  LifeGraph LG;
  int src;
  int dst;

  SearchResult* result;
  SearchResultT<long>* lresult;
  std::function<SearchResult*()> search;
  std::function<SearchResultT<long>*()> lsearch;
  std::function<int(int)> heuristic;
  std::function<long(int)> lheuristic;
  std::function<int(int,int)> pase_heuristic;
  std::function<long(int,int)> lpase_heuristic;

  auto init = [&] {
    split_cutoff = (int)pasl::util::cmdline::parse_or_default_int("K", 8);
    poll_cutoff = (int)pasl::util::cmdline::parse_or_default_int("D", 1);
    fname = pasl::util::cmdline::parse_or_default_string("map", "maps/simple_map.map");
    cost_type = pasl::util::cmdline::parse_or_default_string("cost", "unit");
    srow = (int)pasl::util::cmdline::parse_or_default_int("sr", 1);
    scol = (int)pasl::util::cmdline::parse_or_default_int("sc", 1);
    drow = (int)pasl::util::cmdline::parse_or_default_int("dr", 1);
    dcol = (int)pasl::util::cmdline::parse_or_default_int("dc", 1);
    w = (double)pasl::util::cmdline::parse_or_default_float("w", 1.0);
    exptime = (double)pasl::util::cmdline::parse_or_default_float("exptime", 0.0);
    pathcorrect = pasl::util::cmdline::parse_or_default_bool("pathcorrect", false);
    opt = (double)pasl::util::cmdline::parse_or_default_float("opt", 1.0);
    visualize = pasl::util::cmdline::parse_or_default_string("visualize", "");

    pebble = !visualize.empty();

    if (cost_type == "unit") {
      G = Graph(fname.c_str());

      src = G.vertex_at(srow, scol);
      dst = G.vertex_at(drow, dcol);
      if (src == -1 || dst == -1) {
        std::cerr << "ERROR (main): invalid source or destination" << std::endl;
        std::exit(EXIT_FAILURE);
      }

      heuristic = [&] (int v) { return G.weighted_euclidean(w, v, dst); };
      pase_heuristic = [&] (int u, int v) { return G.weighted_euclidean(w, u, v); };

      pasl::util::cmdline::argmap<std::function<SearchResult*()>> m;
      m.add("wA*",           [&] { return astar<Graph, Heap<int>, decltype(heuristic)>(G, heuristic, src, dst, exptime, pebble); });
      m.add("wPA*NRE",       [&] { return wpanre<Graph, Heap<int>, decltype(heuristic)>(G, heuristic, src, dst, exptime, pebble); });
      m.add("wPA*SE",        [&] { return pase<Graph, decltype(pase_heuristic)>(G, pase_heuristic, src, dst, exptime, pebble); });
      m.add("wPWSA*",        [&] { return pwsa_pc<Graph, Heap<int>, decltype(heuristic)>(G, heuristic, src, dst, split_cutoff, poll_cutoff, exptime, pebble); });
      m.add("simple_wPWSA*", [&] { return pwsa<Graph, Heap<int>, decltype(heuristic)>(G, heuristic, src, dst, split_cutoff, poll_cutoff, exptime, pebble); });
      search = m.find_by_arg("algo");
    }
    else if (cost_type == "life") {
      LG = LifeGraph(fname.c_str());

      src = LG.vertex_at(srow, scol);
      dst = LG.vertex_at(drow, dcol);
      if (src == -1 || dst == -1) {
        std::cerr << "ERROR (main): invalid source or destination" << std::endl;
        std::exit(EXIT_FAILURE);
      }

      lheuristic = [&] (int v) { return LG.weighted_manhattan(w, v, dst); };
      lpase_heuristic = [&] (int u, int v) { return LG.weighted_manhattan(w, u, v); };

      pasl::util::cmdline::argmap<std::function<SearchResultT<long>*()>> m;
      m.add("wA*",           [&] { return astarT<LifeGraph, Heap<long>, decltype(lheuristic), long>(LG, lheuristic, src, dst, exptime, pebble); });
      //m.add("wPA*NRE",       [&] { return wpanre<LifeGraph, Heap<int>, decltype(heuristic)>(LG, heuristic, src, dst, exptime, pebble); });
      //m.add("wPA*SE",        [&] { return pase<LifeGraph, decltype(pase_heuristic)>(LG, pase_heuristic, src, dst, exptime, pebble); });
      m.add("wPWSA*",        [&] { return pwsa_pcT<LifeGraph, Heap<long>, decltype(lheuristic), long>(LG, lheuristic, src, dst, split_cutoff, poll_cutoff, exptime, pebble); });
      //m.add("simple_wPWSA*", [&] { return pwsa<LifeGraph, Heap<int>, decltype(heuristic)>(LG, heuristic, src, dst, split_cutoff, poll_cutoff, exptime, pebble); });
      lsearch = m.find_by_arg("algo");
    }
    else {
      std::cerr << "ERROR (main): cost_type must be either \"unit\" or \"life\"" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  };

  auto run = [&] (bool sequential) {
    if (cost_type == "unit") result = search();
    else if (cost_type == "life") lresult = lsearch();
    else {
      std::cerr << "ERROR (main): cost_type must be either \"unit\" or \"life\"" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  };

  auto output = [&] {
    if (cost_type == "unit") {
      int num_expanded = 0;
      for (int i = 0; i < G.number_vertices(); i++)
        if (result->is_expanded(i)) num_expanded++;
      double pathlen = double(G.pathlen(result, src, dst)) / 10000.0;

      std::cout << "expanded " << num_expanded << std::endl;
      std::cout << "pathlen " << std::setprecision(8) << pathlen << std::endl;
      std::cout << "deviation " << pathlen / opt << std::endl;

      if (pebble) G.pebble_dump(result, src, dst, visualize.c_str());
    }
    else if (cost_type == "life") {
      int num_expanded = 0;
      for (int i = 0; i < LG.number_vertices(); i++)
        if (lresult->is_expanded(i)) num_expanded++;
      double pathlen = double(LG.pathlen(lresult, src, dst)) / 10000.0;

      std::cout << "expanded " << num_expanded << std::endl;
      std::cout << "pathlen " << std::setprecision(8) << pathlen << std::endl;
      std::cout << "deviation " << pathlen / opt << std::endl;

      if (pebble) LG.pebble_dump(lresult, src, dst, visualize.c_str());
    }
  };

  auto destroy = [&] {;};

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
