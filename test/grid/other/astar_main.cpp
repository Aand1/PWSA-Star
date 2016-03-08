#include "benchmark.hpp"
#include "weighted_graph.hpp"
#include "bin_heap.hpp"
#include "array_util.hpp"
#include <sys/time.h>

uint64_t GetTimeStamp() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

template <class GRAPH, class HEAP, class HEURISTIC>
int* astar(GRAPH& graph, const HEURISTIC& heuristic,
           const int& source, const int& destination,
           double exptime, bool debug = false) {
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
        uint64_t t0 = GetTimeStamp();
        uint64_t t1;
        uint64_t dt;
        do{
          t1 = GetTimeStamp();
          dt = t1-t0;
        } while(dt < (exptime*1000000.0));
      });
    }
  }
  return dist;
}

int main(int argc, char** argv) {
  int split_cutoff; // (K)
  int poll_cutoff; // (D)
  std::string fname;
  int srow;
  int scol;
  int drow;
  int dcol;
  double w;
  double exptime;
  bool debug;
  double opt;

  Graph G;
  int src;
  int dst;

  int* res;

  auto init = [&] {
    fname = pasl::util::cmdline::parse_or_default_string("map", "maps/simple_map.map");
    srow = (int)pasl::util::cmdline::parse_or_default_int("sr", 1);
    scol = (int)pasl::util::cmdline::parse_or_default_int("sc", 1);
    drow = (int)pasl::util::cmdline::parse_or_default_int("dr", 1);
    dcol = (int)pasl::util::cmdline::parse_or_default_int("dc", 1);
    w = (double)pasl::util::cmdline::parse_or_default_float("w", 1.0);
    exptime = (double)pasl::util::cmdline::parse_or_default_float("exptime", 0.0000000001);
    opt = (double)pasl::util::cmdline::parse_or_default_float("opt", 1.0);
    debug = pasl::util::cmdline::parse_or_default_bool("debug", false);

    G = Graph(fname.c_str());

    src = G.vertex_at(srow, scol);
    dst = G.vertex_at(drow, dcol);
  };

  auto run = [&] (bool sequential) {
    auto heuristic = [&] (int v) {
      return G.weighted_euclidean(w, v, dst);
    };
    res = astar<Graph, Heap<std::pair<int,int>>, decltype(heuristic)>(G, heuristic, src, dst, exptime, debug);
  };

  auto output = [&] {
    int num_expanded = 0;
    for (int i = 0; i < G.number_vertices(); i++) {
      if (res[i] != -1) num_expanded++;
    }
    double pathlen = double(res[dst])/10000.0;
    std::cout << "expanded " << num_expanded << std::endl;
    std::cout << "pathlen " << pathlen << std::endl;
    std::cout << "deviation " << (pathlen / opt) << std::endl;
  };

  auto destroy = [&] {
    ;
  };

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
