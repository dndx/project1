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

        cs229_to_aiff(file, ofile, &fileinfo);
    }
    else
    {
        LOGE("Unrecognized file %s, terminating...", in_name);
        exit(EXIT_FAILURE);
    }
}

