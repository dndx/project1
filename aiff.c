#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <bits/byteswap.h>
#include "aiff.h"
#include "utils.h"

unsigned long long int ByteSwap64(unsigned long long int x)
{
      register union { __extension__ uint64_t __ll;
      uint32_t __l[2]; } __x;
      __asm("xchgl  %0,%1":
            "=r"(__x.__l[0]),"=r"(__x.__l[1]):
            "0"(ntohl((unsigned long)x)),"1"(ntohl((unsigned long)(x>>32))));
      return __x.__ll;
}

int is_aiff_file(FILE *file)
{
    char buffer[128];

    rewind(file);

    fread(buffer, 1, 4, file);

    if (memcmp(buffer, "FORM", 4))
    {
        return 0;
    }

    /* Skip file size block */

    fseek(file, 4, SEEK_CUR);

    fread(buffer, 1, 4, file);

    if (memcmp(buffer, "AIFF", 4))
    {
        return 0;
    }

    return 1;
}

struct soundfile aiff_fileinfo(FILE *file)
{
    char buffer[128];
    struct soundfile info;

    rewind(file);

    fseek(file, 12, SEEK_CUR);

    while (fread(buffer, 1, 4, file) == 4)
    {
        unsigned int size;

        if (memcmp(buffer, "COMM", 4)) /* Not COMM */
        {
            fread(&size, 4, 1, file);
            size = ntohl(size);
            fseek(file, (size % 2) ? size + 1 : size, SEEK_CUR);
            continue;
        } else {
            fread(&size, 4, 1, file);
            size = ntohl(size);
            if (size < 18)
            {
                LOGE("COMM Trunk size incorrect");
            }

            fread(&info.channels, 2, 1, file);
            info.channels = ntohs(info.channels);

            fread(&info.sample_num, 4, 1, file);
            info.sample_num = ntohl(info.sample_num);

            fread(&info.bit_depth, 2, 1, file);
            info.bit_depth = ntohs(info.bit_depth);

            short exponent;
            fread(&exponent, 2, 1, file);
            exponent = ntohs(exponent);
            unsigned long fraction;
            fread(&fraction, 8, 1, file);
            fraction = ByteSwap64(fraction);
            fraction >>= (64 - (exponent - 16382));

            info.sample_rate = fraction;
        }
    }
    return info;
}

