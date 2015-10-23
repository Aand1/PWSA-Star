#include <iostream>
#include <vector>

#include "treap-frontier.hpp"

using namespace std;

struct sec {

  int vertexId;
  int low;
  int high;
  bool mustProcess;
  int distance;

  int weight() {
    return 1;
  } 

  void split_at(int w, sec &other) {
  }

  sec() { }

};

int main(int argc, char** argv) {
  int n = 10000000;
  sec val;
  Treap<int, sec> tmp;
  cout << "doing work" << endl;
  for (int i = 0; i < n; i++) {
    int ins = rand() % 100;
    tmp.insert(ins, val); 
    if (i % 100000 == 0) {
      cout << "i = " << i << endl;
    }
  }

  std::pair<int, sec> res;
  int properMin = -1;
  while (!tmp.isEmpty()) {
    res = tmp.delete_min();
    if (res.first < properMin) {
      cout << "you're fucked!" << std::endl;
    }
    properMin = res.first;
  }
  return 0;
}
