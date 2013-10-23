#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"

#define DASHES "------------------------------------------------------------"
void show_file_info(FILE *file, char *filename);

int main(int argc, char *argv[])
{
    char filename[128];
    char *nl;
    FILE *file;
    char opt;
    int i;

    while ((opt = getopt(argc, argv, "h1")) != -1)
    {
        switch (opt)
        {
            case 'h':
                fprintf(stderr, 
                        "Usage: sndinfo [-h1] [file] [file]...\n"
                        "Show information about files. If no argument specified, read from standard input.\n\n"
                        "Options:\n"
                        "  -h  Display this information and exit\n"
                        "  -1  Ignore all arguments and read input file name from stdin\n"
                        );
                return EXIT_SUCCESS;

            case '1':
                fprintf(stderr, "Enter the pathname of a sound file: \n");
                
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

                file = fopen(filename, "r");

                if (!file)
                {
                    FATAL("Could not open file");
                }

                show_file_info(file, filename);
                fclose(file);

                return EXIT_SUCCESS;
                break;

            case '?':
                return EXIT_FAILURE;
        }
    }

    if (optind < argc) /* There are files specified through command line */
    {
        for (i=optind; i<argc; i++)
        {
            file = fopen(argv[i], "r");

            if (!file)
            {
                FATAL("Could not open file");
            }

            show_file_info(file, argv[i]);
            fclose(file);
        }
    }
    else
    {
        file = tmpfile(); /* Use tmpfile() so that rewind() works for pipe */
        int c;
        
        while ((c = fgetc(stdin)) != EOF)
            fputc(c, file);

        show_file_info(file, "(standard input)");
        
        fclose(file);
    }

    puts(DASHES);

    return EXIT_SUCCESS;
}

void show_file_info(FILE *file, char *filename)
{
    struct soundfile fileinfo;
    
    puts(DASHES);

    printf("%12s %s\n", "Filename:", filename);

    if (is_cs229_file(file))
    {
        printf("%12s %s\n", "Format:", "CS229");
        fileinfo = cs229_fileinfo(file);
    }
    else if (is_aiff_file(file))
    {
        printf("%12s %s\n", "Format:", "AIFF");
        fileinfo = aiff_fileinfo(file);
    }
    else
        FATAL("Unknown sound file format");

    printf("%12s %d\n", "Sample Rate:", fileinfo.sample_rate);
    printf("%12s %hd\n", "Bit Depth:", fileinfo.bit_depth);
    printf("%12s %hd\n", "Channels:", fileinfo.channels);
    printf("%12s %d\n", "Samples:", fileinfo.sample_num);

    double duration = (double)fileinfo.sample_num / fileinfo.sample_rate;

    printf("%12s %.0f:%02.0f:%05.2f\n", "Duration:", floor(duration / 3600), floor(fmod(duration, 3600) / 60), fmod(fmod(duration, 3600), 60));
}

