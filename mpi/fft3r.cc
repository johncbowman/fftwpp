#include "Array.h"
#include "mpifftw++.h"
#include "utils.h"
#include "mpiutils.h"
#include <unistd.h>
using namespace Array;

using namespace std;
using namespace fftwpp;

// Number of iterations.
unsigned int N0=10000000;
unsigned int N=0;
int divisor=0; // Test for best block divisor
int alltoall=-1; // Test for best alltoall routine

void init(double *f,
          unsigned int X, unsigned int Y, unsigned int Z,
          unsigned int x0, unsigned int y0, unsigned int z0,
          unsigned int x, unsigned int y, unsigned int z)
{
  unsigned int c=0;
  for(unsigned int i=0; i < x; ++i) {
    unsigned int ii=x0+i;
    for(unsigned int j=0; j < y; j++) {
      unsigned int jj=y0+j;
      for(unsigned int k=0; k < Z; k++) {
        unsigned int kk=k;
        f[c++] = ii + jj + kk;
      }
    }
  }
}

void init(double *f, split3 d)
{
  init(f,d.X,d.Y,d.Z,d.x0,d.y0,d.z0,d.x,d.y,d.z);
}

unsigned int outlimit=3000;

int main(int argc, char* argv[])
{
#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif
  int retval=0;

  unsigned int nx=4;
  unsigned int ny=0;
  unsigned int nz=0;

  bool quiet=false;
  bool test=false;
  bool shift=false;

#ifdef __GNUC__ 
  optind=0;
#endif  
  for (;;) {
    int c = getopt(argc,argv,"S:htN:T:a:m:n:s:x:y:z:q");
    if (c == -1) break;
                
    switch (c) {
    case 0:
      break;
    case 'a':
      divisor=atoi(optarg);
      break;
    case 'N':
      N=atoi(optarg);
      break;
    case 'm':
      nx=ny=nz=atoi(optarg);
      break;
    case 's':
      alltoall=atoi(optarg);
      break;
    case 'x':
      nx=atoi(optarg);
      break;
    case 'y':
      ny=atoi(optarg);
      break;
    case 'z':
      nz=atoi(optarg);
      break;
    case 'n':
      N0=atoi(optarg);
      break;
    case 'S':
      shift=atoi(optarg);
      break;
    case 'T':
      fftw::maxthreads=atoi(optarg);
      break;
    case 'q':
      quiet=true;
      break;
    case 't':
      test=true;
      break;
    case 'h':
    default:
      usage(3);
      usageTranspose();
      exit(1);
    }
  }

  int provided;
  MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE,&provided);

  if(ny == 0) ny=nx;
  if(nz == 0) nz=nx;

  if(N == 0) {
    N=N0/nx/ny;
    if(N < 10) N=10;
  }
  
  MPIgroup group(MPI_COMM_WORLD,nz,nx,ny);

  if(group.size > 1 && provided < MPI_THREAD_FUNNELED)
    fftw::maxthreads=1;

  if(!quiet) {
    if(group.rank == 0) {
      cout << "provided: " << provided << endl;
      cout << "fftw::maxthreads: " << fftw::maxthreads << endl;
    }
    
    if(group.rank == 0) {
      cout << "Configuration: " 
           << group.size << " nodes X " << fftw::maxthreads 
           << " threads/node" << endl;
    }
  }
  
  if(group.rank < group.size) {
    int nzp=nz/2+1;
    bool main=group.rank == 0;
    if(!quiet && main) {
      cout << "nx=" << nx << ", ny=" << ny << ", nz=" << nz <<
        ", nzp=" << nzp << endl;
      cout << "size=" << group.size << endl;
    }

    split3 df(nx,ny,nz,group);
    split3 dg(nx,ny,nzp,group);
    
    double *f=FFTWdouble(df.n);
    Complex *g=ComplexAlign(dg.n);
    
    rcfft3dMPI fft(df,dg,f,g,mpiOptions(fftw::maxthreads,divisor,alltoall));

    if(test) {
      init(f,df);

      if(!quiet && nx*ny < outlimit) {
        if(main) cout << "\ninput:" << endl;
        show(f,df.x,df.y,df.Z,group.active);
      }

      size_t align=sizeof(Complex);

      array3<double> flocal(nx,ny,nz,align);
      array3<Complex> glocal(nx,ny,nzp,align);
      array3<double> fgathered(nx,ny,nz,align);
      array3<Complex> ggathered(nx,ny,nzp,align);
      
      rcfft3d localForward(nx,ny,nz,fgathered(),ggathered());
      crfft3d localBackward(nx,ny,nz,ggathered(),fgathered());

      gatherxy(f, fgathered(), df, group.active);

      init(flocal(),df.X,df.Y,df.Z,0,0,0,df.X,df.Y,df.Z);
      if(main) {
        if(!quiet) {
          cout << "Gathered input:\n" <<  fgathered << endl;
          cout << "Local input:\n" <<  flocal << endl;
        }
        retval += checkerror(flocal(),fgathered(),df.X*df.Y*df.Z);
      }
      
      if(shift)
        fft.Forwards0(f,g);
      else
        fft.Forwards(f,g);

      // FIXME: temp
      //if(main) cout << "\ntwiddled input:" << endl;
      //show(f,df.x,df.y,df.Z,group.active);

      
      if(main) {
        if(shift)
          localForward.fft0(flocal,glocal);
        else
          localForward.fft(flocal,glocal);
        cout << endl;
      }
        
      if(!quiet) {
        if(main) cout << "Distributed output:" << endl;
        show(g,dg.X,dg.xy.y,dg.z,group.active);
      }
      gatheryz(g,ggathered(),dg,group.active); 

      if(!quiet && main) {
        cout << "Gathered output:\n" <<  ggathered << endl;
        cout << "Local output:\n" <<  glocal << endl;
      }
      
      if(main) {
        retval += checkerror(glocal(),ggathered(),dg.X*dg.Y*dg.Z);
        cout << endl;
      }

      if(shift)
        fft.Backwards0Normalized(g,f);
      else
        fft.BackwardsNormalized(g,f);

      if(main) {
        if(shift)
          localBackward.fft0Normalized(glocal,flocal);
        else 
          localBackward.fftNormalized(glocal,flocal);
      }
      
      if(!quiet) {
        if(main) cout << "Distributed back to input:" << endl;
        show(f,df.x,df.y,df.Z,group.active);
      }

      gatherxy(f,fgathered(),df,group.active);
      
      if(!quiet && main) {
        cout << "Gathered back to input:\n" <<  fgathered << endl;
        cout << "Local back to input:\n" <<  flocal << endl;
      }
      
      if(main)
        retval += checkerror(flocal(),fgathered(),df.X*df.Y*df.Z);
      
      if(!quiet && group.rank == 0) {
        cout << endl;
        if(retval == 0)
          cout << "pass" << endl;
        else
          cout << "FAIL" << endl;
      }
      
    } else {

      if(main)
        cout << "N=" << N << endl;
      if(N > 0) {
    
        double *T=new double[N];
        for(unsigned int i=0; i < N; ++i) {
          init(f,df);
          seconds();
          fft.Forwards(f,g);
          fft.BackwardsNormalized(g,f);
          T[i]=seconds();
        }
        if(!quiet)
          show(f,df.x,df.y,df.Z,group.active);
        
        if(main) timings("FFT timing:",nx,T,N);
        delete[] T;
      }

    }
  
    deleteAlign(f);
  }
  
  MPI_Finalize();
  
  return retval;
}