#include "array-util.hpp"

#ifndef _PAIRING_HEAP_H_
#define _PAIRING_HEAP_H_

template <class KEY, class VALUE>
class Heap {
private:
  struct Node {
    KEY key;
    VALUE value;
//    Node* parent;
    Node* sibling;
    Node* child;
  };

  Node* root;

  Node* merge_node(Node* X, Node* Y) {
    assert(X == nullptr || X->sibling == nullptr);
    assert(Y == nullptr || Y->sibling == nullptr);

    if (X == nullptr) {
      return Y;
    }
    else if (Y == nullptr) {
      return X;
    }
    else if (X->key <= Y->key) {
      Y->sibling = X->child;
      X->child = Y;
      return X;
    }
    else {
      X->sibling = Y->child;
      Y->child = X;
      return Y;
    }
  }

  Node* merge_pairs_node(Node* X) {
    if (X == nullptr || X->sibling == nullptr) {
      return X;
    }
    else {
      Node* Y = X->sibling;
      Node* rest = Y->sibling;
      X->sibling = nullptr;
      Y->sibling = nullptr;
      return merge_node(merge_node(X, Y), merge_pairs_node(rest));
    }
  }

  void display_node(Node* X) {
    if (X == nullptr) {
      std::cout << "-";
    }
    else {
      std::cout << "(" << X->key << " [";
      for (Node* child = X->child; child != nullptr; child = child->sibling) {
        display_node(child);
        if (child->sibling != nullptr) std::cout << ", ";
      }
      std::cout << "])";
    }
  }

public:

  Heap() {
    root = nullptr;
  }

  Heap(const KEY& k, const VALUE& v) {
    root = array_util::my_malloc<Node>(1);
    root->key = k;
    root->value = v;
    root->sibling = nullptr;
    root->child = nullptr;
  }

  void insert(const KEY& k, const VALUE& v) {
    Node* X = array_util::my_malloc<Node>(1);
    X->key = k;
    X->value = v;
    X->sibling = nullptr;
    X->child = nullptr;

    root = merge_node(root, X);
  }

  void merge_with(Heap<KEY,VALUE>& other) {
    root = merge_node(root, other.root);
    other.root = nullptr;
  }

  std::pair<KEY,VALUE> delete_min() {
    assert(root != nullptr);
    Node* oldroot = root;
    root = merge_pairs_node(oldroot->child);

    auto result = std::pair<KEY,VALUE>(oldroot->key, oldroot->value);
    free(oldroot);
    return result;
  }

  void is_empty() {
    return root == nullptr;
  }

  void display() {
    display_node(root);
  }
};

#endif
