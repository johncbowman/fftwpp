#include "Complex.h"
#include "convolution.h"
#include "Array.h"

// Compile with:
// g++ -I .. -fopenmp examplecconv.cc ../convolution.cc ../fftw++.cc -lfftw3 -lfftw3_omp

using namespace std;
using namespace utils;
using namespace fftwpp;
using namespace parallel;

void init(Complex *f, Complex *g, size_t m)
{
  for(size_t k=0; k < m; k++) {
    f[k]=Complex(k,k+1);
    g[k]=Complex(k,2*k+1);
  }
}

int main(int argc, char* argv[])
{
  fftw::maxthreads=get_max_threads();

  // size of problem
  size_t m=8;

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

  // allocate arrays:
  Complex *f=ComplexAlign(m);
  Complex *g=ComplexAlign(m);

  cout << "1d non-centered complex convolution:" << endl;
  init(f,g,m);
  cout << "\ninput:\nf\tg" << endl;
  for(size_t i=0; i < m; i++)
    cout << f[i] << "\t" << g[i] << endl;

  ImplicitConvolution C(m);

  C.convolve(f,g);

  cout << "\noutput:" << endl;
  for(size_t i=0; i < m; i++) cout << f[i] << endl;

  deleteAlign(g);
  deleteAlign(f);

  return 0;
}
