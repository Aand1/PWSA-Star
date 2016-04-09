#ifndef _PWSA_TIMING_H_
#define _PWSA_TIMING_H_

#include <sys/time.h>

namespace timing {

inline uint64_t now() {
  struct timeval X;
  gettimeofday(&X, NULL);
  return X.tv_sec * ((uint64_t)1000000) + X.tv_usec;
}


}

#endif
