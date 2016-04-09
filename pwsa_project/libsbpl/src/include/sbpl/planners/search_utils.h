#ifndef _SEARCH_UTILS_H_
#define _SEARCH_UTILS_H_

const bool debug_print = false;
template <class Body>
void atomic_log(const Body& b) {
  if (debug_print) pasl::util::atomic::msg(b);
}

class SearchResult {
public:
  virtual ~SearchResult() { }
  virtual bool is_expanded(int vertex) { return false; }
  virtual int predecessor(int vertex) { return -1; }
  virtual int g(int vertex) { return -1; }
  virtual int pebble(int vertex) { return -1; }
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

#endif // _SEARCH_UTILS_H_
