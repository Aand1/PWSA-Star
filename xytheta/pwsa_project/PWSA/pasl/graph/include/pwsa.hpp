/* COPYRIGHT (c) 2015 Umut Acar, Arthur Chargueraud, Michael
 * Rainey, Sam Westrick, Laxman Dhulipala
 * All rights reserved.
 *
 * \file pwsa.hpp
 *
 */

#include "../../sched/native.hpp"

#ifndef _PASL_GRAPH_PWSA_H_
#define _PASL_GRAPH_PWSA_H_

template <class T>
T* my_malloc(std::size_t n) {
  return (T*) malloc(n * sizeof(T));
}

/***********************************************************************/

namespace pasl {
namespace graph {

typedef std::size_t nat;
typedef std::size_t vertex;
typedef std::size_t weight;

class Graph {

public:

  nat number_vertices() {
    return N;
  }

  nat out_degree(const vertex& v) {
    return vertex_offsets[v+1] - vertex_offsets[v];
  }

  // Expand a vertex by sequentially applying a function to each of its
  // out-neighbors.
  template <class FUNC>
  void expand(const vertex& v, const FUNC& f) {
    for (nat i = vertex_offsets[v]; i < vertex_offsets[v+1]; i++) {
      f(edges[i].first, edges[i].second);
    }
    return;
  }

private:

  nat N;
  nat M;
  std::pair<vertex,weight>* edges;
  nat* vertex_offsets;

}

void pwsa(Graph G, const vertex& source) {
  std::atomic<bool>* visited = my_malloc<std::atomic<bool>>(G.number_vertices());
}

} // end namespace graph
} // end namespace pasl

/***********************************************************************/

#endif /*! _PASL_GRAPH_PWSA_H_ */

// template <class ALPHA, class BETA>
// class MyMap {
// private:
//   ALPHA* data
//   nat capacity;
//
// public:
//   MyMap(nat cap) {
//     data = my_malloc<ALPHA>(cap);
//     capacity = cap;
//   }
//
//   BETA operator [] (const ALPHA& x) const { return data[x]; }
//   BETA& operator [] (const ALPHA& x) { return data[x]; }
// }
