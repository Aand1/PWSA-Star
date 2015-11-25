#include <sys/time.h>

#ifndef _PWSA_TIMING_H_
#define _PWSA_TIMING_H_

namespace timing {

// takes approximately 5e-5 secs on PBBS
// int contention_free_loop() {
//   int x = 0;
//   for (int i = 1; i < 56; i++) {
//     for (int j = 1; j < 100; j++) {
//       x += i / j;
//       __asm__ __volatile__("");
//     }
//   }
//   return x;
// }

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
