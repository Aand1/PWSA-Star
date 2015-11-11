#include "array_util.hpp"
#include "timing.hpp"

template <class GRAPH, class HEAP, class HEURISTIC>
int* astar(GRAPH& graph, const HEURISTIC& heuristic,
           const int& source, const int& destination,
           double exptime) {
  int N = graph.number_vertices();
  int* dist = array_util::my_malloc<int>(N);
  for (int i = 0; i < N; i++) {
    dist[i] = -1;
  }
  HEAP frontier = HEAP();
  frontier.insert(heuristic(source), std::make_pair(source, 0));
  while (dist[destination] == -1) {
    auto pair = frontier.delete_min();
    int v = pair.first;
    int vdist = pair.second;
    if (dist[v] == -1) {
      dist[v] = vdist;

      graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
        int nghdist = vdist + weight;
        if (dist[ngh] == -1) {
          frontier.insert(heuristic(ngh) + nghdist, std::make_pair(ngh, nghdist));
        }

        // SIMULATE EXPANSION TIME
        timing::busy_loop_secs(exptime);
        // uint64_t t0 = GetTimeStamp();
        // uint64_t t1;
        // uint64_t dt;
        // do{
        //   t1 = GetTimeStamp();
        //   dt = t1-t0;
        // } while(dt < (exptime*1000000.0));
      });
    }
  }
  return dist;
}
