/* COPYRIGHT (c) 2015 Sam Westrick and Laxman Dhulipala
* All rights reserved.
 *
 * \file weighted-graph.hpp
 *
 */

#ifndef _PWSA_WEIGHTED_GRAPH_H_
#define _PWSA_WEIGHTED_GRAPH_H_

#include <fstream>
#include "timing.hpp"
#include "search_utils.hpp"

using namespace std;

class Graph {
public:

  class coords {
    public:
      int row;
      int col;
  };

  class neighbor {
    public:
      int vertex;
      int weight;
  };

  vector<vector<int>> grid; // holds vertex id, or -1 if obstacle
  vector<coords> vertices;
  int height;
  int width;

  vector<int> neighbor_offsets;
  vector<neighbor> neighbors;

  static const int OBSTACLE = -1;

  int drow[8] = {   -1,    -1,    -1,     0,     0,     1,     1,     1};
  int dcol[8] = {   -1,     0,     1,    -1,     1,    -1,     0,     1};
  int dist[8] = {14142, 10000, 14142, 10000, 10000, 14142, 10000, 14142};

  // assume `grid` and `vertices` have already been populated
  void populate_neighbors() {
    int curr_offset = 0;
    neighbor_offsets.resize(vertices.size() + 1);
    for (int v = 0; v < vertices.size(); v++) {
      neighbor_offsets[v] = curr_offset;
      for (int i = 0; i < 8; i++) {
        int nghrow = vertices[v].row + drow[i];
        int nghcol = vertices[v].col + dcol[i];
        if (0 <= nghrow && nghrow < height &&
            0 <= nghcol && nghcol < width &&
            grid[nghrow][nghcol] != OBSTACLE) {
          neighbor u;
          u.vertex = grid[nghrow][nghcol];
          u.weight = dist[i];
          neighbors.push_back(u);
          curr_offset++;
        }
      }
    }
    neighbor_offsets[vertices.size()] = curr_offset; // number of edges
  }

  bool check_neighbors() {
    std::cout << "Checking graph... ";
    for (int v = 0; v < vertices.size(); v++) {
      int deg = 0;
      for (int i = 0; i < 8; i++) {
        int nghrow = vertices[v].row + drow[i];
        int nghcol = vertices[v].col + dcol[i];
        if (0 <= nghrow && nghrow < height &&
            0 <= nghcol && nghcol < width &&
            grid[nghrow][nghcol] != OBSTACLE) {
          if (neighbors[neighbor_offsets[v] + deg].vertex != grid[nghrow][nghcol]) return false;
          if (neighbors[neighbor_offsets[v] + deg].weight != dist[i]) return false;
          deg++;
        }
      }
      if (deg != degree(v)) return false;
    }
    std::cout << "done!" << std::endl;
    return true;
  }

  Graph() {
    height = 0;
    width = 0;
  }

  Graph(const char* fname) {
    FILE* fin = fopen(fname,"r");
    fscanf(fin,"type octile\nheight %d\n",&height);
    fscanf(fin,"width %d\n map\n",&width);
    grid.clear();
    grid.resize(height);
    vertices.clear();
    vertices.resize(width * height);
    int next_id = 0;
    for (int i = 0; i < grid.size(); i++) {
      grid[i].resize(width);
    }
    for (int i = 0; i < grid.size(); i++) {
      for (int j = 0; j < grid[i].size(); j++) {
        char c;
        fscanf(fin,"%c",&c);
        if (c == '\n'){
          j--;
          continue;
        }
        else if (c == '.' || c == 'G') {
          vertices[next_id].row = i;
          vertices[next_id].col = j;
          grid[i][j] = next_id;
          next_id++;
        }
        else if (c == '@' || c == 'O' || c == 'T' || c == 'S' || c == 'W') {
          grid[i][j] = OBSTACLE;
        }
        else {
          printf("Bad map character: ",c);
        }
      }
    }
    vertices.resize(next_id);
    fclose(fin);

    populate_neighbors();
    assert(check_neighbors());
  }

  void pebble_dump(int* pebbles, int* predecessors, int src, int dst, const char* fname) {
    FILE* fout = fopen(fname,"w");
    fprintf(fout, "width %d\nheight %d\n", width, height);
    for (int row = 0; row < height; row++) {
      for (int col = 0; col < width; col++) {
        if (grid[row][col] == OBSTACLE) fprintf(fout, "@");
        else if (grid[row][col] == src) fprintf(fout, "s");
        else if (grid[row][col] == dst) fprintf(fout, "t");
        else if (pebbles[grid[row][col]] == -1) fprintf(fout, ".");
        else fprintf(fout, "%d", pebbles[grid[row][col]]);

        if (col != width - 1) fprintf(fout, " ");
      }
      fprintf(fout, "\n");
    }

    int cur_vtx = dst;
    vector<coords> path;
    while (cur_vtx != src) {
      coords coord = vertices[cur_vtx];
      path.push_back(coord);
      cur_vtx = predecessors[cur_vtx];
    }
    path.push_back(vertices[cur_vtx]);
    std::reverse(path.begin(),path.end());

    for (std::vector<coords>::iterator it=path.begin(); it!=path.end(); ++it) {
      auto r = *it;
      fprintf(fout, "%d %d\n", r.row, r.col);
    }

    fclose(fout);
  }

  void pebble_dump(SearchResult* result, int src, int dst, const char* fname) {
    int* pebbles = new int[vertices.size()];
    int* predecessors = new int[vertices.size()];
    for (int i = 0; i < vertices.size(); i++) {
      pebbles[i] = result->pebble(i);
      predecessors[i] = result->predecessor(i);
    }
    pebble_dump(pebbles, predecessors, src, dst, fname);
    delete [] pebbles;
    delete [] predecessors;
  }

  int number_vertices() {
    return vertices.size();
  }

  std::pair<int,int> coords_of(int v) {
    return std::make_pair(vertices[v].row, vertices[v].col);
  }

  int vertex_at(int row, int col) {
    return grid[row][col];
  }

  int degree(int v) {
    return neighbor_offsets[v+1] - neighbor_offsets[v];
  }

  void simulate_get_successors(double exptime) {
    timing::busy_loop_secs(8.0 * exptime);
  }

  template <class FUNC>
  void for_each_neighbor_in_range(int v, int start, int end, const FUNC& f) {
    assert(0 <= start && start <= end && end <= degree(v));
    for (int i = start; i < end; i++) {
      neighbor u = neighbors[neighbor_offsets[v] + i];
      f(u.vertex, u.weight);
    }
  }

  template <class FUNC>
  void for_each_neighbor_of(int v, const FUNC& f) {
    int degv = degree(v);
    for (int i = 0; i < degv; i++) {
      neighbor u = neighbors[neighbor_offsets[v] + i];
      f(u.vertex, u.weight);
    }
  }

  int weight_between(int u, int v) {
    int u_row = vertices[u].row;
    int u_col = vertices[u].col;

    int v_row = vertices[v].row;
    int v_col = vertices[v].col;

    int diff = abs(u_row - v_row) + abs(u_col - v_col);
    if (diff == 0) return 0;
    else if (diff == 1) return 10000;
    else if (diff == 2) return 14142;
    else {
      std::cerr << "ERROR (weight_between): vertices (" << u << "," << v << ") not adjacent" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  int pathlen(SearchResult* res, int src, int dst) {
    int len = 0;
    for (int curr = dst; curr != src; curr = res->predecessor(curr)) {
      len += weight_between(curr, res->predecessor(curr));
    }
    return len;
  }

  int pathlen(int* predecessors, int src, int dst) {
    int len = 0;
    for (int curr = dst; curr != src; curr = predecessors[curr]) {
      len += weight_between(curr, predecessors[curr]);
    }
    return len;
  }

  int weighted_euclidean(double w, int u, int v) {
    return (int) (10000.0 * w * sqrt(pow(vertices[u].row - vertices[v].row, 2) +
                                     pow(vertices[u].col - vertices[v].col, 2)));
  }
};

#endif
