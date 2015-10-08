/* COPYRIGHT (c) 2015 Sam Westrick and Laxman Dhulipala
 * All rights reserved.
 *
 * \file weighted-graph.hpp
 *
 */

#ifndef _PWSA_WEIGHTED_GRAPH_H_
#define _PWSA_WEIGHTED_GRAPH_H_

template <class T, class NUMBER>
class Range {
public:
  T source;
  NUMBER low;
  NUMBER high;

  Range() { }

  Range(const T& source, const NUMBER& low, const NUMBER& high)
    : source(source), low(low), high(high) { }

  Range<T,NUMBER> split_at(NUMBER w) {
    high = low + w;
    return Range<T,NUMBER>(source, low + w, high);
  }
};

typedef std::size_t nat;
typedef std::size_t vertex;
typedef std::size_t weight;

class WeightedGraph {

private:

  const nat num_vertices;
  const nat num_edges;

  const std::pair<vertex,weight>* neighbors;
  const nat* offsets; // has num_vertices+1 entries, where the neighbors of v
                      // are stored between
                      // neighbors[offsets[v]] and neighbors[offsets[v+1]]

public:

  nat number_vertices() {
    return num_vertices;
  }

  nat degree(const vertex& v) {
    return offsets[v+1] - offsets[v];
  }

  Range<vertex,nat> out_neighbors(const vertex& v) {
    return Range<vertex,nat>(v, offsets[v], offsets[v+1]);
  }

  // f expects arguments (u, v, edge weight between u and v)
  template <class FUNC>
  void apply_to_each_in_range(const Range<vertex,nat>& r, const FUNC& f) {
    for (nat i = r.low; i < r.high; i++) {
      f(r.source, neighbors[i].first, neighbors[i].second);
    }
  }

};



#endif
