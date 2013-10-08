#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"

struct draw_req {
    unsigned int sample_num;
    int c;
    int w;
    int z;
    int *largest;
};

/**
 * Draw a sample accoring to passed paramater. 
 * w must be even number
 */
void draw_sample(double value, int w)
{
    int plot_width = (w - 12) / 2;
    int rounded = (int) round(value * plot_width);
    int i;

    printf("|");

    if (rounded < 0) /* Negative sample */
    {
        for (i=0; i<plot_width+rounded; i++)
            printf(" ");

        for (i=0; i<-rounded; i++)
            printf("-");

        printf("|");
        
        for (i=0; i<plot_width; i++)
            printf(" ");
    }
    else if (rounded > 0) /* Positive sample */
    {
 
        for (i=0; i<plot_width; i++)
            printf(" ");
        
        printf("|");
        
        for (i=0; i<rounded; i++)
            printf("-");

        for (i=0; i<plot_width-rounded; i++)
            printf(" ");
    }
    else /* Zero */
    {
        for (i=0; i<plot_width; i++)
            printf(" ");

        printf("|");

        for (i=0; i<plot_width; i++)
            printf(" ");
    }

    printf("|\n");
}

void draw_cb(int *samples, const struct soundfile *info, void *data)
{
    struct draw_req *req = (struct draw_req *) data;
    int i;

    if (req->z > 1)
    {
        
        for (i=0; i<info->channels; i++)
        {
            if (abs(samples[i]) > abs(req->largest[i]))
                req->largest[i] = samples[i];
            else if (abs(samples[i]) == abs(req->largest[i]))
                req->largest[i] = samples[i];
        }

        if ((req->sample_num % (req->z - 1)) || !req->sample_num)
        {
            req->sample_num++;
            return;
        }

        for (i=0; i<info->channels; i++)
        {
            samples[i] = req->largest[i];
        }

        if (!(req->sample_num % (req->z - 1)))
            memset(req->largest, 0, info->channels * sizeof(int));
        
        printf("%9d", (((req->sample_num / (req->z - 1))) - 1) * req->z);
    }
    else
    {
        printf("%9d", req->sample_num);
    }

    if (req->c > 0)
    {
        draw_sample((double) samples[req->c - 1] / (pow(2, info->bit_depth - 1) - 1), req->w);
    }
    else
    {
        for (i=0; i<info->channels; i++)
        {
            if (i > 0)
                printf("         ");
            draw_sample((double) samples[i] / (pow(2, info->bit_depth - 1) - 1), req->w);
        }
    }

    req->sample_num++;
}

int main(int argc, char *argv[])
{
    FILE *file;
    char opt;
    int c = 0;
    int w = 80;
    int z = 1;

    while ((opt = getopt(argc, argv, "hc:w:z:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                fprintf(stderr, 
                        "Usage: sndchow [OPTIONS]\n"
                        "Read a file from stdin then plot it using specified options to stdout.\n\n"
                        "Options:\n"
                        "  -h                  Display this information and exit\n"
                        "  -c (channel number) Show the output only for channel c\n"
                        "  -w (width)          Specify the total output width, "
                                               "in number of characters. If not specied"
                                               ", the default is 80\n"
                        "  -z (factor)          Zoom out by a factor of n. If not specied, the default is 1\n"
                        );
                break;

            case 'c':
                c = atoi(optarg);
                break;

            case 'w':
                w = atoi(optarg);
                break;

            case 'z':
                z = atoi(optarg);
                break;

            case '?':
                return EXIT_FAILURE;
        }
    }

    struct soundfile fileinfo;

    file = tmpfile();

    int cc;

    while ((cc = fgetc(stdin)) != EOF)
        fputc(cc, file);

    if (is_cs229_file(file))
        fileinfo = cs229_fileinfo(file);
    else if (is_aiff_file(file))
        fileinfo = aiff_fileinfo(file);
    else
        FATAL("Unrecognized file (standard input)");

    struct draw_req req;
    int tmp[fileinfo.channels];
    req.sample_num = 0;
    req.c = c;
    req.w = w;
    req.z = z;
    memset(tmp, 0, fileinfo.channels * sizeof(int));
    req.largest = tmp;

    if (fileinfo.format == AIFF)
        aiff_enumerate(file, &fileinfo, draw_cb, &req);
    else /* CS229 */
        cs229_enumerate(file, &fileinfo, draw_cb, &req);

    fclose(file);

    return EXIT_SUCCESS;
}


