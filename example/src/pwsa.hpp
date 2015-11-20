/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.hpp
 */
#include "container.hpp"
#include "native.hpp"
#include "bin_heap.hpp"
#include <cstring>
#include <climits>

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

// d1 could be -1 (infinite), while d2 must be >= 0
// inline bool dist_greater(int d1, int d2) {
//   return d1 == -1 || d1 > d2;
// }

struct vertpack {
  int distance;
  int pred;
};

template <class GRAPH, class HEAP, class HEURISTIC>
std::pair<std::atomic<vertpack>*, std::atomic<bool>*>
pwsa_pathcorrect(GRAPH& graph, const HEURISTIC& heuristic,
                 const int& source, const int& destination,
                 int split_cutoff, int poll_cutoff, double exptime,
                 int* pebbles = nullptr) {
  int N = graph.number_vertices();
  std::atomic<bool>* is_expanded = pasl::data::mynew_array<std::atomic<bool>>(N);
  std::atomic<vertpack>* gpred = pasl::data::mynew_array<std::atomic<vertpack>>(N);
  fill_array_par(is_expanded, N, false);
  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    vertpack ith;
    ith.distance = INT_MAX; // "infinite"
    ith.pred = -1;
    gpred[i].store(ith);
  });

  vertpack src;
  src.distance = 0;
  src.pred = source;
  gpred[source].store(src);

  HEAP initF = HEAP();
  int heur = heuristic(source);
  initF.insert(heur, source);

  pasl::data::perworker::array<int> expanded_since_exchange;
  expanded_since_exchange.init(0);

  auto size = [&] (HEAP& frontier) {
    auto sz = frontier.size();
    if (sz == 0) {
      expanded_since_exchange.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (expanded_since_exchange.mine() > split_cutoff && sz > 1)) {
      return 2; // split
    }
    else {
      return 1; // don't split
    }
  };

  auto fork = [&] (HEAP& src, HEAP& dst) {
    src.split(dst);
    expanded_since_exchange.mine() = 0;
  };

  auto do_work = [&] (std::atomic<bool>& is_done, HEAP& frontier) {
  //auto do_work = [&] (bool& is_done, HEAP& frontier) {
    int expanded_this_round = 0;
    while (expanded_this_round < poll_cutoff && frontier.size() > 0 && !is_done.load()) {
      int v = frontier.delete_min();
      expanded_this_round++;

      bool orig = is_expanded[v].load();
      if (!orig && is_expanded[v].compare_exchange_strong(orig, true)) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();

        if (v == destination) {
          is_done.store(true);
          //is_done = true;
          return;
        }

        // SIMULATE EXPANSION
        graph.simulate_get_successors(exptime);

        graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {
          vertpack gpred_v = gpred[v].load();

          vertpack mine;
          mine.distance = gpred_v.distance + weight;
          mine.pred = v;

          while (true) {
            vertpack gpred_nbr = gpred[nbr].load();
            if (mine.distance < gpred_nbr.distance) {
              if (gpred[nbr].compare_exchange_weak(gpred_nbr, mine)) {
                if (!is_expanded[nbr].load()) {
                  frontier.insert(mine.distance + heuristic(nbr), nbr);
                }
                break;
              }
            }
            else {
              break;
            }
          }
        });
      } 
    }
    expanded_since_exchange.mine() += expanded_this_round;
  };

  pasl::sched::native::parallel_while_pwsa_maybe_faster(initF, size, fork, do_work);
  //pasl::sched::native::parallel_while_pwsa_new(initF, size, fork, do_work);
  return std::make_pair(gpred, is_expanded);
}

template <class GRAPH, class HEAP, class HEURISTIC>
std::atomic<int>* pwsa(GRAPH& graph, const HEURISTIC& heuristic,
                       const int& source, const int& destination,
                       int split_cutoff, int poll_cutoff, double exptime,
                       int* pebbles = nullptr, int* predecessors = nullptr) {
  int N = graph.number_vertices();
  std::atomic<int>* finalized = pasl::data::mynew_array<std::atomic<int>>(N);
  fill_array_par(finalized, N, -1);

  HEAP initF = HEAP();
  int heur = heuristic(source);
  initF.insert(heur, std::make_tuple(source, 0/*, source*/));

  pasl::data::perworker::array<int> expanded_since_exchange;
  expanded_since_exchange.init(0);

  auto size = [&] (HEAP& frontier) {
    auto sz = frontier.size();
    if (sz == 0) {
      expanded_since_exchange.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (expanded_since_exchange.mine() > split_cutoff && sz > 1)) {
      return 2; // split
    }
    else {
      return 1; // don't split
    }
  };

  auto fork = [&] (HEAP& src, HEAP& dst) {
    src.split(dst);
    expanded_since_exchange.mine() = 0;
  };

  auto do_work = [&] (std::atomic<bool>& is_done, HEAP& frontier) {
  //auto do_work = [&] (bool& is_done, HEAP& frontier) {
    int expanded_this_round = 0;
    while (expanded_this_round < poll_cutoff && frontier.size() > 0 && !is_done.load()) {
      auto tup = frontier.delete_min();
      int v = std::get<0>(tup);
      int vdist = std::get<1>(tup);
      //int pred = std::get<2>(tup);
      expanded_this_round++;

      int orig = finalized[v].load();
      if (orig == -1 && finalized[v].compare_exchange_strong(orig, vdist)) {
        //if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
        //if (predecessors) predecessors[v] = pred;
        if (v == destination) {
          is_done.store(true);
          //is_done = true;
          return;
        }

        graph.simulate_get_successors(exptime);

        graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
          int nghdist = vdist + weight;
          frontier.insert(heuristic(ngh) + nghdist, std::make_tuple(ngh, nghdist/*, v*/));
        });
      }
    }
    expanded_since_exchange.mine() += expanded_this_round;
  };

  //pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
  //pasl::sched::native::parallel_while_pwsa_new(initF, size, fork, do_work);
  pasl::sched::native::parallel_while_pwsa_maybe_faster(initF, size, fork, do_work);
  return finalized;
}
