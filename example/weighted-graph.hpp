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

class state {
  public:
    int obs;
    int x;
    int y;
    int id = -1;
};

class VertexPackage {
public:
  intT vertexId;
  intT low;
  intT high;
  bool mustProcess;
  intT distance;

  VertexPackage() { }

  VertexPackage(const intT& vertexId, const intT& low, const intT& high,
        const bool& mustProcess, const intT& distance)
    : vertexId(vertexId), low(low), high(high), mustProcess(mustProcess),
       distance(distance){ }

  long weight() {
    return std::max(1, high - low);
  }

  void split_at(intT w, VertexPackage &other) {
    other.vertexId = vertexId;
    other.low = low + w;
    other.mustProcess = mustProcess;
    other.distance = distance;
    other.high = high;
    high = low + w;
  }

  void setMustProcess(bool _mustProcess) {
    mustProcess = _mustProcess;
  }

  void display() {
    std::cout << "{vertexId=" << vertexId << ",low=" << low << ",high=" << high << ",mustProcess=" << mustProcess << ",distance=" << distance << "}" << std::endl;
  }
};

struct symmetricVertex {
  intT* neighbors;
  uintT degree;
  std::pair<int, int> gridValue;
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
  void setGridValue(std::pair<int, int> _gridValue) { gridValue = _gridValue; }
  uintT getInDegree() { return degree; }
  uintT getOutDegree() { return degree; }
  std::pair<int, int> getGridValue() { return gridValue; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
  void printOutNeighbors(intT vertexId) {
    std::cout << "vertex " << vertexId << "{";
    for (intT i = 0; i < degree; i++) {
      if (i < degree - 1) {
        std::cout << "(" << getOutNeighbor(i) << "," << getOutWeight(i) << "), ";
      } else{
        std::cout << "(" << getOutNeighbor(i) << "," << getOutWeight(i) << ") }";
      }
    }
  }
};

struct asymmetricVertex {
  intT* inNeighbors, *outNeighbors;
  uintT outDegree;
  uintT inDegree;
  std::pair<int, int> gridValue;
  void del() {free(inNeighbors); free(outNeighbors);}
  asymmetricVertex(intT* iN, intT* oN, uintT id, uintT od)
    : inNeighbors(iN), outNeighbors(oN), inDegree(id), outDegree(od) {}
  intT getInNeighbor(uintT j) { return inNeighbors[2*j]; }
  intT getOutNeighbor(uintT j) { return outNeighbors[2*j]; }
  intT getInWeight(uintT j) { return inNeighbors[2*j+1]; }
  intT getOutWeight(uintT j) { return outNeighbors[2*j+1]; }
  void setInNeighbors(intT* _i) { inNeighbors = _i; }
  void setOutNeighbors(intT* _i) { outNeighbors = _i; }
  void setGridValue(std::pair<int, int> _gridValue) { gridValue = _gridValue; }
  uintT getInDegree() { return inDegree; }
  uintT getOutDegree() { return outDegree; }
  std::pair<int, int> getGridValue() { return gridValue; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
  void printOutNeighbors(intT vertexId) {
    std::cout << "vertex " << vertexId << "{";
    for (intT i = 0; i < outDegree; i++) {
      if (i < outDegree - 1) {
        std::cout << "(" << getOutNeighbor(i) << "," << getOutWeight(i) << "), ";
      } else{
        std::cout << "(" << getOutNeighbor(i) << "," << getOutWeight(i) << ") }";
      }
    }
  }
};

struct gridGraph {
  vector<vector<state> > grid;
  vector<std::pair<int, int> > idToGrid; 
  intT width;
  intT height;
  intT n;
  gridGraph(vector<vector<state> > _grid) : grid(_grid) { }

  intT number_vertices() {
    return n;
  }

  void populateIndices() {
    height = grid.size();
    width = grid[0].size();
    n = 0;
    for_each(grid.begin(), grid.end(), [&] (vector<state>& inner) {
      for_each(inner.begin(), inner.end(), [&] (state& elm) {
        if (elm.obs == 0) {
          // good state
          elm.id = n;
          n++;
          idToGrid.push_back(std::make_pair(elm.x, elm.y));
        }
      });
    });
  }

  std::pair<int, int> getHeuristic(intT vtx) {
    return idToGrid[vtx];
  }

  int findVtxWithCoords(intT x, intT y) {
    return grid[x][y].id;
  }


  bool inbounds(intT x, intT y) {
    return (x >= 0 && x < height) && (y >= 0 && y < width);
  }


  template<class F>
  void iterateNghs(intT x, intT y, F f) {
    for (intT i = -1; i < 2; i++) {
      for (intT j = -1; j < 2; j++) {
        if (inbounds(x + i, y + j) && !(i == 0 && j == 0)) {
          if (abs(i) + abs(j) > 1) {
            f(x+i, y+j, 14142);
          } else {
            f(x+i, y+j, 10000);
          }
        }
      }
    }
  }

  uintT outDegree(const intT& v) {
    intT degree = 0;
    auto f = [&] (intT x, intT y, intT weight) {
      degree += 1;
    };
    // non-memoized, terrible. fix later
    auto pair = idToGrid[v];
    iterateNghs<decltype(f)>(pair.first, pair.second, f);
    return degree;
  }

  VertexPackage make_vertex_package(const intT& vertexId,
                                    const bool& mustProcess,
                                    const intT& distance) {
    return VertexPackage(vertexId, 0, outDegree(vertexId),
                         mustProcess, distance);
  }

  template <class FUNC>
  void apply_to_each_in_range(const VertexPackage& r, const FUNC& f) {
    auto pair = idToGrid[r.vertexId];
    auto innerF = [&] (intT x, intT y, intT weight) {
      auto nghId = grid[x][y].id;
      f(nghId, weight);
    };
    iterateNghs<decltype(innerF)>(pair.first, pair.second, innerF);
  }


  // f expects arguments (u, v, edge weight between u and v)
//  template <class FUNC>
//  void apply_to_each_in_range(const VertexPackage& r, const FUNC& f) {
//    auto pair = idToGrid[r.vertexId];
//    auto innerF = [&] (std::tuple<int, int, int> elm) {
//      auto nghId = grid[std::get<0>(elm)][std::get<1>(elm)].id;
//      f(nghId, std::get<2>(elm));
//    };
//    iterateNghs<decltype(innerF)>(pair.first, pair.second, innerF);
//  }

  void printGraph() {
    for (intT i = 0; i < grid.size(); i++) {
      for (intT j = 0; j < grid[i].size(); j++) {
        if (grid[i][j].obs == 0) {
          std::cout << '.';
        } else {
          std::cout << "@";
        }
      }
      std::cout << std::endl;
    }
  }

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

  graph(vertex* _V, long _n, long _m, intT* _edges, intT* _inEdges)
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

  std::pair<int, int> getHeuristic(intT vtx) {
    return V[vtx].getGridValue();
  }

  int findVtxWithCoords(intT x, intT y) {
    for (intT i = 0; i < n; i++) {
      auto id = V[i].getGridValue();
      if (id.first == x && id.second == y) {
        return i;
      }
    }
    return -1;
  }

  uintT outDegree(const intT& v) {
    return V[v].getOutDegree();
  }

  VertexPackage make_vertex_package(const intT& vertexId,
                                    const bool& mustProcess,
                                    const intT& distance) {
    return VertexPackage(vertexId, 0, V[vertexId].getOutDegree(),
                         mustProcess, distance);
  }

  // f expects arguments (u, v, edge weight between u and v)
  template <class FUNC>
  void apply_to_each_in_range(const VertexPackage& r, const FUNC& f) {
    vertex v = V[r.vertexId];
    for (intT i = r.low; i < r.high; i++) {
      f(v.getOutNeighbor(i), v.getOutWeight(i));
    }
  }

  void printGraph() {
    for (intT i = 0; i < n; i++) {
      V[i].printOutNeighbors(i);
      std::cout << std::endl;
    }
  }

};

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
graph<vertex> readGraphFromFile(char const* fname, 
                                bool isSymmetric,
                                bool hasGrid) {
  pbbs::_seq<char> S = readStringFromFile(fname);
  words W = stringToWords(S.A, S.n);
  if (W.Strings[0] != (string) "WeightedAdjacencyGraph") {
    cout << "Bad input file" << endl;
    abort();
  }

  long len = W.m - 1;
  long n = atol(W.Strings[1]);
  long m = atol(W.Strings[2]);
  if ((!hasGrid && (len < n + 2*m + 2)) || (hasGrid && (len != 3*n + 2*m + 2))) {
    std::cout << "Bad input file" << endl;
    std::cout << (n + 2*m + 2) << " " << len << std::endl;
    std::cout << (3*n + 2*m + 2) << " " << len << std::endl;
    abort();
  }

  uintT* offsets = newA(uintT,n);
  intT* edges = newA(intT,2*m);

  pbbs::native::parallel_for(long(0), n, [&] (long i) {
    offsets[i] = atol(W.Strings[i + 3]);
  });

  vertex* v = newA(vertex,n);

  pbbs::native::parallel_for(long(0), m, [&] (long i) {
    edges[2*i] = atol(W.Strings[i+n+3]);
    edges[2*i+1] = atol(W.Strings[i+n+m+3]);
  });

  if (hasGrid) {
    for (long i = 0; i < n; i++) {
      v[i].setGridValue(std::make_pair(atol(W.Strings[2*i+n+m+m+3]), 
                                       atol(W.Strings[2*i+n+m+m+4])));
    }
  }

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

vector<vector<state> > readMap(const char* fname) {
  int size_x;
  int size_y;

  vector<vector<state> > grid;

  FILE* fin = fopen(fname,"r");
  fscanf(fin,"type octile\nheight %d\n",&size_x);
  fscanf(fin,"width %d\n map\n",&size_y);
  grid.clear();
  grid.resize(size_x);
  for(int i=0; i<grid.size(); i++) {
    grid[i].resize(size_y);
  }
  for(int i=0; i<grid.size(); i++){
    for(int j=0; j<grid[i].size(); j++){
      char c;
      fscanf(fin,"%c",&c);
      if(c=='\n'){
        j--;
        continue;
      }
      else if(c == '.' || c=='G')
        grid[i][j].obs = 0;
      else if(c == '@' || c=='O' || c=='T' || c=='S' || c=='W')
        grid[i][j].obs = 1;
      else {
        printf("Bad map character: ",c);
      }
      grid[i][j].x = i;
      grid[i][j].y = j;
    }
  }
  return grid;
}

//
//template<class lam>
//void processElm(const vector<vector<state > >& grid, 
//                const std::pair<int, int>& elm, 
//                lam f) {
//  intT height = grid.size();
//  intT width = grid[0].size();
//
//  int x = elm.first;
//  int y = elm.second;
//
//  if (x > 0) {
//    if (y > 0) {
//      if (grid[x - 1][y - 1].obs == 0) {
//        f(std::make_pair(x-1, y-1));
//      }
//    }
//    if (grid[x - 1][y].obs == 0) {
//      f(std::make_pair(x-1, y));
//    }
//    if (y < width - 1) {
//      if (grid[x - 1][y + 1].obs == 0) {
//        f(std::make_pair(x-1, y+1));
//      }
//    } 
//  }
//
//  if (y > 0) {
//    if (grid[x][y - 1].obs == 0) {
//      f(std::make_pair(x, y-1));
//    }
//  }
//  if (y < width - 1) {
//    if (grid[x][y+1].obs == 0) {
//      f(std::make_pair(x, y+1));
//    }
//  } 
//
//  if (x < height - 1)  {
//    if (y > 0) {
//      if(grid[x + 1][y - 1].obs == 0) {
//        f(std::make_pair(x+1, y-1));
//      }
//    }
//    if (grid[x + 1][y].obs == 0) {
//      f(std::make_pair(x+1, y));
//    }
//    if (y < width - 1) {
//      if (grid[x + 1][y + 1].obs == 0) {
//        f(std::make_pair(x+1, y+1));
//      }
//    }
//  }
//}
//
//
//graph<symmetricVertex> mapToGraph(
//    vector<vector<state > >& grid,
//    vector<std::pair<int, int> >& idToGrid) {
//  // first assign [0, n) to the valid vertices
//  long n = 0;
//  map<std::pair<int, int>, int> gridToId;
//
//  intT height = grid.size();
//  intT width = grid[0].size();
//
//  for_each(grid.begin(), grid.end(), [&] (vector<state>& inner) {
//    for_each(inner.begin(), inner.end(), [&] (state& elm) {
//      if (elm.obs == 0) {
//        // good state - assign id 
//        idToGrid.push_back(std::make_pair(elm.x, elm.y));
//        gridToId.insert(std::pair<std::pair<int, int>, int>(
//            std::make_pair(elm.x, elm.y), n));
//        n++;
//      }
//    });
//  });
//
//  uintT* offsets = newA(uintT,n);
//  symmetricVertex* v = newA(symmetricVertex,n);
//  long m = 0;
//
//  // iterate over n, check neighbors, add edges in <n>
//  for (int i = 0; i < idToGrid.size(); i++) {
//    auto gridId = idToGrid[i];
//    int x = gridId.first;
//    int y = gridId.second;
//    // check all 8 poss for degree
//    int degree = 0;
//
//    auto f = [&](std::pair<int, int> elm) {
//      degree += 1;
//    };
//
//    processElm<decltype(f)>(grid, gridId, f);
//
//    offsets[i+1] = degree;
//    m += degree;
//  }
//  // m is really 2m
//  m /= 2;
//
//  offsets[0] = 0;
//  for (int i = 1; i < n + 1; i++) {
//    offsets[i] += offsets[i-1];
//  }
//
//  intT* edges = newA(intT,2*m);
//
//  for (int i = 0; i < idToGrid.size(); i++) {
//    auto gridId = idToGrid[i];
//    int x = gridId.first;
//    int y = gridId.second;
//    int edgeOffset = offsets[i];
//
//    auto f = [&](std::pair<int, int> elm) {
//      auto res = gridToId.at(elm);
//      edges[2*edgeOffset] = res;
//      if (elm.first == x || elm.second == y) {
//        edges[2*edgeOffset + 1] = 10000;
//      } else {
//        edges[2*edgeOffset + 1] = 14142;
//      }
//      edgeOffset++;
//    };
//    processElm<decltype(f)>(grid, gridId, f);
//  }
//
//  pbbs::native::parallel_for(long(0), n, [&] (long i) {
//    uintT o = offsets[i];
//    uintT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
//    v[i].setOutDegree(l);
//    v[i].setOutNeighbors(edges+2*o);
//  });
//
//  free(offsets);
//  return graph<symmetricVertex>(v,n,m,edges);
//}
//
#endif
