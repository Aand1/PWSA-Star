/* COPYRIGHT (c) 2015 Sam Westrick and Laxman Dhulipala
 * All rights reserved.
 *
 * \file treap-frontier.hpp
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <queue>

#ifndef _PWSA_BIN_HEAP_H_
#define _PWSA_BIN_HEAP_H_

using namespace std;

#define NONE -1

int largestPowTwo(int x) {
  return (int)pow(2, ceil(log(x) / log(2)));
}

template <class VALUE>
class Heap {

private:

  struct KV {
    public:
      int key;
      VALUE value;
  };

  vector<KV> values;
  int size;

  int parent(int index) {
    return (index - 1)>>1;
  }

  int leftChild(int index) {
    int ch = (index<<1)+1;
    return size <= ch ? NONE : ch;
  }

  int rightChild(int index) {
    int ch = (index<<1)+2;
    return size <= ch ? NONE : ch;
  }

  void swap(int a, int b) {
    KV tmp = values[a];
    values[a].key = values[b].key;
    values[a].value = values[b].value;
    values[b].key = tmp.key;
    values[b].value = tmp.value;
  }

  int shiftDown(int startNode, vector<KV>& ov) {
    std::queue<int> frontier;
    frontier.push(startNode);
    int numPushed = 0;

    while (!frontier.empty()) {
      int index = frontier.front();
      frontier.pop();
      ov[numPushed].key = values[index].key;
      ov[numPushed].value = values[index].value;
      int lc = leftChild(index);
      int rc = rightChild(index);
      if (lc != NONE) {
        frontier.push(lc);
      }
      if (rc != NONE) {
        frontier.push(rc);
      }
      numPushed++;
    }
    return numPushed;
  }

  void heapifyDown() {
    int index = 0;
    while (index != size-1) {
      int lIndex = leftChild(index);
      int rIndex = rightChild(index);
      if (lIndex == NONE) return;
      int childIndex = lIndex;
      if (rIndex == NONE) {
        childIndex = lIndex;
      } else if(values[lIndex].key > values[rIndex].key) {
        childIndex = rIndex;
      }
      if (values[index].key <= values[childIndex].key) {
        break;
      }
      swap(index,childIndex);
      index = childIndex;
    }
  }

  void heapifyUp(int index) {
    while (index) {
      int pIndex = parent(index);
      if (values[index].key < values[pIndex].key) {
        swap(index,pIndex);
        index = pIndex;
      } else {
        break;
      }
    }
  }

public: 

  Heap() : size(0) { }

  void insert(const int& key, const VALUE& value) {
    if (size == values.capacity()) {
//      std::cout << "resizing to " << ((size + 1) * 2) << endl;
      values.resize((size + 1) * 2);
    }
    int newindex = size++;
    values[newindex].key = key;
    values[newindex].value = value;
    heapifyUp(size-1);
  }

  std::pair<int, VALUE> delete_min() {
    std::pair<int, VALUE> res = std::make_pair(values[0].key, values[0].value);
    values[0] = values[--size];
    heapifyDown();
    return res;
  }

  bool isEmpty() {
    return size == 0;
  }

  int total_weight() {
    return size;
  }

  void display() {

  }

  // TODO: we're callously ignoring 'w'. Let' fix later
  void split_at(long w, Heap<VALUE>& other) {
    // have size, will split by giving essentially the left child 
    // plus the root to us, and the right-child to them
    if (size < 2) {
      other.size = 0;
    }
    int halfHeap = largestPowTwo(size);
    halfHeap /= 2; // giving half to each side (upper bound)
    other.values.resize(halfHeap);
    int frontKey = values[0].key;
    VALUE frontValue = values[0].value; // TODO: have to make sure copy constructor works on values
    int otherSize = shiftDown(2, other.values);
    other.size = otherSize;
    size = shiftDown(1, values);
    insert(frontKey, frontValue);
  }

  void swap(Heap<VALUE>& other) {
    std::swap(values, other.values);
    std::swap(size, other.size);
  }

  void print () {
    for (int i = 0; i < size; ++i) {
      cout << values[i].key << " " << values[i].value << endl;
    }
    cout << endl;
  }
};

#endif
