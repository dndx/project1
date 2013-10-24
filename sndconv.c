#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"

#define DASHES "------------------------------------------------------------"
void snd_conv(FILE *file, FILE *ofile, char *in_name, enum fileformat output_format);

int main(int argc, char *argv[])
{
    char filename[128];
    char *nl;
    FILE *file, *ofile;
    char opt;
    enum fileformat output_format = OTHER;

    while ((opt = getopt(argc, argv, "h1ac")) != -1) /* went over command line options */
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
                return EXIT_SUCCESS;
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
                    *nl = '\0'; /* Trim \n */
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

    file = tmpfile(); /* use tmpfile otherwise rewind won't work with pipes */

    int c;
    
    while ((c = fgetc(stdin)) != EOF)
    {
        fputc(c, file);
    }

    snd_conv(file, stdout, "(standard input)", output_format);

    fclose(file);

    return EXIT_SUCCESS;
}

/**
 * This function converts from one file into another file
 *
 * file is the input file
 * ofile is the output file
 * in_name is the inpit file name, for error displaying
 * output_format is the output format
 */
void snd_conv(FILE *file, FILE *ofile, char *in_name, enum fileformat output_format)
{
    struct soundfile fileinfo;

    if (is_aiff_file(file))
    {
        if (output_format == AIFF) /* If the input and output file format are the same, just copy over */
        {
            copy_file(file, ofile);
            return;
        }

        LOGI("Input file is AIFF file, converting to CS229...\n");
        fileinfo = aiff_fileinfo(file);
        write_cs229_header(ofile, &fileinfo);
        aiff_enumerate(file, &fileinfo, write_to_cs229, ofile);
    }
    else if (is_cs229_file(file))
    {
        if (output_format == CS229) /* Same as above */
        {
            copy_file(file, ofile);
            return;
        }

        LOGI("Input file is CS229 file, converting to AIFF...\n");
        fileinfo = cs229_fileinfo(file);
        write_aiff_header(ofile, &fileinfo);
        cs229_enumerate(file, &fileinfo, write_to_aiff, ofile);

        if (fileinfo.bit_depth == 8 && (fileinfo.channels * fileinfo.sample_num) % 2)
            fputc('\0', ofile); /* AIFF padding */
    }
    else
    {
        FATAL("Unrecognized file %s, terminating...", in_name);
    }
}

