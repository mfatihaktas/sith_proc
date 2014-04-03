#include <assert.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include "mpi.h"
#include "adios.h"
#include "adios_read.h"
#include "adios_error.h"
#include <libgen.h>

//#define _POSIX_SOURCE
#include <sys/stat.h>
#include <float.h>

//mfa added
#include <stdint.h>
#include <time.h>
//
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define DUMP(fmt, ...) fprintf(stdout, ">>> "fmt"\n", ## __VA_ARGS__)

#ifndef DIMX
#define DIMX 24
#endif

#ifndef DIMY
#define DIMY 8
#endif

#ifndef DIMLX
#define DIMLX 24
#endif

#ifndef SCALE
#define SCALE 8
#endif

#ifndef READ_METHOD
#define READ_METHOD ADIOS_READ_METHOD_BP
#endif

#ifndef WRITE_METHOD
#define WRITE_METHOD MPI
#endif

#ifndef READ_METHOD_PARAM
#define READ_METHOD_PARAM verbose=4
#endif

#ifndef WRITE_METHOD_PARAM
#define WRITE_METHOD_PARAM
#endif

#define str(s) #s
#define xstr(s) str(s)

void print_3dmat(uint64_t len, size_t dimx, size_t dimy, double matX[][dimy][dimx])
{
  printf(">>>3d matX\n");
  for (uint64_t t = 0; t < len; t++){
    printf("t=%lld\n", t);
    for (uint64_t i = 0; i < dimx; i++){
      for (uint64_t j = 0; j < dimy; j++){
        printf("%2.2f - ", matX[t][j][i]);
      }
      printf("\n");
    }
    printf("\n");
  }
}

int main (int argc, char** argv)
{
  int  rank, size;
  MPI_Comm  comm = MPI_COMM_WORLD;
  ADIOS_FILE * fp;
  ADIOS_VARINFO * vi;
  ADIOS_SELECTION * sel;
  int method = READ_METHOD;
  float timeout_sec = 100.0; // Should be large enough when using with ICEE
  float timeout_sec2 = 10.0; // timeout seconds for reading next streams

  char fname[80];
  char fout[80];
  int step = 100;
  double ratio = 1.0E-6; //1.0E-1

  //data to write
  int NX, NY, LX, len, offset;
  double *data;
  int c;
  opterr = 0;
  
  sprintf(fname, "%s", "./../ecei-hfs.007131.bp");
  //
  double (*matX)[DIMY][DIMX];
  uint64_t start[3], count[3], bytes_read = 0;
  struct timeval t0, t1;
  
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (comm, &rank);
  MPI_Comm_size (comm, &size);
  //define adios output
  adios_init_noxml (comm);
  adios_allocate_buffer (ADIOS_BUFFER_ALLOC_NOW, 500);

  int64_t  m_adios_group;

  adios_declare_group (&m_adios_group, "out", "", adios_flag_yes);
  adios_select_method (m_adios_group, xstr(WRITE_METHOD), xstr(WRITE_METHOD_PARAM), "");

  adios_define_var (m_adios_group, "NX"
                    ,"", adios_integer
                    ,0, 0, 0);
  adios_define_var (m_adios_group, "NY"
                    ,"", adios_integer
                    ,0, 0, 0);
  adios_define_var (m_adios_group, "LX"
                    ,"", adios_integer
                    ,0, 0, 0);
  adios_define_var (m_adios_group, "offset"
                    ,"", adios_integer
                    ,0, 0, 0);
  adios_define_var (m_adios_group, "len"
                    ,"", adios_integer
                    ,0, 0, 0);
  adios_define_var (m_adios_group, "vol"
                    ,"", adios_double
                    ,"len,NY,LX", "len,NY,NX", "0,0,offset");
  //load data
  adios_read_init_method (method, comm, xstr(READ_METHOD_PARAM));
  fp = adios_read_open (fname, method,
                        comm, ADIOS_LOCKMODE_NONE, timeout_sec);
  if (adios_errno == err_file_not_found)
  {
    printf ("rank %d: Stream not found after waiting %f seconds: %s\n",
            rank, timeout_sec, adios_errmsg());
    return adios_errno;
  }
  else if (adios_errno == err_end_of_stream)
  {
    printf ("rank %d: Stream terminated before open. %s\n", rank, adios_errmsg());
    return adios_errno;
  }
  else if (fp == NULL) {
    printf ("rank %d: Error at opening stream: %s\n", rank, adios_errmsg());
    return adios_errno;
  }
  //read data
  ADIOS_VARINFO *v = adios_inq_var(fp, "vol");
  DUMP("len : %lld", v->dims[0]);
        
  len = (int) v->dims[0];  // ECEI data layout: len-by-DIMY-by-DIMX
  offset = DIMX * rank;

  matX = malloc (len * sizeof(*matX));

  start[0] = 0;
  start[1] = 0;
  start[2] = offset;
  count[0] = len;
  count[1] = DIMY;
  count[2] = DIMX;

  DUMP("selection : %lld %lld", start[2], count[2]);
    
  sel = adios_selection_boundingbox (3, start, count);
  adios_schedule_read (fp, sel, "vol", 0, 1, matX);
  adios_perform_reads (fp, 1);
  //
  print_3dmat(1, DIMX, DIMY, matX);

  return 0;
}
