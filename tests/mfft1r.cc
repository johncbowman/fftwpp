#include "../Complex.h"
#include "Array.h"
#include "fftw++.h"
#include "utils.h"

using namespace std;
using namespace Array;
using namespace fftwpp;

// Number of iterations.
unsigned int N0=10000000;

bool Direct=false, Implicit=true, Explicit=false, Pruned=false;

inline void init(array2<double>& f, unsigned int mx, unsigned int my) 
{
  for(unsigned int i=0; i < mx; ++i)
    for(unsigned int j=0; j < my; j++)
      f[i][j] = 10 * i + j;
}
  
unsigned int outlimit=100;

int main(int argc, char* argv[])
{
  fftw::maxthreads=get_max_threads();

  unsigned int N=0;
  unsigned int mx=4;
  unsigned int my=4;

  unsigned int stats=0; // Type of statistics used in timing test.

  bool Nset = false;

#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif  
  
#ifdef __GNUC__	
  optind=0;
#endif	
  for (;;) {
    int c = getopt(argc,argv,"hN:m:x:y:n:T:S:");
    if (c == -1) break;
    switch (c) {
    case 0:
      break;
    case 'N':
      Nset = true;
      N=atoi(optarg);
      break;
    case 'm':
      mx=my=atoi(optarg);
      break;
    case 'x':
      mx=atoi(optarg);
      break;
    case 'y':
      my=atoi(optarg);
      break;
    case 'n':
      N0=atoi(optarg);
      break;
    case 'T':
      fftw::maxthreads=max(atoi(optarg),1);
      break;
    case 'S':
      stats=atoi(optarg);
      break;
    case 'h':
    default:
      fft_usage(2);
      exit(0);
    }
  }

  if(my == 0) my=mx;

  cout << "mx=" << mx << ", my=" << my << endl;
  unsigned int n = my;
  unsigned int np = mx / 2 + 1;
  cout << "np=" << np << endl;
  unsigned int M = mx;

  if(!Nset) {
    N=N0/mx/my;
    if(N < 10) N=10;
  }
  cout << "N=" << N << endl;
  
  size_t align=sizeof(Complex);


  array2<double> f(mx,my,align);
  array2<Complex> g(n,np,align);
  
  mrcfft1d Forward(n, // length of transform
		   M,  // number of transforms
		   1,  // stride
		   my, // dist
		   f,  // input array
		   g); // output array
  mcrfft1d Backward(n, // length of transform
		    M,  // number of transforms
		    1,  // stride
		    np,// dist
		    g,  // input array
		    f); // output array

  cout << "\nInput:" << endl;
  init(f,mx,my);
  if(mx*my < outlimit)
    cout << f << endl;
  else
    cout << f[0][0] << endl;

  cout << "\nOutput:" << endl;
  Forward.fft(f,g);
  if(mx*my < outlimit)
    cout << g << endl;
  else
    cout << g[0][0] << endl;

  cout << "\nBack to input:" << endl;
  Backward.fftNormalized(g,f);
  if(mx*my < outlimit)
    cout << f << endl;
  else
    cout << f[0][0] << endl;


  cout << endl;
  if(N > 0) {
    double *T=new double[N];
    for(unsigned int i=0; i < N; ++i) {
      init(f,mx,my);
      seconds();
      Forward.fft(f,g);
      Backward.fft(g,f);
      T[i]=seconds();
    }
    timings("fft2d, in-place",mx,T,N,stats);
    delete [] T;
  }  

  return 0;
}
