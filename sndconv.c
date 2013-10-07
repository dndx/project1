#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"

#define DASHES "------------------------------------------------------------"
enum format {
    OTHER,
    AIFF,
    CS229
};
void snd_conv(FILE *file, FILE *ofile, char *in_name, enum format output_format);

int main(int argc, char *argv[])
{
    char filename[128];
    char *nl;
    FILE *file, *ofile;
    char opt;
    enum format output_format = OTHER;

    while ((opt = getopt(argc, argv, "h1ac")) != -1)
    {
        switch (opt)
        {
            case 'h':
                fprintf(stderr,
                        "Usage: sndconv [-h1ac]\n"
                        "Convert file format of a sound file.\n\n"
                        "Options:\n"
                        "  -h  Display this information and exit\n"
                        "  -1  Read file name from stdin with prompt\n"
                        "  -a  Output should be AIFF, regardless of the input format\n"
                        "  -c  Output should be CS229, regardless of the input format\n"
                        );
                break;

            case '1':
                fprintf(stderr, "Enter the pathname of a input sound file: \n");

                if (!fgets(filename, sizeof(filename), stdin))
                {
                    FATAL("fgets failed");
                    return 1;
                }

                nl = strchr(filename, '\n');
                
                if (nl)
                {
                    *nl = '\0';
                }

                file = fopen(filename, "r");
                
                if (!file)
                {
                    FATAL("Could not open file");
                }

                fprintf(stderr, "Enter the pathname of the output file: \n");
                
                if (!fgets(filename, sizeof(filename), stdin))
                {
                    FATAL("fgets failed");
                    return 1;
                }
                
                nl = strchr(filename, '\n');
                
                if (nl)
                {
                    *nl = '\0';
                }

                ofile = fopen(filename, "w");
                
                if (!file)
                {
                    FATAL("Could not open output file");
                }

                puts(DASHES);

                snd_conv(file, ofile, filename, output_format);
                
                LOGI("Done. \n");

                puts(DASHES);

                fclose(file);
                fclose(ofile);
                return EXIT_SUCCESS;

            case 'a':
                output_format = AIFF;
                break;
            
            case 'c':
                output_format = CS229;
                break;

            case '?':
                return EXIT_FAILURE;
        }
    }

    file = tmpfile();

    int c;
    
    while ((c = fgetc(stdin)) != EOF)
    {
        fputc(c, file);
    }

    snd_conv(file, stdout, "(standard input)", output_format);

    fclose(file);

    return EXIT_SUCCESS;
}

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

void write_to_cs229(int *samples, const struct soundfile *info, void *data)
{
    FILE *ofile = (FILE *) data;

    fprintf(ofile, "%d", samples[0]);

    int j;
    for (j=1; j<info->channels; j++)
        fprintf(ofile, " %d", samples[j]);

    fprintf(ofile, "\n");
}

void write_to_aiff(int *samples, const struct soundfile *info, void *data)
{
    FILE *ofile = (FILE *) data;
    
    int i;
    for (i=0; i<info->channels; i++)
    {
        samples[i] <<= info->bit_depth;
        samples[i] = htonl(samples[i]);
        fwrite(&samples[i], info->bit_depth / 8, 1, ofile);
    }
}

void snd_conv(FILE *file, FILE *ofile, char *in_name, enum format output_format)
{
    struct soundfile fileinfo;

    if (is_aiff_file(file))
    {
        if (output_format == AIFF)
        {
            copy_file(file, ofile);
            return;
        }

        LOGI("Input file is AIFF file, converting to CS229...\n");
        fileinfo = aiff_fileinfo(file);

        fprintf(ofile, "CS229\n");
        fprintf(ofile, "Samples %u\n", fileinfo.sample_num);
        fprintf(ofile, "Channels %u\n", fileinfo.channels);
        fprintf(ofile, "BitDepth %u\n", fileinfo.bit_depth);
        fprintf(ofile, "SampleRate %u\n\n", fileinfo.sample_rate);
        fprintf(ofile, "StartData\n");
        
        aiff_enumerate(file, &fileinfo, write_to_cs229, ofile);
    }
    else if (is_cs229_file(file))
    {
        if (output_format == CS229)
        {
            copy_file(file, ofile);
            return;
        }

        LOGI("Input file is CS229 file, converting to AIFF...\n");
        fileinfo = cs229_fileinfo(file);

        unsigned int temp_32;
        unsigned short temp_16;

        fputs("FORM", ofile);
        
        temp_32 = 4 + 4 + 4 + 18 + 4 + 4 + 4 + 4
                  + (fileinfo.bit_depth / 8) * fileinfo.channels * fileinfo.sample_num;
        temp_32 = htonl(temp_32); /* File SIze */
        fwrite(&temp_32, 4, 1, ofile);

        fputs("AIFFCOMM", ofile);

        temp_32 = htonl(18); /* COMM Chunk size */
        fwrite(&temp_32, 4, 1, ofile);

        temp_16 = htons(fileinfo.channels); /* NumChannels  */
        fwrite(&temp_16, 2, 1, ofile);

        temp_32 = htonl(fileinfo.sample_num); /* NumSampleFrames */
        fwrite(&temp_32, 4, 1, ofile);

        temp_16 = htons(fileinfo.bit_depth); /* SampleSize */
        fwrite(&temp_16, 2, 1, ofile);

        unsigned long fraction;
        fraction = fileinfo.sample_rate;
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

        temp_32 = (fileinfo.bit_depth / 8) * fileinfo.channels * fileinfo. sample_num + 8;
        temp_32 = htonl(temp_32);
        /* Chunk size */
        fwrite(&temp_32, 4, 1, ofile);

        temp_32 = 0;
        fwrite(&temp_32, 4, 1, ofile); /* Offset */
        fwrite(&temp_32, 4, 1, ofile); /* BlockSize */

        cs229_enumerate(file, &fileinfo, write_to_aiff, ofile);
    }
    else
    {
        LOGE("Unrecognized file %s, terminating...", in_name);
        exit(EXIT_FAILURE);
    }
}

