/* COPYRIGHT (c) 2014 Umut Acar, Arthur Chargueraud, and Michael
 * Rainey
 * All rights reserved.
 *
 * \file dfs.hpp
 *
 */

#include "benchmark.hpp"

template <class KEY, class VALUE>
class Treap {
private:
  struct Node {
    KEY key;
    VALUE value;
    long weight;
    long priority;

    long weight_sum;

    Node* left;
    Node* right;
  };

  Node* root;

  inline long get_total_weight(Node* N) {
    return N == nullptr ? 0l : N->weight_sum;
  }

  // void fix_weight_sum(Node* N) {
  //   N->weight_sum = get_total_weight(N->left) + N->weight + get_total_weight(N->right);
  // }

  Node* maybe_rotate_right(Node* N) {
    assert(N != nullptr);
    assert(N->left != nullptr);
    if (N->left->priority > N->priority) {
      Node* newroot = N->left;
      N->left = newroot->right;
      newroot->right = N;

      N->weight_sum -= newroot->weight_sum;
      newroot->weight_sum += N->weight_sum;
      N->weight_sum += get_total_weight(N->left);
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

      N->weight_sum -= newroot->weight_sum;
      newroot->weight_sum += N->weight_sum;
      N->weight_sum += get_total_weight(N->right);
      return newroot;
    }
    return N;
  }

  Node* node_insert(Node* N, const KEY& key, const VALUE& value, long weight) {
    if (N == nullptr) {
      Node* newN = new Node;
      newN->key = key;
      newN->value = value;
      newN->weight = weight;
      newN->weight_sum = weight;
      newN->priority = rand();

      newN->left = nullptr;
      newN->right = nullptr;
      return newN;
    }

    if (key <= N->key) {
      N->weight_sum -= get_total_weight(N->left);
      N->left = node_insert(N->left, key, value, weight);
      N->weight_sum += get_total_weight(N->left);
      return maybe_rotate_right(N);
    }
    else {
      N->weight_sum -= get_total_weight(N->right);
      N->right = node_insert(N->right, key, value, weight);
      N->weight_sum += get_total_weight(N->right);
      return maybe_rotate_left(N);
    }
  }

  Node* node_delete_min(Node* N, KEY& kresult, VALUE& vresult) {
    if (N->left == nullptr) {
      kresult = N->key;
      vresult = N->value;
      Node* result = N->right;
      delete N;
      return result;
    }

    N->weight_sum -= get_total_weight(N->left);
    N->left = node_delete_min(N->left, kresult, vresult);
    N->weight_sum += get_total_weight(N->left);
    return N;
  }

  std::pair<Node*,Node*> node_split_at(Node* N, long w) {
    if (N == nullptr) return std::pair<Node*,Node*>(nullptr, nullptr);

    long weightL = get_total_weight(N->left);
    if (w < weightL) {
      N->weight_sum -= weightL;
      auto pair = node_split_at(N->left, w);
      N->left = pair.second;
      N->weight_sum += get_total_weight(N->left);
      return std::pair<Node*,Node*>(pair.first, N);
    }
    else if (w < weightL + N->weight) {
      auto pair = std::pair<Node*,Node*>(N, N->right);
      N->weight_sum -= get_total_weight(N->right);
      N->right = nullptr;
      return pair;
    }
    else {
      N->weight_sum -= get_total_weight(N->right);
      auto pair = node_split_at(N->right, w - weightL - N->weight);
      N->right = pair.first;
      N->weight_sum += get_total_weight(N->right);
      return std::pair<Node*,Node*>(N, pair.second);
    }
  }

  void display_node(Node* N) {
    if (N == nullptr) std::cout << "-";
    else {
      std::cout << "(";
      display_node(N->left);
      std::cout << " " /*<< N->key << " " << N->value << N->weight*/ << " " << N->weight_sum << " ";
      display_node(N->right);
      std::cout << ")";
    }
  }

public:

  Treap() : root(nullptr) { }

  void insert(const KEY& key, const VALUE& value, long weight) {
    root = node_insert(root, key, value, weight);
  }

  std::pair<KEY,VALUE> delete_min() {
    auto pair = std::pair<KEY,VALUE>();
    root = node_delete_min(root, pair.first, pair.second);
    return pair;
  }

  long total_weight() {
    return get_total_weight(root);
  }

  Treap<KEY,VALUE>* split_at(long w) {
    auto pair = node_split_at(root, w);
    root = pair.first;

    auto other = new Treap<KEY,VALUE>();
    other->root = pair.second;
    return other;
  }

  void display() {
    display_node(root);
    std::cout << std::endl;
  }
};

// **************************************************************************

int main(int argc, char** argv) {
  long n;

  auto init = [&] {
    n = (long)pasl::util::cmdline::parse_or_default_long("n", 24);
  };

  auto run = [&] (bool sequential) {
    auto T = Treap<long,long>();
    for (int i = 0; i < n; i++) {
      long x = rand();
      T.insert(x,x,1);
      //T.display();
    }

    auto M = T.split_at(n/3);
    auto R = M->split_at(n/3);

    std::cout << T.total_weight() << std::endl;
    std::cout << M->total_weight() << std::endl;
    std::cout << R->total_weight() << std::endl;

    // long recent = -1l;
    // for (int i = 0; i < n; i++) {
    //   auto pair = T.delete_min();
    //   assert(recent <= pair.first);
    //   recent = pair.first;
    //   // std::cout << pair.first << " " << pair.second << std::endl;
    //   //T.display();
    // }
  };

  auto output = [&] {
    ;
  };

  auto destroy = [&] {
    ;
  };

  pasl::sched::launch(argc, argv, init, run, output, destroy);
  return 0;
}
