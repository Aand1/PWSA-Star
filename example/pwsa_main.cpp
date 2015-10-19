/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 */
#include "pwsa.hpp"
#include <math.h>


template<class G>
void printRes(G g) {
  int numExpanded = 0;
  for (int i = 0; i < g.n; i++) {
    if (res[i].load() != -1) {
      numExpanded++;
    }
  }
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
  int w;
  bool isGrid;
  bool useEuc;

  auto init = [&] {
    split_cutoff = (int)pasl::util::cmdline::parse_or_default_int("K", 10000);
    poll_cutoff = (int)pasl::util::cmdline::parse_or_default_int("D", 10000);
    fname = pasl::util::cmdline::parse_or_default_string("graph", "graphs/simple_weighted.txt");
    isGrid = pasl::util::cmdline::parse_or_default_bool("isGrid", false);
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
  };

  auto run = [&] (bool sequential) {

    bool isSym = false;

    std::atomic<long>* res;
    if (isGrid == 1) {
      // use grid parsing functionalities 
      auto r = readMap(fname.c_str());

      gridGraph grid = gridGraph(r);
      grid.populateIndices();

      if (srcX != -1) {
        src = grid.findVtxWithCoords(srcX, srcY);
        dst = grid.findVtxWithCoords(dstX, dstY);
        std::cout << "src " << src << " " << dst << std::endl;
        if (src == -1 || dst == -1) {
          std::cout << "you're fucked" << std::endl;
        }
      }

      std::cout << "n = " << grid.number_vertices();
      std::cout << std::endl;

      if (useEuc) {
        std::pair<int, int> dstCoords = grid.getHeuristic(dst);
        std::cout << "dstCoords = " << dstCoords.first << " " << dstCoords.second;

        // can't express raw type of a lambda (uninitialized), so we have to
        // do mad hacks
        auto heuristic = [&] (intT vtx) {
          auto vtxCoords = grid.getHeuristic(vtx);
          auto h = (int) (sqrt(pow(vtxCoords.first - dstCoords.first, 2) + 
                      pow(vtxCoords.second - dstCoords.second, 2)) * 10000);
          return w * h;
        };

        res = pwsa<Treap<intT, VertexPackage>, 
                          decltype(heuristic),
                          gridGraph>(
                              grid, heuristic, src, dst, 
                              split_cutoff, poll_cutoff);
      } else {
        auto heuristic = [&] (intT vtx) { return 0; };
        res = pwsa<Treap<intT, VertexPackage>, 
                          decltype(heuristic),
                          gridGraph>(
                              grid, heuristic, src, dst, 
                              split_cutoff, poll_cutoff);
      }

    } else {
      graph<asymmetricVertex> gr = readGraphFromFile<asymmetricVertex>(fname.c_str(), isSym);
      auto heuristic = [&] (intT vtx) { return 0; };
      res = pwsa<Treap<intT, VertexPackage>, 
                              decltype(heuristic), 
                              graph<asymmetricVertex> >(gr, heuristic, src, dst, 
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
