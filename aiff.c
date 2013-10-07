#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "aiff.h"
#include "utils.h"

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
            fraction = byte_swap_64(fraction);
            fraction >>= (64 - (exponent - 16382));

            info.sample_rate = fraction;
        }
    }
    return info;
}

void write_aiff_header(FILE *ofile, const struct soundfile *info)
{
    unsigned int temp_32;
    unsigned short temp_16;

    fputs("FORM", ofile);
    
    temp_32 = 4 + 4 + 4 + 18 + 4 + 4 + 4 + 4
              + (info->bit_depth / 8) * info->channels * info->sample_num;
    temp_32 = htonl(temp_32); /* File SIze */
    fwrite(&temp_32, 4, 1, ofile);

    fputs("AIFFCOMM", ofile);

    temp_32 = htonl(18); /* COMM Chunk size */
    fwrite(&temp_32, 4, 1, ofile);

    temp_16 = htons(info->channels); /* NumChannels  */
    fwrite(&temp_16, 2, 1, ofile);

    temp_32 = htonl(info->sample_num); /* NumSampleFrames */
    fwrite(&temp_32, 4, 1, ofile);

    temp_16 = htons(info->bit_depth); /* SampleSize */
    fwrite(&temp_16, 2, 1, ofile);

    unsigned long fraction;
    fraction = info->sample_rate;
    int i;

    for (i=63; i>=0; i--)
    {
        if (fraction & (0x01UL << i)) /* Found the integer */
            break; 
    }

    fraction <<= 63 - i;
    fraction = byte_swap_64(fraction);

    temp_16 = htons(i + 16383);

    fwrite(&temp_16, 2, 1, ofile);
    fwrite(&fraction, 8, 1, ofile);

    fputs("SSND", ofile);

    temp_32 = (info->bit_depth / 8) * info->channels * info-> sample_num + 8;
    temp_32 = htonl(temp_32);
    /* Chunk size */
    fwrite(&temp_32, 4, 1, ofile);

    temp_32 = 0;
    fwrite(&temp_32, 4, 1, ofile); /* Offset */
    fwrite(&temp_32, 4, 1, ofile); /* BlockSize */
}

int aiff_enumerate(FILE *file, const struct soundfile *info, sample_cb cb, void *data)
{
    char buffer[128];

    rewind(file);

    fseek(file, 12, SEEK_CUR);

    while (fread(buffer, 1, 4, file) == 4)
    {
        unsigned int size;

        if (memcmp(buffer, "SSND", 4)) /* Not SSND */
        {
            fread(&size, 4, 1, file);
            size = ntohl(size);
            fseek(file, (size % 2) ? size + 1 : size, SEEK_CUR);
            continue;
        }
        else
        {
            fseek(file, 12, SEEK_CUR);

            int samples[info->channels];
            
            int i; 
            for (i=0; i<info->sample_num; i++)
            {
                int j;
                for (j=0; j<info->channels; j++)
                {
                    fread(&samples[j], info->bit_depth / 8, 1, file);
                    samples[j] = ntohl(samples[j]);
                    samples[j] >>= 32 - info->bit_depth;
                }

                cb(samples, info, data);
            }
        }
    }
    return 0;
}

