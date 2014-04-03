all: mfa_proc

ADIOS_INC=$(shell adios_config -c)
ADIOS_LIB=$(shell adios_config -l)
FFTW_INC=-I$(FFTW_DIR)/include
FFTW_LIB=-L$(FFTW_DIR)/lib -lfftw3

mfa_proc: mfa_proc.c
        gcc -g ${ADIOS_INC} ${FFTW_INC} -o mfaproc mfa_proc.c ${ADIOS_LIB} ${FFTW_LIB}

clean:
	rm -rf *o mfaproc 

