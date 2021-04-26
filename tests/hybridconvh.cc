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
unsigned int M=768; // minimum padded length

int main(int argc, char* argv[])
{
  fftw::maxthreads=1;//get_max_threads();

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

  optionsHybrid(argc,argv);

  ForwardBackward FB(A,B);

  fftPadHermitian fft(L,M,FB,C);

  unsigned int H=ceilquotient(L,2);

  Complex *f=ComplexAlign(C*H);
  Complex *g=ComplexAlign(C*H);

#if OUTPUT
  for(unsigned int c=0; c < C; ++c) {
    f[c]=1.0;
    g[c]=2.0;
  }
  for(unsigned int j=1; j < H; ++j) {
    for(unsigned int c=0; c < C; ++c) {
      f[C*j+c]=Complex(j,j+1);
      g[C*j+c]=Complex(j,2*j+1);
    }
  }
#else
  for(unsigned int j=0; j < C*H; ++j) {
    f[j]=0.0;
    g[j]=0.0;
  }
#endif

  ConvolutionHermitian Convolve(fft,A,B);

  Complex *F[]={f,g};
//  Complex *h=ComplexAlign(H);
//  Complex *H[]={h};
#if OUTPUT
  unsigned int K=1;
#else
  unsigned int K=1000000;
#endif
  double t0=totalseconds();

  for(unsigned int k=0; k < K; ++k)
    Convolve.convolve(F,F,realmultbinary);
//    Convolve.convolve(F,F,multadvection2);

  double t=totalseconds();
  cout << (t-t0)/K << endl;
  cout << endl;
#if OUTPUT
  for(unsigned int j=0; j < H; ++j)
    for(unsigned int c=0; c < C; ++c)
      cout << F[0][C*j+c] << endl;
#endif

  return 0;
}
