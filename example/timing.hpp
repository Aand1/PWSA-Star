#include <sys/time.h>

#ifndef _PWSA_TIMING_H_
#define _PWSA_TIMING_H_

namespace timing {

inline uint64_t now() {
  struct timeval X;
  gettimeofday(&X, NULL);
  return X.tv_sec * ((uint64_t)1000000) + X.tv_usec;
}

void busy_loop_secs(double secs) {
  uint64_t t0 = now();
  while ((now() - t0) < (secs * 1000000.0));
}

}

#endif
