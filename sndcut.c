#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>
#include <string.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"

/* Range for deletion */
struct deletion {
    unsigned int low;
    unsigned int high;
};

/* Struct containing information for callback function */
struct write_req {
    unsigned int sample_num; /* current sample num, updated by callback itself */
    struct deletion *ranges; /* array of ranges to delete */
    unsigned int num_ranges; /* size of array */
};

/*
 * Write to an aiff file, with certain ranges cutted
 *
 * data is a struct write_req*
 */
void write_to_aiff_cut(int *samples, const struct soundfile *info, void *data)
{
    struct write_req *req = (struct write_req *) data;
    int i;
    
    for (i=0; i<req->num_ranges; i++)
    {
        /* Since low is always lower or equal than high, so this if statement is suffcient to determine overlapping */
        if (req->ranges[i].low <= req->sample_num && req->ranges[i].high >= req->sample_num)
         {
            req->sample_num++; /* We need update counter by ourselves */
            return; /* Skip this sample */
         }
    }

    write_to_aiff(samples, info, stdout);
    req->sample_num++;
}

/*
 * Write to a cs229 file, with certain ranges cutted
 *
 * data is a struct write_req*
 */
void write_to_cs229_cut(int *samples, const struct soundfile *info, void *data)
{
    struct write_req *req = (struct write_req *) data;
    int i;


    for (i=0; i<req->num_ranges; i++)
    {
        /* Same as above */
        if (req->ranges[i].low <= req->sample_num && req->ranges[i].high >= req->sample_num)
        {
            req->sample_num++; /* Same as above */
            return; /* skip */
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
                return EXIT_SUCCESS;
                break;

            case '?':
                return EXIT_FAILURE;
        }
    }

    int i;
    file = tmpfile(); /* In order to make rewind() work with pipes */

    int c;
        
    while ((c = fgetc(stdin)) != EOF)
        fputc(c, file);

    if (optind < argc) /* we have command line specified files */
    {
        struct soundfile fileinfo;
        int total_sample_num = 0; /* new sample number */
        /* array big enough to keep all ranges, may not use all if some of the ranges overlap*/
        struct deletion ranges[argc - optind];

        if (is_cs229_file(file))
            fileinfo = cs229_fileinfo(file);
        else if (is_aiff_file(file))
            fileinfo = aiff_fileinfo(file);
        else
            FATAL("Unrecognized file (standard input)");

        total_sample_num = fileinfo.sample_num;

        int total_ranges = 0;

        for (i=optind; i<argc; i++)
        {
            unsigned int low, high;
            
            if (strchr(argv[i], '-'))
                FATAL("Range should not contain negative number");

            if (sscanf(argv[i], "%u..%u", &low, &high) != 2)
                FATAL("Unrecognized argument %s", argv[i]);

            if (low > high)
                FATAL("Invalid argument %s, low must less or equal to high", argv[i]);
            
            if (high > fileinfo.sample_num - 1)
                FATAL("Invalid argument %s, exceed maximum sample number", argv[i]);

            int j;
            int set = 0; /* This range got added to existing range */
            for (j=0; j<i-optind; j++)
            {
                if (low <= ranges[j].high &&
                    ranges[j].low <= high) /* Overlaps with existing range */
                {
                    ranges[j].low = MIN(low, ranges[j].low);
                    ranges[j].high = MAX(high, ranges[j].high);
                    set = 1;
                    break;
                }
            }

            if (!set) /* Not overlapping with existing one, add as new range */
            {
                ranges[i - optind].low = low;
                ranges[i - optind].high = high;
                total_ranges++;
            }
        }

        for (i=0; i<total_ranges; i++)
        {
            total_sample_num -= ranges[i].high - ranges[i].low + 1; /* Update sample number after cut */
        }

        struct soundfile output_file;
        output_file = fileinfo; /* Copy all fields */
        output_file.sample_num = total_sample_num;

        if (output_file.format == AIFF)
            write_aiff_header(stdout, &output_file);
        else if (output_file.format == CS229)
            write_cs229_header(stdout, &output_file);

        struct write_req req; /* This is used by callback */
        req.sample_num = 0;
        req.ranges = ranges;
        req.num_ranges = argc - optind;

        if (fileinfo.format == AIFF)
            aiff_enumerate(file, &fileinfo, output_file.format == AIFF ? write_to_aiff_cut : write_to_cs229_cut, &req);
        else /* CS229 */
            cs229_enumerate(file, &fileinfo, output_file.format == AIFF ? write_to_aiff_cut : write_to_cs229_cut, &req);

        if (output_file.format == AIFF && output_file.bit_depth == 8 && (output_file.channels * output_file.sample_num) % 2)
            fputc('\0', stdout); /* AIFF padding */

    }
    else /* Read from stdin */
    {
        if (is_cs229_file(file) || is_aiff_file(file))
            copy_file(file, stdout);
        else
            FATAL("Sound file not recognized or format invalid");
    }

    fclose(file);
    return EXIT_SUCCESS;
}

