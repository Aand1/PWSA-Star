/* COPYRIGHT (c) 2016 Sam Westrick, Laxman Dhulipala.
 * All rights reserved.
 *
 * \file astar.hpp
 */
#include "result.hpp"
#include <climits>

#ifndef _ASTAR_SEARCH_H_
#define _ASTAR_SEARCH_H_

struct astar_state {
  bool is_expanded;
  int dist;
  int pred;
};

class AStarResult : public SearchResult {
private:
  int n;
  astar_state* states;
  int* pebbles;

public:
  AStarResult(int n, astar_state* states, int* pebbles)
  : n(n), states(states), pebbles(pebbles) { }

  ~AStarResult() override { free(states); if (pebbles) free(pebbles); }

  bool is_expanded(int vertex) override { return states[vertex].is_expanded; }
  int predecessor(int vertex) override { return states[vertex].pred; }
  int g(int vertex) override { return states[vertex].dist; }
  int pebble(int vertex) override { return (pebbles ? pebbles[vertex] : -1); }
};

template <class GRAPH, class HEAP, class HEURISTIC>
SearchResult*
astar(GRAPH& graph, const HEURISTIC& heuristic,
      const int& source, const int& destination,
      double exptime, bool pebble) {
  int N = graph.number_vertices();
  astar_state* states = pasl::data::mynew_array<astar_state>(N);
  for (int i = 0; i < N; i++) {
    states[i].is_expanded = false;
    states[i].dist = INT_MAX;
    states[i].pred = -1;
  }
  int* pebbles = nullptr;
  if (pebble) {
    pebbles = pasl::data::mynew_array<int>(N);
    for (int i = 0; i < N; i++) pebbles[i] = -1;
  }
  states[source].dist = 0;
  HEAP frontier = HEAP();
  frontier.insert(heuristic(source), source);
  while (true) {
    int v = frontier.delete_min();
    if (v == destination) break;
    if (!states[v].is_expanded) {
      states[v].is_expanded = true;
      if (pebbles) pebbles[v] = 0;
      graph.simulate_get_successors(exptime);
      graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {
        int nbrdist = states[v].dist + weight;
        if (nbrdist < states[nbr].dist) {
          frontier.insert(heuristic(nbr) + nbrdist, nbr);
          states[nbr].dist = nbrdist;
          states[nbr].pred = v;
        }
      });
    }
  }
  SearchResult* result = new AStarResult(N, states, pebbles);
  return result;
}

#endif // _ASTAR_SEARCH_H_
