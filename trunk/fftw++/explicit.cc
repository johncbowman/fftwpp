#include "Complex.h"
#include "convolution.h"
#include "explicit.h"

namespace fftwpp {

#ifndef FFTWPP_SINGLE_THREAD
#define PARALLEL(code)                                  \
  if(threads > 1) {                                     \
    _Pragma("omp parallel for num_threads(threads)")    \
      code                                              \
      } else {                                          \
    code                                                \
      }
#else
#define PARALLEL(code)                          \
  {                                             \
    code                                        \
      }
#endif

void DirectConvolution::convolve(Complex *h, Complex *f, Complex *g)
{
  for(unsigned int i=0; i < m; ++i) {
    Complex sum=0.0;
    for(unsigned int j=0; j <= i; ++j) sum += f[j]*g[i-j];
    h[i]=sum;
  }
}       

void DirectHConvolution::convolve(Complex *h, Complex *f, Complex *g)
{
  for(unsigned int i=0; i < m; ++i) {
    Complex sum=0.0;
    for(unsigned int j=0; j <= i; ++j) sum += f[j]*g[i-j];
    for(unsigned int j=i+1; j < m; ++j) sum += f[j]*conj(g[j-i]);
    for(unsigned int j=1; j < m-i; ++j) sum += conj(f[j])*g[i+j];
    h[i]=sum;
  }
}       

void DirectConvolution2::convolve(Complex *h, Complex *f, Complex *g)
{
  for(unsigned int i=0; i < mx; ++i) {
    for(unsigned int j=0; j < my; ++j) {
      Complex sum=0.0;
      for(unsigned int k=0; k <= i; ++k)
        for(unsigned int p=0; p <= j; ++p)
          sum += f[k*my+p]*g[(i-k)*my+j-p];
      h[i*my+j]=sum;
    }
  }
}       

void DirectHConvolution2::convolve(Complex *h, Complex *f, Complex *g,
                                   bool symmetrize)
{
  unsigned int xorigin=mx-1;
    
  if(symmetrize) {
    HermitianSymmetrizeX(mx,my,xorigin,f);
    HermitianSymmetrizeX(mx,my,xorigin,g);
  }
    
  int xstart=-(int)xorigin;
  int ystart=1-(int) my;
  int xstop=mx;
  int ystop=my;
  for(int kx=xstart; kx < xstop; ++kx) {
    for(int ky=0; ky < ystop; ++ky) {
      Complex sum=0.0;
      for(int px=xstart; px < xstop; ++px) {
        for(int py=ystart; py < ystop; ++py) {
          int qx=kx-px;
          if(qx >= xstart && qx < xstop) {
            int qy=ky-py;
            if(qy >= ystart && qy < ystop) {
              sum += ((py >= 0) ? f[(xorigin+px)*my+py] : 
                      conj(f[(xorigin-px)*my-py])) *
                ((qy >= 0) ? g[(xorigin+qx)*my+qy] : 
                 conj(g[(xorigin-qx)*my-qy]));
            }
          }
        }
        h[(xorigin+kx)*my+ky]=sum;
      }
    }
  }     
}

void DirectConvolution3::convolve(Complex *h, Complex *f, Complex *g)
{
  for(unsigned int i=0; i < mx; ++i) {
    for(unsigned int j=0; j < my; ++j) {
      for(unsigned int k=0; k < mz; ++k) {
        Complex sum=0.0;
        for(unsigned int r=0; r <= i; ++r)
          for(unsigned int p=0; p <= j; ++p)
            for(unsigned int q=0; q <= k; ++q)
              sum += f[r*myz+p*mz+q]*g[(i-r)*myz+(j-p)*mz+(k-q)];
        h[i*myz+j*mz+k]=sum;
      }
    }
  }
}       

void DirectHConvolution3::convolve(Complex *h, Complex *f, Complex *g, 
                                   bool symmetrize)
{
  unsigned int xorigin=mx-1;
  unsigned int yorigin=my-1;
  unsigned int ny=2*my-1;
  
  if(symmetrize) {
    HermitianSymmetrizeXY(mx,my,mz,ny,xorigin,yorigin,f);
    HermitianSymmetrizeXY(mx,my,mz,ny,xorigin,yorigin,g);
  }
    
  int xstart=-(int) xorigin;
  int ystart=-(int) yorigin;
  int zstart=1-(int) mz;
  int xstop=mx;
  int ystop=my;
  int zstop=mz;
  for(int kx=xstart; kx < xstop; ++kx) {
    for(int ky=ystart; ky < ystop; ++ky) {
      for(int kz=0; kz < zstop; ++kz) {
        Complex sum=0.0;
        for(int px=xstart; px < xstop; ++px) {
          for(int py=ystart; py < ystop; ++py) {
            for(int pz=zstart; pz < zstop; ++pz) {
              int qx=kx-px;
              if(qx >= xstart && qx < xstop) {
                int qy=ky-py;
                if(qy >= ystart && qy < ystop) {
                  int qz=kz-pz;
                  if(qz >= zstart && qz < zstop) {
                    sum += ((pz >= 0) ? 
                            f[((xorigin+px)*ny+yorigin+py)*mz+pz] : 
                            conj(f[((xorigin-px)*ny+yorigin-py)*mz-pz])) *
                      ((qz >= 0) ? g[((xorigin+qx)*ny+yorigin+qy)*mz+qz] :    
                       conj(g[((xorigin-qx)*ny+yorigin-qy)*mz-qz]));
                  }
                }
              }
            }
          }
        }
        h[((xorigin+kx)*ny+yorigin+ky)*mz+kz]=sum;
      }
    }
  }     
}

void DirectHTConvolution::convolve(Complex *h, Complex *e, Complex *f,
                                   Complex *g)
{
  int stop=m;
  int start=1-m;
  for(int k=0; k < stop; ++k) {
    Complex sum=0.0;
    for(int p=start; p < stop; ++p) {
      Complex E=(p >= 0) ? e[p] : conj(e[-p]);
      for(int q=start; q < stop; ++q) {
        int r=k-p-q;
        if(r >= start && r < stop)
          sum += E*
            ((q >= 0) ? f[q] : conj(f[-q]))*
            ((r >= 0) ? g[r] : conj(g[-r]));
      }
    }
    h[k]=sum;
  }
}

void DirectHTConvolution2::convolve(Complex *h, Complex *e, Complex *f,
                                    Complex *g, bool symmetrize)
{
  if(symmetrize) {
    HermitianSymmetrizeX(mx,my,mx-1,e);
    HermitianSymmetrizeX(mx,my,mx-1,f);
    HermitianSymmetrizeX(mx,my,mx-1,g);
  }
    
  unsigned int xorigin=mx-1;
  int xstart=-(int) xorigin;
  int xstop=mx;
  int ystop=my;
  int ystart=1-(int) my;
  for(int kx=xstart; kx < xstop; ++kx) {
    for(int ky=0; ky < ystop; ++ky) {
      Complex sum=0.0;
      for(int px=xstart; px < xstop; ++px) {
        for(int py=ystart; py < ystop; ++py) {
          Complex E=(py >= 0) ? e[(xorigin+px)*my+py] : 
            conj(e[(xorigin-px)*my-py]);
          for(int qx=xstart; qx < xstop; ++qx) {
            for(int qy=ystart; qy < ystop; ++qy) {
              int rx=kx-px-qx;
              if(rx >= xstart && rx < xstop) {
                int ry=ky-py-qy;
                if(ry >= ystart && ry < ystop) {
                  sum += E *
                    ((qy >= 0) ? f[(xorigin+qx)*my+qy] : 
                     conj(f[(xorigin-qx)*my-qy])) *
                    ((ry >= 0) ? g[(xorigin+rx)*my+ry] : 
                     conj(g[(xorigin-rx)*my-ry]));
                }
              }
            }
          }
        }
        h[(xorigin+kx)*my+ky]=sum;
      }
    }
  }     
}

void ExplicitConvolution::pad(Complex *f)
{
  PARALLEL(
    for(unsigned int k=m; k < n; ++k) f[k]=0.0;
    )
    }

void ExplicitConvolution::backwards(Complex *f)
{
  Backwards->fft(f);
}
  
void ExplicitConvolution::forwards(Complex *f)
{
  Forwards->fft(f);
}
  
void ExplicitConvolution::convolve(Complex *f, Complex *g)
{
  pad(f);
  backwards(f);
  
  pad(g);
  backwards(g);
      
  double ninv=1.0/n;
#ifdef __SSE2__      
  Vec Ninv=LOAD(ninv);
#endif
  
  PARALLEL(
#ifdef __SSE2__      
    for(unsigned int k=0; k < n; ++k)
      STORE(f+k,Ninv*ZMULT(LOAD(f+k),LOAD(g+k)));
#else    
    for(unsigned int k=0; k < n; ++k)
      f[k] *= g[k]*ninv;
#endif    
    )
        
    forwards(f);
}

void ExplicitHConvolution::pad(Complex *f)
{
  unsigned int n2=n/2;
  PARALLEL(
    for(unsigned int i=m; i <= n2; ++i) f[i]=0.0;
    )
    }
  
void ExplicitHConvolution::backwards(Complex *f)
{
  cr->fft(f);
}
  
void ExplicitHConvolution::forwards(Complex *f)
{
  rc->fft(f);
}

void ExplicitHConvolution::convolve(Complex *f, Complex *g)
{
  pad(f);
  backwards(f);
  
  pad(g);
  backwards(g);
      
  double *F=(double *) f;
  double *G=(double *) g;
    
  double ninv=1.0/n;
  PARALLEL(
    for(unsigned int k=0; k < n; ++k)
      F[k] *= G[k]*ninv;
    )
  
    forwards(f);
}

void ExplicitConvolution2::pad(Complex *f)
{
  PARALLEL(
    for(unsigned int i=0; i < mx; ++i) {
      unsigned int nyi=ny*i;
      unsigned int stop=nyi+ny;
      for(unsigned int j=nyi+my; j < stop; ++j)
        f[j]=0.0;
    }
    )
    
    PARALLEL(
      for(unsigned int i=mx; i < nx; ++i) {
        unsigned int nyi=ny*i;
        unsigned int stop=nyi+ny;
        for(unsigned int j=nyi; j < stop; ++j)
          f[j]=0.0;
      }
      )
    }

void ExplicitConvolution2::backwards(Complex *f)
{
  if(prune) {
    xBackwards->fft(f);
    yBackwards->fft(f);
  } else
    Backwards->fft(f);
}
  
void ExplicitConvolution2::forwards(Complex *f)
{
  if(prune) {
    yForwards->fft(f);
    xForwards->fft(f);
  } else
    Forwards->fft(f);
}
  
void ExplicitConvolution2::convolve(Complex *f, Complex *g)
{
  pad(f);
  backwards(f);
  
  pad(g);
  backwards(g);
      
  unsigned int n=nx*ny;
  double ninv=1.0/n;
#ifdef __SSE2__      
  Vec Ninv=LOAD(ninv);
#endif  
  PARALLEL(
#ifdef __SSE2__      
    for(unsigned int k=0; k < n; ++k)
      STORE(f+k,Ninv*ZMULT(LOAD(f+k),LOAD(g+k)));
#else    
    for(unsigned int k=0; k < n; ++k)
      f[k] *= g[k]*ninv;
#endif    
    )
        
    forwards(f);
}

void ExplicitHConvolution2::pad(Complex *f)
{
  unsigned int nyp=ny/2+1;
  unsigned int nx2=nx/2;

  // zero-pad left block
  unsigned int stop=(nx2-mx+1)*nyp;
  PARALLEL(
    for(unsigned int i=0; i < stop; ++i) 
      f[i]=0.0;
    )
    
    // zero-pad top-middle block
    unsigned int stop2=stop+2*mx*nyp;
  unsigned int diff=nyp-my;
  PARALLEL(
    for(unsigned int i=stop+nyp; i < stop2; i += nyp) {
      for(unsigned int j=i-diff; j < i; ++j)
        f[j]=0.0;
    }
    )
    
    // zero-pad right block
    stop=nx*nyp;
  PARALLEL(
    for(unsigned int i=(nx2+mx)*nyp; i < stop; ++i) 
      f[i]=0.0;
    )
    }

void oddShift(unsigned int nx, unsigned int ny, Complex *f, int sign,
              unsigned int s, Complex *ZetaH, Complex *ZetaL)
{
  unsigned int nyp=ny/2+1;
  int Sign=-1;
  sign=-sign;
  unsigned int stop=s;
  Complex *ZetaL0=ZetaL;
  for(unsigned int a=0, k=1; k < nx; ++a) {
    Complex H=ZetaH[a];
    for(; k < stop; ++k) {
      Complex zeta=Sign*H*ZetaL0[k];
      zeta.im *= sign;
      unsigned int j=nyp*k;
      unsigned int stop=j+nyp;
      for(; j < stop; ++j)
        f[j] *= zeta;
      Sign=-Sign;
    }
    stop=std::min(k+s,nx);
    ZetaL0=ZetaL-k;
  }
}

void ExplicitHConvolution2::backwards(Complex *f, bool shift)
{
  if(prune) {
    xBackwards->fft(f);
    if(nx % 2 == 0) {
      if(shift) fftw::Shift(f,nx,ny);
    } else oddShift(nx,ny,f,-1,s,ZetaH,ZetaL);
    yBackwards->fft(f);
  } else {
    if(shift)
      Backwards->fft0(f);
    else
      Backwards->fft(f);
  }
}

void ExplicitHConvolution2::forwards(Complex *f)
{
  if(prune) {
    yForwards->fft(f);
    if(nx % 2 == 0) {
      fftw::Shift(f,nx,ny);
    } else oddShift(nx,ny,f,1,s,ZetaH,ZetaL);
    xForwards->fft(f);
  } else
    Forwards->fft0(f);
}

void ExplicitHConvolution2::convolve(Complex *f, Complex *g, bool symmetrize)
{
  unsigned int xorigin=nx/2;
  unsigned int nyp=ny/2+1;
    
  if(symmetrize) HermitianSymmetrizeX(mx,nyp,xorigin,f);
  pad(f);
  backwards(f,false);
  
  if(symmetrize) HermitianSymmetrizeX(mx,nyp,xorigin,g);
  pad(g);
  backwards(g,false);
    
  double *F=(double *) f;
  double *G=(double *) g;
    
  double ninv=1.0/(nx*ny);
  unsigned int nyp2=2*nyp;

  PARALLEL(
    for(unsigned int i=0; i < nx; ++i) {
      unsigned int nyp2i=nyp2*i;
      unsigned int stop=nyp2i+ny;
      for(unsigned int j=nyp2i; j < stop; ++j)
        F[j] *= G[j]*ninv;
    }
    )
        
    forwards(f);
}

void ExplicitConvolution3::pad(Complex *f)
{
  PARALLEL(
  for(unsigned int i=0; i < mx; ++i) {
    unsigned int nyi=ny*i;
    for(unsigned int j=0; j < my; ++j) {
      unsigned int nyzij=nz*(nyi+j);
      unsigned int stop=nyzij+nz;
      for(unsigned int k=nyzij+mz; k < stop; ++k)
        f[k]=0.0;
    }
  }
    )
    
  unsigned int nyz=ny*nz;
  PARALLEL(
  for(unsigned int i=mx; i < nx; ++i) {
    unsigned int nyzi=nyz*i;
    for(unsigned int j=0; j < ny; ++j) {
      unsigned int nyzij=nyzi+nz*j;
      unsigned int stop=nyzij+nz;
      for(unsigned int k=nyzij; k < stop; ++k)
        f[k]=0.0;
    }
  }
    )
    
  PARALLEL(
  for(unsigned int i=0; i < nx; ++i) {
    unsigned int nyzi=nyz*i;
    for(unsigned int j=mx; j < ny; ++j) {
      unsigned int nyzij=nyzi+nz*j;
      unsigned int stop=nyzij+nz;
      for(unsigned int k=nyzij; k < stop; ++k)
        f[k]=0.0;
    }
  }
    )
}

void ExplicitConvolution3::backwards(Complex *f)
{
  unsigned int nyz=ny*nz;
  if(prune) {
    for(unsigned int i=0; i < mx; ++i)
      yBackwards->fft(f+i*nyz);
    for(unsigned int j=0; j < ny; ++j)
      xBackwards->fft(f+j*nz);
    zBackwards->fft(f);
  } else
    Backwards->fft(f);
}
  
void ExplicitConvolution3::forwards(Complex *f)
{
  if(prune) {
    zForwards->fft(f);
    for(unsigned int j=0; j < ny; ++j)
      xForwards->fft(f+j*nz);
    unsigned int nyz=ny*nz;
    for(unsigned int i=0; i < mx; ++i)
      yForwards->fft(f+i*nyz);
  } else
    Forwards->fft(f);
}

void ExplicitConvolution3::convolve(Complex *f, Complex *g)
{
  pad(f);
  backwards(f);
  
  pad(g);
  backwards(g);
    
  unsigned int n=nx*ny*nz;
  double ninv=1.0/n;
#ifdef __SSE2__      
  Vec Ninv=LOAD(ninv);
#endif
  
  PARALLEL(
    for(unsigned int k=0; k < n; ++k) {
#ifdef __SSE2__      
      STORE(f+k,Ninv*ZMULT(LOAD(f+k),LOAD(g+k)));
#else    
      f[k] *= g[k]*ninv;
#endif    
    }
    )
    
    forwards(f);
}


void ExplicitHTConvolution::pad(Complex *f)
{
  unsigned int n2=n/2;
  PARALLEL(
    for(unsigned int i=m; i <= n2; ++i) f[i]=0.0;
    )
}

void ExplicitHTConvolution::backwards(Complex *f)
{
  cr->fft(f);
}

void ExplicitHTConvolution::forwards(Complex *f)
{
  rc->fft(f);
}

void ExplicitHTConvolution::convolve(Complex *f, Complex *g, Complex *h)
{
  pad(f);
  backwards(f);
  
  pad(g);
  backwards(g);
      
  pad(h);
  backwards(h);
        
  double *F=(double *) f;
  double *G=(double *) g;
  double *H=(double *) h;
    
  double ninv=1.0/n;
  PARALLEL(
    for(unsigned int k=0; k < n; ++k)
      F[k] *= G[k]*H[k]*ninv;
    )
    
    forwards(f);
}

void ExplicitHTConvolution2::pad(Complex *f)
{
  unsigned int nyp=ny/2+1;
  unsigned int nx2=nx/2;
  unsigned int end=nx2-mx;
  PARALLEL(
  for(unsigned int i=0; i <= end; ++i) {
    unsigned int nypi=nyp*i;
    unsigned int stop=nypi+nyp;
    for(unsigned int j=nypi; j < stop; ++j)
      f[j]=0.0;
  }
    )
    
    PARALLEL(
  for(unsigned int i=nx2+mx; i < nx; ++i) {
    unsigned int nypi=nyp*i;
    unsigned int stop=nypi+nyp;
    for(unsigned int j=nypi; j < stop; ++j)
      f[j]=0.0;
  }
      )
    
    PARALLEL(
  for(unsigned int i=0; i < nx; ++i) {
    unsigned int nypi=nyp*i;
    unsigned int stop=nypi+nyp;
    for(unsigned int j=nypi+my; j < stop; ++j)
      f[j]=0.0;
  }
      )
}

void ExplicitHTConvolution2::backwards(Complex *f, bool shift)
{
  if(prune) {
    xBackwards->fft(f);
    if(nx % 2 == 0) {
      if(shift) fftw::Shift(f,nx,ny);
    } else oddShift(nx,ny,f,-1,s,ZetaH,ZetaL);
    yBackwards->fft(f);
  } else
    return Backwards->fft(f);
}

void ExplicitHTConvolution2::forwards(Complex *f, bool shift)
{
  if(prune) {
    yForwards->fft(f);
    if(nx % 2 == 0) {
      if(shift) fftw::Shift(f,nx,ny);
    } else oddShift(nx,ny,f,1,s,ZetaH,ZetaL);
    xForwards->fft(f);
  } else
    Forwards->fft(f);
}

void ExplicitHTConvolution2::convolve(Complex *f, Complex *g, Complex *h,
                                      bool symmetrize)
{
  unsigned int xorigin=nx/2;
  unsigned int nyp=ny/2+1;
    
  if(symmetrize) HermitianSymmetrizeX(mx,nyp,xorigin,f);
  pad(f);
  backwards(f,false);
  
  if(symmetrize) HermitianSymmetrizeX(mx,nyp,xorigin,g);
  pad(g);
  backwards(g,false);
      
  if(symmetrize) HermitianSymmetrizeX(mx,nyp,xorigin,h);
  pad(h);
  backwards(h,false);
        
  double *F=(double *) f;
  double *G=(double *) g;
  double *H=(double *) h;
    
  double ninv=1.0/(nx*ny);
  unsigned int nyp2=2*nyp;

  PARALLEL(
  for(unsigned int i=0; i < nx; ++i) {
    unsigned int nyp2i=nyp2*i;
    unsigned int stop=nyp2i+ny;
    for(unsigned int j=nyp2i; j < stop; ++j)
      F[j] *= G[j]*H[j]*ninv;
  }
    )
        
  forwards(f,false);
}



#undef PARALLEL

}
