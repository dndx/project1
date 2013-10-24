#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "aiff.h"
#include "utils.h"

/**
 * Determine if file is an AIFF file. 
 * Return 1 for valid file, 0 otherwise. 
 * May terminate program with error message in extreme cases. 
 */
int is_aiff_file(FILE *file)
{
    char buffer[LOAD_BUFFER]; /* Should be enough for most files */

    rewind(file);

    fread(buffer, 1, 4, file);

    if (memcmp(buffer, "FORM", 4))
    {
        return 0;
    }

    /* Skip file size block */

    fseek(file, 4, SEEK_CUR);

    fread(buffer, 1, 4, file);

    /* Expecting "AIFF" */
    if (memcmp(buffer, "AIFF", 4))
    {
        return 0;
    }

    struct soundfile info = aiff_fileinfo(file);
    int counter = 0;

    /* Enumerate whole file to count actual samples after "StartData" */
    /* sample_count is in utils.c */
    aiff_enumerate(file, &info, sample_count, &counter);

    /* Actual number differs from claimed */
    if (counter != info.sample_num)
        FATAL("Not a valid AIFF file, missing or too much samples!");

    return 1;
}

/**
 * Get AIFF file information according to meta data
 */
struct soundfile aiff_fileinfo(FILE *file)
{
    char buffer[LOAD_BUFFER];
    struct soundfile info;
    memset(&info, 0, sizeof(info));
    info.format = AIFF;

    rewind(file);

    fseek(file, 12, SEEK_CUR);

    while (fread(buffer, 1, 4, file) == 4)
    {
        unsigned int size;

        if (memcmp(buffer, "COMM", 4)) /* Not COMM */
        {
            fread(&size, 4, 1, file);
            size = ntohl(size); /* big endian to little endian */
            fseek(file, (size % 2) ? size + 1 : size, SEEK_CUR); /* Align to even number */
            continue;
        } else {
            /* These code reads chunk data and fill into the struct after converting */
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

            /* Converting extended floating number to integer according to piazza post */
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

/**
 * Write the AIFF header to an AIFF file
 */
void write_aiff_header(FILE *ofile, const struct soundfile *info)
{
    unsigned int temp_32;
    unsigned short temp_16;

    fputs("FORM", ofile);
    
    temp_32 = 4 + 4 + 4 + 18 + 4 + 4 + 4 + 4
              + (info->bit_depth / 8) * info->channels * info->sample_num;
    if (temp_32 % 2) /* Add padding byte if needed */
        temp_32++;
    temp_32 = htonl(temp_32); /* File Size */
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

    /* The following code convets an integer back to extended floating point number */
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

    fputs("SSND", ofile); /* Always put this even no sample are there */

    temp_32 = (info->bit_depth / 8) * info->channels * info-> sample_num + 8;
    temp_32 = htonl(temp_32);
    /* Chunk size */
    fwrite(&temp_32, 4, 1, ofile);

    temp_32 = 0;
    fwrite(&temp_32, 4, 1, ofile); /* Offset */
    fwrite(&temp_32, 4, 1, ofile); /* BlockSize */
}

/**
 * This function writes one sample into file. 
 * This function is defined according to sample_cb function type so that 
 * it can be used as a callback. 
 *
 * samples are all channels of that sample
 * data is the file being wrrtten to
 */
void write_to_aiff(int *samples, const struct soundfile *info, void *data)
{
    FILE *ofile = (FILE *) data;
    
    int i;
    for (i=0; i<info->channels; i++)
    {
        samples[i] <<= (sizeof(int) * 8) - info->bit_depth; /* Shift int to left in case bit depth is less than 32 */
        samples[i] = htonl(samples[i]);
        fwrite(&samples[i], info->bit_depth / 8, 1, ofile);
    }
}

/**
 * Enumerate an AIFF file
 *
 * file is the file to enumerate
 * cb is the callback being called for each sample
 * data is the data paramater passed to callback
 */
int aiff_enumerate(FILE *file, const struct soundfile *info, sample_cb cb, void *data)
{
    char buffer[LOAD_BUFFER];

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

            int samples[info->channels]; /* All channel of that sample */
            
            int i; 
            
            /* In case SSND does not contain as much sample as claimed in COMM, we check EOF for that file  */
            for (i=0; i<info->sample_num && !feof(file); i++)
            {
                int j;
                for (j=0; j<info->channels; j++) /* Read channel by channel */
                {
                    fread(&samples[j], info->bit_depth / 8, 1, file);
                    samples[j] = ntohl(samples[j]);
                    samples[j] >>= 32 - info->bit_depth;
                }

                cb(samples, info, data); /* Call the callback with sample and data */
            }
        }
    }
    return 0;
}

