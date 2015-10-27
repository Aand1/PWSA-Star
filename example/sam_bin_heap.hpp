#include <climits>
#include "array-util.hpp"

#ifndef _PWSA_BIN_HEAP_H_
#define _PWSA_BIN_HEAP_H_

template <class VALUE>
class Heap {
private:

  struct KV {
    long priority;
    VALUE value;
  };

  long size;
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
    return left_child(i) < size;
  }

  inline bool has_right_child(long i) {
    return right_child(i) < size;
  }

  inline long priority(long i) {
    return i >= size ? LONG_MAX : values[i].priority;
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
//    std::cout << "heapifying up from " << i << std::endl;
    long curr = i;
    while (has_parent(curr) && (priority(curr) < priority(parent(curr)))) {
//      std::cout << "swapping with parent... " << std::endl;
      std::swap(values[curr], values[parent(curr)]);
      curr = parent(curr);
    }
//    std::cout << "done heapifying" << std::endl;
  }

  void resize_bigger() {
    assert(size == capacity);

    capacity *= 2;
    KV* old_values = values;
    values = array_util::my_malloc<KV>(capacity);

    for (long i = 0; i < size; i++) {
      values[i] = old_values[i];
    }

    free(old_values);
  }

public:

  void resize_bigger(long new_cap) {
    assert(size <= new_cap);

    capacity = new_cap;
    KV* old_values = values;
    values = array_util::my_malloc<KV>(capacity);

    for (long i = 0; i < size; i++) {
      values[i] = old_values[i];
    }

    free(old_values);
  }

  Heap() {
    size = 0;
    capacity = 2;
//    std::cout << "allocating values... " << std::endl;
    values = array_util::my_malloc<KV>(2);
  }

  void insert(long priority, const VALUE& value) {
    if (size == capacity) resize_bigger(capacity * 2);

//    std::cout << "inserting " << value << " with priority " << priority << " at values[" << size << "]" << std::endl;
    values[size].priority = priority;
    values[size].value = value;
    size++;
    heapify_up_from(size-1);
//    std::cout << "done inserting. size is now " << size << std::endl;
  }

  std::pair<long,VALUE> delete_min() {
    assert(size > 0);
    auto result = std::pair<long,VALUE>(values[0].priority, values[0].value);
    values[0] = values[size-1];
    size--;
    heapify_down_from(0);
    return result;
  }

  bool isEmpty() {
    return size == 0;
  }

  long total_weight() {
    return size;
  }

  // ignore w for now.
  void split_at(long w, Heap<VALUE>& other) {
    if (size < 2) {
      return;
    }

    long n = size;
    size = 0;

    KV root = values[0];
    other.resize_bigger(n / 2);
    other.size = 0;

    long write_this_idx = 0;
    long write_other_idx = 0;
    long read_idx = 1;

    for (long level = 2; read_idx < n; level *= 2) {
      for (long i = 0; read_idx < n && i < level / 2; i++) {
        values[write_this_idx] = values[read_idx];
        write_this_idx++;
        read_idx++;
        size++;
      }
      for (long i = 0; read_idx < n && i < level / 2; i++) {
        other.values[write_other_idx] = values[read_idx];
        write_other_idx++;
        read_idx++;
        other.size++;
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
    std::swap(size, other.size);
    std::swap(capacity, other.capacity);
    std::swap(values, other.values);
  }

  void display() {
    std::cout << "{" << std::flush;
    for (long i = 0; i < size; i++) {
      std::cout << "(" << std::flush << values[i].priority << "," << std::flush << values[i].value << ")" << std::flush;
      if (i != size - 1) {
        std::cout << "," << std::flush;
      }
    }
    std::cout << "}" << std::flush;
  }
};

// template <class VALUE>
// class Heap {
//
// private:
//
//   struct KV {
//     public:
//       int key;
//       VALUE value;
//   };
//
//   vector<KV> values;
//   int size;
//
//   int parent(int index) {
//     return (index - 1)>>1;
//   }
//
//   int leftChild(int index) {
//     int ch = (index<<1)+1;
//     return size <= ch ? NONE : ch;
//   }
//
//   int rightChild(int index) {
//     int ch = (index<<1)+2;
//     return size <= ch ? NONE : ch;
//   }
//
//   void swap(int a, int b) {
//     KV tmp = values[a];
//     values[a].key = values[b].key;
//     values[a].value = values[b].value;
//     values[b].key = tmp.key;
//     values[b].value = tmp.value;
//   }
//
//   int shiftDown(int startNode, vector<KV>& ov) {
//     std::queue<int> frontier;
//     frontier.push(startNode);
//     int numPushed = 0;
//
//     while (!frontier.empty()) {
//       int index = frontier.front();
//       frontier.pop();
//       ov[numPushed].key = values[index].key;
//       ov[numPushed].value = values[index].value;
//       int lc = leftChild(index);
//       int rc = rightChild(index);
//       if (lc != NONE) {
//         frontier.push(lc);
//       }
//       if (rc != NONE) {
//         frontier.push(rc);
//       }
//       numPushed++;
//     }
//     return numPushed;
//   }
//
//   void heapifyDown() {
//     int index = 0;
//     while (index != size-1) {
//       int lIndex = leftChild(index);
//       int rIndex = rightChild(index);
//       if (lIndex == NONE) return;
//       int childIndex = lIndex;
//       if (rIndex == NONE) {
//         childIndex = lIndex;
//       } else if(values[lIndex].key > values[rIndex].key) {
//         childIndex = rIndex;
//       }
//       if (values[index].key <= values[childIndex].key) {
//         break;
//       }
//       swap(index,childIndex);
//       index = childIndex;
//     }
//   }
//
//   void heapifyUp(int index) {
//     while (index) {
//       int pIndex = parent(index);
//       if (values[index].key < values[pIndex].key) {
//         swap(index,pIndex);
//         index = pIndex;
//       } else {
//         break;
//       }
//     }
//   }
//
// public:
//
//   Heap() : size(0) { }
//
//   void insert(const int& key, const VALUE& value) {
//     if (size == values.capacity()) {
// //      std::cout << "resizing to " << ((size + 1) * 2) << endl;
//       values.resize((size + 1) * 2);
//     }
//     int newindex = size++;
//     values[newindex].key = key;
//     values[newindex].value = value;
//     heapifyUp(size-1);
//   }
//
//   std::pair<int, VALUE> delete_min() {
//     std::pair<int, VALUE> res = std::make_pair(values[0].key, values[0].value);
//     values[0] = values[--size];
//     heapifyDown();
//     return res;
//   }
//
//   bool isEmpty() {
//     return size == 0;
//   }
//
//   int total_weight() {
//     return size;
//   }
//
//   void display() {
//
//   }
//
//   // TODO: we're callously ignoring 'w'. Let' fix later
//   void split_at(long w, Heap<VALUE>& other) {
//     // have size, will split by giving essentially the left child
//     // plus the root to us, and the right-child to them
//     if (size < 2) {
//       other.size = 0;
//     }
//     int halfHeap = largestPowTwo(size);
//     halfHeap /= 2; // giving half to each side (upper bound)
//     other.values.resize(halfHeap);
//     int frontKey = values[0].key;
//     VALUE frontValue = values[0].value; // TODO: have to make sure copy constructor works on values
//     int otherSize = shiftDown(2, other.values);
//     other.size = otherSize;
//     size = shiftDown(1, values);
//     insert(frontKey, frontValue);
//   }
//
//   void swap(Heap<VALUE>& other) {
//     std::swap(values, other.values);
//     std::swap(size, other.size);
//   }
//
//   void print () {
//     for (int i = 0; i < size; ++i) {
//       cout << values[i].key << " " << values[i].value << endl;
//     }
//     cout << endl;
//   }
// };

#endif
