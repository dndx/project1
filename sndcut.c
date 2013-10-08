#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"
struct deletion {
    unsigned int low;
    unsigned int high;
};

struct write_req {
    unsigned int sample_num;
    struct deletion *ranges;
    unsigned int num_ranges;
};

void write_to_aiff_cut(int *samples, const struct soundfile *info, void *data)
{
    struct write_req *req = (struct write_req *) data;
    int i;

    
    for (i=0; i<req->num_ranges; i++)
    {
        if (req->ranges[i].low <= req->sample_num && req->ranges[i].high >= req->sample_num)
         {
            req->sample_num++;
            return;
         }
    }

    write_to_aiff(samples, info, stdout);
    req->sample_num++;
}

void write_to_cs229_cut(int *samples, const struct soundfile *info, void *data)
{
    struct write_req *req = (struct write_req *) data;
    int i;


    for (i=0; i<req->num_ranges; i++)
    {
        if (req->ranges[i].low <= req->sample_num && req->ranges[i].high >= req->sample_num)
        {
            req->sample_num++;
            return;
        }
    }

    write_to_cs229(samples, info, stdout);
    req->sample_num++;
}

int main(int argc, char *argv[])
{
    FILE *file;
    char opt;

    while ((opt = getopt(argc, argv, "h")) != -1)
    {
        switch (opt)
        {
            case 'h':
                fprintf(stderr, 
                        "Usage: sndcut [-h] [low..high] [low..high]...\n"
                        "Read a file from stdin, remove specified sample, then output processed file to standard output, with same format.\n\n"
                        "Options:\n"
                        "  -h  Display this information and exit\n"
                        );
                break;

            case '?':
                return EXIT_FAILURE;
        }
    }

    int i;

    if (optind < argc)
    {
        struct soundfile fileinfo;
        int total_sample_num = 0;
        struct deletion ranges[argc - optind];

        file = tmpfile();

        int c;
        
        while ((c = fgetc(stdin)) != EOF)
            fputc(c, file);
    
        if (is_cs229_file(file))
            fileinfo = cs229_fileinfo(file);
        else if (is_aiff_file(file))
            fileinfo = aiff_fileinfo(file);
        else
            FATAL("Unrecognized file (standard input)");

        total_sample_num = fileinfo.sample_num;

        for (i=optind; i<argc; i++)
        {
            if (sscanf(argv[i], "[%u..%u]", &ranges[i - optind].low, &ranges[i - optind].high) != 2)
                FATAL("Unrecognized argument %s", argv[i]);

            if (ranges[i - optind].low > ranges[i-optind].high)
                FATAL("Invalid argument %s, low must less or equal to high", argv[i]);
            
            if (ranges[i - optind].high > fileinfo.sample_num - 1)
                FATAL("Invalid argument %s, exceed maximum sample number", argv[i]);

            total_sample_num -= (ranges[i - optind].high - ranges[i - optind].low + 1);

            int j;
            for (j=0; j<i-optind; j++)
            {
                if (ranges[i - optind].low <= ranges[j].high &&
                    ranges[j].low <= ranges[i - optind].high)
                    FATAL("Range %s overlaps with other ranges, please correct your input", argv[i]);
            }
        }

        struct soundfile output_file;
        output_file = fileinfo;
        output_file.sample_num = total_sample_num;

        if (output_file.format == AIFF)
            write_aiff_header(stdout, &output_file);
        else if (output_file.format == CS229)
            write_cs229_header(stdout, &output_file);

        struct write_req req;
        req.sample_num = 0;
        req.ranges = ranges;
        req.num_ranges = argc - optind;

        if (fileinfo.format == AIFF)
            aiff_enumerate(file, &fileinfo, output_file.format == AIFF ? write_to_aiff_cut : write_to_cs229_cut, &req);
        else /* CS229 */
            cs229_enumerate(file, &fileinfo, output_file.format == AIFF ? write_to_aiff_cut : write_to_cs229_cut, &req);

        fclose(file);
    }
    else /* Read from stdin */
    {
        copy_file(stdin, stdout);
    }

    return EXIT_SUCCESS;
}


