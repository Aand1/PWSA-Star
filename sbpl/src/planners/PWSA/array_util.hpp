#include "../../../pasl/sched/granularity.hpp"

#ifndef _EXAMPLE_ARRAY_UTIL_
#define _EXAMPLE_ARRAY_UTIL_

namespace par = pasl::sched::granularity;

#if defined(CONTROL_BY_FORCE_SEQUENTIAL)
using controller_type = par::control_by_force_sequential;
#elif defined(CONTROL_BY_FORCE_PARALLEL)
using controller_type = par::control_by_force_parallel;
#else
using controller_type = par::control_by_prediction;
#endif
using loop_controller_type = par::loop_by_eager_binary_splitting<controller_type>;

namespace array_util {

template <class T>
T* my_malloc(long n) {
  T* result = (T*)malloc(n*sizeof(T));
  assert(result != nullptr);
  return result;
}

template <class T>
void display(T* xs, long n) {
  std::cout << "[";
  for (long i = 0; i < n; i++) {
    std::cout << xs[i];
    if (i != n-1) std::cout << " ";
  }
  std::cout << "]";
}

template <class LIFT, class T>
class plus_up_sweep_contr {
public:
  static controller_type contr;
};

template <class LIFT, class T>
controller_type plus_up_sweep_contr<LIFT,T>::contr("plus_up_sweep" + par::string_of_template_arg<LIFT>() + par::string_of_template_arg<T>());

template <class LIFT, class T>
void plus_up_sweep(const LIFT& lift, T* xs, long lo, long hi, long* tree, long tree_idx) {
  long n = hi - lo;

  if (n == 0) return;

  if (n == 1) {
    tree[tree_idx] = lift(lo, xs[lo]);
    return;
  }

  par::cstmt(plus_up_sweep_contr<LIFT,T>::contr, [&] { return n; }, [&] {
    long half = n/2;
    long left_idx = tree_idx + 1;
    long right_idx = tree_idx + 2*half;
    par::fork2( [&] { plus_up_sweep<LIFT,T>(lift, xs, lo     , lo+half, tree, left_idx); }
              , [&] { plus_up_sweep<LIFT,T>(lift, xs, lo+half, hi     , tree, right_idx); }
              );
    tree[tree_idx] = tree[left_idx] + tree[right_idx]; // associative op is +
  });
}

controller_type plus_down_sweep_contr("down_sweep");

void plus_down_sweep(long left_val, long* tree, long tree_idx, long* out, long lo, long hi) {
  long n = hi - lo;

  if (n == 0) return;

  if (n == 1) {
    out[lo+1] = left_val + tree[tree_idx]; // associative op is +
    return;
  }

  par::cstmt(plus_down_sweep_contr, [&] { return n; }, [&] {
    // PARALLEL
    long half = n/2;
    long left_idx = tree_idx + 1;
    long right_idx = tree_idx + 2*half;
    long right_left_val = left_val + tree[left_idx]; // associative op is +
    par::fork2( [&] { plus_down_sweep(left_val      , tree, left_idx , out, lo     , lo+half); }
              , [&] { plus_down_sweep(right_left_val, tree, right_idx, out, lo+half, hi     ); }
              );
  });
}

template <class LIFT, class T>
long* plus_scan(const LIFT& lift, T* xs, long n) {
  long* tree = my_malloc<long>(2*n - 1);
  plus_up_sweep<LIFT,T>(lift, xs, 0, n, tree, 0);

  long* out = my_malloc<long>(n + 1);
  plus_down_sweep(0l, tree, 0, out, 0, n);
  out[0] = 0l;

  free(tree);
  return out;
}

template <class PRED, class T>
class filter_contr {
public:
  static loop_controller_type contr;
};

template <class PRED, class T>
loop_controller_type filter_contr<PRED,T>::contr("filter" + par::string_of_template_arg<PRED>() + par::string_of_template_arg<T>());

template <class PRED, class T>
std::pair<T*, long> filter(const PRED& pred, T* xs, long n) {
  long* offsets = plus_scan<PRED,T>(pred, xs, n);
  long final_len = offsets[n];

  T* out = my_malloc<T>(final_len);
  par::parallel_for(filter_contr<PRED,T>::contr, 0l, n, [&] (long i) {
    if ((offsets[i] + 1) == offsets[i+1])
      out[offsets[i]] = xs[i];
  });

  free(offsets);
  return std::pair<T*, long>(out, final_len);
}

template <class T>
class append_contr {
public:
  static loop_controller_type contr;
};

template <class T>
loop_controller_type append_contr<T>::contr("append" + par::string_of_template_arg<T>());

template <class T>
T* append(T* xs, long n, T* ys, long m) {
  T* out = my_malloc<T>(n + m);
  par::parallel_for(append_contr<T>::contr, 0l, n+m, [&] (long i) {
    if (i < n) out[i] = xs[i];
    else out[i] = ys[i-n];
  });
  return out;
}

template <class T, class COMPARE_FUNC>
T quickselect(const COMPARE_FUNC& cmp, long k, T* xs, long n) {
  //std::cout << "find " << k << "th smallest of "; display(xs, n); std::cout << std::endl;
  assert(0 <= k && k < n);

  if (n == 1) return xs[0];

  T& pivot = xs[rand() % n];

  T result;

  auto L = filter([&] (long i, T& other) { return cmp(other, pivot) < 0; }, xs, n);
  auto G = filter([&] (long i, T& other) { return cmp(other, pivot) > 0; }, xs, n);

  if (k < L.second) {
    result = quickselect<T,COMPARE_FUNC>(cmp, k, L.first, L.second);
  }
  else if (k < (n - G.second)) {
    result = pivot;
  }
  else {
    result = quickselect<T,COMPARE_FUNC>(cmp, k - (n - G.second), G.first, G.second);
  }

  free(L.first);
  free(G.first);
  return result;
}

// template <class A, class B, class C, class D>
// class map_flatten_contr_1 {
// public:
//   static loop_controller_type contr;
// };
//
// template <class A, class B, class C, class D>
// loop_controller_type map_flatten_contr_1<A,B,C,D>::contr("map_flatten_1" + par::string_of_template_arg<A>() + par::string_of_template_arg<B>() + par::string_of_template_arg<C>() + par::string_of_template_arg<D>());
//
// template <class A, class B, class C, class D>
// class map_flatten_contr_2 {
// public:
//   static loop_controller_type contr;
// };
//
// template <class A, class B, class C, class D>
// loop_controller_type map_flatten_contr_2<A,B,C,D>::contr("map_flatten_2" + par::string_of_template_arg<A>() + par::string_of_template_arg<B>() + par::string_of_template_arg<C>() + par::string_of_template_arg<D>());
//
// template <class SIZE_FUNC, class MAP_FUNC, class T1, class T2>
// std::pair<T2*, long> map_flatten(const SIZE_FUNC& size, const MAP_FUNC& f, T1* xs, long n) {
//   long* offsets = plus_scan<SIZE_FUNC,T1>(size, xs, n)
//   long final_len = offsets[n];
//
//   T2* out = my_malloc<T2>(final_len);
//   auto weight = [&] (long lo, long hi) {
//     return offsets[hi] - offsets[lo];
//   }
//   par::parallel_for(map_flatten_contr_1<A,B,C,D>::contr, weight, 0l, n, [&] (long i) {
//     par::parallel_for(map_flatten_contr_2<A,B,C,D>::contr, 0l, size(xs[i]), [&] (long j) {
//       out[offsets[i] + j] = f(j, xs[i]);
//     });
//   });
//   free(offsets);
//   return std::pair<T2*, long>(out, final_len);
// }
//
// template <class T>
// class append_controller {
// public:
//   static loop_controller_type contr;
// };
//
// template <class T>
// loop_controller_type append_controller<T>::contr("append" + par::string_of_template_arg<T>());
//
// template <class T>
// T* append(const T* xs, long n, const T* ys, long m) {
//   T* out = array_util::my_malloc<T>(n + m);
//   par::parallel_for(append_controller, 0l, n + m, [&] (long i) {
//     out[i] = (i < n ? xs[i] : ys[n + i]);
//   });
//   return out;
// }

// **************************************************************************
// **************************************************************************
// **************************************************************************


// **************************************************************************
// **************************************************************************
// **************************************************************************

template <class A, class B, class C, class D>
class reduce_controller_type {
public:
  static controller_type contr;
};
template <class A, class B, class C, class D>
controller_type reduce_controller_type<A,B,C,D>::contr("reduce"+
                                                       par::string_of_template_arg<A>()+
                                                       par::string_of_template_arg<B>()+
                                                       par::string_of_template_arg<C>()+
                                                       par::string_of_template_arg<D>());

template <class ALPHA, class BETA, class BINOP, class LIFT>
BETA reduce(const LIFT& lift, const BINOP& binop, BETA id, const ALPHA* xs, long lo, long hi) {
  using contr_type = reduce_controller_type<ALPHA,BETA,BINOP,LIFT>;

  BETA result = id;

  auto seq = [&] {
    for (long i = lo; i < hi; i++) {
      result = binop(result, lift(xs[i]));
    }
  };

  par::cstmt(contr_type::contr, [&] { return hi-lo; }, [&] {
    if (hi <= lo + 2) {
      seq();
    }
    else {
      long m = lo + (hi - lo)/2;
      BETA v1;
      BETA v2;
      par::fork2([&] {
        v1 = reduce(lift, binop, id, xs, lo, m);
      }, [&] {
        v2 = reduce(lift, binop, id, xs, m, hi);
      });
      result = binop(v1, v2);
    }
  }, seq);

  return result;
}

template <class A, class B>
class all_controller_type {
public:
  static controller_type contr;
};
template <class A, class B>
controller_type all_controller_type<A,B>::contr("all"+
                                                       par::string_of_template_arg<A>()+
                                                       par::string_of_template_arg<B>());

template <class ALPHA, class LIFT>
bool all(const LIFT& lift, ALPHA* xs, long lo, long hi) {
  using contr_type = all_controller_type<ALPHA,LIFT>;

  bool result = true;

  auto seq = [&] {
    for (long i = lo; i < hi; i++) {
      result = result && lift(xs[i]);
    }
  };

  par::cstmt(contr_type::contr, [&] { return hi-lo; }, [&] {
    if (hi <= lo + 2) {
      seq();
    }
    else {
      long m = lo + (hi - lo)/2;
      bool v1;
      bool v2;
      par::fork2([&] {
        v1 = all<ALPHA,LIFT>(lift, xs, lo, m);
      }, [&] {
        v2 = all<ALPHA,LIFT>(lift, xs, m, hi);
      });
      result = v1 && v2;
    }
  }, seq);

  return result;
}

//
// template <class A, class B>
// class reduce_destroy_controller_type {
// public:
//   static controller_type contr;
// };
// template <class A, class B>
// controller_type reduce_destroy_controller_type<A,B>::contr("reduce_destroy"+
//                                                        par::string_of_template_arg<A>()+
//                                                        par::string_of_template_arg<B>());
//
// template <class ALPHA, class BINOP>
// ALPHA* reduce_destroy(const BINOP& binop, ALPHA* xs, long lo, long hi) {
//   using contr_type = reduce_destroy_controller_type<ALPHA,BINOP>;
//
//   ALPHA* result;
//
//   auto seq = [&] {
//     result = &(xs[lo]);
//     for (long i = lo+1; i < hi; i++) {
//       result = binop(result, &(xs[i]));
//     }
//   };
//
//   par::cstmt(contr_type::contr, [&] { return hi-lo; }, [&] {
//     if (hi <= lo + 2) {
//       seq();
//     }
//     else {
//       long m = lo + (hi - lo)/2;
//       ALPHA* v1;
//       ALPHA* v2;
//       par::fork2([&] {
//         v1 = reduce_destroy(binop, xs, lo, m);
//       }, [&] {
//         v2 = reduce_destroy(binop, xs, m, hi);
//       });
//       result = binop(v1, v2);
//     }
//   }, seq);
//
//   return result;
// }
//
// template <class A, class B>
// class apply_to_each_controller_type {
// public:
//   static loop_controller_type contr;
// };
// template <class A, class B>
// loop_controller_type apply_to_each_controller_type<A,B>::contr("reduce"+
//                                                                par::string_of_template_arg<A>()+
//                                                                par::string_of_template_arg<B>());
//
// template <class ALPHA, class FUNC>
// void apply_to_each_with_index(const FUNC& f, const ALPHA* xs, long lo, long hi) {
//   using contr_type = apply_to_each_controller_type<ALPHA,FUNC>;
//
//   par::parallel_for(contr_type::contr, lo, hi, [&] (long i) {
//     f(i, xs[i]);
//   });
// }
//
// template <class ALPHA, class FUNC>
// void apply_to_each(const FUNC& f, const ALPHA* xs, long lo, long hi) {
//   apply_to_each_with_index([&] (long i, const ALPHA& x) { return f(x); }, xs, lo, hi);
// }

} // end namespace array_util

#endif
