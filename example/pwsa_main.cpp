/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 */
#include "pwsa.hpp"
#include "bin_heap.hpp"
#include <math.h>

template<class G>
void printRes(G g, std::atomic<long>* res, intT dst) {
  int numExpanded = 0;
  for (int i = 0; i < g.n; i++) {
    if (res[i].load() != -1) {
      numExpanded++;
    }
  }
//  std::cout << "expanded=" << numExpanded << std::endl;
//  std::cout << "path length=" << res[dst].load() <<std::endl;
}

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
  double w;
  bool isGrid;
  bool useEuc;

  std::atomic<long>* res;

//  long result;

  gridGraph grid;
  graph<asymmetricVertex> g;

  auto init = [&] {
    split_cutoff = (int)pasl::util::cmdline::parse_or_default_int("K", 10000);
    poll_cutoff = (int)pasl::util::cmdline::parse_or_default_int("D", 10000);
    fname = pasl::util::cmdline::parse_or_default_string("graph", "graphs/simple_weighted.txt");
    isGrid = pasl::util::cmdline::parse_or_default_bool("isGrid", true);
    useEuc = pasl::util::cmdline::parse_or_default_bool("useEuc", true);
    src = (int)pasl::util::cmdline::parse_or_default_int("src", 0);
    dst = (int)pasl::util::cmdline::parse_or_default_int("dst", 0);
    // These are passed as in x=col, y=row, so invert them
    // if we get a srcX != -1, we ignore src,dst and replace them
    // by looking up in our grid
    srcY = (int)pasl::util::cmdline::parse_or_default_int("srcX", -1);
    srcX = (int)pasl::util::cmdline::parse_or_default_int("srcY", -1);
    dstY = (int)pasl::util::cmdline::parse_or_default_int("dstX", -1);
    dstX = (int)pasl::util::cmdline::parse_or_default_int("dstY", -1);
    w = (double)pasl::util::cmdline::parse_or_default_float("w", 1.0);
    if (isGrid == 1) {
      auto r = readMap(fname.c_str());

      grid = gridGraph(r);
      grid.populateIndices();

      if (srcX != -1) {
        src = grid.findVtxWithCoords(srcX, srcY);
        dst = grid.findVtxWithCoords(dstX, dstY);
        if (src == -1 || dst == -1) {
          std::cout << "Couldn't find node" << std::endl;
          return -1;
        }
      }
    } else {
      bool isSym = false;
      graph<asymmetricVertex> g = readGraphFromFile<asymmetricVertex>(fname.c_str(), isSym);
    }
  };

  auto run = [&] (bool sequential) {

    if (isGrid == 1) {
      // use grid parsing functionalities

//      std::cout << "n=" << grid.number_vertices() << std::endl;

      if (useEuc) {
        std::pair<int, int> dstCoords = grid.getHeuristic(dst);
        auto heuristic = [&] (intT vtx) {
          auto vtxCoords = grid.getHeuristic(vtx);
          auto h = (int) (w * (sqrt(pow(vtxCoords.first - dstCoords.first, 2) +
                      pow(vtxCoords.second - dstCoords.second, 2)) * 10000));
          return h;
        };
        res = pwsa<Heap<VertexPackage>,
                          decltype(heuristic),
                          gridGraph>(
                              grid, heuristic, src, dst,
                              split_cutoff, poll_cutoff);
//        printRes(grid, res, dst);
      } else {
        auto heuristic = [&] (intT vtx) { return 0; };
        res = pwsa<Heap<VertexPackage>,
                          decltype(heuristic),
                          gridGraph>(
                              grid, heuristic, src, dst,
                              split_cutoff, poll_cutoff);
//        printRes(grid, res, dst);
      }
    } else {
//      std::cout << "n=" << g.number_vertices() << std::endl;
      auto heuristic = [&] (intT vtx) { return 0; };
      res = pwsa<Heap<VertexPackage>,
                              decltype(heuristic),
                              graph<asymmetricVertex> >(g, heuristic, src, dst,
                                             split_cutoff, poll_cutoff);
//      printRes(g, res, dst);
    }
  };

  auto output = [&] {
    std::cout << "pathlen " << double(res[dst].load())/10000.0 << std::endl;
  };

  auto destroy = [&] {;};

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
