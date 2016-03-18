#include "container.hpp"
#include "native.hpp"
#include "timing.hpp"
#include "weighted_graph.hpp"
#include "search_utils.hpp"
#include <climits>
#include <pthread.h>
#include <map>

#ifndef _PASE_SEARCH_H_
#define _PASE_SEARCH_H_

struct pase_state {
  bool is_expanded;
  bool being_expanded;
  int g;
  int pred;
};

class PASEResult : public SearchResult {
private:
  int n;
  pase_state* states;
  int* pebbles;

public:
  PASEResult(int n, pase_state* states, int* pebbles)
  : n(n), states(states), pebbles(pebbles) { }

  ~PASEResult() override { free(states); if (pebbles) free(pebbles); }

  bool is_expanded(int vertex) override { return states[vertex].is_expanded; }
  int predecessor(int vertex) override { return states[vertex].pred; }
  int g(int vertex) override { return states[vertex].g; }
  int pebble(int vertex) override { return (pebbles ? pebbles[vertex] : -1); }
};

// ===========================================================================
// ===========================================================================
// ===========================================================================

template <class GRAPH, class HEURISTIC>
SearchResult*
pase(GRAPH& graph, const HEURISTIC& heuristic,
     const int& source, const int& destination,
     double exptime, bool pebble) {
  // ============= some PASL helpers =============
  auto yield = [] { pasl::sched::native::yield(); };
  auto hiccup = [] { pasl::util::ticks::microseconds_sleep(10.0); };
  auto my_id = [] { return pasl::sched::threaddag::get_my_id(); };

  // ============= initialization and queue manipulations =============
  int N = graph.number_vertices();
  pase_state* states = pasl::data::mynew_array<pase_state>(N);
  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    states[i].is_expanded = false;
    states[i].being_expanded = false;
    states[i].g = INT_MAX; // "infinite"
    states[i].pred = -1;
  });

  int* pebbles = nullptr;
  if (pebble) {
    pebbles = pasl::data::mynew_array<int>(N);
    pasl::sched::native::parallel_for(0, N, [&] (int i) { pebbles[i] = -1; });
  }

  multimap<int,int> M;
  Locked locked;
  std::atomic<bool> is_done(false);

  auto insert_locked = [&] (int fv, int v) {
    atomic_log([&] { std::cout << my_id() << " inserting " << v << std::endl; });
    M.insert(std::pair<int,int>(fv, v));
  };

  auto remove_locked = [&] (multimap<int,int>::iterator it) {
    atomic_log([&] { std::cout << my_id() << " removing " << it->second << std::endl; });
    return M.erase(it);
  };

  auto insert = [&] (int fv, int v) {
    locked.action([&] {
      atomic_log([&] { std::cout << my_id() << " inserting " << v << std::endl; });
      M.insert(std::pair<int,int>(fv, v));
    });
    yield();
  };

  auto remove = [&] (multimap<int,int>::iterator it) {
    locked.action([&] {
      atomic_log([&] { std::cout << my_id() << " removing " << it->second << std::endl; });
      M.erase(it);
    });
    yield();
  };

  auto find_independent = [&] () -> multimap<int,int>::iterator {
    multimap<int,int>::iterator result = M.end();

    locked.action([&] {
      multimap<int,int>::iterator itv = M.begin();
      while (!is_done.load() && itv != M.end()) {
        int v = itv->second;
        if (states[v].is_expanded) { itv = remove_locked(itv); continue; }
        bool valid = ! states[v].being_expanded;

        multimap<int,int>::iterator itu;
        for (itu = M.begin(); valid && itu != itv; itu++) {
          int u = itu->second;
          valid = valid && (states[u].g + heuristic(u,v) >= states[v].g);
        }

        if (valid) {
          states[v].being_expanded = true;
          result = itv;
          break;
        }

        itv++;
      }
    });

    yield();
    return result;
  };

  states[source].g = 0;
  insert_locked(heuristic(source, destination), source);

  // ============= MAIN BODY =============

  auto body = [&] {

    while (!is_done.load()) {

      multimap<int,int>::iterator itv = find_independent();
      if (itv == M.end()) { hiccup(); continue; }

      int v = itv->second;
      atomic_log([&] { std::cout << my_id() << " expanding " << v << std::endl; });
      if (pebbles) pebbles[v] = my_id();
      if (v == destination) {
        is_done.store(true);
        return;
      }

      graph.simulate_get_successors(exptime);

      int vdist = states[v].g;
      locked.action([&] {
        graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {

          int nbrdist = vdist + weight;
          int olddist = states[nbr].g;
          if (nbrdist < olddist) {
            states[nbr].g = nbrdist;
            states[nbr].pred = v;
            insert_locked(nbrdist + heuristic(nbr, destination), nbr);
          }

        });

        atomic_log([&] { std::cout << my_id() << " removing expanded vertex " << v << std::endl; });
        states[v].is_expanded = true;
        M.erase(itv);
      });
      yield();

    }
  };

  pasl::sched::native::parallel_while(body);
  SearchResult* result = new PASEResult(N, states, pebbles);
  return result;
}

#endif // _PASE_SEARCH_H_
