#ifndef __mpitranspose_h__
#define __mpitranspose_h__ 1

#include <iostream>
#include <cstdlib>

#define ALLTOALL 0

void fill1_comm_sched(int *sched, int which_pe, int npes);

// Globally transpose an N x M matrix distributed over the second dimension.
// Here "in" is a local N x m matrix and "out" is a local n x M matrix.
// Currently, both N and M must be divisible by the number of processors.
// work is a temporary array of size N*m.
class transpose {
private:
  unsigned int N,m,n,M;
  bool wait;
  Complex *work;
  MPI_Comm communicator;
  bool allocated;
  int *request;
  int Request;
  int srequest;
  int size;
  int rank;
  int splitsize;
  int splitrank;
  int *sched;
  MPI_Status status;
  MPI_Comm split;
public: //temp  
  unsigned int d;

public:
  transpose(unsigned int N, unsigned int m, unsigned int n,
            unsigned int M, Complex *work=NULL,
            MPI_Comm communicator=MPI_COMM_WORLD) : 
    N(N), m(m), n(n), M(M), work(work), communicator(communicator),
    allocated(false) {
    d=1;
    wait=false;
    MPI_Comm_size(communicator,&size);
    if(size == 1) return;
    MPI_Comm_rank(communicator,&rank);
    
    if(N % size != 0 || M % size != 0) {
      if(rank == 0)
        std::cout << 
          "ERROR: Matrix dimensions must be divisible by number of processors" 
                  << std::endl << std::endl; 
      MPI_Finalize();
      exit(0);
    }

    if(work == NULL) {
      this->work=new Complex[N*m];
      allocated=true;
    }
    
    if(d > (unsigned int) size) d=size;
    if(d == 1) {
      split=communicator;
      splitsize=size;
      splitrank=rank;
    } else {
      unsigned int q=size/d;
      MPI_Comm_split(communicator,rank/q,0,&split);
      MPI_Comm_size(split,&splitsize);
      MPI_Comm_rank(split,&splitrank);
    }
#if !ALLTOALL    
    request=new int[std::max((unsigned int) splitsize-1,d-1)];
    sched=new int[splitsize];
    fill1_comm_sched(sched,splitrank,splitsize);
#else    
    request=new int[d-1];
#endif    
  }
  
  ~transpose() {
    if(size == 1) return;
    if(allocated) delete[] work;
    
#if !ALLTOALL
    delete[] sched;
    delete[] request;
#endif    
  }
  
// Globally transpose data, applying an additional local transposition
// to the input.
  void inTransposed(Complex *data);
  
  void inwait(Complex *data);
  
// Globally transpose data, applying an additional local transposition
// to the output.
  void outTransposed(Complex *data);
  
  void InTransposed(Complex *data) {
    inTransposed(data);
    inwait(data);
  }
  
  void OutTransposed(Complex *data) {
    outTransposed(data);
    outwait(data);
  }
  
  void outwait(Complex *data);
};

#if MPI_VERSION < 3
inline int MPI_Ialltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                         void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                         MPI_Comm comm, MPI_Request *)
{
  return MPI_Alltoall(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,
                      comm);
}
inline void Wait(MPI_Request *)
{ 
}
#else
inline void Wait(MPI_Request *request)
{ 
  MPI_Status status;
  MPI_Wait(request,&status);
}
#endif

#endif
