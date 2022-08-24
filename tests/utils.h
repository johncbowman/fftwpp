#ifndef __fftwpputils_h__
#define __fftwpputils_h__ 1

#include <iostream>
#include "seconds.h"
#include "Complex.h"

extern unsigned int K; // number of iterations
extern unsigned int C; // number of padded FFTs to compute
extern unsigned int S; // stride between padded FFTs
extern int stats;      // type of statistics used in timing test

#ifdef _WIN32
#include "getopt.h"
inline double cbrt(double x)
{
  if(x == 0.0) return 0.0;
  static double third=1.0/3.0;
  return x > 0.0 ? exp(third*log(x)) : -exp(third*log(-x));
}
#else
#include <getopt.h>
#endif

namespace utils {

template<class T, class S>
inline T max(const T a, const S b)
{
  return a > (T) b ? a : b;
}

template<class T>
inline T pow(T x, unsigned int y)
{
  if(y == 0) return 1;
  if(x == 0) return 0;

  unsigned int r = 1;
  while(true) {
    if(y & 1) r *= x;
    if((y >>= 1) == 0) return r;
    x *= x;
  }
}

extern void optionsHybrid(int argc, char* argv[], bool fft=false);

inline void usageCommon(int n)
{
  std::cerr << "Options: " << std::endl;
  std::cerr << "-h\t\t help" << std::endl;
  std::cerr << "-T\t\t number of threads" << std::endl;
  std::cerr << "-N\t\t number of iterations" << std::endl;
  std::cerr << "-m\t\t size" << std::endl;
  std::cerr << "-S<int>\t\t stats used in timing test: "
            << "0=mean, 1=min, 2=max, 3=median, "
            << "4=90th percentile, 5=80th percentile, 6=50th percentile"
            << std::endl;
  if(n > 1) {
    std::cerr << "-x\t\t x size" << std::endl;
    std::cerr << "-y\t\t y size" << std::endl;
  }
  if(n > 2)
    std::cerr << "-z\t\t z size" << std::endl;
}

inline void usageDirect()
{
  std::cerr << "-i\t\t implicitly padded convolution" << std::endl;
  std::cerr << "-d\t\t direct convolution (slow)" << std::endl;
}

inline void usage(int n)
{
  usageCommon(n);
  std::cerr << "-A\t\t number of data blocks in input" << std::endl;
  std::cerr << "-B\t\t number of data blocks in output" << std::endl;
}

inline void usageInplace(int n)
{
  usageCommon(n);
  std::cerr << "-i\t\t 0=out-of-place, 1=in-place" << std::endl;
}

inline void usageTest()
{
  std::cerr << "-t\t\t accuracy test" << std::endl;
}

inline void usageExplicit(unsigned int n)
{
  usageDirect();
  std::cerr << "-e\t\t explicitly padded convolution" << std::endl;
  if(n > 1)
    std::cerr << "-p\t\t pruned explicitly padded convolution" << std::endl;
}

inline void usageCompact(unsigned int n)
{
  std::cerr << "-X\t\t x Hermitian padding (0 or 1)" << std::endl;
  if(n > 1)
    std::cerr << "-Y\t\t y Hermitian padding (0 or 1)" << std::endl;
  if(n > 2)
    std::cerr << "-Z\t\t z Hermitian padding (0 or 1)" << std::endl;
}

inline void usageb()
{
  std::cerr << "-b\t\t which output block to check" << std::endl;
}

inline void usageTranspose()
{
  std::cerr << "-a<int>\t\t block divisor: -1=sqrt(size), [0]=Tune"
            << std::endl;
  std::cerr << "-s<int>\t\t alltoall: [-1]=Tune, 0=Optimized, 1=MPI, 2=compact"
            << std::endl;
  std::cerr << "-q\t\t quiet" << std::endl;
}

inline void usageShift()
{
  std::cerr << "-O<int>\t\t [0]=Standard, 1=Shift origin"
            << std::endl;
}

inline void usageFFT(int n)
{
  usageCommon(n);
  std::cerr << "-r\t\t type of run:\n"
            << "\t\t r=-1: all runs\n"
            << "\t\t r=0: in-place\n"
            << "\t\t r=1: out-of-place\n";
  if(n > 1)
    std::cerr << "\t\t r=2: transpose, in-place\n"
              << "\t\t r=3: transpose, out-of-place\n"
              << "\t\t r=4: full transpose, in-place\n"
              << "\t\t r=5: full transpose, out-of-place\n"
              << "\t\t r=6: strided, in-place\n"
              << "\t\t r=7: strided, out-of-place\n";
}

inline void usageGather()
{
  std::cerr << "Options: " << std::endl;
  std::cerr << "-h\t\t help" << std::endl;
  std::cerr << "-m\t\t size" << std::endl;
  std::cerr << "-x\t\t x size" << std::endl;
  std::cerr << "-y\t\t y size" << std::endl;
  std::cerr << "-z\t\t z size" << std::endl;
  std::cerr << "-q\t\t quiet" << std::endl;
}

inline void usageHybrid(bool fft=false)
{
  std::cerr << "Options: " << std::endl;
  std::cerr << "-h\t\t help" << std::endl;
  std::cerr << "-m\t\t subtransform size" << std::endl;
  if(fft)
    std::cerr << "-C\t\t number of padded FFTs to compute"
              << std::endl;
  std::cerr << "-D\t\t number of blocks to process at a time" << std::endl;
  std::cerr << "-I\t\t use in-place FFTs [by default only for multiple FFTs]" << std::endl;
  std::cerr << "-K\t\t number of iterations" << std::endl;
  std::cerr << "-L\t\t number of physical data values" << std::endl;
  std::cerr << "-M\t\t minimal number of padded data values" << std::endl;
  if(fft)
    std::cerr << "-S\t\t stride between padded FFTs (defaults to C)" << std::endl;
  else
    std::cerr << "-S\t\t statistics (defaults to MEDIAN)" << std::endl;
  std::cerr << "-T\t\t number of threads" << std::endl;
}

// logM returns the logarithm of base M.
inline double log3(double n) {return log(n)/log(3.0);}

inline double log5(double n) {return log(n)/log(5.0);}

inline double log7(double n) {return log(n)/log(7.0);}

// ceilpow2(n) returns the smallest power of 2 greater than or equal to n.
inline unsigned int ceilpow2(unsigned int n)
{
  --n;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  return ++n;
}

// Return the smallest power of p greater than or equal to n.
inline unsigned int ceilpow(unsigned int p, unsigned int n)
{
  unsigned int x=p;
  unsigned int u=1;
  unsigned int l=0;
  while(n > x) {
    x *= x;
    l=u;
    u *= 2;
  }
  if(n == x) return n;

  while (l < u) {
    unsigned int i=(l+u) >> 1;
    if(n > pow(p,i))
      l=i+1;
    else
      u=i;
  }
  return pow(p,u);
}

inline unsigned int padding(unsigned int n)
{
  std::cout << "min padded buffer=" << n << std::endl;
  // Choose next power of 2 for maximal efficiency.
  return ceilpow2(n);
}

inline unsigned int cpadding(unsigned int m)
{
  return padding(2*m);
}

inline unsigned int hpadding(unsigned int m)
{
  return padding(3*m-2);
}

inline unsigned int tpadding(unsigned int m)
{
  return padding(4*m-3);
}

inline int hash(Complex* f, unsigned int m)
{
  int h=0;
  unsigned int i;
  for(i=0; i<m; ++i) {
    h= (h+ (324723947+(int)(f[i].im+0.5)))^93485734985;
    h= (h+ (324723947+(int)(f[i].im+0.5)))^93485734985;
  }
  return h;
}

}

#endif
