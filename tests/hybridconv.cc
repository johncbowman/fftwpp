#include "convolve.h"

#define OUTPUT 1

using namespace std;
using namespace utils;
using namespace Array;
using namespace fftwpp;

unsigned int A=2; // number of inputs
unsigned int B=1; // number of outputs
unsigned int C=1; // number of copies
unsigned int L=512; // input data length
unsigned int M=1024; // minimum padded length

int main(int argc, char* argv[])
{
  fftw::maxthreads=1;//get_max_threads();

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

  optionsHybrid(argc,argv);

  ForwardBackward FB(A,B);
  fftPad fft(L,M,FB,C);

  Complex *f=ComplexAlign(C*L);
  Complex *g=ComplexAlign(C*L);

  for(unsigned int j=0; j < L; ++j) {
    for(unsigned int c=0; c < C; ++c) {
#if OUTPUT
      f[C*j+c]=Complex(j,j+1);
      g[C*j+c]=Complex(j,2*j+1);
#else
      f[C*j+c]=0.0;
      g[C*j+c]=0.0;
#endif
    }
  }

  Convolution Convolve(fft,A,B);

  Complex *F[]={f,g};
//  Complex *h=ComplexAlign(L);
//  Complex *H[]={h};
#if OUTPUT
  unsigned int K=1;
#else
  unsigned int K=100000;
#endif
  double t0=totalseconds();

  for(unsigned int k=0; k < K; ++k)
    Convolve.convolve(F,F,multbinary);

  double t=totalseconds();
  cout << (t-t0)/K << endl;
  cout << endl;
#if OUTPUT
  for(unsigned int j=0; j < L; ++j)
    for(unsigned int c=0; c < C; ++c)
      cout << F[0][C*j+c] << endl;
#endif

  return 0;
}
