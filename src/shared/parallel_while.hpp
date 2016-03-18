#include "native.hpp" // pasl/sched/native.hpp

#ifndef _PARALLEL_WHILE_PWSA_H_
#define _PARALLEL_WHILE_PWSA_H_

namespace pasl {
namespace sched {
namespace native {

template <class Input, class Size_input, class Fork_input, class Body>
void parallel_while_pwsa(Input& input, const Size_input& size_input, const Fork_input& fork_input, const Body& body) {
  using request_type = worker_id_t;
  const request_type REQUEST_BLOCKED = -2;
  const request_type REQUEST_WAITING = -1;
  using answer_type = enum { ANSWER_WAITING, ANSWER_REJECTED, ANSWER_TRANSFERRED };
  using size_type = int;
  const size_type SIZE_EMPTY = 0;
  const size_type SIZE_KEEP = 1;
  const size_type SIZE_SPLIT = 2;

  data::perworker::array<Input> frontiers;
  data::perworker::array<std::atomic<request_type>> requests;
  data::perworker::array<std::atomic<answer_type>> answers;
  worker_id_t leader_id = threaddag::get_my_id();
  requests.for_each([&] (worker_id_t i, std::atomic<request_type>& r) {
    r.store(i == leader_id ? REQUEST_WAITING : REQUEST_BLOCKED);
  });
  // answers.for_each([] (worker_id_t, std::atomic<answer_type>& a) {
  //   a.store(ANSWER_WAITING);
  // });

  std::atomic<bool> is_done(false);
  auto b = [&] {
    worker_id_t my_id = threaddag::get_my_id();
    scheduler_p sched = threaddag::my_sched();
    multishot* thread = my_thread();
    Input my_frontier;
    if (my_id == leader_id) my_frontier.swap(input);

    while (!is_done.load()) {
      int sz = size_input(my_frontier);

      if (sz == 0) {
        // Block incoming requests
        while (true) {
          request_type r = requests[my_id].load();
          if (r == REQUEST_BLOCKED) break;
          if (r == REQUEST_WAITING) {
            if (requests[my_id].compare_exchange_strong(r, REQUEST_BLOCKED)) break;
          }
          else {
            worker_id_t other = r;
            requests[my_id].store(REQUEST_BLOCKED);
            answers[other].store(ANSWER_REJECTED);
            break;
          }
        }

        // Acquire work from random other
        while (!is_done.load()) {
          answers[my_id].store(ANSWER_WAITING);
          worker_id_t other = sched->random_other();
          if (requests[other].load() != REQUEST_WAITING) {
            thread->yield(); util::ticks::microseconds_sleep(1.0); continue;
          }

          request_type expected = REQUEST_WAITING;
          if (!requests[other].compare_exchange_strong(expected, my_id)) {
            thread->yield(); util::ticks::microseconds_sleep(1.0); continue;
          }

          while (answers[my_id].load() == ANSWER_WAITING) {
            thread->yield(); // why?
            util::ticks::microseconds_sleep(1.0);
            if (is_done.load()) return;
          }

          if (answers[my_id].load() == ANSWER_TRANSFERRED) break;
        }

        frontiers[my_id].swap(my_frontier);
        sz = size_input(my_frontier);
      } // end case sz == 0

      assert(sz > 0);

      // Respond to possible incoming requests
      request_type r = requests[my_id].load();
      if (r == REQUEST_BLOCKED) requests[my_id].store(REQUEST_WAITING);
      if (r >= 0) {
        worker_id_t other = r;
        if (sz == SIZE_SPLIT) {
          fork_input(my_frontier, frontiers[other]);
          answers[other].store(ANSWER_TRANSFERRED);
        }
        else {
          answers[other].store(ANSWER_REJECTED);
        }
        requests[my_id].store(REQUEST_WAITING);
      }

      // Time to do work!
      bool found_dst = body(my_frontier);
      if (found_dst) {
        is_done.store(true);
        return;
      }

    }
  };
  parallel_while(b);
}

} // end namespace
} // end namespace
} // end namespace

#endif // _PARALLEL_WHILE_PWSA_H_
