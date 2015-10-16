/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 */
#include "pwsa.hpp"
#include <math.h>

int main(int argc, char** argv) {
  int split_cutoff; // (K)
  int poll_cutoff; // (D)
  std::string fname;
  int src;
  int dst;
  int srcX; 
  int srcY;
  int dstX;
  int dstY;
  int w;
  bool hasGrid;
  bool useEuc;

  auto init = [&] {
    split_cutoff = (int)pasl::util::cmdline::parse_or_default_int("K", 100);
    poll_cutoff = (int)pasl::util::cmdline::parse_or_default_int("D", 100);
    fname = pasl::util::cmdline::parse_or_default_string("graph", "graphs/simple_weighted.txt");
    hasGrid = pasl::util::cmdline::parse_or_default_bool("hasGrid", false);
    useEuc = pasl::util::cmdline::parse_or_default_bool("useEuc", true);
    src = (int)pasl::util::cmdline::parse_or_default_int("src", 0);
    dst = (int)pasl::util::cmdline::parse_or_default_int("dst", 0);
    srcX = (int)pasl::util::cmdline::parse_or_default_int("srcX", -1);
    srcY = (int)pasl::util::cmdline::parse_or_default_int("srcY", -1);
    dstX = (int)pasl::util::cmdline::parse_or_default_int("dstX", -1);
    dstY = (int)pasl::util::cmdline::parse_or_default_int("dstY", -1);
    w = (int)pasl::util::cmdline::parse_or_default_int("w", 1);
  };

  auto run = [&] (bool sequential) {

    bool isSym = false;

//    graph<asymmetricVertex> g = readGraphFromFile<asymmetricVertex>(fname.c_str(), isSym, 
//        hasGrid);

    std::atomic<long>* res;

    auto r = readMap(fname.c_str());
    auto g = gridGraph(r);
    g.populateIndices();
    std::cout << "n = " << g.number_vertices();
//    gg.printGraph();
    std::cout << std::endl;
//    auto pack = gg.make_vertex_package(0, false, 0);
//    auto ff = [&] (intT id, intT weight) {
//      std::cout << "pair = " << id << " " << weight << std::endl;
//    };
//    gg.apply_to_each_in_range<decltype(ff)>(pack, ff);

    if (srcX != -1) {
      src = g.findVtxWithCoords(srcX, srcY);
      dst = g.findVtxWithCoords(dstX, dstY);
      std::cout << "src " << src << " " << dst << std::endl;
      if (src == -1 || dst == -1) {
        std::cout << "you're fucked" << std::endl;
      }
    }

    if (hasGrid && useEuc) {
      std::pair<int, int> dstCoords = g.getHeuristic(dst);
      std::cout << "dstCoords = " << dstCoords.first << " " << dstCoords.second;

      auto heuristic = [&] (intT vtx) {
        auto vtxCoords = g.getHeuristic(vtx);
        auto res = (int) (sqrt(pow(vtxCoords.first - dstCoords.first, 2) + 
                    pow(vtxCoords.second - dstCoords.second, 2)) * 10000);
        return w * res;
      };

      res = pwsa<Treap<intT, VertexPackage>, 
                        decltype(heuristic),
                        gridGraph>(
                            g, heuristic, src, dst, 
                            split_cutoff, poll_cutoff);
    } else {
      auto heuristic = [&] (intT vtx) { return 0; };
      res = pwsa<Treap<intT, VertexPackage>, 
                              decltype(heuristic), 
                              gridGraph>(g, heuristic, src, dst, 
                                         split_cutoff, poll_cutoff);
    }

    int numExpanded = 0;
    for (int i = 0; i < g.n; i++) {
      if (res[i].load() != -1) {
        numExpanded++;
      }
    }
    std::cout << "expanded : " << numExpanded << " many nodes out of " << g.n << std::endl;
    std::cout << "path length is : " << res[dst].load() <<std::endl; 
  };

  auto output = [&] {;};

  auto destroy = [&] {;};

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
