#include <iostream>

#include "wAStar.hpp"

#include "../../../pasl/sched/benchmark.hpp"

using namespace std;

uint64_t GetTimeStamp3() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}


