/* COPYRIGHT (c) 2015 Sam Westrick and Laxman Dhulipala
 * All rights reserved.
 *
 * \file treap-frontier.hpp
 *
 */

#ifndef _PWSA_TREAP_FRONTIER_H_
#define _PWSA_TREAP_FRONTIER_H_

template <class KEY, class VALUE>
class Treap {

private:
  struct Node {
    KEY key;
    VALUE value;
    long weight;
    long priority;

    long total_weight;

    Node* left;
    Node* right;
  };

  Node* root;

  inline long node_total_weight(Node* N) {
    return N == nullptr ? 0l : N->total_weight;
  }

  Node* maybe_rotate_right(Node* N) {
    assert(N != nullptr);
    assert(N->left != nullptr);
    if (N->left->priority > N->priority) {
      if (N->left->key == N->key) {
        N->left->total_weight += N->weight - N->left->weight;
        std::swap(N->left->key, N->key);
        std::swap(N->left->weight, N->weight);
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

  Node* node_insert(Node* N, const KEY& key, const VALUE& value, long weight) {
    if (N == nullptr) {
      Node* newN = new Node;
      newN->key = key;
      newN->value = value;
      newN->weight = weight;
      newN->total_weight = weight;
      newN->priority = rand();

      newN->left = nullptr;
      newN->right = nullptr;
      return newN;
    }

    if (key <= N->key) {
      N->total_weight -= node_total_weight(N->left);
      N->left = node_insert(N->left, key, value, weight);
      N->total_weight += node_total_weight(N->left);
      return maybe_rotate_right(N);
    }
    else {
      N->total_weight -= node_total_weight(N->right);
      N->right = node_insert(N->right, key, value, weight);
      N->total_weight += node_total_weight(N->right);
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

    N->total_weight -= node_total_weight(N->left);
    N->left = node_delete_min(N->left, kresult, vresult);
    N->total_weight += node_total_weight(N->left);
    return N;
  }

  std::pair<Node*,Node*> node_split_at(Node* N, long w) {
    if (N == nullptr) return std::pair<Node*,Node*>(nullptr, nullptr);

    long weightL = node_total_weight(N->left);
    if (w <= weightL) {
      N->total_weight -= weightL;
      auto pair = node_split_at(N->left, w);
      N->left = pair.second;
      N->total_weight += node_total_weight(N->left);
      return std::pair<Node*,Node*>(pair.first, N);
    }
    else if (w <= weightL + N->weight) {
      long leftw = w - weightL;
      long rightw = N->weight - leftw;
      if (rightw == 0) {
        N->total_weight -= node_total_weight(N->right);
        auto pair = std::pair<Node*,Node*>(N, N->right);
        N->right = nullptr;
        return pair;
      }
      else {
        Node* rightN = new Node;
        rightN->key = N->key;
        rightN->value = N->value.split_at(leftw);
        rightN->weight = rightw;
        rightN->priority = N->priority;
        rightN->left = nullptr;
        rightN->right = N->right;
        rightN->total_weight = rightw + node_total_weight(rightN->right);

        N->weight = leftw;
        N->total_weight = leftw + node_total_weight(N->left);
        N->right = nullptr;
        return std::pair<Node*,Node*>(N, rightN);
      }
    }
    else {
      N->total_weight -= node_total_weight(N->right);
      auto pair = node_split_at(N->right, w - weightL - N->weight);
      N->right = pair.first;
      N->total_weight += node_total_weight(N->right);
      return std::pair<Node*,Node*>(N, pair.second);
    }
  }

  void display_node(Node* N) {
    if (N == nullptr) std::cout << "-";
    else {
      std::cout << "(";
      display_node(N->left);
      std::cout << " " << N->key /*<< " " << N->value << N->weight*/ << " " << N->total_weight << " ";
      display_node(N->right);
      std::cout << ")";
    }
  }

  void node_check(Node* N, KEY* left_bound, KEY* right_bound, long* p_bound) {
    if (N == nullptr) return;
    assert(left_bound == nullptr || (*left_bound) < N->key);
    assert(right_bound == nullptr || N->key <= (*right_bound));
    assert(p_bound == nullptr || N->priority <= (*p_bound));
    assert(N->total_weight == node_total_weight(N->left) + N->weight + node_total_weight(N->right));
    node_check(N->left, left_bound, &(N->key), &(N->priority));
    node_check(N->right, &(N->key), right_bound, &(N->priority));
  }

public:

  using self_type = Treap<KEY,VALUE>;

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
    return node_total_weight(root);
  }

  self_type split_at_weight(long w) {
    auto pair = node_split_at(root, w);
    root = pair.first;

    auto other = Treap<KEY,VALUE>();
    other.root = pair.second;
    return other;
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
