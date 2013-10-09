#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"

int main(int argc, char *argv[])
{
    FILE *file;
    char opt;
    enum fileformat output_format = OTHER;

    while ((opt = getopt(argc, argv, "hac")) != -1)
    {
        switch (opt)
        {
            case 'h':
                fprintf(stderr, 
                        "Usage: sndcat [-hac] [file] [file]...\n"
                        "Concatenate file(s), or standard input, to standard output.\n"
                        "Output format will be default to CS229, unless specified by user.\n\n"
                        "Options:\n"
                        "  -h  Display this information and exit\n"
                        "  -a  Output should be AIFF\n"
                        "  -c  Output should be CS229\n"
                        );
                return EXIT_SUCCESS;
                break;

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

    if (output_format == OTHER)
        output_format = CS229; /* Default to CS229 */

    int i;

    if (optind < argc)
    {
        struct soundfile files[argc - optind];
        int total_sample_num = 0;

        for (i=optind; i<argc; i++)
        {
            file = fopen(argv[i], "r");

            if (!file)
            {
                FATAL("Could not open file");
            }

            if (is_cs229_file(file))
                files[i - optind] = cs229_fileinfo(file);
            else if (is_aiff_file(file))
                files[i - optind] = aiff_fileinfo(file);
            else
                FATAL("Unrecognized file %s", argv[i]);

            fclose(file);

            if (i > optind)
            {
                if (files[i - optind].channels != files[0].channels ||
                    files[i - optind].bit_depth != files[0].bit_depth ||
                    files[i - optind].sample_rate != files[0].sample_rate
                   )
                {
                    FATAL("File %s have different channel number, bit depth or sample rate from previous files", argv[i]);
                }
            }

            total_sample_num += files[i - optind].sample_num;
        }

        struct soundfile output_file;
        output_file = files[0];
        output_file.sample_num = total_sample_num;

        if (output_format == AIFF)
            write_aiff_header(stdout, &output_file);
        else if (output_format == CS229)
            write_cs229_header(stdout, &output_file);

        for (i=optind; i<argc; i++)
        {
            file = fopen(argv[i], "r");

            if (files[i - optind].format == AIFF)
                aiff_enumerate(file, &files[i - optind], output_format == AIFF ? write_to_aiff : write_to_cs229, stdout);
            else /* CS229 */
                cs229_enumerate(file, &files[i - optind], output_format == AIFF ? write_to_aiff : write_to_cs229, stdout);

            fclose(file);
        }
    }
    else /* Read from stdin */
    {
        file = tmpfile();

        int c;
        struct soundfile fileinfo;

        while ((c = fgetc(stdin)) != EOF)
            fputc(c, file);
    
        if (is_cs229_file(file))
            fileinfo = cs229_fileinfo(file);
        else if (is_aiff_file(file))
            fileinfo = aiff_fileinfo(file);
        else
            FATAL("Unrecognized file (standard input)");

        if (output_format == AIFF)
            write_aiff_header(stdout, &fileinfo);
        else if (output_format == CS229)
            write_cs229_header(stdout, &fileinfo);

        if (fileinfo.format == AIFF)
            aiff_enumerate(file, &fileinfo, output_format == AIFF ? write_to_aiff : write_to_cs229, stdout);
        else /* CS229 */
            cs229_enumerate(file, &fileinfo, output_format == AIFF ? write_to_aiff : write_to_cs229, stdout);

        fclose(file);
    }

    return EXIT_SUCCESS;
}


