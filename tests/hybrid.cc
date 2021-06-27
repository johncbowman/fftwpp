#include "convolve.h"

#define OUTPUT 0
#define CENTERED 1

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

  cout << "Explicit:" << endl;
  // Minimal explicit padding
#if CENTERED
  fftPadCentered fft0(L,M,FB,C,true,true);
#else
  fftPad fft0(L,M,FB,C,true,true);
#endif

  double mean0=fft0.report(FB);

  // Optimal explicit padding
#if CENTERED
  fftPadCentered fft1(L,M,FB,C,true,false);
#else
  fftPad fft1(L,M,FB,C,true,false);
#endif
  double mean1=min(mean0,fft1.report(FB));

  // Hybrid padding
#if CENTERED
  fftPadCentered fft(L,M,FB,C);
#else
  fftPad fft(L,M,FB,C);
#endif

  double mean=fft.report(FB);

  if(mean0 > 0)
    cout << "minimal ratio=" << mean/mean0 << endl;
  cout << endl;

  if(mean1 > 0)
    cout << "optimal ratio=" << mean/mean1 << endl;

  Complex *f=ComplexAlign(fft.ninputs());
  Complex *h=ComplexAlign(fft.ninputs());
  Complex *F=ComplexAlign(fft.noutputs());
  Complex *W0=ComplexAlign(fft.workSizeW());

  unsigned int Length=L;

  for(unsigned int j=0; j < Length; ++j)
    for(unsigned int c=0; c < C; ++c)
      f[C*j+c]=Complex(j+1+c,j+2+c);

#if CENTERED
  fftPadCentered fft2(L,fft.M,C,fft.M,1,1,fft.q == 1);
#else
  fftPad fft2(L,fft.M,C,fft.M,1,1);
#endif

  Complex *F2=ComplexAlign(fft2.noutputs());

  fft2.forward(f,F2);

  fft.pad(W0);
  double error=0.0, error2=0.0;
  double norm=0.0, norm2=0.0;
  for(unsigned int r=0; r < fft.R; r += fft.increment(r)) {
    fft.forward(f,F,r,W0);
    for(unsigned int k=0; k < fft.noutputs(r); ++k) {
      unsigned int i=fft.index(r,k);
      error += abs2(F[k]-F2[i]);
      norm += abs2(F2[i]);
#if OUTPUT
      if(k%fft.Cm == 0) cout << endl;
      cout << fft.index(r,k) << ": " << F[k] << endl;
#endif
    }
    fft.backward(F,h,r,W0);
  }

#if OUTPUT
  cout << endl;
  for(unsigned int j=0; j < fft2.noutputs(); ++j)
    cout << j << ": " << F2[j] << endl;
#endif

  double scale=1.0/fft.normalization();

  if(L < 30) {
    cout << endl;
    cout << "Inverse:" << endl;
    for(unsigned int j=0; j < fft.ninputs(); ++j)
      cout << h[j]*scale << endl;
    cout << endl;
  }

  for(unsigned int r=0; r < fft.R; r += fft.increment(r)) {
    for(unsigned int k=0; k < fft.noutputs(r); ++k)
      F[k]=F2[fft.index(r,k)];
    fft.backward(F,h,r,W0);
  }

  for(unsigned int j=0; j < fft.ninputs(); ++j) {
    error2 += abs2(h[j]*scale-f[j]);
    norm2 += abs2(f[j]);
  }

  if(norm > 0) error=sqrt(error/norm);
  if(norm2 > 0) error2=sqrt(error2/norm2);

  double eps=1e-12;
  if(error > eps || error2 > eps)
    cerr << endl << "WARNING: " << endl;
  cout << "forward error=" << error << endl;
  cout << "backward error=" << error2 << endl;

  return 0;
}
