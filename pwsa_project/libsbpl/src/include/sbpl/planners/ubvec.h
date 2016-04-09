#ifndef __UBVEC_H_
#define __UBVEC_H_

#define MIN_F 2047
#define NUM_INNER_VECS 32
#define MIN_VEC_SIZE 2048
#define LOG_MIN_VEC_SIZE 11

template<class T>
class ubvec {
  public:

    ubvec<T>() {
      init = new std::atomic<int>[NUM_INNER_VECS];
      vecs = new T*[NUM_INNER_VECS];
      for (int i = 0; i < NUM_INNER_VECS; i++) {
        init[i].store(-1);
        vecs[i] = 0;
      }
    }

    ~ubvec() {
      for (int i = 0; i < NUM_INNER_VECS; i++) {
        if (vecs[i] != 0) {
          delete[] vecs[i];
        }
      }
      delete[] init;
    }

    inline int log_2(int x) {
      return (int)log2(x);
    }

    inline int ceil_log2(unsigned long long x)
    {
      static const unsigned long long t[6] = {
        0xFFFFFFFF00000000ull,
        0x00000000FFFF0000ull,
        0x000000000000FF00ull,
        0x00000000000000F0ull,
        0x000000000000000Cull,
        0x0000000000000002ull
      };

      int y = (((x & (x - 1)) == 0) ? 0 : 1);
      int j = 32;
      int i;

      for (i = 0; i < 6; i++) {
        int k = (((x & t[i]) == 0) ? 0 : j);
        y += k;
        x >>= k;
        j >>= 1;
      }

      return y;
    }

    // seems to be the fastest variant 
    inline int uint64_log2(uint64_t n)
    {
      #define S(k) if (n >= (UINT64_C(1) << k)) { i += k; n >>= k; }
      int i = -(n == 0); S(32); S(16); S(8); S(4); S(2); S(1); return i;
      #undef S
    }

    inline int speclog(int i) {
      if (i == 0) return 0;
      return std::max(uint64_log2(i) - LOG_MIN_VEC_SIZE + 1, 0);
    }

    // memoize
    int get_index(int logI, int i) {
      int nb = (1 << logI) - 1;
      // sub MIN_VEC_SIZE-1 to rescale as we exclude MIN_VEC_SIZE-1 vals
      int pos = i;
      if (i >= MIN_VEC_SIZE) {
        pos -= nb;
        pos -= (MIN_VEC_SIZE - 1);
      }
      return pos;
    }

    void createVec(int logI, int i) {
      while (true) {
        T* val = vecs[logI];
        if (val == 0) {
          while (init[logI].load() == 0) {
            // hammer
          }
        }
        if (val != 0) {
          break;
        }
        int expected = -1;
        if (init[logI].compare_exchange_weak(expected, 0)) {
          T* nv;
          int vecSize = 1 << (logI + LOG_MIN_VEC_SIZE);
          nv = new T[vecSize];
          // is there a race here?
          vecs[logI] = nv;
          init[logI].store(1);
        }
      }
    }

    T* getState(int i) {
      int logI = speclog(i);
      if (vecs[logI] == 0) {
        createVec(logI, i);
      }
      int ind = get_index(logI, i);
      return &(vecs[logI][ind]);
    }

  private:
    std::atomic<int>* init;
    T ** vecs;
};

#endif
