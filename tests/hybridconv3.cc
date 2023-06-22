#include <vector>

#include "convolve.h"
#include "timing.h"
#include "direct.h"

using namespace std;
using namespace utils;
using namespace Array;
using namespace fftwpp;

size_t A=2; // number of inputs
size_t B=1; // number of outputs

int main(int argc, char *argv[])
{
  Lx=Ly=Lz=4;  // input data length
  Mx=My=Mz=8; // minimum padded length

  fftw::maxthreads=parallel::get_max_threads();

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

  optionsHybrid(argc,argv);

  cout << "Lx=" << Lx << endl;
  cout << "Ly=" << Ly << endl;
  cout << "Lz=" << Lz << endl;
  cout << "Mx=" << Mx << endl;
  cout << "My=" << My << endl;
  cout << "Mz=" << Mz << endl;

  if(Output || testError)
    K=0;
  if(K == 0) minCount=1;
  cout << "K=" << K << endl << endl;
  K *= 1.0e9;

  vector<double> T;

  if(Sy == 0) Sy=Lz;
  if(Sx == 0) Sx=Ly*Sy;

  Application appx(A,B,multNone,fftw::maxthreads,mx,Dx,Ix);
  fftPad fftx(Lx,Mx,appx,Sy == Lz ? Ly*Lz : Lz,Sx);
  Application appy(A,B,multNone,appx,my,Dy,Iy);
  fftPad ffty(Ly,My,appy,Lz,Sy);
  Application appz(A,B,multbinary,appy,mz,Dz,Iz);
  fftPad fftz(Lz,Mz,appz);
  Convolution3 Convolve3(&fftx,&ffty,&fftz);

  Complex **f=ComplexAlign(max(A,B),fftx.inputSize());

  for(size_t a=0; a < A; ++a) {
    Complex *fa=f[a];
    for(size_t i=0; i < Lx; ++i) {
      for(size_t j=0; j < Ly; ++j) {
        for(size_t k=0; k < Lz; ++k) {
          fa[Sx*i+Sy*j+k]=Output || testError ?
            Complex((1.0+a)*i+k,j+k+a) : 0.0;
        }
      }
    }
  }

  Complex *h=NULL;
  if(testError) {
    h=ComplexAlign(Lx*Ly*Lz);
    DirectConvolution3 C(Lx,Ly,Lz,Sx,Sy);
    C.convolve(h,f[0],f[1]);
  }

  double sum=0.0;
  while(sum <= K || T.size() < minCount) {
    double t;
    if(normalized || testError) {
      cpuTimer c;
      Convolve3.convolve(f);
      t=c.nanoseconds();
    } else {
      cpuTimer c;
      Convolve3.convolveRaw(f);
      t=c.nanoseconds();
    }
    T.push_back(t);
    sum += t;
  }

  cout << endl;
  timings("Hybrid",Lx*Ly*Lz,T.data(),T.size(),stats);
  cout << endl;

  if(Output) {
    if(testError)
      cout << "Hybrid:" << endl;
    for(size_t b=0; b < B; ++b) {
      Complex *fb=f[b];
      for(size_t i=0; i < Lx; ++i) {
        for(size_t j=0; j < Ly; ++j) {
          for(size_t k=0; k < Lz; ++k) {
            cout << fb[Sx*i+Sy*j+k] << " ";
          }
          cout << endl;
        }
        cout << endl;
      }
      cout << endl;
    }
  }
  if(testError) {
    if(Output) {
      cout << endl;
      cout << "Direct:" << endl;
      for(size_t i=0; i < Lx; ++i) {
        for(size_t j=0; j < Ly; ++j) {
          for(size_t k=0; k < Lz; ++k) {
            cout << h[Lz*(Ly*i+j)+k] << " ";
          }
          cout << endl;
        }
        cout << endl;
      }
      cout << endl;
    }
    double err=0.0;
    double norm=0.0;

    // Assumes B=1
    for(size_t i=0; i < Lx; ++i)
        for(size_t j=0; j < Ly; ++j)
          for(size_t k=0; k < Lz; ++k){
            Complex hijk=h[Lz*(Ly*i+j)+k];
            err += abs2(f[0][Sx*i+Sy*j+k]-hijk);
            norm += abs2(hijk);
          }
    double relError=sqrt(err/norm);
    cout << "Error: "<< relError << endl;
    deleteAlign(h);
  }
  return 0;
}
