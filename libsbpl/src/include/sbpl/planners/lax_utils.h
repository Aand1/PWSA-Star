// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of 
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef LAXUTIL_H
#define LAXUTIL_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
using namespace std;

namespace lax_utils {
  // Needed to make frequent large allocations efficient with standard
  // malloc implementation.  Otherwise they are allocated directly from
  // vm.
#include <malloc.h>
  static int __ii =  mallopt(M_MMAP_MAX,0);
  static int __jj =  mallopt(M_TRIM_THRESHOLD,-1);

#define newA(__E,__n) (__E*) malloc((__n)*sizeof(__E))

  template <class E>
  struct identityF { E operator() (const E& x) {return x;}};

  template <class E>
  struct addF { E operator() (const E& a, const E& b) const {return a+b;}};

  template <class E>
  struct minF { E operator() (const E& a, const E& b) const {return (a < b) ? a : b;}};

  template <class E>
  struct maxF { E operator() (const E& a, const E& b) const {return (a>b) ? a : b;}};

  template <class T>
  struct _seq {
    T* A;
    long n;
    _seq() {A = NULL; n=0;}
  _seq(T* _A, long _n) : A(_A), n(_n) {}
    void del() {free(A);}
  };

  namespace sequence {
    template <class intT>
    struct boolGetA {
      bool* A;
      boolGetA(bool* AA) : A(AA) {}
      intT operator() (intT i) {return (intT) A[i];}
    };

    template <class ET, class intT>
    struct getA {
      ET* A;
      getA(ET* AA) : A(AA) {}
      ET operator() (intT i) {return A[i];}
    };

    template <class IT, class OT, class intT, class F>
    struct getAF {
      IT* A;
      F f;
      getAF(IT* AA, F ff) : A(AA), f(ff) {}
      OT operator () (intT i) {return f(A[i]);}
    };

#define _F_BSIZE (2*_SCAN_BSIZE)

  template <class ET>
  inline bool CAS(ET *ptr, ET oldv, ET newv) {
    if (sizeof(ET) == 4) {
      return __sync_bool_compare_and_swap((int*)ptr, *((int*)&oldv), *((int*)&newv));
    } else if (sizeof(ET) == 8) {
      return __sync_bool_compare_and_swap((long*)ptr, *((long*)&oldv), *((long*)&newv));
    } 
    else {
      std::cout << "CAS bad length" << std::endl;
      abort();
    }
  }

  template <class ET>
  inline bool writeMin(ET *a, ET b) {
    ET c; bool r=0;
    do c = *a; 
    while (c > b && !(r=CAS(a,c,b)));
    return r;
  }

  template <class ET>
  inline bool writeMax(ET *a, ET b) {
    ET c; bool r=0;
    do c = *a; 
    while (c < b && !(r=CAS(a,c,b)));
    return r;
  }

  template <class ET>
  inline void writeAdd(ET *a, ET b) {
    volatile ET newV, oldV; 
    do {oldV = *a; newV = oldV + b;}
    while (!CAS(a, oldV, newV));
  }

  inline uint hash(uint a) {
     a = (a+0x7ed55d16) + (a<<12);
     a = (a^0xc761c23c) ^ (a>>19);
     a = (a+0x165667b1) + (a<<5);
     a = (a+0xd3a2646c) ^ (a<<9);
     a = (a+0xfd7046c5) + (a<<3);
     a = (a^0xb55a4f09) ^ (a>>16);
     return a;
  }

  inline ulong hash(ulong a) {
     a = (a+0x7ed55d166bef7a1d) + (a<<12);
     a = (a^0xc761c23c510fa2dd) ^ (a>>9);
     a = (a+0x165667b183a9c0e1) + (a<<59);
     a = (a+0xd3a2646cab3487e3) ^ (a<<49);
     a = (a+0xfd7046c5ef9ab54c) + (a<<3);
     a = (a^0xb55a4f090dd4a67b) ^ (a>>32);
     return a;
  }
}
}

#endif
