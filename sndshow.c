#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"

/* A "draw request" for callback function */
struct draw_req {
    unsigned int sample_num; /* Current sample number, callback needs increment this manually */
    int c; /* Show output for channel c only, 0 draws all channels */
    int w; /* Output width */
    int z; /* Zoom factor */
    int *largest; /* For keeping largest samoples if zooming */
};

/**
 * Draw a sample accoring to passed paramater. 
 * 
 * w is the output width and must be even number
 * value is the percentage of "bars" to draw
 */
void draw_sample(double value, int w)
{
    int plot_width = (w - 12) / 2; /* Half width */
    int rounded = (int) round(value * plot_width); /* Get actual bar number according to width */
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

/**
 * Draws a sample, this is a valid callback for enumerator
 *
 * data is the drawing request
 */
void draw_cb(int *samples, const struct soundfile *info, void *data)
{
    struct draw_req *req = (struct draw_req *) data;
    int i;

    if (req->z > 1) /* If we are zooming */
    {
        int is_key_sample = (!((req->sample_num + 1) % req->z)) || 
                            (req->sample_num + 1 == info->sample_num); /* "Key sample" means we should draw out now */

        for (i=0; i<info->channels; i++) /* Find the largest sample for that group of sample */
        {
            if (abs(samples[i]) > abs(req->largest[i]))
                req->largest[i] = samples[i];
            else if (abs(samples[i]) == abs(req->largest[i]))
                req->largest[i] = samples[i];
        }

        if (!is_key_sample) /* not a key sample and do not draw */
        {
            req->sample_num++;
            return;
        }

        for (i=0; i<info->channels; i++) /* Replace this sample to largest sample, so that we can re-use existing code */
        {
            samples[i] = req->largest[i];
        }

        if (is_key_sample) /* We no longer need largest sample array */
            memset(req->largest, 0, info->channels * sizeof(int));

        if (req->sample_num + 1 == info->sample_num) /* This is the sample number to draw after zooming */
            req->sample_num += (req->z - (req->sample_num % req->z));

        printf("%9d", ((((req->sample_num + 1) / req->z)) - 1) * req->z);
    }
    else /* Not zooming, draw as normal */
    {
        printf("%9d", req->sample_num);
    }

    if (req->c > 0) /* Draw only channel req->c */
    {
        draw_sample((double) samples[req->c - 1] / (pow(2, info->bit_depth - 1) - 1), req->w);
    }
    else /* Draw all channels */
    {
        for (i=0; i<info->channels; i++)
        {
            if (i > 0)
                printf("         ");
            draw_sample((double) samples[i] / (pow(2, info->bit_depth - 1) - 1), req->w);
        }
    }

    req->sample_num++; /* Increment sample counter */
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
                return EXIT_SUCCESS;
                break;

            case 'c':
                c = atoi(optarg);

                if (c < 1)
                    FATAL("Channel number must be greater than 0");
                break;

            case 'w':
                w = atoi(optarg);

                if (w % 2)
                    FATAL("Width must be even");

                if (w < 20)
                    FATAL("Width must be greater than 20");
                break;

            case 'z':
                z = atoi(optarg);
                
                if (z < 1)
                    FATAL("Zoom factor must greater than 0");
                break;

            case '?':
                return EXIT_FAILURE;
        }
    }

    struct soundfile fileinfo;

    file = tmpfile(); /* Using tmpfile() in order for pipe to work */

    int cc;

    while ((cc = fgetc(stdin)) != EOF)
        fputc(cc, file);

    if (is_cs229_file(file))
        fileinfo = cs229_fileinfo(file);
    else if (is_aiff_file(file))
        fileinfo = aiff_fileinfo(file);
    else
        FATAL("Unrecognized file (standard input)");

    if (c > fileinfo.channels)
        FATAL("Channel number is too big for this file");

    struct draw_req req;
    int tmp[fileinfo.channels]; /* This is the array that keeps largest channel values */
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

