/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.cpp
 *
 */

#include "benchmark.hpp"
#include "treap-frontier.hpp"
//include "hash.hpp"
#include "weighted-graph.hpp"
#include "native.hpp"
#include "defaults.hpp"
#include <cstring>

static inline void pmemset(char * ptr, int value, size_t num) {
  const size_t cutoff = 100000;
  if (num <= cutoff) {
    std::memset(ptr, value, num);
  } else {
    long m = num/2;
    pasl::sched::native::fork2([&] {
      pmemset(ptr, value, m);
    }, [&] {
      pmemset(ptr+m, value, num-m);
    });
  }
}

template <class Number, class Size>
void fill_array_par(std::atomic<Number>* array, Size sz, Number val) {
  pmemset((char*)array, val, sz*sizeof(Number));
}

template <class FRONTIER, class HEURISTIC, class VERTEX>
std::atomic<long>* pwsa(graph<VERTEX>& graph, const HEURISTIC& heuristic,
                        const intT& source, const intT& destination,
                        int split_cutoff, int poll_cutoff) {
  intT N = graph.number_vertices();
  std::atomic<long>* finalized = pasl::data::mynew_array<std::atomic<long>>(N);
  fill_array_par(finalized, N, -1l);

  FRONTIER initF = FRONTIER();
  int heur = heuristic(source);
  VertexPackage vtxPackage = graph.make_vertex_package(source, false, 0);
  initF.insert(heur, vtxPackage);

  pasl::data::perworker::array<int> work_since_split;
  work_since_split.init(0);

  auto size = [&] (FRONTIER& frontier) {
    auto sz = frontier.total_weight();
    if (sz == 0) {
      work_since_split.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (work_since_split.mine() > split_cutoff && sz > 1))
      return 2; // split
    else
      return 1; // don't split
  };

  auto fork = [&] (FRONTIER& src, FRONTIER& dst) {
    src.split_at(src.total_weight() / 2, dst);
    work_since_split.mine() = 0;
  };

  auto set_in_env = [&] (FRONTIER& f) {
    ; // nothing to do
  };

  auto do_work = [&] (FRONTIER& frontier) {
    int work_this_round = 0;
    while (work_this_round < poll_cutoff && frontier.total_weight() > 0) {
      auto pair = frontier.delete_min();
      VertexPackage vpack = pair.second;
      long orig = -1l;
      if (vpack.mustProcess || (finalized[vpack.vertexId].load() == -1 && finalized[vpack.vertexId].compare_exchange_strong(orig, vpack.distance))) {
        if (vpack.vertexId == destination) {
          return true;
        }
        if (work_this_round + vpack.weight() > poll_cutoff) {
          // need to split vpack
          VertexPackage other = VertexPackage();
          vpack.split_at(poll_cutoff - work_this_round, other);
          other.mustProcess = true;
          frontier.insert(pair.first, other);
        }
        // Have to process our vpack
        graph.apply_to_each_in_range(vpack, [&] (intT ngh, intT weight) {
          VertexPackage nghpack = graph.make_vertex_package(ngh, false, vpack.distance + weight);
          int heur = heuristic(ngh) + vpack.distance + weight; 
          frontier.insert(heur, nghpack);
        });
        work_this_round += vpack.weight();
      } else {
        // Account 1 for popping.
        work_this_round += 1;
      }
    }
    return false;
  };

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
  return finalized;
}

// class X {
// public:
//   long x;
//
//   X(long x) : x(x) { }
//
//   long hash() const { return hash_signed(x); }
//
//   inline bool operator < (const X& b) const { return x < b.x; }
//   inline bool operator <= (const X& b) const { return x <= b.x; }
//   inline bool operator == (const X& b) const { return x == b.x; }
// };
//
// std::ostream& operator << (std::ostream& out, const X& a) {
//   return out << a.x;
// }
//
// class Y {
// public:
//   long y;
//
//   Y(long y) : y(y) { }
//
//   long weight() const { return y; }
//
//   void split_at(long w, Y& dst) {
//     dst.y = y - w;
//     y = w;
//   }
// };
//
// std::ostream& operator << (std::ostream& out, const Y& a) {
//   return out << a.y;
// }

int main(int argc, char** argv) {
  long n;

  auto init = [&] {
    n = (long)pasl::util::cmdline::parse_or_default_long("n", 24);
  };

  auto run = [&] (bool sequential) {
    std::cout << n << std::endl;

    char const* fname = "simple_weighted.txt";
    bool isSym = false;
    graph<asymmetricVertex> g = readGraphFromFile<asymmetricVertex>(fname, isSym);

    g.printGraph();
    auto heuristic = [] (intT vtx) { return 0; };
    std::atomic<long>* res = pwsa<Treap<intT, VertexPackage>, decltype(heuristic), asymmetricVertex>(g, heuristic, 0, 4, 1, 1);
    for (int i = 0; i < g.n; i++) {
      std::cout << "res[" << i << "] = " << res[i].load() << std::endl;
    }
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
