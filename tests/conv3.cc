#include <vector>

#include "convolution.h"
#include "explicit.h"
#include "direct.h"
#include "utils.h"
#include "Array.h"

using namespace std;
using namespace utils;
using namespace Array;
using namespace fftwpp;

// Number of iterations.
unsigned int N0=10000000;
unsigned int N=0;
unsigned int nx=0;
unsigned int ny=0;
unsigned int nz=0;
unsigned int mx=4;
unsigned int my=0;
unsigned int mz=0;
unsigned int nxp;
unsigned int nyp;
unsigned int nzp;
bool xcompact=true;
bool ycompact=true;
bool zcompact=true;

unsigned int threads;

bool Explicit=false;

inline void init(Complex **F,
                 unsigned int mx, unsigned int my, unsigned int mz,
                 unsigned int nxp, unsigned int nyp, unsigned int nzp,
                 unsigned int A, bool xcompact, bool ycompact, bool zcompact)
{
  if(A % 2 == 0) {
    unsigned int M=A/2;
    unsigned int xoffset=Explicit ? nxp/2-mx+1 : !xcompact;
    unsigned int yoffset=Explicit ? nyp/2-my+1 : !ycompact;
    unsigned int nx=2*mx-1;
    unsigned int ny=2*my-1;

    double factor=1.0/sqrt((double) M);
    for(unsigned int s=0; s < M; ++s) {
      double S=sqrt(1.0+s);
      double ffactor=S*factor;
      double gfactor=1.0/S*factor;

      array3<Complex> f(nxp,nyp,nzp,F[s]);
      array3<Complex> g(nxp,nyp,nzp,F[M+s]);

      if(!xcompact) {
        for(unsigned int j=0; j < ny+!ycompact; ++j) {
          for(unsigned int k=0; k < mz+!zcompact; ++k) {
            f[0][j][k]=0.0;
            g[0][j][k]=0.0;
          }
        }
      }
      if(!ycompact) {
        for(unsigned int i=0; i < nx+!xcompact; ++i) {
          for(unsigned int k=0; k < mz+!zcompact; ++k) {
            f[i][0][k]=0.0;
            g[i][0][k]=0.0;
          }
        }
      }

      if(!zcompact) {
        for(unsigned int i=0; i < nx+!xcompact; ++i) {
          for(unsigned int j=0; j < ny+!ycompact; ++j) {
            f[i][j][mz]=0.0;
            g[i][j][mz]=0.0;
          }
        }
      }

      PARALLEL(
        for(unsigned int i=0; i < nx; ++i) {
          unsigned int I=i+xoffset;
          for(unsigned int j=0; j < ny; ++j) {
            unsigned int J=j+yoffset;
            for(unsigned int k=0; k < mz; ++k) {
              f[I][J][k]=ffactor*Complex(i+k,j+k);
              g[I][J][k]=gfactor*Complex(2*i+k,j+1+k);
            }
          }
        }
        );
    }
  } else {
    cerr << "Init not implemented for A=" << A << endl;
    exit(1);
  }
}

int main(int argc, char *argv[])
{
  threads=fftw::maxthreads=get_max_threads();

  bool Direct=false;
  bool Implicit=true;
  bool Output=false;
  bool Normalized=true;

  double K=1.0; // Time limit (seconds)
  unsigned int minCount=20;

  unsigned int A=2; // Number of independent inputs
  unsigned int B=1; // Number of outputs

  int stats=0; // Type of statistics used in timing test.

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif

#ifdef __GNUC__
  optind=0;
#endif
  for (;;) {
    int c = getopt(argc,argv,"hdeipA:B:K:Om:x:y:z:n:T:uS:X:Y:Z:");
    if (c == -1) break;

    switch (c) {
      case 0:
        break;
      case 'd':
        Direct=true;
        break;
      case 'e':
        Explicit=true;
        Implicit=false;
        break;
      case 'i':
        Implicit=true;
        Explicit=false;
        break;
      case 'A':
        A=atoi(optarg);
        break;
      case 'B':
        B=atoi(optarg);
        break;
      case 'K':
        K=atof(optarg);
        break;
      case 'O':
        Output=true;
        break;
      case 'm':
        mx=my=mz=atoi(optarg);
        break;
      case 'x':
        mx=atoi(optarg);
        break;
      case 'y':
        my=atoi(optarg);
        break;
      case 'z':
        mz=atoi(optarg);
        break;
      case 'u':
        Normalized=false;
        break;
      case 'T':
        threads=fftw::maxthreads=max(atoi(optarg),1);
        break;
      case 'S':
        stats=atoi(optarg);
        break;
      case 'X':
        xcompact=atoi(optarg) == 0;
        break;
      case 'Y':
        ycompact=atoi(optarg) == 0;
        break;
      case 'Z':
        zcompact=atoi(optarg) == 0;
        break;
      case 'h':
      default:
        usage(3);
        usageExplicit(3);
        usageCompact(3);
        exit(1);
    }
  }

  if(my == 0) my=mx;
  if(mz == 0) mz=mx;

  nx=hpadding(mx);
  ny=hpadding(my);
  nz=hpadding(mz);

  cout << "nx=" << nx << ", ny=" << ny << ", nz=" << ny << endl;
  cout << "mx=" << mx << ", my=" << my << ", mz=" << mz << endl;

  if(K == 0) minCount=1;
  cout << "K=" << K << endl;
  K *= 1.0e9;

  size_t align=ALIGNMENT;
  nxp=Explicit ? nx : 2*mx-xcompact;
  nyp=Explicit ? ny : 2*my-ycompact;
  nzp=Explicit ? nz/2+1 : mz+!zcompact;

  cout << "nxp=" << nxp << ", nyp=" << nyp << ", nzp=" << nzp << endl;

  if(B < 1) B=1;
  if(B > A) {
    cerr << "B=" << B << " is not yet implemented for A=" << A << endl;
    exit(1);
  }

  Complex **F=new Complex *[A];
  for(unsigned int a=0; a < A; ++a)
    F[a]=ComplexAlign(nxp*nyp*nzp);

  // For easy access of first element
  array3<Complex> f(nxp,nyp,nzp,F[0]);

  array3<Complex> h0;
  if(Direct) {
    h0.Allocate(mx,my,mz,align);
    if(!Normalized) {
      cerr << "-u option is incompatible with -d." << endl;
      exit(-1);
    }
  }

  vector<double> T;

  if(Implicit) {
    ImplicitHConvolution3 C(mx,my,mz,xcompact,ycompact,zcompact,A,B);
    cout << "threads=" << C.Threads() << endl << endl;

    realmultiplier *mult;
    switch(A) {
      case 2: mult=multbinary; break;
      case 4: mult=multbinary2; break;
      default: cerr << "A=" << A << " is not yet implemented" << endl; exit(1);
    }

    double sum=0.0;
    while(sum <= K || T.size() < minCount) {
      init(F,mx,my,mz,nxp,nyp,nzp,A,xcompact,ycompact,zcompact);
      double t0=nanoseconds();
      C.convolve(F,mult);
//      C.convolve(f,g);
      double t=nanoseconds()-t0;
      T.push_back(t);
      sum += t;
    }

    timings("Implicit",(2*mx-1)*(2*my-1)*(2*mz-1),T.data(),T.size(),stats);
    T.clear();
    cout << endl;

    if(Normalized) {
      double norm=1.0/(27.0*mx*my*mz);
      for(unsigned int i=!xcompact; i < nxp; ++i) {
        for(unsigned int j=!ycompact; j < nyp; ++j) {
          for(unsigned int k=0; k < mz; k++)
          f[i][j][k] *= norm;
        }
      }
    }

    if(Direct) {
      for(unsigned int i=0; i < mx; i++)
        for(unsigned int j=0; j < my; j++)
          for(unsigned int k=0; k < mz; k++)
            h0[i][j][k]=f[i+!xcompact][j+!ycompact][k];
    }

    if(Output) {
      for(unsigned int i=!xcompact; i < nxp; ++i) {
        for(unsigned int j=!ycompact; j < nyp; ++j) {
          for(unsigned int k=0; k < mz; ++k)
            cout << f[i][j][k] << "\t";
          cout << endl;
        }
        cout << endl;
      }
    }
  }

  if(Explicit) {
    unsigned int M=A/2;
    ExplicitHConvolution3 C(nx,ny,nz,mx,my,mz,f,M);
    cout << "threads=" << C.Threads() << endl << endl;

    array3<Complex> g(nxp,nyp,nzp,F[1]);

    double sum=0.0;
    while(sum <= K || T.size() < minCount) {
      double t0=nanoseconds();
      f=0.0;
      g=0.0;
      double t1=nanoseconds();
      // Conservative estimate accounting for extra zero padding above.
      double Sum=(t1-t0)*19.0/27.0;
      init(F,mx,my,mz,nxp,nyp,nzp,A,true,true,true);
      double t2=nanoseconds();
      C.convolve(F,F+M);
      double t=Sum+nanoseconds()-t2;
      T.push_back(t);
      sum += t;
    }


    timings("Explicit",(2*mx-1)*(2*my-1)*(2*mz-1),T.data(),T.size(),stats);
    cout << endl;

    unsigned int xoffset=nx/2-mx+1;
    unsigned int yoffset=ny/2-my+1;

    if(Direct) {
      for(unsigned int i=0; i < mx; i++)
        for(unsigned int j=0; j < my; j++)
          for(unsigned int k=0; k < mz; ++k)
            h0[i][j][k]=f[xoffset+i][yoffset+j][k];
    }

    if(Output) {
      for(unsigned int i=xoffset; i < xoffset+2*mx-1; i++) {
        for(unsigned int j=yoffset; j < yoffset+2*my-1; j++) {
          for(unsigned int k=0; k < mz; ++k)
            cout << f[i][j][k] << "\t";
          cout << endl;
        }
        cout << endl;
      }
      cout << endl;
    }
  }

  if(Direct) {
    unsigned int nxp=2*mx-1;
    unsigned int nyp=2*my-1;

    array3<Complex> h(nxp,nyp,mz,align);

    DirectHConvolution3 C(mx,my,mz);
    init(F,mx,my,mz,nxp,nyp,mz,A,true,true,true);
    double t0=nanoseconds();
    C.convolve(h,F[0],F[1]);
    T[0]=nanoseconds()-t0;

    timings("Direct",(2*mx-1)*(2*my-1)*(2*mz-1),T.data(),1);
    T.clear();
    cout << endl;

    if(Output)
      for(unsigned int i=0; i < nxp; ++i) {
        for(unsigned int j=0; j < nyp; ++j) {
          for(unsigned int k=0; k < mz; ++k)
            cout << h[i][j][k] << "\t";
          cout << endl;
        }
        cout << endl;
      }

    { // compare implicit or explicit version with direct verion:
      double error=0.0;
      double norm=0.0;
      for(unsigned int i=0; i < mx; i++) {
        for(unsigned int j=0; j < my; j++) {
          for(unsigned int k=0; k < mz; k++) {
            error += abs2(h0[i][j][k]-h[i][j][k]);
            norm += abs2(h[i][j][k]);
          }
        }
      }
      if(norm > 0) error=sqrt(error/norm);
      cout << "error=" << error << endl;
      if (error > 1e-12) cerr << "Caution! error=" << error << endl;
    }

  }

  for(unsigned int a=0; a < A; ++a)
    deleteAlign(F[a]);
  delete [] F;


  return 0;
}
