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

template <class Body>
void print(bool debug, const Body& b) {
  if (debug) {
    pasl::util::atomic::msg(b);
  }
}

// uint64_t GetTimeStamp() {
//   struct timeval tv;
//   gettimeofday(&tv,NULL);
//   return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
// }

// we use std::atomic only to interface nicely with pwsa
template <class GRAPH, class HEAP, class HEURISTIC>
std::atomic<int>* astar(GRAPH& graph, const HEURISTIC& heuristic,
                        const int& source, const int& destination,
                        double exptime,
                        int* pebbles = nullptr, int* predecessors = nullptr) {
  int N = graph.number_vertices();
  std::atomic<int>* dist = pasl::data::mynew_array<std::atomic<int>>(N);
  for (int i = 0; i < N; i++) {
    dist[i].store(-1);
  }
  HEAP frontier = HEAP();
  frontier.insert(heuristic(source), std::make_tuple(source, 0, 0));
  while (dist[destination].load() == -1) {
    auto tup = frontier.delete_min();
    int v = std::get<0>(tup);
    int vdist = std::get<1>(tup);
    int pred = std::get<2>(tup);
    if (dist[v].load() == -1) {
      dist[v].store(vdist);
      if (pebbles) pebbles[v] = 0; // normally would write processor ID, but only one proc in this case
      if (predecessors) predecessors[v] = pred;

      graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
        int nghdist = vdist + weight;
        if (dist[ngh].load() == -1) {
          frontier.insert(heuristic(ngh) + nghdist, std::make_tuple(ngh, nghdist, v));
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

template <class GRAPH, class HEAP, class HEURISTIC>
std::atomic<int>* pwsa(GRAPH& graph, const HEURISTIC& heuristic,
                        const int& source, const int& destination,
                        int split_cutoff, int poll_cutoff, double exptime,
                        int* pebbles = nullptr, int* predecessors = nullptr) {
  int N = graph.number_vertices();
  std::atomic<int>* finalized = pasl::data::mynew_array<std::atomic<int>>(N);
  fill_array_par(finalized, N, -1);

  HEAP initF = HEAP();
  int heur = heuristic(source);
  initF.insert(heur, std::make_tuple(source, 0, 0));

  pasl::data::perworker::array<int> work_since_split;
  work_since_split.init(0);

  auto size = [&] (HEAP& frontier) {
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

  auto fork = [&] (HEAP& src, HEAP& dst) {
    src.split(dst);
    work_since_split.mine() = 0;
  };

  auto set_in_env = [&] (HEAP& f) {;};

  auto do_work = [&] (HEAP& frontier) {
    int work_this_round = 0;
    while (work_this_round < poll_cutoff && frontier.size() > 0) {
      auto tup = frontier.delete_min();
      int v = std::get<0>(tup);
      int vdist = std::get<1>(tup);
      int pred = std::get<2>(tup);
      int orig = -1;
      if (finalized[v].load() == -1 && finalized[v].compare_exchange_strong(orig, vdist)) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
        if (predecessors) predecessors[v] = pred;
        if (v == destination) {
          return true;
        }
        graph.for_each_neighbor_of(v, [&] (int ngh, int weight) {
          int nghdist = vdist + weight;
          frontier.insert(heuristic(ngh) + nghdist, std::make_tuple(ngh, nghdist, v));

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
      work_this_round++;
    }
    work_since_split.mine() += work_this_round;
    return false;
  };

  pasl::sched::native::parallel_while_pwsa(initF, size, fork, set_in_env, do_work);
  return finalized;
}
