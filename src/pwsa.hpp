/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.hpp
 */
#include "container.hpp" // pasl/sequtil/container.hpp, for pasl::data::mynew_array
#include "native.hpp"
#include "parallel_while.hpp"
#include "bin_heap.hpp"
#include "search_utils.hpp"
#include <climits>

#ifndef _PWSA_SEARCH_H_
#define _PWSA_SEARCH_H_

struct vertpack {
  int distance;
  int pred;
};

struct pwsa_state {
  std::atomic<bool> is_expanded;
  std::atomic<vertpack> gpred;
  int h;
};

class PWSAResult : public SearchResult {
private:
  int n;
  pwsa_state* states;
  int* pebbles;

public:
  PWSAResult(int n, pwsa_state* states, int* pebbles)
  : n(n), states(states), pebbles(pebbles) { }

  ~PWSAResult() override { free(states); if (pebbles) free(pebbles); }

  bool is_expanded(int vertex) override { return states[vertex].is_expanded.load(); }
  int predecessor(int vertex) override { return states[vertex].gpred.load().pred; }
  int g(int vertex) override { return states[vertex].gpred.load().distance; }
  int pebble(int vertex) override { return (pebbles ? pebbles[vertex] : -1); }
};

template <class GRAPH, class HEAP, class HEURISTIC>
SearchResult*
pwsa_pc(GRAPH& graph, const HEURISTIC& heuristic,
        const int& source, const int& destination,
        int split_cutoff, int poll_cutoff, double exptime,
        bool pebble) {
  int N = graph.number_vertices();
  pwsa_state* states = pasl::data::mynew_array<pwsa_state>(N);

  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    states[i].is_expanded.store(false);
    states[i].h = -1;

    vertpack x;
    x.distance = INT_MAX; 
    x.pred = -1;
    states[i].gpred.store(x);
  });

  int* pebbles = nullptr;
  if (pebble) {
    pebbles = pasl::data::mynew_array<int>(N);
    pasl::sched::native::parallel_for(0, N, [&] (int i) { pebbles[i] = -1; });
  }

  // memoize heuristic
  auto my_heur = [&] (int v) {
    if (states[v].h == -1) states[v].h = heuristic(v);
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

  auto do_work = [&] (HEAP& frontier) {
    int steps_this_round = 0;
    while (steps_this_round < poll_cutoff && frontier.size() > 0) {
      int v = frontier.delete_min();
      steps_this_round++;

      bool orig = states[v].is_expanded.load();
      if (!orig && states[v].is_expanded.compare_exchange_strong(orig, true)) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
        if (v == destination) return true;

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

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, do_work);
  SearchResult* result = new PWSAResult(N, states, pebbles);
  return result;
}

// ===========================================================================
// ===========================================================================
// ===========================================================================

// now exactly the same as pwsa_pc
template <class GRAPH, class HEAP, class HEURISTIC>
SearchResult*
pwsa(GRAPH& graph, const HEURISTIC& heuristic,
     const int& source, const int& destination,
     int split_cutoff, int poll_cutoff, double exptime,
     bool pebble) {
  int N = graph.number_vertices();
  pwsa_state* states = pasl::data::mynew_array<pwsa_state>(N);
  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    states[i].is_expanded.store(false);
    states[i].h = -1;

    vertpack x;
    x.distance = INT_MAX;
    x.pred = -1;
    states[i].gpred.store(x);
  });

  int* pebbles = nullptr;
  if (pebble) {
    pebbles = pasl::data::mynew_array<int>(N);
    pasl::sched::native::parallel_for(0, N, [&] (int i) { pebbles[i] = -1; });
  }

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

  auto do_work = [&] (HEAP& frontier) {
    int steps_this_round = 0;
    while (steps_this_round < poll_cutoff && frontier.size() > 0) {
      int v = frontier.delete_min();
      steps_this_round++;

      bool orig = states[v].is_expanded.load();
      if (!orig && states[v].is_expanded.compare_exchange_strong(orig, true)) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
        if (v == destination) return true;

        // SIMULATE EXPANSION
        graph.simulate_get_successors(exptime);

        int vdistance = states[v].gpred.load().distance;
        graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {

          vertpack mine;
          mine.distance = vdistance + weight;
          mine.pred = v;

          vertpack nbr_prev = states[nbr].gpred.load();
          int prev_distance = nbr_prev.distance;
          while (nbr_prev.distance  == prev_distance) {
            nbr_prev = states[nbr].gpred.load();
            if (states[nbr].gpred.compare_exchange_weak(gpred_nbr, mine)) {
              if (!states[nbr].is_expanded.load()) {
                frontier.insert(mine.distance + my_heur(nbr), nbr);
              }
              break;
            } else {
              break;
            }
          }
        });
      }
    }
    steps_since_load_balancing.mine() += steps_this_round;
    return false;
  };

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, do_work);
  SearchResult* result = new PWSAResult(N, states, pebbles);
  return result;
}

#endif // _PWSA_SEARCH_H_
