#include "convolve.h"
#include "timing.h"

using namespace std;
using namespace fftwpp;

namespace fftwpp {
unsigned int L,Lx,Ly,Lz;
unsigned int M,Mx,My,Mz;
unsigned int mx=0,my=0,mz=0;
unsigned int Dx=0,Dy=0,Dz=0;
unsigned int Sx=0,Sy=0;
int Ix=-1,Iy=-1,Iz=-1;
}

unsigned int K=0;
unsigned int C=1;
unsigned int S=0;
int stats=MEDIAN;

namespace utils {

int Atoi(char *optarg, int min=1)
{
  int val=atoi(optarg);
  if(val < min) {
    usageHybrid();
    exit(-1);
  }
  return val;
}

int atoD(char *optarg)
{
  int D=Atoi(optarg,0);
  if(D > 1 && D % 2) ++D;
  return D;
}

void optionsHybrid(int argc, char* argv[], bool fft)
{
#ifdef __GNUC__
  optind=0;
#endif

  enum Parameters {LXYZ=256,LX,LY,LZ,MXYZ,MX,MY,MZ,SX,SY,mXYZ,mX,mY,mZ,
    DXYZ,DX,DY,DZ,IXYZ,IX,IY,IZ,T,THRESHOLD};

    int option_index = 0;
  static struct option long_options[] =
  {
    {"L", 1, 0, LXYZ},
    {"Lx", 1, 0, LX},
    {"Ly", 1, 0, LY},
    {"Lz", 1, 0, LZ},
    {"M", 1, 0, MXYZ},
    {"Mx", 1, 0, MX},
    {"My", 1, 0, MY},
    {"Mz", 1, 0, MZ},
    {"Sx", 1, 0, SX},
    {"Sy", 1, 0, SY},
    {"m", 1, 0, mXYZ},
    {"mx", 1, 0, mX},
    {"my", 1, 0, mY},
    {"mz", 1, 0, mZ},
    {"D", 1, 0, DXYZ},
    {"Dx", 1, 0, DX},
    {"Dy", 1, 0, DY},
    {"Dz", 1, 0, DZ},
    {"I", 1, 0, IXYZ},
    {"Ix", 1, 0, IX},
    {"Iy", 1, 0, IY},
    {"Iz", 1, 0, IZ},
    {"T", 1, 0, T},
    {"threshold", 1, 0, THRESHOLD},
    {0, 0, 0, 0}
  };

  for (;;) {
    int c=getopt_long_only(argc,argv,"hC:D:I:K:L:M:ctEORS:T:m:u",
                           long_options,&option_index);

    if (c == -1) break;

    switch(c) {
      case 0:
        break;
      case 'C':
        C=Atoi(optarg);
        break;
      case 'D':
      case DXYZ:
        Dx=Dy=Dz=atoD(optarg);
        break;
      case DX:
        Dx=atoD(optarg);
        break;
      case DY:
        Dy=atoD(optarg);
        break;
      case DZ:
        Dz=atoD(optarg);
        break;
      case 'E':
        testError=true;
        break;
      case 'K':
        K=Atoi(optarg);
        break;
      case 'I':
      case IXYZ:
        Ix=Iy=Iz=atoi(optarg) > 0;
        break;
      case IX:
        Ix=atoi(optarg) > 0;
        break;
      case IY:
        Iy=atoi(optarg) > 0;
        break;
      case IZ:
        Iz=atoi(optarg) > 0;
        break;
      case 'L':
      case LXYZ:
        L=Lx=Ly=Lz=Atoi(optarg);
        break;
      case LX:
        L=Lx=Atoi(optarg);
        break;
      case LY:
        Ly=Atoi(optarg);
        break;
      case LZ:
        Lz=Atoi(optarg);
        break;
      case 'M':
      case MXYZ:
        M=Mx=My=Mz=Atoi(optarg);
        break;
      case MX:
        M=Mx=Atoi(optarg);
        break;
      case MY:
        My=Atoi(optarg);
        break;
      case MZ:
        Mz=Atoi(optarg);
        break;
      case 'm':
      case mXYZ:
        mx=my=mz=Atoi(optarg,0);
        break;
      case mX:
        mx=Atoi(optarg,0);
        break;
      case mY:
        my=Atoi(optarg,0);
        break;
      case mZ:
        mz=Atoi(optarg,0);
        break;
      case 'O':
        Output=true;
        break;
      case 'R':
        showRoutines=true;
        break;
      case 'S':
        if(fft)
          S=atoi(optarg);
        else
          stats=atoi(optarg);
        break;
      case SX:
        Sx=Atoi(optarg,0);
        break;
      case SY:
        Sy=Atoi(optarg,0);
        break;
      case 'T':
      case T:
        fftw::maxthreads=Atoi(optarg);
        break;
      case THRESHOLD:
        threshold=Atoi(optarg);
        break;
      case 'c':
        Centered=true;
        break;
      case 'u':
        normalized=false;
        break;
      case 't':
        showOptTimes=true;
        break;
      case 'h':
      default:
        usageHybrid(fft);
        exit(1);
    }
  }

  if(fft)
    cout << "C=" << C << endl;
}

}
