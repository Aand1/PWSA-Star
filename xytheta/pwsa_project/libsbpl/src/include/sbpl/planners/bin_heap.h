#ifndef _PWSA_BIN_HEAP_H_
#define _PWSA_BIN_HEAP_H_

#include <climits>
#include <pasl/sequtil/container.hpp> 

template <class VALUE>
class Heap {
private:

  struct KV {
    long priority;
    VALUE value;
  };

  long n;
  long capacity;
  KV* values;

  inline long parent(long i) {
    return (i - 1) / 2;
  }

  inline long left_child(long i) {
    return 2*i + 1;
  }

  inline long right_child(long i) {
    return 2*i + 2;
  }

  inline bool has_parent(long i) {
    return i > 0;
  }

  inline bool has_left_child(long i) {
    return left_child(i) < n;
  }

  inline bool has_right_child(long i) {
    return right_child(i) < n;
  }

  inline long priority(long i) {
    return i >= n ? LONG_MAX : values[i].priority;
  }

  // inline void swap(long i, long j) {
  //   std::swap(values[i], values[j]);
  // }

  inline long min_child(long i) {
    return priority(left_child(i)) < priority(right_child(i)) ?
           left_child(i) : right_child(i);
  }

  void heapify_down_from(long i) {
    long curr = i;
    while ((priority(curr) > priority(left_child(curr)))
        || (priority(curr) > priority(right_child(curr)))) {
      long next = min_child(curr);
      std::swap(values[curr], values[next]);
      curr = next;
    }
  }

  void heapify_up_from(long i) {
    long curr = i;
    while (has_parent(curr) && (priority(curr) < priority(parent(curr)))) {
      std::swap(values[curr], values[parent(curr)]);
      curr = parent(curr);
    }
  }

  // void resize_bigger() {
  //   assert(n == capacity);
  //
  //   capacity *= 2;
  //   KV* old_values = values;
  //   values = array_util::my_malloc<KV>(capacity);
  //
  //   for (long i = 0; i < n; i++) {
  //     values[i] = old_values[i];
  //   }
  //
  //   free(old_values);
  // }

public:

  //typedef VALUE value_type;

  void resize_bigger(long new_cap) {
    assert(n <= new_cap);

    capacity = new_cap;
    KV* old_values = values;
    values = pasl::data::mynew_array<KV>(capacity);

    for (long i = 0; i < n; i++) {
      values[i] = old_values[i];
    }

    free(old_values);
  }

  Heap() {
    n = 0;
    capacity = 4096;
    values = pasl::data::mynew_array<KV>(capacity);
  }

  void insert(long priority, const VALUE& value) {
    if (n == capacity) resize_bigger(capacity * 2);

    values[n].priority = priority;
    values[n].value = value;
    n++;
    heapify_up_from(n-1);
  }

  VALUE delete_min() {
    assert(n > 0);
    VALUE result = values[0].value;
    values[0] = values[n-1];
    n--;
    heapify_down_from(0);
    return result;
  }

  void discard_min() {
    assert(n > 0);
    values[0] = values[n-1];
    n--;
    heapify_down_from(0);
  }

  VALUE* peek() {
    assert(n > 0);
    return &(values[0].value);
  }

  // bool isEmpty() {
  //   return n == 0;
  // }

  long size() {
    return n;
  }

  void split(Heap<VALUE>& other) {
    if (n < 2) {
      return;
    }

    long m = n;
    n = 0;

    KV root = values[0];
    other.resize_bigger(m / 2);
    other.n = 0;

    long write_this_idx = 0;
    long write_other_idx = 0;
    long read_idx = 1;

    for (long level = 2; read_idx < m; level *= 2) {
      for (long i = 0; read_idx < m && i < level / 2; i++) {
        values[write_this_idx] = values[read_idx];
        write_this_idx++;
        read_idx++;
        n++;
      }
      for (long i = 0; read_idx < m && i < level / 2; i++) {
        other.values[write_other_idx] = values[read_idx];
        write_other_idx++;
        read_idx++;
        other.n++;
      }
    }

    other.insert(root.priority, root.value);
  }

  // friend std::ostream& operator<< (std::ostream& stream, const Heap<VALUE>& H) {
  //   stream << "{" << std::flush;
  //   for (long i = 0; i < H.size; i++) {
  //     stream << "(" << std::flush << H.values[i].priority << "," << std::flush << H.values[i].value << ")" << std::flush;
  //     if (i != H.size - 1) {
  //       stream << "," << std::flush;
  //     }
  //   }
  //   stream << "}" << std::flush;
  // }

  void swap(Heap<VALUE>& other) {
    std::swap(n, other.n);
    std::swap(capacity, other.capacity);
    std::swap(values, other.values);
  }

  void display() {
    std::cout << "{" << std::flush;
    for (long i = 0; i < n; i++) {
      std::cout << "(" << std::flush << values[i].priority << "," << std::flush << values[i].value << ")" << std::flush;
      if (i != n - 1) {
        std::cout << "," << std::flush;
      }
    }
    std::cout << "}" << std::flush;
  }
};

#endif
