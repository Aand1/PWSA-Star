/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.hpp
 */
#include "benchmark.hpp"
#include "treap-frontier.hpp"
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

bool shouldPrint = false;
template <class Body>
void print(const Body& b) {
  if (shouldPrint) {
    pasl::util::atomic::msg(b);
  }
}

template <class FRONTIER, class HEURISTIC, class GRAPH>
std::atomic<long>* pwsa(GRAPH& graph, const HEURISTIC& heuristic,
                        const intT& source, const intT& destination,
                        int split_cutoff, int poll_cutoff) {
  intT N = graph.number_vertices();
  std::atomic<long>* finalized = pasl::data::mynew_array<std::atomic<long>>(N);
  fill_array_par(finalized, N, -1l);

  FRONTIER initF = FRONTIER();
  int heur = heuristic(source);
  VertexPackage vtxPackage = graph.make_vertex_package(source, false, 0);
  vtxPackage.display();
  initF.insert(heur, vtxPackage);

  pasl::data::perworker::array<int> work_since_split;
  work_since_split.init(0);

  auto size = [&] (FRONTIER& frontier) {
    auto sz = frontier.total_weight();
    if (sz == 0) {
      work_since_split.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (work_since_split.mine() > split_cutoff && sz > 1)) {
      return 2; // split
    }
    else {
      return 1; // don't split
    }
  };

  auto fork = [&] (FRONTIER& src, FRONTIER& dst) {
    print([&] { 
      std::cout << "splitting "; src.display(); 
      std::cout << std::endl; 
    });
    src.split_at(src.total_weight() / 2, dst);
    print([&] { 
      std::cout << "produced "; src.display(); std::cout << "; "; 
      dst.display(); std::cout << std::endl; 
    });
    work_since_split.mine() = 0;
  };

  auto set_in_env = [&] (FRONTIER& f) {;};

  auto do_work = [&] (FRONTIER& frontier) {
    print([&] {
      std::cout << "Frontier dump: "; frontier.display(); 
      std::cout << std::endl;
    });

    int work_this_round = 0;
    while (work_this_round < poll_cutoff && frontier.total_weight() > 0) {
      auto pair = frontier.delete_min();
      VertexPackage vpack = pair.second;
      long orig = -1l;
      if (vpack.mustProcess || 
         (finalized[vpack.vertexId].load() == -1 && 
          finalized[vpack.vertexId].compare_exchange_strong(orig, vpack.distance))) {
        if (vpack.vertexId == destination) {
          print([&] { 
            std::cout << "Found destination: distance = " << 
            finalized[destination].load() << std::endl; 
          });
          return true;
        }
        if (work_this_round + vpack.weight() > poll_cutoff) {
          // need to split vpack
          VertexPackage other = VertexPackage();
          vpack.split_at(poll_cutoff - work_this_round, other);
          other.mustProcess = true;
          if (other.weight() != 0) {
            frontier.insert(pair.first, other);
          }
        }
        // Have to process our vpack
        graph.apply_to_each_in_range(vpack, [&] (intT ngh, intT weight) {
          VertexPackage nghpack = graph.make_vertex_package(ngh, false, vpack.distance + weight);
          int heur = heuristic(ngh) + vpack.distance + weight;
          frontier.insert(heur, nghpack);

          print([&] { 
            std::cout << "inserted pack "; nghpack.display(); 
            std::cout << ": "; frontier.display(); std::cout << std::endl; 
          });
        });
        work_this_round += vpack.weight();
      } else {
        // Account 1 for popping.
        work_this_round += 1;
      }
    }
    work_since_split.mine() += work_this_round;
    return false;
  };

  std::cout << "calling par_while" << std::endl;

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
  return finalized;
}


