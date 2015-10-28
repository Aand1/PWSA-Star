/* COPYRIGHT (c) 2015 Sam Westrick and Laxman Dhulipala
* All rights reserved.
 *
 * \file weighted-graph.hpp
 *
 */

#ifndef _PWSA_WEIGHTED_GRAPH_H_
#define _PWSA_WEIGHTED_GRAPH_H_

#include <fstream>

using namespace std;

class Graph {
public:

  class coords {
    public:
      int row;
      int col;
  };

  vector<vector<int>> grid; // holds vertex id, or -1 if obstacle
  vector<coords> vertices;
  int height;
  int width;

  static const int OBSTACLE = -1;

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

  int drow[8] = {   -1,    -1,    -1,     0,     0,     1,     1,     1};
  int dcol[8] = {   -1,     0,     1,    -1,     1,    -1,     0,     1};
  int dist[8] = {14142, 10000, 14142, 10000, 10000, 14142, 10000, 14142};

  template <class FUNC>
  void for_each_neighbor_of(int v, const FUNC& f) {
    for (int i = 0; i < 8; i++) {
      int nghrow = vertices[v].row + drow[i];
      int nghcol = vertices[v].col + dcol[i];
      if (0 <= nghrow && nghrow < height &&
          0 <= nghcol && nghcol < width &&
          grid[nghrow][nghcol] != OBSTACLE) {
        f(grid[nghrow][nghcol], dist[i]);
      }
    }
  }

  int weighted_euclidean(double w, int u, int v) {
    return (int) (10000.0 * w * sqrt(pow(vertices[u].row - vertices[v].row, 2) +
                                     pow(vertices[u].col - vertices[v].col, 2)));
  }
};

#endif
