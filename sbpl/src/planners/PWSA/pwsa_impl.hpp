#include <sys/time.h>

#define MAX_N 100000

namespace timing {
  inline uint64_t now() {
    struct timeval X;
    gettimeofday(&X, NULL);
    return X.tv_sec * ((uint64_t)1000000) + X.tv_usec;
  }

  void busy_loop_secs(double secs) {
    uint64_t t0 = now();
    while ((now() - t0) < (secs * 1000000.0));
  }
}

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



template <class GRAPH, class HEAP, class HEURISTIC>
std::atomic<int>* pwsa(GRAPH& graph, const HEURISTIC& heuristic,
                        const int& source, const int& destination,
                        int split_cutoff, int poll_cutoff,
                        int* pebbles = nullptr, int* predecessors = nullptr) {
  std::atomic<int>* finalized = pasl::data::mynew_array<std::atomic<int>>(MAX_N);
  fill_array_par(finalized, MAX_N, -1);

  HEAP initF = HEAP();
  int heur = heuristic(source);
  initF.insert(heur, std::make_tuple(source, 0, 0));

  pasl::data::perworker::array<int> work_since_split;
  work_since_split.init(0);

  printf("source is : %d, goal is %d \n", source, destination);

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

