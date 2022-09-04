#include "convolve.h"

using namespace std;
using namespace utils;
using namespace Array;
using namespace fftwpp;

unsigned int A=2; // number of inputs
unsigned int B=1; // number of outputs
unsigned int L=7; // input data length
unsigned int M=12; // minimum padded length

int main(int argc, char* argv[])
{
  fftw::maxthreads=get_max_threads();

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

  optionsHybrid(argc,argv);

  unsigned int Lx=L;
  unsigned int Ly=L;
  unsigned int Lz=L;
  unsigned int Mx=M;
  unsigned int My=M;
  unsigned int Mz=M;

  unsigned int K0=10000000;
  if(K == 0) K=max(K0/(Mx*My*Mz),20);
  if(Output || testError)
    K=1;
  cout << "K=" << K << endl << endl;

  unsigned int Sy=0; // y-stride (0 means ceilquotient(L,2))
  unsigned int Sx=0; // x-stride (0 means Ly*Sy)

  cout << "Lx=" << Lx << endl;
  cout << "Ly=" << Ly << endl;
  cout << "Lz=" << Lz << endl;
  cout << "Mx=" << Mx << endl;
  cout << "My=" << My << endl;
  cout << "Mz=" << Mz << endl;
  cout << endl;

  unsigned int Hx=ceilquotient(Lx,2);
  unsigned int Hy=ceilquotient(Ly,2);
  unsigned int Hz=ceilquotient(Lz,2);

  if(Sy == 0) Sy=Hz;
  if(Sx == 0) Sx=Ly*Sy;

  Complex **f=new Complex *[max(A,B)];
  for(unsigned int a=0; a < A; ++a)
    f[a]=ComplexAlign(Lx*Sx);

  array2<Complex> f0(Lx,Sx,f[0]);
  array2<Complex> f1(Lx,Sx,f[1]);

  Application appx(A,B,realmultbinary);
  fftPadCentered fftx(Lx,Mx,appx,Sx == Ly*Hz ? Sx : Hz,Sx);
  Application appy(A,B,realmultbinary,appx.Threads(),fftx.l);
  fftPadCentered ffty(Ly,My,appy,Hz,Sy);
  Application appz(A,B,realmultbinary,appy.Threads(),ffty.l);
  ConvolutionHermitian convolvez(Lz,Mz,appz);
  ConvolutionHermitian2 convolveyz(&ffty,&convolvez);
  ConvolutionHermitian3 Convolve3(&fftx,&convolveyz);

//  ConvolutionHermitian3 Convolve3(Lx,Mx,Ly,My,Lz,Mz,A,B);

  double T=0;

  for(unsigned int c=0; c < K; ++c) {

    for(unsigned int i=0; i < Lx; ++i) {
      for(unsigned int j=0; j < Ly; ++j) {
        for(unsigned int k=0; k < Hz; ++k) {
          int I=Lx % 2 ? i : -1+i;
          int J=Ly % 2 ? j : -1+j;
          f[0][Sx*i+Sy*j+k]=Complex(I+(int) k,J+k);
          f[1][Sx*i+Sy*j+k]=Complex(2*I+(int) k,(J+1+k));
        }
      }
    }

    HermitianSymmetrizeXY(Hx,Hy,Hz,Lx/2,Ly/2,f0,Sx,Sy);
    HermitianSymmetrizeXY(Hx,Hy,Hz,Lx/2,Ly/2,f1,Sx,Sy);

    if(Lx*Ly*Hz < 200 && c == 0) {
      for(unsigned int a=0; a < A; ++a) {
        for(unsigned int i=0; i < Lx; ++i) {
          for(unsigned int j=0; j < Ly; ++j) {
            for(unsigned int k=0; k < Hz; ++k)
              cout << f[a][Sx*i+Sy*j+k] << " ";
            cout << endl;
          }
          cout << endl;
        }
        cout << endl;
      }
    }

    seconds();
    Convolve3.convolve(f);
    T += seconds();
  }

  cout << "median=" << T/K << endl;

  Complex sum=0.0;
  for(unsigned int b=0; b < B; ++b) {
    Complex *fb=f[b];
    for(unsigned int i=0; i < Lx; ++i) {
      for(unsigned int j=0; j < Ly; ++j) {
        for(unsigned int k=0; k < Hz; ++k)
          sum += fb[Sx*i+Sy*j+k];
      }
    }
  }

  cout << "sum=" << sum << endl;
  cout << endl;

  if(Output) {
    for(unsigned int b=0; b < B; ++b) {
      Complex *fb=f[b];
      for(unsigned int i=0; i < Lx; ++i) {
        for(unsigned int j=0; j < Ly; ++j) {
          for(unsigned int k=0; k < Hz; ++k)
            cout << fb[Sx*i+Sy*j+k] << " ";
          cout << endl;
        }
        cout << endl;
      }
      cout << endl;
    }
  }
  return 0;
}
