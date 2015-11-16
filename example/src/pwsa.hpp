/* COPYRIGHT (c) 2015 Sam Westrick, Laxman Dhulipala, Umut Acar,
 * Arthur Chargueraud, and Michael Rainey.
 * All rights reserved.
 *
 * \file pwsa.hpp
 */
//include "benchmark.hpp"
#include "container.hpp"
//include "weighted-graph.hpp"
#include "native.hpp"
#include "timing.hpp"
#include "bin_heap.hpp"
//include "defaults.hpp"
#include <cstring>
//include <sys/time.h>

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

template <class Number, class Size>
void fill_pair_array_par(std::atomic<std::pair<Number,Number>>* array, Size sz, Number val) {
  pmemset((char*)array, val, 2*sz*sizeof(Number)); // this could be super broken
}

template <class Body>
void print(bool debug, const Body& b) {
  if (debug) {
    pasl::util::atomic::msg(b);
  }
}

// template <class GRAPH, class HEAP, class HEURISTIC>
// std::atomic<int>* pwsa(GRAPH& graph, const HEURISTIC& heuristic,
//                         const int& source, const int& destination,
//                         int split_cutoff, int poll_cutoff, double exptime,
//                         int* pebbles = nullptr, int* predecessors = nullptr) {
//   int N = graph.number_vertices();
//   std::atomic<int>* finalized = pasl::data::mynew_array<std::atomic<int>>(N);
//   fill_array_par(finalized, N, -1);
//
//   HEAP initF = HEAP();
//   int heur = heuristic(source);
//   initF.insert(heur, std::make_tuple(source, 0, 0));
//
//   pasl::data::perworker::array<int> work_since_split;
//   work_since_split.init(0);
//
//   auto size = [&] (HEAP& frontier) {
//     auto sz = frontier.size();
//     if (sz == 0) {
//       work_since_split.mine() = 0;
//       return 0; // no work left
//     }
//     if (sz > split_cutoff || (work_since_split.mine() > split_cutoff && sz > 1)) {
//       return 2; // split
//     }
//     else {
//       return 1; // don't split
//     }
//   };
//
//   auto fork = [&] (HEAP& src, HEAP& dst) {
//     src.split(dst);
//     work_since_split.mine() = 0;
//   };
//
//   auto set_in_env = [&] (HEAP& f) {;};
//
//   auto do_work = [&] (std::atomic<bool>& is_done, HEAP& frontier) {
//     int work_this_round = 0;
//     while (work_this_round < poll_cutoff && frontier.size() > 0 && !is_done.load()) {
//       auto tup = frontier.delete_min();
//       int v = std::get<0>(tup);
//       int vdist = std::get<1>(tup);
//       int pred = std::get<2>(tup);
//       int orig = -1;
//       if (finalized[v].load() == -1 && finalized[v].compare_exchange_strong(orig, vdist)) {
//         if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
//         if (predecessors) predecessors[v] = pred;
//         if (v == destination) {
//           is_done.store(true);
//           return;
//         }
//         graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
//           int nghdist = vdist + weight;
//           frontier.insert(heuristic(ngh) + nghdist, std::make_tuple(ngh, nghdist, v));
//
//           // SIMULATE EXPANSION TIME
//           timing::busy_loop_secs(exptime);
//         });
//       }
//       work_this_round++;
//     }
//     work_since_split.mine() += work_this_round;
//   };
//
//   pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
//   return finalized;
// }

// // d1 could be -1 (infinite), while d2 must be >= 0
// inline bool dist_greater(int d1, int d2) {
//   return d1 == -1 || d1 > d2;
// }
//
// struct vertpack {
//   int distance;
//   int pred;
// };

const bool debug_print = true;
template <class Body>
void msg(const Body& b) {
  if (debug_print)
    pasl::util::atomic::msg(b);
}

struct unfinished_pack {
  int vertex;
  int dist;
  int pred;

  int edge_start;
};

class Frontier {

  using HEAP = Heap<std::tuple<int,int,int>>;

public:
  HEAP heap;
  unfinished_pack current;

  Frontier() {
    heap = HEAP();
    current.vertex = -1;
  }

  void insert(int priority, const std::tuple<int,int,int>& value) {
    heap.insert(priority, value);
  }

  long size() {
    return (current.vertex == -1 ? 0 : 1) + heap.size();
  }

  void split(Frontier& other) {
    heap.split(other.heap);
  }

  void swap(Frontier& other) {
    heap.swap(other.heap);
    std::swap(current, other.current);
  }

  std::tuple<int,int,int> delete_min() {
    return heap.delete_min();
  }
};

template <class GRAPH, class HEURISTIC>
std::atomic<int>* pwsa(GRAPH& graph, const HEURISTIC& heuristic,
                       const int& source, const int& destination,
                       int split_cutoff, int poll_cutoff, double exptime,
                       int* pebbles = nullptr, int* predecessors = nullptr) {
  int N = graph.number_vertices();
  std::atomic<int>* finalized = pasl::data::mynew_array<std::atomic<int>>(N);
  fill_array_par(finalized, N, -1);

  Frontier initF = Frontier();
  initF.insert(heuristic(source), std::make_tuple(source, 0, source));

  pasl::data::perworker::array<int> work_since_split;
  work_since_split.init(0);

  auto size = [&] (Frontier& frontier) {
    auto sz = frontier.size();
    if (sz == 0) {
      work_since_split.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (work_since_split.mine() > split_cutoff && sz > 1)) {
      return 2; // split
    }
    else {
      return 1; // don't split
    }
  };

  auto fork = [&] (Frontier& src, Frontier& dst) {
    src.split(dst);
    work_since_split.mine() = 0;
  };

  auto set_in_env = [&] (Frontier& f) {;};

  auto do_work = [&] (std::atomic<bool>& is_done, Frontier& frontier) {

    int work_this_round = 0;
    while (work_this_round < poll_cutoff && frontier.size() > 0) {

      int v;
      int vdist;
      int vpred;
      int edge_start;
      bool continue_previous;
      if (frontier.current.vertex == -1) {
        auto tup = frontier.delete_min();
        v = std::get<0>(tup);
        vdist = std::get<1>(tup);
        vpred = std::get<2>(tup);
        edge_start = 0;
        continue_previous = false;
      }
      else {
        v = frontier.current.vertex;
        vdist = frontier.current.dist;
        vpred = frontier.current.pred;
        edge_start = frontier.current.edge_start;
        continue_previous = true;
      }

      int orig = -1;
      if (continue_previous || (finalized[v].load() == -1 && finalized[v].compare_exchange_strong(orig, vdist))) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
        if (predecessors) predecessors[v] = vpred;
        if (v == destination) {
          is_done.store(true);
          return;
        }

        int vdeg = graph.degree(v);

        int edge_end;
        if (vdeg - edge_start <= poll_cutoff - work_this_round) {
          edge_end = vdeg;
          frontier.current.vertex = -1;
        }
        else {
          edge_end = edge_start + poll_cutoff - work_this_round;
          frontier.current.vertex = v;
          frontier.current.dist = vdist;
          frontier.current.pred = vpred;
          frontier.current.edge_start = edge_end; // for next round...
        }

        graph.for_each_neighbor_in_range(v, edge_start, edge_end, [&] (int nbr, int weight) {
          // SIMULATE EXPANSION TIME
          timing::busy_loop_secs(exptime);

          if (finalized[nbr].load() == -1) {
            int nbrdist = vdist + weight;
            frontier.insert(heuristic(nbr) + nbrdist, std::make_tuple(nbr, nbrdist, v));
          }
        });

        work_this_round += edge_end - edge_start;
      }

    }
  };

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
  return finalized;
}

// d1 could be -1 (infinite), while d2 must be >= 0
inline bool dist_greater(int d1, int d2) {
  return d1 == -1 || d1 > d2;
}

struct vertpack {
  int distance;
  int pred;
};

// template <class GRAPH, class HEAP, class HEURISTIC>
// std::atomic<vertpack>*
// pwsa_pathcorrect(GRAPH& graph, const HEURISTIC& heuristic,
//                  const int& source, const int& destination,
//                  int split_cutoff, int poll_cutoff, double exptime,
//                  int* pebbles = nullptr) {
//   int N = graph.number_vertices();
//   std::atomic<bool>* is_expanded = pasl::data::mynew_array<std::atomic<bool>>(N);
//   std::atomic<vertpack>* gpred = pasl::data::mynew_array<std::atomic<vertpack>>(N);
//   //std::atomic<int>* finalized = pasl::data::mynew_array<std::atomic<int>>(N);
//   fill_array_par(is_expanded, N, false);
//   //fill_pair_array_par(gpred, N, -1); // this could be super broken
// //  pmemset((char*)gpred, -1, 2*N*sizeof(int)); // this could be super broken
//   pasl::sched::native::parallel_for(0, N, [&] (int i) {
//     vertpack ith;
//     ith.distance = -1;
//     ith.pred = -1;
//     gpred[i].store(ith);
//   });
//
//   vertpack src;
//   src.distance = 0;
//   src.pred = source;
//   gpred[source].store(src);
//
//   HEAP initF = HEAP();
//   int heur = heuristic(source);
//   initF.insert(heur, source);
//
//   pasl::data::perworker::array<int> work_since_split;
//   work_since_split.init(0);
//
//   auto size = [&] (HEAP& frontier) {
//     auto sz = frontier.size();
//     if (sz == 0) {
//       work_since_split.mine() = 0;
//       return 0; // no work left
//     }
//     if (sz > split_cutoff || (work_since_split.mine() > split_cutoff && sz > 1)) {
//       return 2; // split
//     }
//     else {
//       return 1; // don't split
//     }
//   };
//
//   auto fork = [&] (HEAP& src, HEAP& dst) {
//     src.split(dst);
//     work_since_split.mine() = 0;
//   };
//
//   auto set_in_env = [&] (HEAP& f) {;};
//
//   auto do_work = [&] (std::atomic<bool>& is_done, HEAP& frontier) {
//     int work_this_round = 0;
//     while (work_this_round < poll_cutoff && frontier.size() > 0) {
//       int v = frontier.delete_min();
//
//       bool orig = false;
//       if (!is_expanded[v].load() && is_expanded[v].compare_exchange_strong(orig, true)) {
//         if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
//
//         if (v == destination) {
//           is_done.store(true);
//           return;
//         }
//
//         graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {
//           // SIMULATE EXPANSION TIME
//           timing::busy_loop_secs(exptime);
//
//           vertpack gpred_v = gpred[v].load();
//
//           vertpack mine;
//           mine.distance = gpred_v.distance + weight;
//           mine.pred = v;
//
//           while (true) {
//             vertpack gpred_nbr = gpred[nbr].load();
//             if (dist_greater(gpred_nbr.distance, mine.distance)) {
//               if (gpred[nbr].compare_exchange_weak(gpred_nbr, mine)) {
//                 if (!is_expanded[nbr].load()) {
//                   frontier.insert(mine.distance + heuristic(nbr), nbr);
//                 }
//                 break;
//               }
//             }
//             else {
//               break;
//             }
//           }
//         });
//       }
//       work_this_round++;
//     }
//     work_since_split.mine() += work_this_round;
//   };
//
//   pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
//   return gpred;
// }

// ===========================================================================

struct unfinished_pack_pathcorrect {
  int vertex;
  int edge_start;
};

class FrontierPC {

  using HEAP = Heap<int>;

public:
  HEAP heap;
  unfinished_pack_pathcorrect current;

  FrontierPC() {
    heap = HEAP();
    current.vertex = -1;
  }

  void insert(int priority, const int& value) {
    heap.insert(priority, value);
  }

  long size() {
    return (current.vertex == -1 ? 0 : 1) + heap.size();
  }

  void split(FrontierPC& other) {
    heap.split(other.heap);
  }

  void swap(FrontierPC& other) {
    heap.swap(other.heap);
    std::swap(current, other.current);
  }

  int delete_min() {
    return heap.delete_min();
  }
};

template <class GRAPH, class HEURISTIC>
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
    ith.distance = -1;
    ith.pred = -1;
    gpred[i].store(ith);
  });

  vertpack src;
  src.distance = 0;
  src.pred = source;
  gpred[source].store(src);

  FrontierPC initF = FrontierPC();
  initF.insert(heuristic(source), source);

  pasl::data::perworker::array<int> work_since_split;
  work_since_split.init(0);

  // pasl::data::perworker::array<process_pack_pwsa_pathcorrect> must_process;
  // process_pack_pwsa_pathcorrect invalid;
  // invalid.vertex = -1;
  // invalid.edge_start = -1;
  // must_process.init(invalid);

  auto size = [&] (FrontierPC& frontier) {
    auto sz = frontier.size();
    if (sz == 0) {
      work_since_split.mine() = 0;
      return 0; // no work left
    }
    if (sz > split_cutoff || (work_since_split.mine() > split_cutoff && sz > 1)) {
      return 2; // split
    }
    else {
      return 1; // don't split
    }
  };

  auto fork = [&] (FrontierPC& src, FrontierPC& dst) {
    src.split(dst);
    work_since_split.mine() = 0;
  };

  auto set_in_env = [&] (FrontierPC& f) {;};

  auto do_work = [&] (std::atomic<bool>& is_done, FrontierPC& frontier) {
    int work_this_round = 0;
    while (work_this_round < poll_cutoff && frontier.size() > 0) {
      // process_pack p = to_process.mine();
      // int v = p.vertex;
      // int edge_start = p.edge_start;
      //
      // if (v != -1 || (!is_expanded[v = frontier.delete_min()].load() &&
      // int v = frontier.delete_min();

      int v;
      int edge_start;
      bool continue_previous;
      if (frontier.current.vertex == -1) {
        v = frontier.delete_min();
        edge_start = 0;
        continue_previous = false;
      }
      else {
        v = frontier.current.vertex;
        edge_start = frontier.current.edge_start;
        continue_previous = true;
      }

      bool orig = false;
      if (continue_previous || (!is_expanded[v].load() && is_expanded[v].compare_exchange_strong(orig, true))) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();

        if (v == destination) {
          is_done.store(true);
          return;
        }

        int edge_end;
        if (graph.degree(v) - edge_start <= poll_cutoff - work_this_round) {
          edge_end = graph.degree(v);
          frontier.current.vertex = -1;
        }
        else {
          edge_end = edge_start + poll_cutoff - work_this_round;
          frontier.current.vertex = v;
          frontier.current.edge_start = edge_end; // for next round...
        }


        graph.for_each_neighbor_in_range(v, edge_start, edge_end, [&] (int nbr, int weight) {
          // SIMULATE EXPANSION TIME
          timing::busy_loop_secs(exptime);

          vertpack gpred_v = gpred[v].load();

          vertpack mine;
          mine.distance = gpred_v.distance + weight;
          mine.pred = v;

          while (true) {
            vertpack gpred_nbr = gpred[nbr].load();
            if (dist_greater(gpred_nbr.distance, mine.distance)) {
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

        work_this_round += edge_end - edge_start;
      }
    }
    work_since_split.mine() += work_this_round;
  };

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
  return std::make_pair(gpred, is_expanded);
}
