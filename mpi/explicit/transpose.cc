#include <mpi.h>
#include <complex.h>
#include <fftw3-mpi.h>
#include <iostream>
#include "getopt.h"
#include "seconds.h"
#include "timing.h"
#include "exmpiutils.h"
#include "../mpiutils.h"

using namespace std;
using namespace utils;

bool test=false;
bool quiet=false;

unsigned int X=8, Y=8, Z=1;
int a=0; // Test for best block divisor
int alltoall=-1; // Test for best alltoall routine

namespace utils {
unsigned int defaultmpithreads=1;
}

const unsigned int showlimit=1024;
unsigned int N0=1000000;
int N=0;
bool outtranspose=false;

void init(Complex *data, unsigned int X, unsigned int y, unsigned int Z,
          int x0, int y0) {
  for(unsigned int i=0; i < X; ++i) { 
    for(unsigned int j=0; j < y; ++j) {
      for(unsigned int k=0; k < Z; ++k) {
        data[(y*i+j)*Z+k].re=x0+i;
        data[(y*i+j)*Z+k].im=y0+j;
      }
    }
  }
}

inline void usage()
{
  cerr << "Options: " << endl;
  cerr << "-h\t\t help" << endl;
  cerr << "-T<int>\t\t number of threads" << endl;
  cerr << "-S<int>\t\t type of statistics" << endl;
  cerr << "-t\t\t test" << endl;
  cerr << "-N<int>\t\t number of timing tests"
       << endl;
  cerr << "-m<int>\t\t size" << endl;
  cerr << "-x<int>\t\t x size" << endl;
  cerr << "-y<int>\t\t y size" << endl;
  cerr << "-z<int>\t\t z size" << endl;
  
  //usageTranspose();
  cerr << "-L\t\t locally transpose output" << endl;
  exit(1);
}

#include <fftw3-mpi.h>
void fftwTranspose(int rank, int size, unsigned int N, int stats, int direction)
{
  Complex *data;
  ptrdiff_t x,x0;
  ptrdiff_t y,y0;
  
  fftw_mpi_init();
  
  /* get local data size and allocate */
  ptrdiff_t NN[2]={Y,X};
  unsigned int block=ceilquotient(Y,size);
  ptrdiff_t alloc=
    fftw_mpi_local_size_many_transposed(2,NN,Z,block,0,
                                        MPI_COMM_WORLD,&y,
                                        &y0,&x,&x0);
  if(rank == 0) {
    cout << "x=" << x << endl;
    cout << "y=" << y << endl;
    cout << "X=" << X << endl;
    cout << "Y=" << Y << endl;
    cout << "Z=" << Z << endl;
    cout << "N=" << N << endl;
  }
  
  data=ComplexAlign(alloc);
  
  if(rank == 0)
    cout << "\nOLD\n" << endl;
  
  fftw_plan inplan=fftw_mpi_plan_many_transpose(Y,X,2*Z,block,0,
                                                (double*) data,(double*) data,
                                                MPI_COMM_WORLD,
                                                FFTW_MPI_TRANSPOSED_IN);
  fftw_plan outplan=fftw_mpi_plan_many_transpose(X,Y,2*Z,0,block,
                                                 (double*) data,(double*) data,
                                                 MPI_COMM_WORLD,
                                                 outtranspose ? 0 : \
						 FFTW_MPI_TRANSPOSED_OUT);
  
  init(data,X,y,Z,0,y0);

  bool showoutput=X*Y < showlimit && N == 1;
  if(showoutput)
    show(data,X,y*Z,MPI_COMM_WORLD);

  double *Tin=new double[N];
  double *Tout=new double[N];

  for(unsigned int i=0; i < N; ++i) {
    seconds();
    fftw_execute(inplan);
    Tin[i]=seconds();

    seconds();
    fftw_execute(outplan);
    Tin[i]=seconds();
    
  }
  if(rank == 0) {
    if(direction == 0)
      timings("in transpose",X,Tin,N,stats);
  }
  if(rank == 0) {
    if(direction == 1)
      timings("out transpose",X,Tout,N,stats);
  }
  
  if(showoutput) {
    if(outtranspose) {
      if(rank == 0) cout << "\nOutput:\n" << endl;
      show(data,y,X*Z,MPI_COMM_WORLD);
    } else {
      if(rank == 0) cout << "\nOriginal:\n" << endl;
      show(data,X,y*Z,MPI_COMM_WORLD);
    }
  }

  fftw_destroy_plan(inplan);
  fftw_destroy_plan(outplan);
}

int main(int argc, char **argv)
{
  int provided;
  MPI_Init_thread(&argc,&argv,MPI_THREAD_FUNNELED,&provided);

  int stats = 0;

  int direction = 0; // 0: in, 1: out
  
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  if(rank != 0) opterr=0;
#ifdef __GNUC__ 
  optind=0;
#endif  
  for (;;) {
    int c=getopt(argc,argv,"hLN:A:a:Im:n:Os:S:T:x:y:z:qt");
    if (c == -1) break;
                
    switch (c) {
      case 0:
        break;
      case 'N':
        N=atoi(optarg);
        break;
      case 'L':
        outtranspose=true;
        break;
      case 's':
        alltoall=atoi(optarg);
        break;
      case 'a':
        a=atoi(optarg);
        break;
      case 'm':
        X=Y=atoi(optarg);
        break;
      case 'x':
        X=atoi(optarg);
        break;
      case 'y':
        Y=atoi(optarg);
        break;
      case 'z':
        Z=atoi(optarg);
        break;
      case 'S':
        stats=atoi(optarg);
        break;
      case 'T':
        defaultmpithreads=atoi(optarg);
        break;
      case 't':
        test=true;
        break;
      case 'q':
        quiet=true;
        break;
      case 'n':
        N0=atoi(optarg);
        break;
      case 'I':
        direction = 0;
      case 'O':
        direction = 1;
        break;
      case 'h':
      default:
        if(rank == 0)
          usage();
    }
  }

  if(provided < MPI_THREAD_FUNNELED)
    defaultmpithreads=1;

  int size;
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  
  if(rank == 0) {
    cout << "size=" << size << endl;
    cout << "threads=" << defaultmpithreads << endl;
  }
  
  if(test) N=1;
  else if(N == 0) {
    N=N0/(X*Y*Z);
    if(N < 10) N=10;
  }

  int retval=0;

  fftwTranspose(rank,size,N,stats, direction);
  
  MPI_Finalize();
  return retval;
}