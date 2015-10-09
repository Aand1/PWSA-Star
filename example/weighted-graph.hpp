/* COPYRIGHT (c) 2015 Sam Westrick and Laxman Dhulipala
 * All rights reserved.
 *
 * \file weighted-graph.hpp
 *
 */

#ifndef _PWSA_WEIGHTED_GRAPH_H_
#define _PWSA_WEIGHTED_GRAPH_H_

#include <fstream>
#include "sequence.hpp"
#include "quicksort.hpp"
#include "benchmark.hpp"
#include "defaults.hpp"
#include "utils.hpp"

using namespace std;

typedef pair<uintT, pair<uintT,intT> > intTriple;
typedef pair<uintT,uintT> intPair;

template <class E>
struct pairFirstCmp {
  bool operator() (pair<uintT,E> a, pair<uintT,E> b) {
    return a.first < b.first; 
  }
};

class Range {
public:
  intT vertexId;
  intT low;
  intT high;
  bool mustProcess;
  intT distance;

  Range() { }

  Range(const intT& source, const intT& low, const intT& high,
        const bool& mustProcess, const intT& distance)
    : source(source), low(low), high(high), mustProcess(mustProcess),
       distance(distance){ }

  Range split_at(intT w) {
    high = low + w;
    return Range(source, low + w, high, mustProcess, distance);
  }

  void setMustProcess(bool _mustProcess) {
    mustProcess = _mustProcess;
  }
};

struct symmetricVertex {
  intT* neighbors;
  uintT degree;
  void del() { free(neighbors); }
  symmetricVertex(intT* n, uintT d) : neighbors(n), degree(d) {}
  //weights are stored in the entry after the neighbor ID
  //so size of neighbor list is twice the degree
  intT getInNeighbor(intT j) { return neighbors[2*j]; }
  intT getOutNeighbor(intT j) { return neighbors[2*j]; }
  intT getInWeight(intT j) { return neighbors[2*j+1]; }
  intT getOutWeight(intT j) { return neighbors[2*j+1]; }
  void setInNeighbors(intT* _i) { neighbors = _i; }
  void setOutNeighbors(intT* _i) { neighbors = _i; }
  uintT getInDegree() { return degree; }
  uintT getOutDegree() { return degree; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
};

struct asymmetricVertex {
  intT* inNeighbors, *outNeighbors;
  uintT outDegree;
  uintT inDegree;
  void del() {free(inNeighbors); free(outNeighbors);}
  asymmetricVertex(intT* iN, intT* oN, uintT id, uintT od)
    : inNeighbors(iN), outNeighbors(oN), inDegree(id), outDegree(od) {}
  intT getInNeighbor(uintT j) { return inNeighbors[2*j]; }
  intT getOutNeighbor(uintT j) { return outNeighbors[2*j]; }
  intT getInWeight(uintT j) { return inNeighbors[2*j+1]; }
  intT getOutWeight(uintT j) { return outNeighbors[2*j+1]; }
  void setInNeighbors(intT* _i) { inNeighbors = _i; }
  void setOutNeighbors(intT* _i) { outNeighbors = _i; }
  uintT getInDegree() { return inDegree; }
  uintT getOutDegree() { return outDegree; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
};

template <class vertex>
struct graph {
  vertex *V;
  intT n;
  intT m; 
  intT* edges;
  intT* inEdges;
  graph(vertex* _V, long _n, long _m, intT* _edges)
  : V(_V), n(_n), m(_m), edges(_edges) { }

  graph(vertex* VV, long nn, long mm, intT* _edges, intT* _inEdges)
  : V(_V), n(_n), m(_m), edges(_edges), inEdges(_inEdges) { }

  void del() {
    for (long i=0; i < n; i++) V[i].del();
    free(V);
    free(edges);
    if(inEdges != NULL) free(inEdges);
  }

  intT number_vertices() {
    return n;
  }

  uintT outDegree(const intT& v) {
    return V[v].outDegree;
  }

  // f expects arguments (u, v, edge weight between u and v)
  template <class FUNC, class vertex>
  void apply_to_each_in_range(const Range& r, const FUNC& f) {
    vertex v = V[r.vertexId];
    for (intT i = r.low; i < r.high; i++) {
      f(v.getOutNeighbor(i), v.getOutWeight(i));
    }
  }

};

// TODO(lax): move this all out into graph-utils

// A structure that keeps a sequence of strings all allocated from
// the same block of memory
struct words {
  long n; // total number of characters
  char* Chars;  // array storing all strings
  long m; // number of substrings
  char** Strings; // pointers to strings (all should be null terminated)
  words() {}
words(char* C, long nn, char** S, long mm)
: Chars(C), n(nn), Strings(S), m(mm) {}
  void del() {free(Chars); free(Strings);}
};

inline bool isSpace(char c) {
  switch (c)  {
  case '\r':
  case '\t':
  case '\n':
  case 0:
  case ' ' : return true;
  default : return false;
  }
}

pbbs::_seq<char> readStringFromFile(char const *fileName) {
  ifstream file (fileName, ios::in | ios::binary | ios::ate);
  if (!file.is_open()) {
    std::cout << "Unable to open file: " << fileName << std::endl;
    abort();
  }
  long end = file.tellg();
  file.seekg (0, ios::beg);
  long n = end - file.tellg();
  char* bytes = newA(char,n+1);
  file.read (bytes,n);
  file.close();
  return pbbs::_seq<char>(bytes,n);
}

// parallel code for converting a string to words
words stringToWords(char *Str, long n) {
  pbbs::native::parallel_for(long(0), n, [&] (long i){
      if (isSpace(Str[i])) Str[i] = 0; 
  });

  // mark start of words
  bool *FL = newA(bool,n);
  FL[0] = Str[0];
  pbbs::native::parallel_for (long(1), n, [&] (long i) {
    FL[i] = Str[i] && !Str[i-1];
  });

  // offset for each start of word
  pbbs::_seq<long> Off = pbbs::sequence::packIndex<long>(FL, n);
  long m = Off.n;
  long *offsets = Off.A;

  // pointer to each start of word
  char **SA = newA(char*, m);
  pbbs::native::parallel_for (long(0), m, [&] (long j) {
    SA[j] = Str+offsets[j];
  });

  free(offsets); free(FL);
  return words(Str,n,SA,m);
}

template <class vertex>
graph<vertex> readGraphFromFile(char const* fname, bool isSymmetric) {
  pbbs::_seq<char> S = readStringFromFile(fname);
  words W = stringToWords(S.A, S.n);
  if (W.Strings[0] != (string) "WeightedAdjacencyGraph") {
    cout << "Bad input file" << endl;
    abort();
  }

  long len = W.m -1;
  long n = atol(W.Strings[1]);
  long m = atol(W.Strings[2]);
  if (len != n + 2*m + 2) {
    cout << "Bad input file" << endl;
    abort();
  }

  uintT* offsets = newA(uintT,n);
  intT* edges = newA(intT,2*m);

  pbbs::native::parallel_for(long(0), n, [&] (long i) {
    offsets[i] = atol(W.Strings[i + 3]);
  });

  pbbs::native::parallel_for(long(0), m, [&] (long i) {
    edges[2*i] = atol(W.Strings[i+n+3]);
    edges[2*i+1] = atol(W.Strings[i+n+m+3]);
  });

  vertex* v = newA(vertex,n);

  pbbs::native::parallel_for(long(0), n, [&] (long i) {
    uintT o = offsets[i];
    uintT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    v[i].setOutDegree(l);
    v[i].setOutNeighbors(edges+2*o);
  });

  if(!isSymmetric) {
    uintT* tOffsets = newA(uintT,n);
    pbbs::native::parallel_for(long(0), n, [&] (long i) {
      tOffsets[i] = INT_T_MAX;
    });
    intT* inEdges = newA(intT,2*m);
    intTriple* temp = newA(intTriple,m);
    pbbs::native::parallel_for(long(0), n, [&] (long i) {
      uintT o = offsets[i];
      for(uintT j=0;j<v[i].getOutDegree();j++){
  temp[o+j] = make_pair(v[i].getOutNeighbor(j),make_pair(i,v[i].getOutWeight(j)));
      }
    });
    free(offsets);

    pbbs::quickSort(temp, m, pairFirstCmp<intPair>());

    tOffsets[temp[0].first] = 0;
    inEdges[0] = temp[0].second.first;
    inEdges[1] = temp[0].second.second;
    pbbs::native::parallel_for(long(1), m, [&] (long i) {
      inEdges[2*i] = temp[i].second.first;
      inEdges[2*i+1] = temp[i].second.second;
      if(temp[i].first != temp[i-1].first) {
  tOffsets[temp[i].first] = i;
      }
    });

    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    pbbs::sequence::scanIBack(tOffsets,tOffsets,n,pbbs::utils::minF<uintT>(),(uintT)m);

    pbbs::native::parallel_for(long(0), n, [&] (long i) {
      uintT o = tOffsets[i];
      uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
      v[i].setInDegree(l);
      v[i].setInNeighbors(inEdges+2*o);
    });

    free(tOffsets);
    return graph<vertex>(v,n,m,edges,inEdges);
  }
  else {
    free(offsets);
    return graph<vertex>(v,n,m,edges);
  }
}


#endif
