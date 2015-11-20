#include "array_util.hpp"
#include "timing.hpp"
#include <climits>

template <class GRAPH, class HEAP, class HEURISTIC>
std::pair<int*,bool*> astar(GRAPH& graph, const HEURISTIC& heuristic,
                            const int& source, const int& destination,
                            double exptime) {
  int N = graph.number_vertices();
  int* dist = array_util::my_malloc<int>(N);
  bool* exp = array_util::my_malloc<bool>(N);
  for (int i = 0; i < N; i++) {
    dist[i] = INT_MAX;
    exp[i] = false;
  }
  dist[source] = 0;
  HEAP frontier = HEAP();
  frontier.insert(heuristic(source), source);
  while (dist[destination] == INT_MAX) {
    int v = frontier.delete_min();
    if (!exp[v]) {
      graph.simulate_get_successors(exptime);
      exp[v] = true;

      graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
        int nghdist = dist[v] + weight;
        if (nghdist < dist[ngh]) {
          frontier.insert(heuristic(ngh) + nghdist, ngh);
          dist[ngh] = nghdist;
        }

      });
    }
  }
  return std::make_pair(dist,exp);
}
