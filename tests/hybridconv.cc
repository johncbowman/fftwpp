#include "convolve.h"
#include "timing.h"
#include "direct.h"

using namespace std;
using namespace utils;
using namespace Array;
using namespace fftwpp;

unsigned int A=2; // number of inputs
unsigned int B=1; // number of outputs

int main(int argc, char* argv[])
{
  L=8;  // input data length
  M=16; // minimum padded length

  fftw::maxthreads=get_max_threads();
#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

  optionsHybrid(argc,argv);

  cout << "L=" << L << endl;
  cout << "M=" << M << endl;

  unsigned int K0=400000000;
  if(K == 0) K=max(K0/M,20);
  if(Output || testError)
    K=1;
  cout << "K=" << K << endl << endl;

  double *T=new double[K];
  unsigned int N=max(A,B);

  Application app(A,B,multbinary,fftw::maxthreads,0,mx,Dx,Ix);
  fftBase *fft=Centered ? new fftPadCentered(L,M,app) : new fftPad(L,M,app);
  bool embed=fft->embed();
  unsigned int size=embed ? fft->outputSize() : fft->inputSize();
  Complex **f=ComplexAlign(N,size);
  Convolution Convolve(fft,A,B,embed ? f : NULL);

  for(unsigned int a=0; a < A; ++a) {
    Complex *fa=f[a];
    for(unsigned int j=0; j < L; ++j)
      fa[j]=Output || testError ? Complex(j,(1.0+a)*j+1) : 0.0;
  }

  Complex *h=NULL;
  if(testError) {
    h=ComplexAlign(L);
    DirectConvolution C(L);
    if(Centered)
      C.Cconvolve(h,f[0],f[1]);
    else
      C.convolve(h,f[0],f[1]);
  }

  if(normalized || testError) {
    for(unsigned int k=0; k < K; ++k) {
      double t0=nanoseconds();
      Convolve.convolve(f);
      T[k]=nanoseconds()-t0;
    }
  } else {
    for(unsigned int k=0; k < K; ++k) {
      double t0=nanoseconds();
      Convolve.convolveRaw(f);
      T[k]=nanoseconds()-t0;
    }
  }

  cout << endl;
  timings("Hybrid",L,T,K,stats);
  cout << endl;

  if(Output) {
    if(testError)
      cout << "Hybrid:" << endl;
    for(unsigned int b=0; b < B; ++b)
      for(unsigned int j=0; j < L; ++j)
        cout << f[b][j] << endl;
  }

  if(testError) {
    if(Output) {
      cout << endl;
      cout << "Direct:" << endl;
      for(unsigned int j=0; j < L; ++j)
        cout << h[j] << endl;
      cout << endl;
    }
    double err=0.0;
    double norm=0.0;

    // Assumes B=1
    Complex* f0=f[0];
    for(unsigned int j=0; j < L; ++j) {
      Complex hj=h[j];
      err += abs2(f0[j]-hj);
      norm += abs2(hj);
    }
    double relError=sqrt(err/norm);
    cout << "Error: "<< relError << endl;
    deleteAlign(h);
  }

  return 0;
}
