//include "benchmark.hpp"
#include "array-util.hpp"
#include "weighted-graph.hpp"
//include "pairing-heap.hpp"
//include "treap-heap.hpp"

#define INFINITE (-1)
#define NOT_A_VERTEX_ID (-1)

loop_controller_type pars_init_controller("pars_init");
loop_controller_type pars_independence_controller("pars_independence");
loop_controller_type pars_set_dist_controller("pars_set_dist");
loop_controller_type pars_copy_controller("pars_copy");
loop_controller_type pars_successors_controller("pars_successors");

// CAN WE GUARANTEE THAT EACH VERT APPEARS IN `FRONTIER` ONCE?
// RECORD SHORTEST POSSIBLE PATH TO EACH VERTEX SEPARATELY... WRITE WITH MIN

template <class HEURISTIC, class PAIR_HEURISTIC, class GRAPH>
long* pars(GRAPH& graph, const HEURISTIC& heur, const PAIR_HEURISTIC& pair_heur,
           const intT& source, const intT& destination, long K) {
//  std::cout << "PA*RS from " << source << " to " << destination << std::endl;
  intT N = graph.number_vertices();
//  std::cout << "Number vertices: " << N << std::endl;

  long* finalized = array_util::my_malloc<long>(N);                             // finalized[v] is optimal distance to v, or INFINITE if not finalized yet
  std::atomic<long>* distance = array_util::my_malloc<std::atomic<long>>(N);    // distance[v] is g(v), the shortest distance to v found so far
  std::atomic<bool>* discovered = array_util::my_malloc<std::atomic<bool>>(N);  // discovered[v] is true if v is OR WAS in the frontier at some point
  par::parallel_for(pars_init_controller, 0, N, [&] (intT i) {
    finalized[i] = INFINITE;
    distance[i].store(INFINITE);
    discovered[i].store(false);
  });

  intT* frontier = array_util::my_malloc<intT>(1); // set of vertices implemented as unsorted array
  frontier[0] = source;
  long frontier_size = 1;

  distance[source].store(0);
  discovered[source].store(true);

  long round = 0;

  bool* independent = array_util::my_malloc<bool>(N);

  while (finalized[destination] == INFINITE) {
//    std::cout << std::endl << std::endl << "ROUND " << (round++) << std::endl;
//    std::cout << "Frontier size: " << frontier_size << std::endl;

    auto cmp = [&] (intT x, intT y) {
      return (distance[x].load() + heur(x)) - (distance[y].load() + heur(y));
    };

    auto lt = [&] (intT x, intT y) {
      return (distance[x].load() + heur(x)) < (distance[y].load() + heur(y));
    };

    // std::pair<intT*,long> Kfront;
    // if (K < frontier_size) {
    //   intT pivot = array_util::quickselect(cmp, K, frontier, frontier_size);
    //   Kfront = array_util::filter([&] (long i, intT v) { return cmp(v, pivot) <= 0; }, frontier, frontier_size);
    // }
    // else {
    //   intT* temp = nullptr;
    //   temp = array_util::append(frontier, frontier_size, temp, 0);
    //   Kfront = std::make_pair(temp, frontier_size);
    // }

    // par::parallel_for(pars_independence_controller,
    //                   [&] (long lo, long hi) { return Kfront.second * (hi - lo); },
    //                   0l, Kfront.second, [&] (long i) {
    //   auto indep_check = [&] (intT other) {
    //     return distance[Kfront.first[i]].load() <= distance[other].load() + pair_heur(Kfront.first[i], other);
    //   };
    //   bool left = array_util::all(indep_check, Kfront.first, 0, i);
    //   bool right = array_util::all(indep_check, Kfront.first, i+1, Kfront.second);
    //   independent[i] = left && right;
    // });
    //
    // auto independents = array_util::filter([&] (long i, intT ignore) { return independent[i]; },
    //                                             Kfront.first, Kfront.second);

    // if (K < frontier_size) {
    //   std::nth_element(frontier, frontier + K, frontier + frontier_size, lt);
    // }
    //
    // par::parallel_for(pars_independence_controller,
    //                   [&] (long lo, long hi) { return std::min(frontier_size, K) * (hi - lo); },
    //                   0l, std::min(frontier_size, K), [&] (long i) {
    //   auto indep_check = [&] (intT other) {
    //     return distance[frontier[i]].load() <= distance[other].load() + pair_heur(frontier[i], other);
    //   };
    //   bool left = array_util::all(indep_check, frontier, 0, i);
    //   bool right = array_util::all(indep_check, frontier, i+1, std::min(frontier_size, K));
    //   independent[i] = left && right;
    // });
    //
    // auto independents = array_util::filter([&] (long i, intT ignore) { return independent[i]; },
    //                                             frontier, std::min(frontier_size, K));

    par::parallel_for(pars_independence_controller,
                      [&] (long lo, long hi) { return frontier_size * (hi - lo); },
                      0l, frontier_size, [&] (long i) {
      auto indep_check = [&] (intT other) {
        return distance[frontier[i]].load(std::memory_order_relaxed) <= distance[other].load(std::memory_order_relaxed) + pair_heur(frontier[i], other);
      };
      bool left = array_util::all(indep_check, frontier, 0, i);
      bool right = array_util::all(indep_check, frontier, i+1, frontier_size);
      independent[i] = left && right;
    });

    auto independents = array_util::filter([&] (long i, intT ignore) { return independent[i]; },
                                                frontier, frontier_size);

//    std::cout << "number independent: " << independents.second << std::endl;

    if (independents.second == 0) {
      // uh oh. time to vomit useful info and abort.
      abort();
    }

    par::parallel_for(pars_set_dist_controller, 0l, independents.second, [&] (long i) {
      intT v = independents.first[i];
      finalized[v] = distance[v].load();
    });


    // generate all successors
    long* offsets = array_util::plus_scan([&] (long ignore, intT v) { return graph.outDegree(v); },
                                          independents.first, independents.second);
    long successors_len = offsets[independents.second];

    intT* successors = array_util::my_malloc<intT>(successors_len);

    auto weight = [&] (long lo, long hi) {
      return offsets[hi] - offsets[lo];
    };
    par::parallel_for(pars_successors_controller, weight, 0l, independents.second, [&] (long i) {
      intT v = independents.first[i];
      long j = 0;
      graph.apply_to_each_in_range(v, [&] (intT ngh, intT weight) {
        successors[offsets[i] + j] = NOT_A_VERTEX_ID; // may be overwritten soon
        long my_dist_to_ngh = finalized[v] + weight;
        if (finalized[ngh] == INFINITE) {
          while (true) {
            long other_dist_to_ngh = distance[ngh].load();
            if ((other_dist_to_ngh != INFINITE && my_dist_to_ngh >= other_dist_to_ngh)
                || distance[ngh].compare_exchange_weak(other_dist_to_ngh, my_dist_to_ngh)) {
              break;
            }
          }
          if (!discovered[ngh].load()) {
            bool orig = false;
            if (discovered[ngh].compare_exchange_strong(orig, true)) {
              successors[offsets[i] + j] = ngh;
            }
          }
        }
        j++;
      });
    });



    auto all = array_util::append(frontier, frontier_size, successors, successors_len);


    free(frontier);
    free(successors);
    free(offsets);
    free(independents.first);
//    free(independent);
//   free(Kfront.first);

    auto newfrontier = array_util::filter([&] (long i, intT v) { return v != NOT_A_VERTEX_ID && finalized[v] == INFINITE; }, all, frontier_size + successors_len);


    free(all);
    frontier = newfrontier.first;
    frontier_size = newfrontier.second;
  }

  free(frontier);
  free(distance);
  free(discovered);
  free(independent);
  return finalized;
}


// int main(int argc, char** argv) {
//
//   long n;
//   long* xs;
//
//   auto init = [&] {
//     n = (long) pasl::util::cmdline::parse_or_default_long("n", 10);
//     ;
//   };
//
//   auto run = [&] (bool sequential) {
//     ;
//   };
//
//   auto output = [&] {
//     std::cout << std::endl;
//     ;
//   };
//
//   auto destroy = [&] {
//     ;
//   };
//
//   pasl::sched::launch(argc, argv, init, run, output, destroy);
//   return 0;
// }
