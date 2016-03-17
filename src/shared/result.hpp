#ifndef _PWSA_RESULT_H_
#define _PWSA_RESULT_H_

class SearchResult {
public:
  virtual ~SearchResult() { }
  virtual bool is_expanded(int vertex) { return false; }
  virtual int predecessor(int vertex) { return -1; }
  virtual int g(int vertex) { return -1; }
  virtual int pebble(int vertex) { return -1; }
};

#endif // _PWSA_RESULT_H_
