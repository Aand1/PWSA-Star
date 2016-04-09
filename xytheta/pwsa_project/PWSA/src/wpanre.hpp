#include "container.hpp"
#include "native.hpp"
#include "timing.hpp"
#include "search_utils.hpp"
#include <climits>
#include <pthread.h>

#ifndef _WPANRE_SEARCH_H_
#define _WPANRE_SEARCH_H_

struct wpanre_state {
  std::atomic<bool> is_expanded;
  int pred;
  int g;
  int h;
};

class WPANREResult : public SearchResult {
private:
  int n;
  wpanre_state* states;
  int* pebbles;

public:
  WPANREResult(int n, wpanre_state* states, int* pebbles)
  : n(n), states(states), pebbles(pebbles) { }

  ~WPANREResult() override { free(states); if (pebbles) free(pebbles); }

  bool is_expanded(int vertex) override { return states[vertex].is_expanded.load(); }
  int predecessor(int vertex) override { return states[vertex].pred; }
  int g(int vertex) override { return states[vertex].g; }
  int pebble(int vertex) override { return (pebbles ? pebbles[vertex] : -1); }
};

template <class GRAPH, class HEAP, class HEURISTIC>
SearchResult*
wpanre(GRAPH& graph, const HEURISTIC& heuristic,
       const int& source, const int& destination,
       double exptime, bool pebble) {
  // ============= some PASL helpers =============
  auto yield = [] { pasl::sched::native::yield(); };
  auto hiccup = [] { pasl::util::ticks::microseconds_sleep(10.0); };
  auto my_id = [] { return pasl::sched::threaddag::get_my_id(); };

  // ============= initialize ====================
  int N = graph.number_vertices();
  wpanre_state* states = pasl::data::mynew_array<wpanre_state>(N);
  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    states[i].is_expanded.store(false);
    states[i].pred = -1;
    states[i].g = INT_MAX; // "infinite"
    states[i].h = -1;
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

  states[source].g = 0;

  Locked locked;
  HEAP frontier = HEAP();
  int heur = my_heur(source);
  frontier.insert(heur, source);

  std::atomic<bool> is_done(false);

  auto body = [&] {

    while (!is_done.load()) {

      int v;
      locked.action([&] {
        if (frontier.size() == 0) v = -1;
        else v = frontier.delete_min();
      });
      yield();
      if (v == -1) { hiccup(); continue; }

      bool orig = states[v].is_expanded.load();
      if (!orig && states[v].is_expanded.compare_exchange_strong(orig, true)) {
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
              frontier.insert(nbrdist + my_heur(nbr), nbr);
              states[nbr].g = nbrdist;
              states[nbr].pred = v;
            }
          });
        });
        yield();
      }

    }
  };

  pasl::sched::native::parallel_while(body);
  SearchResult* result = new WPANREResult(N, states, pebbles);
  return result;
}

#endif // _WPANRE_SEARCH_H_
