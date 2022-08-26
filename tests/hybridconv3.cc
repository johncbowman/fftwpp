#include "convolve.h"

using namespace std;
using namespace utils;
using namespace Array;
using namespace fftwpp;

unsigned int A=2; // number of inputs
unsigned int B=1; // number of outputs
unsigned int L=512; // input data length
unsigned int M=1024; // minimum padded length

int main(int argc, char* argv[])
{
  fftw::maxthreads=get_max_threads();

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

  optionsHybrid(argc,argv);

  unsigned int K0=10000000;
  if(K == 0) K=max(K0/M,20);
  cout << "K=" << K << endl << endl;

  unsigned int Lx=L; // TODO: x
  unsigned int Ly=L;
  unsigned int Lz=L;
  unsigned int Mx=M; // TODO: X
  unsigned int My=M;
  unsigned int Mz=M;

  unsigned int Sy=0; // y-stride (0 means Lz)
  unsigned int Sx=0; // x-stride (0 means Ly*Sy)

  cout << "Lx=" << Lx << endl;
  cout << "Ly=" << Ly << endl;
  cout << "Lz=" << Lz << endl;
  cout << "Mx=" << Mx << endl;
  cout << "My=" << My << endl;
  cout << "Mz=" << Mz << endl;
  cout << endl;

  if(Sy == 0) Sy=Lz;
  if(Sx == 0) Sx=Ly*Sy;

  Complex **f=new Complex *[max(A,B)];
  for(unsigned int a=0; a < A; ++a)
    f[a]=ComplexAlign(Lx*Sx);

  ForwardBackward FBx(A,B);
  fftPad fftx(Lx,Mx,FBx,Sx == Ly*Lz ? Sx : Lz,Sx);
  ForwardBackward FBy(A,B,FBx.Threads(),fftx.l);
  fftPad ffty(Ly,My,FBy,Lz,Sy);
  ForwardBackward FBz(A,B,FBy.Threads(),ffty.l);
  Convolution convolvez(Lz,Mz,FBz);
  Convolution2 convolveyz(&ffty,&convolvez);
  Convolution3 Convolve3(&fftx,&convolveyz);

//  Convolution3 Convolve3(Lx,Mx,Ly,My,Lz,Mz,A,B);

  double T=0;

  for(unsigned int c=0; c < K; ++c) {

    for(unsigned int a=0; a < A; ++a) {
      Complex *fa=f[a];
      for(unsigned int i=0; i < Lx; ++i) {
        for(unsigned int j=0; j < Ly; ++j) {
          for(unsigned int k=0; k < Lz; ++k) {
            fa[Sx*i+Sy*j+k]=Complex((1.0+a)*i+k,j+k+a);
          }
        }
      }
    }

    if(Lx*Ly*Lz < 200 && c == 0) {
      for(unsigned int a=0; a < A; ++a) {
        for(unsigned int i=0; i < Lx; ++i) {
          for(unsigned int j=0; j < Ly; ++j) {
            for(unsigned int k=0; k < Lz; ++k) {
              cout << f[a][Sx*i+Sy*j+k] << " ";
            }
            cout << endl;
          }
          cout << endl;
        }
        cout << endl;
      }
    }

    seconds();
    Convolve3.convolve(f,multbinary);
    T += seconds();
  }

  cout << "mean=" << T/K << endl;

  Complex sum=0.0;
  for(unsigned int b=0; b < B; ++b) {
    Complex *fb=f[b];
    for(unsigned int i=0; i < Lx; ++i) {
      for(unsigned int j=0; j < Ly; ++j) {
        for(unsigned int k=0; k < Lz; ++k)
          sum += fb[Sx*i+Sy*j+k];
      }
    }
  }

  cout << "sum=" << sum << endl;
  cout << endl;

  if(Lx*Ly*Lz < 200) {
    for(unsigned int b=0; b < B; ++b) {
      Complex *fb=f[b];
      for(unsigned int i=0; i < Lx; ++i) {
        for(unsigned int j=0; j < Ly; ++j) {
          for(unsigned int k=0; k < Lz; ++k) {
            cout << fb[Sx*i+Sy*j+k] << " ";
          }
          cout << endl;
        }
        cout << endl;
      }
      cout << endl;
    }
  }
  return 0;
}
