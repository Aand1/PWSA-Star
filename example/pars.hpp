//include "benchmark.hpp"
#include "array-util.hpp"
#include "weighted-graph.hpp"
//include "pairing-heap.hpp"
//include "treap-heap.hpp"

#define INFINITE (-1)

loop_controller_type pars_init_controller("pars_init");
loop_controller_type pars_independence_controller("pars_independence");
loop_controller_type pars_set_dist_controller("pars_set_dist");
loop_controller_type pars_copy_controller("pars_copy");
loop_controller_type pars_successors_controller("pars_successors");

// CAN WE GUARANTEE THAT EACH VERT APPEARS IN `FRONTIER` ONCE?
// RECORD SHORTEST POSSIBLE PATH TO EACH VERTEX SEPARATELY... WRITE WITH MIN

template <class PAIR_HEURISTIC, class GRAPH>
long* pars(GRAPH& graph, const PAIR_HEURISTIC& pair_heur,
           const intT& source, const intT& destination) {
  std::cout << "PA*RS from " << source << " to " << destination << std::endl;
  intT N = graph.number_vertices();
  std::cout << "Number vertices: " << N << std::endl;

  long* distance = array_util::my_malloc<long>(N);
  par::parallel_for(pars_init_controller, 0, N, [&] (intT i) {
    distance[i] = INFINITE;
  });

  VertexPackage* frontier = array_util::my_malloc<VertexPackage>(1);
  frontier[0] = graph.make_vertex_package(source, false, 0);
  long frontier_size = 1;

  std::atomic<bool> done;
  done.store(false);

  long round = 0;

  while (true) {
    std::cout << std::endl << std::endl << "ROUND " << (round++) << std::endl;
    std::cout << "Frontier size: " << frontier_size << std::endl;

    long* independent = array_util::my_malloc<long>(frontier_size);
    par::parallel_for(pars_independence_controller,
                      [&] (long lo, long hi) { return frontier_size * (hi - lo); },
                      0l, frontier_size, [&] (long i) {
      auto indep_check = [&] (VertexPackage& other) {
        return frontier[i].distance <= other.distance + pair_heur(frontier[i].vertexId, other.vertexId);
      };
      bool left = array_util::all(indep_check, frontier, 0, i);
      bool right = array_util::all(indep_check, frontier, i+1, frontier_size);
      independent[i] = left && right;
    });

    //std::cout << "independent?: "; array_util::display(independent, frontier_size); std::cout << std::endl;

    auto independents = array_util::filter([&] (long i, VertexPackage& ignore) { return independent[i]; },
                                                frontier, frontier_size);
    auto remaining = array_util::filter([&] (long i, VertexPackage& ignore) { return !independent[i]; },
                                        frontier, frontier_size);

    std::cout << "number independent: " << independents.second << std::endl;
    //std::cout << "independents: "; array_util::display(independents.first, independents.second); std::cout << std::endl;
    //std::cout << "remaining: "; array_util::display(remaining.first, remaining.second); std::cout << std::endl;

    parallel_for(pars_set_dist_controller, 0l, independents.second, [&] (long i) {
      VertexPackage pack = independents.first[i];
      distance[pack.vertexId] = pack.distance;

      if (pack.vertexId == destination) {
        bool orig = false;
        done.compare_exchange_strong(orig, true);
      }
    });

    if (done.load()) break;

    // generate all successors
    long* offsets = array_util::plus_scan([&] (long ignore, VertexPackage& pack) { return graph.outDegree(pack.vertexId); },
                                          independents.first, independents.second);
    long successors_len = offsets[independents.second];

    //std::cout << "offsets: "; array_util::display(offsets, independents.second + 1); std::cout << std::endl;

    free(frontier);
    frontier = array_util::my_malloc<VertexPackage>(remaining.second + successors_len);
    frontier_size = remaining.second + successors_len;
    parallel_for(pars_copy_controller, 0l, remaining.second, [&] (long i) {
      //std::cout << "copying in remaining[" << i << "]" << std::endl;
      frontier[i] = remaining.first[i];
    });
    free(remaining.first);

    auto weight = [&] (long lo, long hi) {
      return offsets[hi] - offsets[lo];
    };
    parallel_for(pars_successors_controller, weight, 0l, independents.second, [&] (long i) {
      VertexPackage mine = independents.first[i];
      long j = 0;
      graph.apply_to_each_in_range(mine, [&] (intT ngh, intT weight) {
        //std::cout << "copying in " << j << "th neighbor of " << mine.vertexId << " at frontier[" << (remaining.second + offsets[i] + j) << "]" << std::endl;
        frontier[remaining.second + offsets[i] + j] = graph.make_vertex_package(ngh, false, mine.distance + weight);
        j++;
      });
    });
    free(independents.first);

    // filter out already visited
    auto newfrontier = array_util::filter([&] (long i, VertexPackage& pack) { return distance[pack.vertexId] == INFINITE; }, frontier, frontier_size);
    free(frontier);
    frontier = newfrontier.first;
    frontier_size = newfrontier.second;
  }

  return distance;
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
