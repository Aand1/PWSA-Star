/* COPYRIGHT (c) 2015 Sam Westrick and Laxman Dhulipala
 * All rights reserved.
 *
 * \file treap-frontier.hpp
 *
 */

#include "container.hpp"
#include "hash.hpp"

#ifndef _PWSA_TREAP_FRONTIER_H_
#define _PWSA_TREAP_FRONTIER_H_

// Assume objects k of type KEY have method k.hash(), which returns a long.
// Assume objects src of type VALUE have methods
//   * src.weight(), which returns a long. A weight of 0 indicates a useless
//     value whose node can be deleted.
//   * src.split_at(w, VALUE& dst), which "puts w of src into dst".
//     The value src.weight() before the split should be equal to
//     src.weight() + dst.weight() after the split, and src.weight() after
//     should be w.
template <class KEY, class VALUE>
class Treap {

private:
  struct Node {
    KEY key;
    VALUE value;
    long priority;

    long total_weight;

    Node* left;
    Node* right;
  };

  Node* root;

  inline long node_total_weight(Node* N) {
    return N == nullptr ? 0l : N->total_weight;
  }

  inline void fix_weights(Node* N) {
    N->total_weight = node_total_weight(N->left)
                    + N->value.weight()
                    + node_total_weight(N->right);
  }

  Node* maybe_rotate_right(Node* N) {
    assert(N != nullptr);
    assert(N->left != nullptr);
    if (N->left->priority > N->priority) {
      if (N->left->key == N->key) {
        N->left->total_weight += N->value.weight() - N->left->value.weight();
        std::swap(N->left->key, N->key);
        std::swap(N->left->value, N->value);
        std::swap(N->left->priority, N->priority);
        return N;
      }
      Node* newroot = N->left;
      N->left = newroot->right;
      newroot->right = N;

      N->total_weight -= newroot->total_weight;
      newroot->total_weight += N->total_weight;
      N->total_weight += node_total_weight(N->left);
      return newroot;
    }
    return N;
  }

  Node* maybe_rotate_left(Node* N) {
    assert(N != nullptr);
    assert(N->right != nullptr);
    if (N->right->priority > N->priority) {
      Node* newroot = N->right;
      N->right = newroot->left;
      newroot->left = N;

      N->total_weight -= newroot->total_weight;
      newroot->total_weight += N->total_weight;
      N->total_weight += node_total_weight(N->right);
      return newroot;
    }
    return N;
  }

  Node* node_insert(Node* N, KEY& key, VALUE& value) {
    if (N == nullptr) {
      N = pasl::data::mynew<Node>();
      N->key = key;
      N->value = value;
      N->total_weight = value.weight();
      N->priority = hash_signed((long) key);

      N->left = nullptr;
      N->right = nullptr;
      return N;
    }

    if (key <= N->key) {
      N->left = node_insert(N->left, key, value);
      fix_weights(N);
      return maybe_rotate_right(N);
    }
    else {
      N->right = node_insert(N->right, key, value);
      fix_weights(N);
      return maybe_rotate_left(N);
    }
  }

  Node* node_delete_min(Node* N, KEY& kresult, VALUE& vresult) {
    if (N->left == nullptr) {
      kresult = N->key;
      vresult = N->value;
      Node* result = N->right;
      pasl::data::myfree(N);
      return result;
    }

    N->left = node_delete_min(N->left, kresult, vresult);
    fix_weights(N);
    return N;
  }

  std::pair<Node*,Node*> node_split_at(Node* N, long w) {
    if (N == nullptr) return std::pair<Node*,Node*>(nullptr, nullptr);

    if (w <= node_total_weight(N->left)) {
      auto pair = node_split_at(N->left, w);
      N->left = pair.second;
      fix_weights(N);
      return std::pair<Node*,Node*>(pair.first, N);
    }
    else if (w < node_total_weight(N->left) + N->value.weight()) {
      Node* M = pasl::data::mynew<Node>();
      M->key = N->key;
      M->priority = N->priority;
      M->left = nullptr;
      M->right = N->right;
      N->value.split_at(w - node_total_weight(N->left), M->value);

      N->right = nullptr;
      fix_weights(N);
      fix_weights(M);
      return std::pair<Node*,Node*>(N, M);
    }
    else if (w == node_total_weight(N->left) + N->value.weight()) {
      auto pair = std::pair<Node*,Node*>(N, N->right);
      N->right = nullptr;
      fix_weights(N);
      return pair;
    }
    else {
      auto pair = node_split_at(N->right, w - node_total_weight(N->left) - N->value.weight());
      N->right = pair.first;
      fix_weights(N);
      return std::pair<Node*,Node*>(N, pair.second);
    }
  }

  void display_node(Node* N) {
    if (N == nullptr) std::cout << "-";
    else {
      std::cout << "(";
      display_node(N->left);
      std::cout << " " << N->key << " " << N->value /*<< N->weight*/ << " " /*<< N->total_weight << " "*/;
      display_node(N->right);
      std::cout << ")";
    }
  }

  void node_check(Node* N, KEY* left_bound, KEY* right_bound, long* p_bound) {
    if (N == nullptr) return;
    assert(left_bound == nullptr || (*left_bound) < N->key);
    assert(right_bound == nullptr || N->key <= (*right_bound));
    assert(p_bound == nullptr || N->priority <= (*p_bound));
    assert(N->total_weight == node_total_weight(N->left) + N->value.weight() + node_total_weight(N->right));
    node_check(N->left, left_bound, &(N->key), &(N->priority));
    node_check(N->right, &(N->key), right_bound, &(N->priority));
  }

public:

  using self_type = Treap<KEY,VALUE>;

  Treap() : root(nullptr) { }

  void insert(KEY& key, VALUE& value) {
    root = node_insert(root, key, value);
  }

  std::pair<KEY,VALUE> delete_min() {
    auto pair = std::pair<KEY,VALUE>();
    // node_delete_min writes into pair.first, pair.second
    root = node_delete_min(root, pair.first, pair.second);
    return pair;
  }

  long total_weight() {
    return node_total_weight(root);
  }

  void split_at(long w, self_type& other) {
    auto pair = node_split_at(root, w);
    root = pair.first;
    other.root = pair.second;
  }

  void display() {
    display_node(root);
    std::cout << std::endl;
  }

  void swap(self_type& other) {
    std::swap(root, other.root);
  }

  void check() {
    node_check(root, nullptr, nullptr, nullptr);
  }

};

#endif

// **************************************************************************

// int main(int argc, char** argv) {
//   long n;
//
//   auto init = [&] {
//     n = (long)pasl::util::cmdline::parse_or_default_long("n", 24);
//   };
//
//   auto run = [&] (bool sequential) {
//     auto T = Treap<long,long>();
//     for (int i = 0; i < n; i++) {
//       long x = rand() % 1000;
//       T.insert(x,x,1);
//       //T.display();
//     }
//
//     auto M = T.split_at_weight(n/3);
//     auto R = M.split_at_weight(n/3);
//
//     std::cout << T.total_weight() << std::endl;
//     std::cout << M.total_weight() << std::endl;
//     std::cout << R.total_weight() << std::endl;
//
//     T.check();
//     M.check();
//     R.check();
//
//     long recent = -1l;
//     for (int i = 0; i < n; i++) {
//       auto pair = i < n/3 ? T.delete_min() : (i < 2*n/3 ? M.delete_min() : R.delete_min());
//       assert(recent <= pair.first);
//       recent = pair.first;
//       std::cout << recent << " ";
//       // std::cout << pair.first << " " << pair.second << std::endl;
//       //T.display();
//     }
//     std::cout << std::endl;
//   };
//
//   auto output = [&] {
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
