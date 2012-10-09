#include<stdio.h>
#include "cfftw++.h"
#include<complex.h>

void init(double complex *f, double complex *g, unsigned int m) 
{
  for(unsigned int k=0; k < m; k++) {
    f[k]=k + (k+1) * I;
    g[k]=k + (2*k +1) * I;
  }
}

void show(double complex *f, unsigned int m) 
{
  for(unsigned int k=0; k < m; k++) 
    printf("(%.2f,%.2f)\n", creal(f[k]), cimag(f[k]));
}

void init2(double complex** F, double complex** G,
	   unsigned int mx, unsigned int my)
{
  for(unsigned int i=0; i < mx; ++i) {
    for(unsigned int j=0; j < my; j++) {
      F[i][j]=i+j*I;
      G[i][j]=2*i+(j+1)*I;
    }
  }
}

void show2(double complex** F, 
	   unsigned int mx, unsigned int my)
{
  for(unsigned int i=0; i < mx; ++i) {
    for(unsigned int j=0; j < my; j++) {
      printf("(%.2f,%.2f) ", creal(F[i][j]), cimag(F[i][j]));
    }
    printf("\n");
  }
}

void pshow2(double complex* f, 
	   unsigned int mx, unsigned int my)
{
  int i,j,pos=0;
  for(i=0; i < mx; i++) {
    for(j=0; j < my; j++) {
      printf("(%.2f,%.2f) ", creal(f[pos]), cimag(f[pos]));
      pos++;
    }
    printf("\n");
  }
}

void init3(double complex *f, double complex *g, 
	   unsigned int mx, unsigned int my, unsigned int mz)
{
  int pos=0;
  for(unsigned int i=0; i < mx; ++i) {
    for(unsigned int j=0; j < my; j++) {
      for(unsigned int k=0; k < mz; k++) {
	f[pos]=(i+k) +I*(j+k);
	g[pos]=(2*i+k)+I*(j+1+k);
	pos++;
      }
    }
  }
}

void show3(double complex *f, 
	   unsigned int mx, unsigned int my, unsigned int mz)
{
  int pos=0;
  for(unsigned int i=0; i < mx; ++i) {
    for(unsigned int j=0; j < my; j++) {
      for(unsigned int k=0; k < mz; k++) {
	printf("(%.0f,%.0f) ", creal(f[pos]), cimag(f[pos]));
	pos++;
      }
      printf("\n");
    }
    printf("\n");
  }
}

int main()
{
  printf("Example of calling fftw++ convolutions from C:\n");
  
  // 1D examples
  { 
    unsigned int m=8; /* problem size */
    
    /* input arrays must be aligned */
    double complex *f=create_complexAlign(m);
    double complex *g=create_complexAlign(m);
    
    init(f,g,m); /* set the input data */
    
    printf("\ninput f:\n");
    show(f,m);
    printf("\ninput g:\n");
    show(g,m);
    
    printf("\n1d non-centered complex convolution:\n");
    ImplicitConvolution *cconv=fftwpp_create_conv1d(m);
    fftwpp_conv1d_convolve(cconv,f,g);
    fftwpp_conv1d_delete(cconv);
    show(f,m);
    
    init(f,g,m); /* reset the inputs */
    
    printf("\n1d centered Hermitian-symmetric complex convolution:\n");
    ImplicitHConvolution *conv=fftwpp_create_hconv1d(m);
    fftwpp_hconv1d_convolve(conv,f,g);
    fftwpp_hconv1d_delete(conv);
    show(f,m);

    // free memory
    delete_complexAlign(g);
    delete_complexAlign(f);

  }

  // 2D examples
  { 
    printf("\n2d non-centered complex convolution:\n");
    unsigned int mx=4, my=4;  /* problem size */
    double complex *f=create_complexAlign(mx*my);
    double complex *g=create_complexAlign(mx*my);

    // 2D arrays for convenience
    double complex* (F)[mx];
    double complex* (G)[mx];
    int i;
    for(i=0; i < mx; ++i)  {
      F[i]=&f[i*my];
      G[i]=&g[i*my];
    }

    init2(F,G,mx,my);

    printf("\ninput f:\n");
    show2(F,mx,my);
    printf("\ninput g:\n");
    show2(G,mx,my);
    
    ImplicitConvolution2 *cconv=fftwpp_create_conv2d(mx,my);
    fftwpp_conv2d_convolve(cconv,f,g);
    fftwpp_conv2d_delete(cconv);
    printf("\noutput:\n");
    show2(F,mx,my);
    
    delete_complexAlign(g);
    delete_complexAlign(f);
  
  }

  {
    printf("\n2d centered Hermitian-symmetric convolution:\n");
    unsigned int mx=4, my=4;  /* problem size */
    
    unsigned int Mx=2*mx-1;
    double complex *f=create_complexAlign(Mx*my);
    double complex *g=create_complexAlign(Mx*my);
    
    // 2D arrays for convenience
    double complex* (F)[Mx];
    double complex* (G)[Mx];
    int i;
    for(i=0; i < Mx; ++i)  {
      F[i]=&f[i*my];
      G[i]=&g[i*my];
    }

    init2(F,G,Mx,my);
    
    printf("\ninput f:\n");
    show2(F,Mx,my);
    printf("\ninput g:\n");
    show2(G,Mx,my);
    
    ImplicitHConvolution2 *conv=fftwpp_create_hconv2d(mx,my);
    fftwpp_hconv2d_convolve(conv,f,g);
    fftwpp_hconv2d_delete(conv);

    printf("\noutput:\n");
    show2(F,Mx,my);

    delete_complexAlign(g);
    delete_complexAlign(f);
  }

  // 3D examples
  {
    printf("\n3d non-centered complex convolution:\n");
    
    unsigned int mx=4, my=4, mz=4;  /* problem size */
    unsigned int mxyz=mx*my*mz;
    double complex *f=create_complexAlign(mxyz);
    double complex *g=create_complexAlign(mxyz);
    
    init3(f,g,mx,my,mz);
    printf("\ninput f:\n");
    show3(f,mx,my,mz);
    printf("\ninput g:\n");
    show3(g,mx,my,mz);
    
    ImplicitConvolution3 *cconv=fftwpp_create_conv3d(mx,my,mz);
    fftwpp_conv3d_convolve(cconv,f,g); 
    fftwpp_conv3d_delete(cconv);
    
    printf("\noutput:\n");
    show3(f,mx,my,mz);
    delete_complexAlign(g);
    delete_complexAlign(f);

  }

  {
    printf("\n3d non-centered complex convolution:\n");
    
    unsigned int mx=4, my=4, mz=4;  /* problem size */
    unsigned int Mx=2*mx-1;
    unsigned int My=2*my-1;

    unsigned int mxyz=Mx*My*mz;
    double complex *f=create_complexAlign(mxyz);
    double complex *g=create_complexAlign(mxyz);
    
    init3(f,g,Mx,My,mz);
    printf("\ninput f:\n");
    show3(f,Mx,My,mz);
    printf("\ninput g:\n");
    show3(g,Mx,My,mz);
    
    ImplicitHConvolution3 *conv=fftwpp_create_hconv3d(mx,my,mz);
    fftwpp_hconv3d_convolve(conv,f,g); 
    fftwpp_hconv3d_delete(conv);
    
    printf("\noutput:\n");
    show3(f,Mx,My,mz);
    delete_complexAlign(g);
    delete_complexAlign(f);
  }

  /* Ternary convolutions */
  {
    printf("\n1d centered Hermitian-symmetric ternary convolution:\n");
    unsigned int m=12; /* problem size */
    unsigned int m1=m+1;
    double complex *e=create_complexAlign(m1);
    double complex *f=create_complexAlign(m1);
    double complex *g=create_complexAlign(m1);

    e[0]=1.0;
    f[0]=1.0;
    g[0]=2.0;
    for(unsigned int k=1; k < m; k++) {
      e[k]=k+I*(k+1);
      f[k]=k+I*(k+1);
      g[k]=k+I*(2*k+1);
    }
    
    printf("\ninput e:\n");
    show(e,m);
    printf("\ninput f:\n");
    show(f,m);
    printf("\ninput g:\n");
    show(g,m);

    ImplicitHTConvolution *conv=fftwpp_create_htconv1d(m);
    fftwpp_htconv1d_convolve(conv,e,f,g);
    fftwpp_htconv1d_delete(conv);
    printf("\noutput:\n");
    show(e,m);

  }

  {
    printf("\n2d centered Hermitian-symmetric ternary convolution:\n");
    
    unsigned int mx=4, my=4;  /* problem size */
    unsigned int Mx=2*mx, my1=my+1;
    unsigned int Mxy1=Mx*my1;

    double complex *e=create_complexAlign(Mxy1);
    double complex *f=create_complexAlign(Mxy1);
    double complex *g=create_complexAlign(Mxy1);
    
    int i,j,pos=0;
    for(i=0; i < 2*mx-1; i++) {
      for(j=0; j < my; j++) {
	e[pos]=i+I*j;
	f[pos]=(i+1)+I*(j+2);
	g[pos]=(2*i)+I*(j+1);
	pos++;
      }
    }
    printf("\ninput e:\n");
    pshow2(e,2*mx-1,my);
    printf("\ninput f:\n");
    pshow2(f,2*mx-1,my);
    printf("\ninput g:\n");
    pshow2(g,2*mx-1,my);

    /* FIXME: htconv2 ouptut not correct. Data not in correct order?*/
    ImplicitHTConvolution2 *conv=fftwpp_create_htconv2d(mx,my);
    fftwpp_htconv2d_convolve(conv,e,f,g);
    fftwpp_htconv2d_delete(conv);

    printf("\noutput:\n");
    pshow2(e,2*mx-1,my);

  }

  return 0;
}


