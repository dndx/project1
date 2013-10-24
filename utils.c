#include "utils.h"

/*
 * This function flips byte order of a 64 bit int. 
 * Note: only works on 80386 CPUs
 *
 * This code comes from glibc, not written by myself
 */
unsigned long long int byte_swap_64(unsigned long long int x)
{
    register union { __extension__ uint64_t __ll;
                     uint32_t __l[2]; 
                   } __x;
    __asm("xchgl  %0,%1":
          "=r"(__x.__l[0]),"=r"(__x.__l[1]):
          "0"(ntohl((unsigned long)x)),"1"(ntohl((unsigned long)(x>>32))));
    return __x.__ll;
}

/**
 * Copy all contents in file to ofile byte by byte
 */
void copy_file(FILE *file, FILE *ofile)
{
    int c;

    rewind(file);
    rewind(ofile);

    while ((c = fgetc(file)) != EOF)
    {
        fputc(c, ofile);
    }
}

/**
 * Simple callback for dounting sample numbers
 */
void sample_count(int *samples, const struct soundfile *info, void *data)
{
    int *counter = (int *) data;
    (*counter)++;
}

