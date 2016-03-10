//include "array_util.hpp"
#include "container.hpp"
#include "native.hpp"
#include "timing.hpp"
#include "weighted_graph.hpp"
#include <climits>
#include <pthread.h>
#include <map>
#include "utils.hpp"

#ifndef _MY_PASE_H_
#define _MY_PASE_H_

const bool debug_print = false;
template <class Body>
void atomic_log(const Body& b) {
  if (debug_print) pasl::util::atomic::msg(b);
}

struct state_pase {
  bool is_expanded;
  bool being_expanded;
  int g;
  int pred;
};

class Locked {
private:
  pthread_mutex_t mutex;

public:
  Locked() { pthread_mutex_init(&mutex,NULL); }

  template <class Body>
  void action(const Body& b) {
    atomic_log([&] { std::cout << pasl::sched::threaddag::get_my_id() << " waiting for lock " << std::endl; });
    pthread_mutex_lock(&mutex);
    atomic_log([&] { std::cout << pasl::sched::threaddag::get_my_id() << " successfully locked" << std::endl; });

    b();

    atomic_log([&] { std::cout << pasl::sched::threaddag::get_my_id() << " unlocking" << std::endl; });
    pthread_mutex_unlock(&mutex);
    atomic_log([&] { std::cout << pasl::sched::threaddag::get_my_id() << " successfully unlocked" << std::endl; });
  }
};

// ===========================================================================
// ===========================================================================
// ===========================================================================

template <class GRAPH, class HEAP, class HEURISTIC>
state_pase*
pase(GRAPH& graph, const HEURISTIC& heuristic,
     const int& source, const int& destination,
     double exptime, int* pebbles = nullptr) {
  // ============= some PASL helpers =============
  auto yield = [] { pasl::sched::native::yield(); };
  auto hiccup = [] { pasl::util::ticks::microseconds_sleep(10.0); };
  auto my_id = [] { return pasl::sched::threaddag::get_my_id(); };

  // ============= initialization and queue manipulations =============
  int N = graph.number_vertices();
  state_pase* states = pasl::data::mynew_array<state_pase>(N);
  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    states[i].is_expanded = false;
    states[i].being_expanded = false;
    states[i].g = INT_MAX; // "infinite"
    states[i].pred = -1;
  });

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
  return states;
}

#endif // _MY_PASE_H_
