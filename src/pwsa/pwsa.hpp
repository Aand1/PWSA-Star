/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.hpp
 */
#include "container.hpp"
#include "native.hpp"
#include "parallel_while.hpp"
#include "bin_heap.hpp"
#include <cstring>
#include <climits>

#ifndef _PWSA_H_
#define _PWSA_H_

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

struct vertpack {
  int distance;
  int pred;
};

// ===========================================================================
// ===========================================================================
// ===========================================================================

struct state {
  std::atomic<bool> is_expanded;
  std::atomic<vertpack> gpred;
  int h;
};

const bool debug_print = true;
template <class Body>
void msg(const Body& b) {
  if (debug_print)
    pasl::util::atomic::msg(b);
}

template <class GRAPH, class HEAP, class HEURISTIC>
state*
pwsa_pathcorrect_locality(GRAPH& graph, const HEURISTIC& heuristic,
                          const int& source, const int& destination,
                          int split_cutoff, int poll_cutoff, double exptime,
                          int& important, int* pebbles = nullptr) {
  int N = graph.number_vertices();
  state* states = pasl::data::mynew_array<state>(N);
  // std::atomic<bool>* is_expanded = pasl::data::mynew_array<std::atomic<bool>>(N);
  // std::atomic<vertpack>* gpred = pasl::data::mynew_array<std::atomic<vertpack>>(N);
  // fill_array_par(is_expanded, N, false);
  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    states[i].is_expanded.store(false);

    vertpack x;
    x.distance = INT_MAX; // "infinite"
    x.pred = -1;
    states[i].gpred.store(x);

    states[i].h = -1;
  });

  // memoize heuristic
  auto my_heur = [&] (int v) {
    if (states[v].h == -1)
      states[v].h = heuristic(v);
    return states[v].h;
  };

  vertpack src;
  src.distance = 0;
  src.pred = source;
  states[source].gpred.store(src);

  HEAP initF = HEAP();
  int heur = my_heur(source);
  initF.insert(heur, source);

  pasl::data::perworker::array<int> steps_since_load_balancing;
  steps_since_load_balancing.init(0);

  auto size = [&] (HEAP& frontier) {
    auto sz = frontier.size();
    if (sz == 0) {
      steps_since_load_balancing.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (steps_since_load_balancing.mine() > split_cutoff && sz > 1)) {
      return 2; // split
    }
    else {
      return 1; // don't split
    }
  };

  auto fork = [&] (HEAP& src, HEAP& dst) {
    src.split(dst);
    steps_since_load_balancing.mine() = 0;
  };

  auto do_work = [&] (/*std::atomic<bool>& is_done, */HEAP& frontier) {
  //auto do_work = [&] (bool& is_done, HEAP& frontier) {
    int steps_this_round = 0;
    while (steps_this_round < poll_cutoff && frontier.size() > 0/* && !is_done.load()*/) {
      int v = frontier.delete_min();
      steps_this_round++;

      bool orig = states[v].is_expanded.load();
      if (!orig && states[v].is_expanded.compare_exchange_strong(orig, true)) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();

        if (v == destination) {
          //is_done.store(true);

          msg([&] { std::cout << "found dst" << std::endl; });
          return true;
        }

        // SIMULATE EXPANSION
        graph.simulate_get_successors(exptime);

        int vdistance = states[v].gpred.load().distance;
        graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {

          vertpack mine;
          mine.distance = vdistance + weight;
          mine.pred = v;

          while (true) {
            vertpack gpred_nbr = states[nbr].gpred.load();
            if (mine.distance < gpred_nbr.distance) {
              if (states[nbr].gpred.compare_exchange_weak(gpred_nbr, mine)) {
                if (!states[nbr].is_expanded.load()) {
                  frontier.insert(mine.distance + my_heur(nbr), nbr);
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
    steps_since_load_balancing.mine() += steps_this_round;
    return false;
  };

//  pasl::sched::native::parallel_while_pwsa_maybe_faster(initF, size, fork, do_work);
  pasl::sched::native::parallel_while_pwsa(initF, size, fork, do_work);
  return states;
}

// ===========================================================================
// ===========================================================================
// ===========================================================================

struct state_simple {
  std::atomic<bool> is_expanded;
  int g;
  int h;
};

template <class GRAPH, class HEAP, class HEURISTIC>
state_simple*
pwsa_locality(GRAPH& graph, const HEURISTIC& heuristic,
              const int& source, const int& destination,
              int split_cutoff, int poll_cutoff, double exptime,
              int& important, int* pebbles = nullptr) {
  int N = graph.number_vertices();
  state_simple* states = pasl::data::mynew_array<state_simple>(N);
  // std::atomic<bool>* is_expanded = pasl::data::mynew_array<std::atomic<bool>>(N);
  // std::atomic<vertpack>* gpred = pasl::data::mynew_array<std::atomic<vertpack>>(N);
  // fill_array_par(is_expanded, N, false);
  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    states[i].is_expanded.store(false);
    states[i].g = INT_MAX; // "infinite"
    states[i].h = -1;
  });

  // memoize heuristic
  auto my_heur = [&] (int v) {
    if (states[v].h == -1)
      states[v].h = heuristic(v);
    return states[v].h;
  };

  states[source].g = 0;

  HEAP initF = HEAP();
  int heur = my_heur(source);
  initF.insert(heur, source);

  pasl::data::perworker::array<int> steps_since_load_balancing;
  steps_since_load_balancing.init(0);

  auto size = [&] (HEAP& frontier) {
    auto sz = frontier.size();
    if (sz == 0) {
      steps_since_load_balancing.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (steps_since_load_balancing.mine() > split_cutoff && sz > 1)) {
      return 2; // split
    }
    else {
      return 1; // don't split
    }
  };

  auto fork = [&] (HEAP& src, HEAP& dst) {
    src.split(dst);
    steps_since_load_balancing.mine() = 0;
  };

  auto do_work = [&] (/*std::atomic<bool>& is_done, */HEAP& frontier) {
  //auto do_work = [&] (bool& is_done, HEAP& frontier) {
    int steps_this_round = 0;
    while (steps_this_round < poll_cutoff && frontier.size() > 0 /*&& !is_done.load()*/) {
      int v = frontier.delete_min();
      steps_this_round++;

      bool orig = states[v].is_expanded.load();
      if (!orig && states[v].is_expanded.compare_exchange_strong(orig, true)) {
        //if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();

        if (v == destination) {
          return true;
        }

        // SIMULATE EXPANSION
        graph.simulate_get_successors(exptime);

        int vdist = states[v].g;
        graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {

          int nbrdist = vdist + weight;
          if (nbrdist < states[nbr].g) {
            frontier.insert(nbrdist + my_heur(nbr), nbr);
            states[nbr].g = nbrdist;
          }
        });
      }
    }
    steps_since_load_balancing.mine() += steps_this_round;
    return false;
  };

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, do_work);
  return states;
}

#endif // _PWSA_H_
