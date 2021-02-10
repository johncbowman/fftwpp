#include "convolve.h"

using namespace std;
using namespace utils;
using namespace Array;
using namespace fftwpp;

namespace fftwpp {

unsigned int threads=1;

unsigned int mOption=0;
unsigned int DOption=0;

int IOption=-1;

unsigned int A=2; // number of inputs
unsigned int B=1; // number of outputs
unsigned int C=1; // number of copies
unsigned int L;
unsigned int M;

unsigned int surplusFFTsizes=25;

// This multiplication routine is for binary convolutions and takes two inputs
// of size e.
// F0[j] *= F1[j];
void multbinary(Complex **F, unsigned int e, unsigned int threads)
{
  Complex *F0=F[0];
  Complex *F1=F[1];

  PARALLEL(
    for(unsigned int j=0; j < e; ++j)
      F0[j] *= F1[j];
    );
}

unsigned int nextfftsize(unsigned int m)
{
  unsigned int N=-1;
  unsigned int ni=1;
  for(unsigned int i=0; ni < 7*m; ni=pow(7,i), ++i) {
    unsigned int nj=ni;
    for(unsigned int j=0; nj < 5*m; nj=ni*pow(5,j), ++j) {
      unsigned int nk=nj;
      for(unsigned int k=0; nk < 3*m; nk=nj*pow(3,k), ++k) {
        N=std::min(N,nk*utils::ceilpow2(utils::ceilquotient(m,nk)));
      }
    }
  }
  return N;
}

void fftBase::OptBase::check(unsigned int L, unsigned int M,
                             Application& app, unsigned int C, unsigned int m,
                             bool fixed, bool mForced)
{
//    cout << "m=" << m << endl;
  unsigned int q=utils::ceilquotient(M,m);
  unsigned int p=utils::ceilquotient(L,m);

  if(p > 2) return; // Temporary ***********************************

  if(p == q && p > 1 && !mForced) return;

  if(!fixed) {
    unsigned int n=utils::ceilquotient(M,m*p);
    unsigned int q2=p*n;
    if(q2 != q) {
      unsigned int start=DOption > 0 ? std::min(DOption,n) : 1;
      unsigned int stop=DOption > 0 ? std::min(DOption,n) : n;
      if(fixed || C > 1) start=stop=1;
      for(unsigned int D=start; D <= stop; D *= 2) {
        if(2*D > stop) D=stop;
//            cout << "q2=" << q2 << endl;
//            cout << "D2=" << D << endl;
        double t=time(L,M,C,m,q2,D,app);
        if(t < T) {
          this->m=m;
          this->q=q2;
          this->D=D;
          T=t;
        }
      }
    }
  }

//      if(p % 2 == 0) q=utils::ceilquotient(2*M,p*m);
  if(p != 2 && q % p != 0) return;

  unsigned int start=DOption > 0 ? std::min(DOption,q) : 1;
  unsigned int stop=DOption > 0 ? std::min(DOption,q) : q;
  if(fixed || C > 1) start=stop=1;
  for(unsigned int D=start; D <= stop; D *= 2) {
    if(2*D > stop) D=stop;
//        cout << "q=" << q << endl;
//        cout << "D=" << D << endl;
    double t=time(L,M,C,m,q,D,app);

    if(t < T) {
      this->m=m;
      this->q=q;
      this->D=D;
      T=t;
    }
  }
}

void fftBase::OptBase::scan(unsigned int L, unsigned int M, Application& app,
                            unsigned int C, bool Explicit, bool fixed)
{
  if(L > M) {
    std::cerr << "L=" << L << " is greater than M=" << M << "." << std::endl;
    exit(-1);
  }
  m=M;
  q=1;
  D=1;

  if(Explicit && fixed)
    return;

  T=DBL_MAX;
  unsigned int i=0;

  unsigned int stop=M-1;
  for(unsigned int k=0; k < surplusFFTsizes; ++k)
    stop=nextfftsize(stop+1);

  unsigned int m0=1;

  if(mOption >= 1 && !Explicit)
    check(L,M,app,C,mOption,fixed,true);
  else
    while(true) {
      m0=nextfftsize(m0+1);
      if(Explicit) {
        if(m0 > stop) break;
        if(m0 < M) {++i; continue;}
        M=m0;
      } else if(m0 > stop) break;
//        } else if(m0 > L) break;
      if(!fixed || Explicit || M % m0 == 0)
        check(L,M,app,C,m0,fixed || Explicit);
      ++i;
    }

  unsigned int p=utils::ceilquotient(L,m);
  std::cout << std::endl;
  std::cout << "Optimal values:" << std::endl;
  std::cout << "m=" << m << std::endl;
  std::cout << "p=" << p << std::endl;
  std::cout << "q=" << q << std::endl;
  std::cout << "C=" << C << std::endl;
  std::cout << "D=" << D << std::endl;
  std::cout << "Padding:" << m*p-L << std::endl;
}

fftBase::~fftBase() {
  if(q > 1) {
    if(Zetaq)
      utils::deleteAlign(Zetaq);
    utils::deleteAlign(Zetaqm+m);
  }
}

double fftBase::meantime(Application& app, double *Stdev)
{
  unsigned int K=1;
  double eps=0.1;

  utils::statistics Stats;
  app.init(*this);
  while(true) {
    Stats.add(app.time(*this,K));
    double mean=Stats.mean();
    double stdev=Stats.stdev();
    if(Stats.count() < 7) continue;
    int threshold=5000;
    if(mean*CLOCKS_PER_SEC < threshold || eps*mean < stdev) {
      K *= 2;
      Stats.clear();
    } else {
      if(Stdev) *Stdev=stdev/K;
      app.clear();
      return mean/K;
    }
  }
  return 0.0;
}

void fftBase::initialize(Complex *f, Complex *g)
{
  for(unsigned int j=0; j < L; ++j) {
    Complex f0(j,j+1);
    Complex g0(j,2*j+1);
    unsigned int Cj=C*j;
    Complex *fj=f+Cj;
    Complex *gj=g+Cj;
    for(unsigned int c=0; c < C; ++c) {
      fj[c]=f0;
      gj[c]=g0;
    }
  }
}

double fftBase::report(Application& app)
{
  double stdev;
  std::cout << std::endl;

  double mean=meantime(app,&stdev);

  std::cout << "mean=" << mean << " +/- " << stdev << std::endl;

  return mean;
}

void fftPad::init()
{
  common();
//    if(m >  M) M=m;

  if(q == 1) {
    if(C == 1) {
      Forward=&fftBase::forwardExplicit;
      Backward=&fftBase::backwardExplicit;
    } else {
      Forward=&fftBase::forwardExplicitMany;
      Backward=&fftBase::backwardExplicitMany;
    }
    Complex *G;
    G=utils::ComplexAlign(Cm);
    fftm=new mfft1d(m,1,C, C,1, G);
    ifftm=new mfft1d(m,-1,C, C,1, G);
    utils::deleteAlign(G);
    Q=1;
  } else {
    unsigned int N=m*q;
    double twopibyN=twopi/N;
    double twopibyq=twopi/q;

    bool twop=p == 2 && 2*n != q;

    if(twop)
      initZetaq();

    Complex *G,*H;

    unsigned int d=p*D*C;
    G=utils::ComplexAlign(m*d);
    H=inplace ? G : utils::ComplexAlign(m*d);

    if(twop) {
      unsigned Lm=L-m;
      Zetaqm2=utils::ComplexAlign((q-1)*Lm)-L;
      for(unsigned int r=1; r < q; ++r) {
        for(unsigned int s=m; s < L; ++s)
          Zetaqm2[Lm*r+s]=expi(r*s*twopibyN);
      }

      if(C == 1) {
        Forward=&fftBase::forward2;
        Backward=&fftBase::backward2;
      } else {
        Forward=&fftBase::forward2Many;
        Backward=&fftBase::backward2Many;
      }
    } else if(p > 1) { // Implies L > m
      if(C == 1) {
        Forward=&fftBase::forwardInner;
        Backward=&fftBase::backwardInner;
      } else {
        Forward=&fftBase::forwardInnerMany;
        Backward=&fftBase::backwardInnerMany;
      }
      Q=n;
      Zetaqp=utils::ComplexAlign((n-1)*(p-1))-p;
      for(unsigned int r=1; r < n; ++r)
        for(unsigned int t=1; t < p; ++t)
          Zetaqp[p*r-r+t]=expi(r*t*twopibyq);

      // L'=p, M'=q, m'=p, p'=1, q'=n
      fftp=new mfft1d(p,1,Cm, Cm,1, G);
      ifftp=new mfft1d(p,-1,Cm, Cm,1, G);
    } else { // p == 1
      if(C == 1) {
        Forward=&fftBase::forward;
        Backward=&fftBase::backward;
        if(padding())
          Pad=&fftBase::padSingle;
      } else {
        Forward=&fftBase::forwardMany;
        Backward=&fftBase::backwardMany;
        if(padding())
          Pad=&fftBase::padMany;
      }
      Q=q;
    }

    if(C == 1) {
      fftm=new mfft1d(m,1,d, 1,m, G,H);
      ifftm=new mfft1d(m,-1,d, 1,m, G,H);
    } else {
      fftm=new mfft1d(m,1,C, C,1, G,H);
      ifftm=new mfft1d(m,-1,C, C,1, G,H);
    }

    unsigned int extra=Q % D;
    if(extra > 0) {
      d=p*extra;
      fftm2=new mfft1d(m,1,d, 1,m, G,H);
      ifftm2=new mfft1d(m,-1,d, 1,m, G,H);
    }

    if(!inplace)
      utils::deleteAlign(H);
    utils::deleteAlign(G);

    initZetaqm();
  }

  fftBase::Forward=Forward;
  fftBase::Backward=Backward;
}

fftPad:: ~fftPad() {
    if(q == 1) {
      delete fftm;
      delete ifftm;
    } else {
      if(p > 1) {
        utils::deleteAlign(Zetaqp+p);
        delete fftp;
        delete ifftp;
      }
      delete fftm;
      delete ifftm;
      if(Q % D > 0) {
        delete fftm2;
        delete ifftm2;
      }
    }
  }

void fftPad::padSingle(Complex *W)
{
  unsigned int mp=p*m;
  for(unsigned int d=0; d < D; ++d) {
    Complex *F=W+m*d;
    for(unsigned int s=L; s < mp; ++s)
      F[s]=0.0;
  }
}

void fftPad::padMany(Complex *W)
{
  unsigned int mp=p*m;
  for(unsigned int s=L; s < mp; ++s) {
    Complex *F=W+C*s;
    for(unsigned int c=0; c < C; ++c)
      F[c]=0.0;
  }
}

void fftPad::forward(Complex *f, Complex *F)
{
  unsigned int b=Cm*p;
  (this->*Pad)(W0);
  for(unsigned int r=0; r < Q; r += D)
    (this->*Forward)(f,F+b*r,r,W0);
}

void fftPad::backward(Complex *F, Complex *f)
{
  unsigned int b=Cm*p;
  for(unsigned int r=0; r < Q; r += D)
    (this->*Backward)(F+b*r,f,r,W0);
}

void fftPad::forwardExplicit(Complex *f, Complex *F, unsigned int, Complex *W=NULL)
{
  for(unsigned int s=0; s < L; ++s)
    F[s]=f[s];
  for(unsigned int s=L; s < M; ++s)
    F[s]=0.0;
  fftm->fft(F);
}

void fftPad::backwardExplicit(Complex *F, Complex *f, unsigned int, Complex *W) {
  ifftm->fft(F);
  for(unsigned int s=0; s < L; ++s)
    f[s]=F[s];
}

void fftPad::forwardExplicitMany(Complex *f, Complex *F, unsigned int, Complex *W) {
  for(unsigned int s=0; s < L; ++s) {
    Complex *Fs=F+C*s;
    Complex *fs=f+C*s;
    for(unsigned int c=0; c < C; ++c)
      Fs[c]=fs[c];
  }
  padMany(F);
  fftm->fft(F);
}

void fftPad::backwardExplicitMany(Complex *F, Complex *f, unsigned int, Complex *W) {
  ifftm->fft(F);
  for(unsigned int s=0; s < L; ++s) {
    Complex *fs=f+C*s;
    Complex *Fs=F+C*s;
    for(unsigned int c=0; c < C; ++c)
      fs[c]=Fs[c];
  }
}
void fftPad::forward(Complex *f, Complex *F0, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  if(W == F0) {
    for(unsigned int d=0; d < D0; ++d) {
      Complex *F=W+m*d;
      for(unsigned int s=L; s < m; ++s)
        F[s]=0.0;
    }
  }

  unsigned int first=r0 == 0;
  if(first) {
    for(unsigned int s=0; s < L; ++s)
      W[s]=f[s];
  }
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+m*d;
    unsigned int r=r0+d;
    F[0]=f[0];
    Complex *Zetar=Zetaqm+m*r;
    for(unsigned int s=1; s < L; ++s)
      F[s]=Zetar[s]*f[s];
  }
  (D0 == D ? fftm : fftm2)->fft(W,F0);
}

void fftPad::forwardMany(Complex *f, Complex *F, unsigned int r, Complex *W) {
  if(W == NULL) W=F;

  if(W == F) {
    for(unsigned int s=L; s < m; ++s) {
      Complex *Fs=W+C*s;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=0.0;
    }
  }

  if(r == 0) {
    for(unsigned int s=0; s < L; ++s) {
      unsigned Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fs=f+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=fs[c];
    }
  } else {
    for(unsigned int c=0; c < C; ++c)
      W[c]=f[c];
    Complex *Zetar=Zetaqm+m*r;
    for(unsigned int s=1; s < L; ++s) {
      unsigned Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fs=f+Cs;
      Complex Zetars=Zetar[s];
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=Zetars*fs[c];
    }
  }
  fftm->fft(W,F);
}

void fftPad::forward2(Complex *f, Complex *F0, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  unsigned Lm=L-m;
  unsigned int first=r0 == 0;
  if(first) {
    for(unsigned int s=0; s < Lm; ++s)
      W[s]=f[s]+f[m+s];
    for(unsigned int s=Lm; s < m; ++s)
      W[s]=f[s];
  }
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+m*d;
    unsigned int r=r0+d;
    Complex Zetaqr=Zetaq[r];
    F[0]=f[0]+Zetaqr*f[m];
    Complex *Zetar=Zetaqm+m*r;
    for(unsigned int s=1; s < Lm; ++s)
      F[s]=Zetar[s]*(f[s]+Zetaqr*f[m+s]);
    for(unsigned int s=Lm; s < m; ++s)
      F[s]=Zetar[s]*f[s];
  }
  (D0 == D ? fftm : fftm2)->fft(W,F0);
}

void fftPad::forward2Many(Complex *f, Complex *F, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  unsigned Lm=L-m;
  if(r == 0) {
    for(unsigned int s=0; s < Lm; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fs=f+Cs;
      Complex *fms=f+Cm+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=fs[c]+fms[c];
    }
    for(unsigned int s=Lm; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fs=f+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=fs[c];
    }
  } else {
    Complex Zetaqr=Zetaq[r];
    Complex *fm=f+Cm;
    for(unsigned int c=0; c < C; ++c)
      W[c]=f[c]+Zetaqr*fm[c];
    Complex *Zetar=Zetaqm+m*r;
    for(unsigned int s=1; s < Lm; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fs=f+Cs;
      Complex *fms=f+Cm+Cs;
      Complex Zetars=Zetar[s];
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=Zetars*(fs[c]+Zetaqr*fms[c]);
    }
    for(unsigned int s=Lm; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fs=f+Cs;
      Complex Zetars=Zetar[s];
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=Zetars*fs[c];
    }
  }
  fftm->fft(W,F);
}

void fftPad::forwardInner(Complex *f, Complex *F0, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  unsigned int first=r0 == 0;
  unsigned int pm1=p-1;
  unsigned int stop=L-m*pm1;

  if(first) {
    for(unsigned int t=0; t < pm1; ++t) {
      unsigned int mt=m*t;
      Complex *Ft=W+mt;
      Complex *ft=f+mt;
      for(unsigned int s=0; s < m; ++s)
        Ft[s]=ft[s];
    }

    unsigned int mt=m*pm1;
    Complex *Ft=W+mt;
    Complex *ft=f+mt;
    for(unsigned int s=0; s < stop; ++s)
      Ft[s]=ft[s];
    for(unsigned int s=stop; s < m; ++s)
      Ft[s]=0.0;

    fftp->fft(W);
    for(unsigned int t=1; t < p; ++t) {
      unsigned int R=n*t;
      Complex *Ft=W+m*t;
      Complex *Zetar=Zetaqm+m*R;
      for(unsigned int s=1; s < m; ++s)
        Ft[s] *= Zetar[s];
    }
  }

  unsigned int b=m*p;
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+b*d;
    unsigned int r=r0+d;
    for(unsigned int s=0; s < m; ++s)
      F[s]=f[s];
    Complex *Zetaqr=Zetaqp+pm1*r;
    for(unsigned int t=1; t < pm1; ++t) {
      unsigned int mt=m*t;
      Complex *Ft=F+mt;
      Complex *ft=f+mt;
      Complex Zeta=Zetaqr[t];
      for(unsigned int s=0; s < m; ++s)
        Ft[s]=Zeta*ft[s];
    }
    unsigned int mt=m*pm1;
    Complex *Ft=F+mt;
    Complex *ft=f+mt;
    Complex Zeta=Zetaqr[pm1];
    for(unsigned int s=0; s < stop; ++s)
      Ft[s]=Zeta*ft[s];
    for(unsigned int s=stop; s < m; ++s)
      Ft[s]=0.0;

    fftp->fft(F);
    for(unsigned int t=0; t < p; ++t) {
      unsigned int R=n*t+r;
      Complex *Ft=F+m*t;
      Complex *Zetar=Zetaqm+m*R;
      for(unsigned int s=1; s < m; ++s)
        Ft[s] *= Zetar[s];
    }
  }
  (D0 == D ? fftm : fftm2)->fft(W,F0);
}

void fftPad::forwardInnerMany(Complex *f, Complex *F, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  unsigned int pm1=p-1;
  unsigned int stop=L-m*pm1;

  if(r == 0) {
    for(unsigned int t=0; t < pm1; ++t) {
      unsigned int Cmt=Cm*t;
      Complex *Ft=W+Cmt;
      Complex *ft=f+Cmt;
      for(unsigned int s=0; s < m; ++s) {
        unsigned int Cs=C*s;
        Complex *Fts=Ft+Cs;
        Complex *fts=ft+Cs;
        for(unsigned int c=0; c < C; ++c)
          Fts[c]=fts[c];
      }
    }
    unsigned int Cmt=Cm*pm1;
    Complex *Ft=W+Cmt;
    Complex *ft=f+Cmt;
    for(unsigned int s=0; s < stop; ++s) {
      unsigned int Cs=C*s;
      Complex *Fts=Ft+Cs;
      Complex *fts=ft+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fts[c]=fts[c];
    }
    for(unsigned int s=stop; s < m; ++s) {
      Complex *Fts=Ft+C*s;
      for(unsigned int c=0; c < C; ++c)
        Fts[c]=0.0;
    }

    fftp->fft(W);
    for(unsigned int t=1; t < p; ++t) {
      unsigned int R=n*t;
      Complex *Ft=W+Cm*t;
      Complex *Zetar=Zetaqm+m*R;
      for(unsigned int s=1; s < m; ++s) {
        Complex *Fts=Ft+C*s;
        Complex Zetars=Zetar[s];
        for(unsigned int c=0; c < C; ++c)
          Fts[c] *= Zetars;
      }
    }
  } else {
    for(unsigned int s=0; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fs=f+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=fs[c];
    }
    Complex *Zetaqr=Zetaqp+pm1*r;
    for(unsigned int t=1; t < pm1; ++t) {
      unsigned int Cmt=Cm*t;
      Complex *Ft=W+Cmt;
      Complex *ft=f+Cmt;
      Complex Zeta=Zetaqr[t];
      for(unsigned int s=0; s < m; ++s) {
        unsigned int Cs=C*s;
        Complex *Fts=Ft+Cs;
        Complex *fts=ft+Cs;
        for(unsigned int c=0; c < C; ++c)
          Fts[c]=Zeta*fts[c];
      }
    }
    Complex *Ft=W+Cm*pm1;
    Complex *ft=f+Cm*pm1;
    Complex Zeta=Zetaqr[pm1];
    for(unsigned int s=0; s < stop; ++s) {
      unsigned int Cs=C*s;
      Complex *Fts=Ft+Cs;
      Complex *fts=ft+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fts[c]=Zeta*fts[c];
    }
    for(unsigned int s=stop; s < m; ++s) {
      Complex *Fts=Ft+C*s;
      for(unsigned int c=0; c < C; ++c)
        Fts[c]=0.0;
    }

    fftp->fft(W);
    for(unsigned int t=0; t < p; ++t) {
      unsigned int R=n*t+r;
      Complex *Ft=W+Cm*t;
      Complex *Zetar=Zetaqm+m*R;
      for(unsigned int s=1; s < m; ++s) {
        Complex *Fts=Ft+C*s;
        Complex Zetars=Zetar[s];
        for(unsigned int c=0; c < C; ++c)
          Fts[c] *= Zetars;
      }
    }
  }
  for(unsigned int t=0; t < p; ++t) {
    unsigned int Cmt=Cm*t;
    fftm->fft(W+Cmt,F+Cmt);
  }
}

void fftPad::backward(Complex *F0, Complex *f, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  (D0 == D ? ifftm : ifftm2)->fft(F0,W);

  unsigned int first=r0 == 0;
  if(first) {
    for(unsigned int s=0; s < L; ++s)
      f[s]=W[s];
  }
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+m*d;
    unsigned int r=r0+d;
    f[0] += F[0];
    Complex *Zetamr=Zetaqm+m*r;
    for(unsigned int s=1; s < L; ++s)
      f[s] += conj(Zetamr[s])*F[s];
  }
}

void fftPad::backwardMany(Complex *F, Complex *f, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  ifftm->fft(F,W);

  if(r == 0) {
    for(unsigned int s=0; s < L; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Fs=W+Cs;
      for(unsigned int c=0; c < C; ++c)
        fs[c]=Fs[c];
    }
  } else {
    for(unsigned int c=0; c < C; ++c)
      f[c] += W[c];
    Complex *Zetamr=Zetaqm+m*r;
    for(unsigned int s=1; s < L; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Fs=W+Cs;
      Complex Zetamrs=Zetamr[s];
      for(unsigned int c=0; c < C; ++c)
        fs[c] += conj(Zetamrs)*Fs[c];
    }
  }
}

void fftPad::backward2(Complex *F0, Complex *f, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  (D0 == D ? ifftm : ifftm2)->fft(F0,W);

  unsigned int first=r0 == 0;
  if(first) {
    for(unsigned int s=0; s < m; ++s)
      f[s]=W[s];
    Complex *Wm=W-m;
    for(unsigned int s=m; s < L; ++s)
      f[s]=Wm[s];
  }
  unsigned int Lm=L-m;
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+m*d;
    unsigned int r=r0+d;
    f[0] += F[0];
    Complex *Zetamr=Zetaqm+m*r;
    for(unsigned int s=1; s < m; ++s)
      f[s] += conj(Zetamr[s])*F[s];
    Complex *Zetamr2=Zetaqm2+Lm*r;
    Complex *Fm=F-m;
    for(unsigned int s=m; s < L; ++s)
      f[s] += conj(Zetamr2[s])*Fm[s];  // Use a separate table for this case?
  }
}

void fftPad::backward2Many(Complex *F, Complex *f, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  ifftm->fft(F,W);

  if(r == 0) {
    for(unsigned int s=0; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Fs=W+Cs;
      for(unsigned int c=0; c < C; ++c)
        fs[c]=Fs[c];
    }
    Complex *WCm=W-Cm;
    for(unsigned int s=m; s < L; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Fs=WCm+Cs;
      for(unsigned int c=0; c < C; ++c)
        fs[c]=Fs[c];
    }
  } else {
    unsigned int Lm=L-m;
    for(unsigned int c=0; c < C; ++c)
      f[c] += W[c];
    Complex *Zetamr=Zetaqm+m*r;
    for(unsigned int s=1; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Fs=W+Cs;
      Complex Zetamrs=conj(Zetamr[s]);
      for(unsigned int c=0; c < C; ++c)
        fs[c] += Zetamrs*Fs[c];
    }
    Complex *Zetamr2=Zetaqm2+Lm*r;
    Complex *WCm=W-Cm;
    for(unsigned int s=m; s < L; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Fs=WCm+Cs;
      Complex Zetamrs2=conj(Zetamr2[s]);
      for(unsigned int c=0; c < C; ++c)
        fs[c] += Zetamrs2*Fs[c];
    }
  }
}

void fftPad::backwardInner(Complex *F0, Complex *f, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  (D0 == D ? ifftm : ifftm2)->fft(F0,W);

  unsigned int first=r0 == 0;
  unsigned int pm1=p-1;
  unsigned int stop=L-m*pm1;

  if(first) {
    for(unsigned int t=1; t < p; ++t) {
      unsigned int R=n*t;
      Complex *Ft=W+m*t;
      Complex *Zetar=Zetaqm+m*R;
      for(unsigned int s=1; s < m; ++s)
        Ft[s] *= conj(Zetar[s]);
    }
    ifftp->fft(W);
    for(unsigned int t=0; t < pm1; ++t) {
      unsigned int mt=m*t;
      Complex *ft=f+mt;
      Complex *Ft=W+mt;
      for(unsigned int s=0; s < m; ++s)
        ft[s]=Ft[s];
    }
    unsigned int mt=m*pm1;
    Complex *ft=f+mt;
    Complex *Ft=W+mt;
    for(unsigned int s=0; s < stop; ++s)
      ft[s]=Ft[s];
  }

  unsigned int b=m*p;
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+b*d;
    unsigned int r=r0+d;
    for(unsigned int t=0; t < p; ++t) {
      unsigned int R=n*t+r;
      Complex *Ft=F+m*t;
      Complex *Zetar=Zetaqm+m*R;
      for(unsigned int s=1; s < m; ++s)
        Ft[s] *= conj(Zetar[s]);
    }
    ifftp->fft(F);
    for(unsigned int s=0; s < m; ++s)
      f[s] += F[s];
    Complex *Zetaqr=Zetaqp+pm1*r;
    for(unsigned int t=1; t < pm1; ++t) {
      unsigned int mt=m*t;
      Complex *ft=f+mt;
      Complex *Ft=F+mt;
      Complex Zeta=conj(Zetaqr[t]);
      for(unsigned int s=0; s < m; ++s)
        ft[s] += Zeta*Ft[s];
    }
    unsigned int mt=m*pm1;
    Complex *Ft=F+mt;
    Complex *ft=f+mt;
    Complex Zeta=conj(Zetaqr[pm1]);
    for(unsigned int s=0; s < stop; ++s)
      ft[s] += Zeta*Ft[s];
  }
}

void fftPad::backwardInnerMany(Complex *F, Complex *f, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  for(unsigned int t=0; t < p; ++t) {
    unsigned int Cmt=Cm*t;
    ifftm->fft(F+Cmt,W+Cmt);
  }
  unsigned int pm1=p-1;
  unsigned int stop=L-m*pm1;

  if(r == 0) {
    for(unsigned int t=1; t < p; ++t) {
      unsigned int R=n*t;
      Complex *Ft=W+Cm*t;
      Complex *Zetar=Zetaqm+m*R;
      for(unsigned int s=1; s < m; ++s) {
        Complex *Fts=Ft+C*s;
        Complex Zetars=Zetar[s];
        for(unsigned int c=0; c < C; ++c)
          Fts[c] *= conj(Zetar[s]);
      }
    }
    ifftp->fft(W);
    for(unsigned int t=0; t < pm1; ++t) {
      unsigned int Cmt=Cm*t;
      Complex *ft=f+Cmt;
      Complex *Ft=W+Cmt;
      for(unsigned int s=0; s < m; ++s) {
        unsigned int Cs=C*s;
        Complex *fts=ft+Cs;
        Complex *Fts=Ft+Cs;
        for(unsigned int c=0; c < C; ++c)
          fts[c]=Fts[c];
      }
    }
    unsigned int Cmt=Cm*pm1;
    Complex *ft=f+Cmt;
    Complex *Ft=W+Cmt;
    for(unsigned int s=0; s < stop; ++s) {
      unsigned int Cs=C*s;
      Complex *fts=ft+Cs;
      Complex *Fts=Ft+Cs;
      for(unsigned int c=0; c < C; ++c)
        fts[c]=Fts[c];
    }
  } else {
    for(unsigned int t=0; t < p; ++t) {
      unsigned int R=n*t+r;
      Complex *Ft=W+Cm*t;
      Complex *Zetar=Zetaqm+m*R;
      for(unsigned int s=1; s < m; ++s) {
        Complex *Fts=Ft+C*s;
        Complex Zetars=conj(Zetar[s]);
        for(unsigned int c=0; c < C; ++c)
          Fts[c] *= Zetars;
      }
    }
    ifftp->fft(W);
    for(unsigned int s=0; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Fs=W+Cs;
      for(unsigned int c=0; c < C; ++c)
        fs[c] += Fs[c];
    }
    Complex *Zetaqr=Zetaqp+pm1*r;
    for(unsigned int t=1; t < pm1; ++t) {
      unsigned int Cmt=Cm*t;
      Complex *ft=f+Cmt;
      Complex *Ft=W+Cmt;
      Complex Zeta=conj(Zetaqr[t]);
      for(unsigned int s=0; s < m; ++s) {
        unsigned int Cs=C*s;
        Complex *fts=ft+Cs;
        Complex *Fts=Ft+Cs;
        for(unsigned int c=0; c < C; ++c)
          fts[c] += Zeta*Fts[c];
      }
    }
    Complex *ft=f+Cm*pm1;
    Complex *Ft=W+Cm*pm1;
    Complex Zeta=conj(Zetaqr[pm1]);
    for(unsigned int s=0; s < stop; ++s) {
      unsigned int Cs=C*s;
      Complex *fts=ft+Cs;
      Complex *Fts=Ft+Cs;
      for(unsigned int c=0; c < C; ++c)
        fts[c] += Zeta*Fts[c];
    }
  }
}

void fftPadCentered::init()
{
  if(Zetaq && q > 1) {
    if(C == 1) {
      Forward=&fftBase::forward2;
      Backward=&fftBase::backward2;
    } else {
      Forward=&fftBase::forward2Many;
      Backward=&fftBase::backward2Many;
    }
    fftBase::Forward=Forward;
    fftBase::Backward=Backward;
  } else {
    initShift();
    Forward=fftBase::Forward;
    Backward=fftBase::Backward;

    fftBase::Forward=&fftBase::forwardShifted;
    fftBase::Backward=&fftBase::backwardShifted;
  }
}

void fftPadCentered::initShift()
{
  ZetaShift=utils::ComplexAlign(M);
  double factor=L/2*twopi/M;
  for(unsigned int r=0; r < q; ++r) {
    Complex *Zetar=ZetaShift+r;
    for(unsigned int s=0; s < m; ++s) {
      Zetar[q*s]=expi(factor*(q*s+r));
    }
  }
}

void fftPadCentered::forwardShifted(Complex *f, Complex *F, unsigned int r, Complex *W)
{
  (this->*Forward)(f,F,r,W);
  forwardShift(F,r);
}

void fftPadCentered::backwardShifted(Complex *F, Complex *f, unsigned int r, Complex *W)
{
  backwardShift(F,r);
  (this->*Backward)(F,f,r,W);
}

void fftPadCentered::forwardShift(Complex *F, unsigned int r0) {
  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;
  for(unsigned int d=0; d < D0; ++d) {
    Complex *W=F+m*d;
    unsigned int r=r0+d;
    for(unsigned int t=0; t < p; ++t) {
      Complex *Zetar=ZetaShift+n*t+r;
      Complex *Wt=W+Cm*t;
      for(unsigned int s=0; s < m; ++s) {
        Complex Zeta=conj(Zetar[q*s]);
        for(unsigned int c=0; c < C; ++c)
          Wt[C*s+c] *= Zeta;
      }
    }
  }
}

void fftPadCentered::backwardShift(Complex *F, unsigned int r0)
{
  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;
  for(unsigned int d=0; d < D0; ++d) {
    Complex *W=F+m*d;
    unsigned int r=r0+d;
    for(unsigned int t=0; t < p; ++t) {
      Complex *Zetar=ZetaShift+n*t+r;
      Complex *Wt=W+Cm*t;
      for(unsigned int s=0; s < m; ++s) {
        Complex Zeta=Zetar[q*s];
        for(unsigned int c=0; c < C; ++c)
          Wt[C*s+c] *= Zeta;
      }
    }
  }
}

void fftPadCentered::forward2(Complex *f, Complex *F0, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  unsigned int H=L/2;
  unsigned int mH=m-H;
  unsigned int LH=L-H;
  unsigned int first=r0 == 0;
  Complex *fmH=f-mH;
  Complex *fH=f+H;
  if(first) {
    for(unsigned int s=0; s < mH; ++s)
      W[s]=fH[s];
    for(unsigned int s=mH; s < LH; ++s)
      W[s]=fmH[s]+fH[s];
    for(unsigned int s=LH; s < m; ++s)
      W[s]=fmH[s];
  }
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+m*d;
    unsigned int r=r0+d;
    Complex Zetaqr=conj(Zetaq[r]);
    Complex *Zetar=Zetaqm+m*r;
    for(unsigned int s=0; s < mH; ++s)
      F[s]=Zetar[s]*fH[s];
    for(unsigned int s=mH; s < LH; ++s)
      F[s]=Zetar[s]*(Zetaqr*fmH[s]+fH[s]);
    for(unsigned int s=LH; s < m; ++s)
      F[s]=Zetar[s]*Zetaqr*fmH[s];
  }
  (D0 == D ? fftm : fftm2)->fft(W,F0);
}

void fftPadCentered::forward2Many(Complex *f, Complex *F, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  unsigned int H=L/2;
  unsigned int mH=m-H;
  unsigned int LH=L-H;
  Complex *fH=f+C*H;
  Complex *fmH=f-C*mH;
  if(r == 0) {
    for(unsigned int s=0; s < mH; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fHs=fH+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=fHs[c];
    }
    for(unsigned int s=mH; s < LH; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fmHs=fmH+Cs;
      Complex *fHs=fH+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=fmHs[c]+fHs[c];
    }
    for(unsigned int s=LH; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fmHs=fmH+Cs;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=fmHs[c];
    }
  } else {
    Complex Zetaqr=conj(Zetaq[r]);
    Complex *Zetar=Zetaqm+m*r;
    for(unsigned int s=0; s < mH; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fHs=fH+Cs;
      Complex Zetars=Zetar[s];
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=Zetars*fHs[c];
    }
    for(unsigned int s=mH; s < LH; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fHs=fH+Cs;
      Complex *fmHs=fmH+Cs;
      Complex Zetars=Zetar[s];
      Complex Zetarsq=Zetars*Zetaqr;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=Zetarsq*fmHs[c]+Zetars*fHs[c];
    }
    for(unsigned int s=LH; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *Fs=W+Cs;
      Complex *fmHs=fmH+Cs;
      Complex Zetars=Zetar[s]*Zetaqr;
      for(unsigned int c=0; c < C; ++c)
        Fs[c]=Zetars*fmHs[c];
    }
  }
  fftm->fft(W,F);
}

void fftPadCentered::backward2(Complex *F0, Complex *f, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  (D0 == D ? ifftm : ifftm2)->fft(F0,W);

  unsigned int H=L/2;
  unsigned int mH=m-H;
  unsigned int LH=L-H;
  unsigned int first=r0 == 0;
  Complex *fmH=f-mH;
  Complex *fH=f+H;
  if(first) {
    for(unsigned int s=mH; s < m; ++s)
      fmH[s]=W[s];
    for(unsigned int s=0; s < LH; ++s)
      fH[s]=W[s];
  }
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+m*d;
    unsigned int r=r0+d;
    Complex Zetaqr=Zetaq[r];
    Complex *Zetamr=Zetaqm+m*r;
    for(unsigned int s=mH; s < m; ++s)
      fmH[s] += conj(Zetamr[s])*Zetaqr*F[s];
    for(unsigned int s=0; s < LH; ++s)
      fH[s] += conj(Zetamr[s])*F[s];
  }
}

void fftPadCentered::backward2Many(Complex *F, Complex *f, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  ifftm->fft(F,W);

  unsigned int H=L/2;
  unsigned int mH=m-H;
  unsigned int LH=L-H;
  Complex *fmH=f-C*mH;
  Complex *fH=f+C*H;
  if(r == 0) {
    for(unsigned int s=mH; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *fmHs=fmH+Cs;
      Complex *Fs=W+Cs;
      for(unsigned int c=0; c < C; ++c)
        fmHs[c]=Fs[c];
    }
    for(unsigned int s=0; s < LH; ++s) {
      unsigned int Cs=C*s;
      Complex *fHs=fH+Cs;
      Complex *Fs=W+Cs;
      for(unsigned int c=0; c < C; ++c)
        fHs[c]=Fs[c];
    }
  } else {
    Complex Zetaqr=Zetaq[r];
    Complex *Zetamr=Zetaqm+m*r;
    for(unsigned int s=mH; s < m; ++s) {
      unsigned int Cs=C*s;
      Complex *fmHs=fmH+Cs;
      Complex *Fs=W+Cs;
      Complex Zetamrs=conj(Zetamr[s])*Zetaqr;
      for(unsigned int c=0; c < C; ++c)
        fmHs[c] += Zetamrs*Fs[c];
    }
    for(unsigned int s=0; s < LH; ++s) {
      unsigned int Cs=C*s;
      Complex *fHs=fH+Cs;
      Complex *Fs=W+Cs;
      Complex Zetamrs=conj(Zetamr[s]);
      for(unsigned int c=0; c < C; ++c)
        fHs[c] += Zetamrs*Fs[c];
    }
  }
}

void fftPadHermitian::init()
{
  common();
  e=m/2;
  if(q == 1) {
    if(C == 1) {
      Forward=&fftBase::forwardExplicit;
      Backward=&fftBase::backwardExplicit;
    } else {
      Forward=&fftBase::forwardExplicitMany;
      Backward=&fftBase::backwardExplicitMany;
    }

    Complex *G=utils::ComplexAlign(C*(e+1));

    crfftm=new mcrfft1d(m,C, C,C,1,1, G);
    rcfftm=new mrcfft1d(m,C, C,C,1,1, (double *) G);
    utils::deleteAlign(G);
    Q=1;
  } else {
    bool twop=p == 2;
    if(twop)
      initZetaq();
    else {
      std::cerr << "Unimplemented!" << std::endl;
      exit(-1);
    }

    unsigned int size=(e+1)*D*C;
    Complex *G=utils::ComplexAlign(size);
    Complex *H=inplace ? G : utils::ComplexAlign(size);

    if(C == 1) {
      crfftm=new mcrfft1d(m,D, 1,1, e+1,m, G,(double *) H);
      rcfftm=new mrcfft1d(m,D, 1,1, m,e+1, (double *) G,H);
      Forward=&fftBase::forward2;
      Backward=&fftBase::backward2;
    } else {
      crfftm=new mcrfft1d(m,C, C,C, 1,1, G,(double *) H);
      rcfftm=new mrcfft1d(m,C, C,C, 1,1, (double *) G,H);
      Forward=&fftBase::forward2Many;
      Backward=&fftBase::backward2Many;
    }

    unsigned int x=Q % D;
    if(x > 0) {
      crfftm2=new mcrfft1d(m,x, 1,1, e+1,m, G,(double *) H);
      rcfftm2=new mrcfft1d(m,x, 1,1, m,e+1, (double *) G,H);
    }

    if(!inplace)
      utils::deleteAlign(H);
    utils::deleteAlign(G);

    initZetaqm();
  }

  fftBase::Forward=Forward;
  fftBase::Backward=Backward;
}

fftPadHermitian::~fftPadHermitian()
{
  delete crfftm;
  delete rcfftm;
}

void fftPadHermitian::forward(Complex *f, Complex *F)
{
  unsigned int b=C*e;
  for(unsigned int r=0; r < Q; r += D)
    (this->*Forward)(f,F+b*r,r,W0);
}

void fftPadHermitian::backward(Complex *F, Complex *f)
{
  unsigned int b=C*e;
  for(unsigned int r=0; r < Q; r += D)
    (this->*Backward)(F+b*r,f,r,W0);
}

void fftPadHermitian::forwardExplicit(Complex *f, Complex *F, unsigned int, Complex *W)
{
  unsigned int H=L/2;
  for(unsigned int s=0; s <= H; ++s)
    F[s]=f[s];
  for(unsigned int s=H+1; s <= e; ++s)
    F[s]=0.0;

  crfftm->fft(F);
}

void fftPadHermitian::forwardExplicitMany(Complex *f, Complex *F, unsigned int, Complex *W)
{
  unsigned int H=L/2;
  for(unsigned int s=0; s <= H; ++s) {
    Complex *Fs=F+C*s;
    Complex *fs=f+C*s;
    for(unsigned int c=0; c < C; ++c)
      Fs[c]=fs[c];
  }
  for(unsigned int s=H+1; s <= e; ++s) {
    Complex *Fs=F+C*s;
    for(unsigned int c=0; c < C; ++c)
      Fs[c]=0.0;
  }

  crfftm->fft(F);
}

void fftPadHermitian::backwardExplicit(Complex *F, Complex *f, unsigned int, Complex *W)
{
  unsigned int H=L/2;
  rcfftm->fft(F);
  for(unsigned int s=0; s <= H; ++s)
    f[s]=F[s];
}

void fftPadHermitian::backwardExplicitMany(Complex *F, Complex *f,
                                           unsigned int, Complex *W)
{
  unsigned int H=L/2;
  rcfftm->fft(F);
  for(unsigned int s=0; s <= H; ++s) {
    Complex *fs=f+C*s;
    Complex *Fs=F+C*s;
    for(unsigned int c=0; c < C; ++c)
      fs[c]=Fs[c];
  }
}

void fftPadHermitian::forward2(Complex *f, Complex *F0, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  unsigned int first=r0 == 0;
  Complex *fm=f+m;
  if(first) {
    W[0]=f[0];
    for(unsigned int s=1; s <= e; ++s)
      W[s]=f[s]+conj(*(fm-s));
  }
  unsigned int e1=e+1;
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+e1*d;
    F[0]=f[0];
    unsigned int r=r0+d;
    Complex Zetaqr=Zetaq[r];
    Complex *Zetar=Zetaqm+m*r;
    for(unsigned int s=1; s <= e; ++s)
      F[s]=Zetar[s]*(f[s]+conj(*(fm-s)*Zetaqr));
  }
  (D0 == D ? crfftm : crfftm2)->fft(W,F0);
}

void fftPadHermitian::forward2Many(Complex *f, Complex *F, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  Complex *fm=f+Cm;
  if(r == 0) {
    for(unsigned int c=0; c < C; ++c)
      W[c]=f[c];
    for(unsigned int s=1; s <= e; ++s) {
      unsigned int Cs=C*s;
      Complex *Ws=W+Cs;
      Complex *fs=f+Cs;
      Complex *fms=fm-Cs;
      for(unsigned int c=0; c < C; ++c)
        Ws[c]=fs[c]+conj(fms[c]);
    }
  } else {
    for(unsigned int c=0; c < C; ++c)
      W[c]=f[c];
    Complex Zetaqr=Zetaq[r];
    Complex *Zetar=Zetaqm+m*r;
    for(unsigned int s=1; s <= e; ++s) {
      unsigned int Cs=C*s;
      Complex *Ws=W+Cs;
      Complex *fs=f+Cs;
      Complex *fms=fm-Cs;
      Complex Zetars=Zetar[s];
      for(unsigned int c=0; c < C; ++c)
        Ws[c]=Zetars*(fs[c]+conj(fms[c]*Zetaqr));
    }
  }
  crfftm->fft(W,F);
}

void fftPadHermitian::backward2(Complex *F0, Complex *f, unsigned int r0, Complex *W)
{
  if(W == NULL) W=F0;

  unsigned int D0=Q-r0;
  if(D0 > D) D0=D;

  Complex Nyquist[D0];
  if(W == F0) {
    for(unsigned int d=0; d < D0; ++d)
      Nyquist[d]=F0[D0*e+d]; // Save before being overwritten
  }

  (D0 == D ? rcfftm : rcfftm2)->fft(F0,W);

  unsigned int first=r0 == 0;
  if(first) {
    for(unsigned int s=0; s <= e; ++s)
      f[s]=W[s];
    for(unsigned int s=1; s < m-e; ++s)
      f[m-s]=conj(W[s]);
  }
  unsigned int e1=e+1;
  for(unsigned int d=first; d < D0; ++d) {
    Complex *F=W+e1*d;
    unsigned int r=r0+d;
    Complex Zetaqr=Zetaq[r];
    Complex *Zetamr=Zetaqm+m*r;
    for(unsigned int s=0; s <= e; ++s)
      f[s] += conj(Zetamr[s])*F[s];
    for(unsigned int s=1; s < m-e; ++s)
      f[m-s] += Zetamr[s]*conj(Zetaqr*F[s]);
  }

  if(W == F0) {
    for(unsigned int d=0; d < D0; ++d)
      F0[D0*e+d]=Nyquist[d]; // Restore initial input of next residue
  }
}

void fftPadHermitian::backward2Many(Complex *F, Complex *f, unsigned int r, Complex *W)
{
  if(W == NULL) W=F;

  Complex Nyquist[C];
  if(W == F) {
    for(unsigned int c=0; c < C; ++c)
      Nyquist[c]=F[C*e+c]; // Save before being overwritten
  }

  rcfftm->fft(F,W);

  Complex *fm=f+Cm;

  if(r == 0) {
    for(unsigned int s=0; s <= e; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Ws=W+Cs;
      for(unsigned int c=0; c < C; ++c)
        fs[c]=Ws[c];
    }
    for(unsigned int s=1; s < m-e; ++s) {
      unsigned int Cs=C*s;
      Complex *fms=fm-Cs;
      Complex *Ws=W+Cs;
      for(unsigned int c=0; c < C; ++c)
        fms[c]=conj(Ws[c]);
    }
  } else {
    Complex Zetaqr=conj(Zetaq[r]);
    Complex *Zetamr=Zetaqm+m*r;
    for(unsigned int s=0; s <= e; ++s) {
      unsigned int Cs=C*s;
      Complex *fs=f+Cs;
      Complex *Ws=W+Cs;
      Complex Zetamrs=conj(Zetamr[s]);
      for(unsigned int c=0; c < C; ++c)
        fs[c] += Zetamrs*Ws[c];
    }
    for(unsigned int s=1; s < m-e; ++s) {
      unsigned int Cs=C*s;
      Complex *fms=fm-Cs;
      Complex *Ws=W+Cs;
      Complex Zeta=Zetamr[s]*Zetaqr;
      for(unsigned int c=0; c < C; ++c)
        fms[c] += Zeta*conj(Ws[c]);
    }
  }

  if(W == F) {
    for(unsigned int c=0; c < C; ++c)
      F[C*e+c]=Nyquist[c]; // Restore initial input of next residue
  }
}

unsigned int fftPadHermitian::worksizeF()
{
  return C*(q == 1 ? M : (e+1)*D);
}

void ForwardBackward::init(fftBase &fft)
{
  Forward=fft.Forward;
  Backward=fft.Backward;
  C=fft.C;
  D=fft.D;
  Q=fft.Q;

  unsigned int L=fft.L;
  unsigned int Lf=C*fft.length();
  unsigned int LF=fft.worksizeF();
  unsigned int E=std::max(A,B);

  f=new Complex*[E];
  F=new Complex*[E];
  h=new Complex*[B];

  for(unsigned int a=0; a < A; ++a)
    f[a]=utils::ComplexAlign(Lf);

  for(unsigned int e=0; e < E; ++e)
    F[e]=utils::ComplexAlign(LF);

  for(unsigned int b=0; b < B; ++b)
    h[b]=utils::ComplexAlign(Lf);

  W=utils::ComplexAlign(fft.worksizeW());
  (fft.*fft.Pad)(W);

  for(unsigned int a=0; a < E; ++a) {
    Complex *fa=f[a];
    for(unsigned int j=0; j < L; ++j) {
      unsigned int Cj=C*j;
      Complex *faj=fa+Cj;
      for(unsigned int c=0; c < C; ++c) {
        faj[c]=0.0;
      }
    }
  }
}

double ForwardBackward::time(fftBase &fft, unsigned int K)
{
  double t0=utils::totalseconds();
  for(unsigned int k=0; k < K; ++k) {
    for(unsigned int r=0; r < Q; r += D) {
      for(unsigned int a=0; a < A; ++a)
        (fft.*Forward)(f[a],F[a],r,W);
      for(unsigned int b=0; b < B; ++b)
        (fft.*Backward)(F[b],h[b],r,W);
    }
  }
  double t=utils::totalseconds();
  return t-t0;
}

void ForwardBackward::clear()
{
  if(W) {
    utils::deleteAlign(W);
    W=NULL;
  }

  unsigned int E=std::max(A,B);

  if(h) {
    for(unsigned int b=0; b < B; ++b)
      utils::deleteAlign(h[b]);
    delete[] h;
    h=NULL;
  }

  if(F) {
    for(unsigned int e=0; e < E; ++e)
      utils::deleteAlign(F[e]);
    delete[] F;
    F=NULL;
  }

  if(f) {
    for(unsigned int a=0; a < A; ++a)
      utils::deleteAlign(f[a]);
    delete[] f;
    f=NULL;
  }
}

void optionsHybrid(int argc, char* argv[])
{
#ifdef __GNUC__
  optind=0;
#endif
  for (;;) {
    int c = getopt(argc,argv,"hC:D:I:L:M:O:S:T:m:");
    if (c == -1) break;

    switch (c) {
      case 0:
        break;
      case 'C':
        C=max(atoi(optarg),1);
        break;
      case 'D':
        DOption=max(atoi(optarg),0);
        break;
      case 'L':
        L=atoi(optarg);
        break;
      case 'I':
        IOption=atoi(optarg) > 0;
        break;
      case 'M':
        M=atoi(optarg);
        break;
      case 'S':
        surplusFFTsizes=atoi(optarg);
        break;
      case 'T':
        fftw::maxthreads=max(atoi(optarg),1);
        break;
      case 'm':
        mOption=max(atoi(optarg),0);
        break;
      case 'h':
      default:
        usageHybrid();
        exit(1);
    }
  }
  cout << "L=" << L << endl;
  cout << "M=" << M << endl;
  cout << "C=" << C << endl;

  cout << endl;
}

}