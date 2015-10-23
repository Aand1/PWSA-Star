/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 */
#include "pars.hpp"
#include <math.h>


template<class G>
void printRes(G g, long* res, intT dst) {
  int numExpanded = 0;
  for (int i = 0; i < g.n; i++) {
    if (res[i] != -1) {
      numExpanded++;
    }
  }
//  std::cout << "expanded=" << numExpanded << std::endl;
//  std::cout << "path length=" << res[dst] << std::endl;
}

int main(int argc, char** argv) {
  long K;
  std::string fname;
  int src;
  int dst;
  int srcX;
  int srcY;
  int dstX;
  int dstY;
  int w;
  bool isGrid;
  bool useEuc;

  gridGraph grid;
  graph<asymmetricVertex> g;

  auto init = [&] {
    // split_cutoff = (int)pasl::util::cmdline::parse_or_default_int("K", 10000);
    // poll_cutoff = (int)pasl::util::cmdline::parse_or_default_int("D", 10000);
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
    w = (int)pasl::util::cmdline::parse_or_default_int("w", 1);
    K = (long)pasl::util::cmdline::parse_or_default_long("K", 100);
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

    long* res;
    if (isGrid == 1) {
      // use grid parsing functionalities

//      std::cout << "n=" << grid.number_vertices() << std::endl;

      if (useEuc) {
        std::pair<int, int> dstCoords = grid.getHeuristic(dst);
        auto heur = [&] (intT vtx) {
          auto vtxCoords = grid.getHeuristic(vtx);
          auto h = (int) (sqrt(pow(vtxCoords.first - dstCoords.first, 2) +
                      pow(vtxCoords.second - dstCoords.second, 2)) * 10000);
          return w * h;
        };
        auto pair_heur = [&] (intT v1, intT v2) {
          auto v1Coords = grid.getHeuristic(v1);
          auto v2Coords = grid.getHeuristic(v2);
          auto h = (long) (sqrt(pow(v1Coords.first - v2Coords.first, 2) +
                      pow(v1Coords.second - v2Coords.second, 2)) * 10000);
          return w * h;
        };
        res = pars<decltype(heur),decltype(pair_heur),gridGraph>(grid, heur, pair_heur, src, dst, K);
        // res = pwsa<Treap<intT, VertexPackage>,
        //                   decltype(heuristic),
        //                   gridGraph>(
        //                       grid, heuristic, src, dst,
        //                       split_cutoff, poll_cutoff);
        printRes(grid, res, dst);
      } else {
        auto heur = [&] (intT v) { return 0l; };
        auto pair_heur = [&] (intT v1, intT v2) { return 0l; };
        res = pars<decltype(heur),decltype(pair_heur),gridGraph>(grid, heur, pair_heur, src, dst, K);
        // res = pwsa<Treap<intT, VertexPackage>,
        //                   decltype(heuristic),
        //                   gridGraph>(
        //                       grid, heuristic, src, dst,
        //                       split_cutoff, poll_cutoff);
        printRes(grid, res, dst);
      }
    } else {
      // std::cout << "n=" << g.number_vertices() << std::endl;
      // auto heuristic = [&] (intT vtx) { return 0; };
      // res = pwsa<Treap<intT, VertexPackage>,
      //                         decltype(heuristic),
      //                         graph<asymmetricVertex> >(g, heuristic, src, dst,
      //                                        split_cutoff, poll_cutoff);
      // printRes(g, res, dst);
      ;
      std::cout << "Must be grid... exiting..." << std::endl;
    }
  };

  auto output = [&] {;};

  auto destroy = [&] {;};

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
