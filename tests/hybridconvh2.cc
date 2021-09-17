#include "convolve.h"

using namespace std;
using namespace utils;
using namespace Array;
using namespace fftwpp;

unsigned int A=2; // number of inputs
unsigned int B=1; // number of outputs
unsigned int C=1; // number of copies
unsigned int S=0; // stride between copies (0 means ceilquotient(L,2))
unsigned int L=512; // input data length
unsigned int M=768; // minimum padded length

int main(int argc, char* argv[])
{
  fftw::maxthreads=get_max_threads();

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

  optionsHybrid(argc,argv);

  unsigned int Lx=L;
  unsigned int Ly=L;
  unsigned int Mx=M;
  unsigned int My=M;
  unsigned int Sy=S;

  cout << "Lx=" << Lx << endl;
  cout << "Mx=" << Mx << endl;
  cout << endl;

  unsigned int Hx=ceilquotient(Lx,2);
  unsigned int Hy=ceilquotient(Ly,2);

  if(Sy == 0) Sy=Hy;

  Complex **f=new Complex *[A];

  for(unsigned int a=0; a < A; ++a)
    f[a]=ComplexAlign(Lx*Sy);

  array2<Complex> f0(Lx,Sy,f[0]);
  array2<Complex> f1(Lx,Sy,f[1]);

  array2<Complex> h0(Lx,Sy,f[0]);

  ForwardBackward FB(A,B);
  fftPadCentered fftx(Lx,Mx,FB,Sy);
  ConvolutionHermitian convolvey(Ly,My,FB);
  Convolution2 Convolve2(&fftx,&convolvey);

  unsigned int K=500;
  double T=0;

  for(unsigned int k=0; k < K; ++k) {

    for(unsigned int i=0; i < Lx; ++i) {
      for(unsigned int j=0; j < Hy; ++j) {
        int I=Lx % 2 ? i : -1+i;
        f0[i][j]=Complex(I,j);
        f1[i][j]=Complex(2*I,(j+1));
      }
    }

    HermitianSymmetrizeX(Hx,Sy,Lx/2,f0);
    HermitianSymmetrizeX(Hx,Sy,Lx/2,f1);

    if(Lx*Hy < 200 && k == 0) {
      for(unsigned int i=0; i < Lx; ++i) {
        for(unsigned int j=0; j < Hy; ++j) {
          cout << f0[i][j] << " ";
        }
        cout << endl;
      }
      cout << endl;
    }

    seconds();
    Convolve2.convolve(f,realmultbinary);
//    Convolve2.convolve(f,multadvection2);
    T += seconds();
  }

  cout << "mean=" << T/K << endl;

  Complex sum=0.0;
  for(unsigned int i=0; i < Lx; ++i) {
    for(unsigned int j=0; j < Hy; ++j) {
      sum += h0[i][j];
    }
  }

  cout << "sum=" << sum << endl;
  cout << endl;

  if(Lx*Hy < 200) {
    for(unsigned int i=0; i < Lx; ++i) {
      for(unsigned int j=0; j < Hy; ++j) {
        cout << h0[i][j] << " ";
      }
      cout << endl;
    }
  }
  return 0;
}
