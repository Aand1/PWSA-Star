//include "array_util.hpp"
#include "container.hpp"
#include "native.hpp"
#include "timing.hpp"
#include <climits>
#include <pthread.h>
#include <map>

// ===========================================================================
// ===========================================================================
// ===========================================================================

const bool debug_print = false;
template <class Body>
void msg(const Body& b) {
  if (debug_print)
    pasl::util::atomic::msg(b);
}

struct state_simple {
  std::atomic<bool> is_expanded;
  std::atomic<int> pred;
  std::atomic<int> g;
  int h;
};

template <class GRAPH, class HEAP, class HEURISTIC>
state_simple*
wanre(GRAPH& graph, const HEURISTIC& heuristic,
      const int& source, const int& destination,
      double exptime, int* pebbles = nullptr) {
  int N = graph.number_vertices();
  state_simple* states = pasl::data::mynew_array<state_simple>(N);
  pasl::sched::native::parallel_for(0, N, [&] (int i) {
    states[i].is_expanded.store(false);
    states[i].g.store(INT_MAX); // "infinite"
    states[i].pred.store(-1);
    states[i].h = -1;
  });

  // memoize heuristic
  auto my_heur = [&] (int v) {
    if (states[v].h == -1)
      states[v].h = heuristic(v);
    return states[v].h;
  };

  states[source].g.store(0);

  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex,NULL);
  // HEAP frontier = HEAP();
  int heur = my_heur(source);
  // frontier.insert(heur, source);
  using Q = std::multimap<int, int>;
  Q frontier = Q();
  frontier.insert(std::make_pair(heur, source));

  std::atomic<bool> is_done(false);

  auto body = [&] {

    msg([&] { std::cout << pasl::sched::threaddag::get_my_id() << ": there are " << pasl::sched::threaddag::get_nb_workers() << " workers" << std::endl; });

    while (!is_done.load()) {

      pthread_mutex_lock(&mutex);
      if (frontier.size() == 0) {
        pthread_mutex_unlock(&mutex);
        pasl::sched::native::yield();
        continue;
      }
      int v = frontier.begin()->second;
      frontier.erase(frontier.begin());
      msg([&] { std::cout << pasl::sched::threaddag::get_my_id() << " popped " << v << std::endl; });
//      int v = frontier.delete_min();
      pthread_mutex_unlock(&mutex);
      pasl::sched::native::yield();


      bool orig = states[v].is_expanded.load();
      if (!orig && states[v].is_expanded.compare_exchange_strong(orig, true)) {
        if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();

        if (v == destination) {
          is_done.store(true);
          return;
        }

        //graph.simulate_get_successors(exptime);

        int vdist = states[v].g.load();
        graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {

          timing::busy_loop_secs(exptime);

          int nbrdist = vdist + weight;
          if (nbrdist < states[nbr].g.load()) {
            pthread_mutex_lock(&mutex);
//            frontier.insert(nbrdist + my_heur(nbr), nbr);
            frontier.insert(std::make_pair(nbrdist + my_heur(nbr), nbr));
            pthread_mutex_unlock(&mutex);
            pasl::sched::native::yield();
            states[nbr].g.store(nbrdist);
            states[nbr].pred.store(v);
          }

        });

      }

    }
  };

  // auto do_work = [&] (std::atomic<bool>& is_done, HEAP& frontier) {
  // //auto do_work = [&] (bool& is_done, HEAP& frontier) {
  //   int steps_this_round = 0;
  //   while (steps_this_round < poll_cutoff && frontier.size() > 0 && !is_done.load()) {
  //     int v = frontier.delete_min();
  //     steps_this_round++;
  //
  //     bool orig = states[v].is_expanded.load();
  //     if (!orig && states[v].is_expanded.compare_exchange_strong(orig, true)) {
  //       //if (pebbles) pebbles[v] = pasl::sched::threaddag::get_my_id();
  //
  //       if (v == destination) {
  //         is_done.store(true);
  //         //is_done = true;
  //         return;
  //       }
  //
  //       // SIMULATE EXPANSION
  //       graph.simulate_get_successors(exptime);
  //
  //       int vdist = states[v].g;
  //       graph.for_each_neighbor_of(v, [&] (int nbr, int weight) {
  //
  //         int nbrdist = vdist + weight;
  //         if (nbrdist < states[nbr].g) {
  //           frontier.insert(nbrdist + my_heur(nbr), nbr);
  //           states[nbr].g = nbrdist;
  //         }
  //       });
  //     }
  //   }
  //   steps_since_load_balancing.mine() += steps_this_round;
  // };

  pasl::sched::native::parallel_while(body);
  return states;
}
