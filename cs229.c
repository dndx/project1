#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "cs229.h"
#include "utils.h"

/*
 * Checks is a given file is valid CS229 file
 */
int is_cs229_file(FILE *file)
{
    char buffer[1024];

    rewind(file);

    struct soundfile info;

    while (fgets(buffer, sizeof(buffer), file))
    {
        buffer[strlen(buffer) - 1] = '\0'; /* Strip the \n */
        if (buffer[0] == '\0' || buffer[0] == '#') /* If comment line or empty line, skip */
        {
            continue;
        }

        if (!strcmp(buffer, "CS229"))
        {
            info = cs229_fileinfo(file);
            
            if (!(info.sample_rate && info.channels && info.bit_depth)) /* Missing meta data */
                FATAL("Not a valid CS229 file!");

            int counter = 0;

            cs229_enumerate(file, &info, sample_count, &counter);

            if (counter != info.sample_num) /* Number of samples does not match Samples meta */
                FATAL("Not a valid CS229 file, Samples does not match actual sample number!");

            return 1;
        }
    }
    return 0;
}

/**
 * Write the cs229 meta data  to file
 */
void write_cs229_header(FILE *ofile, const struct soundfile *info)
{
    fprintf(ofile, "CS229\n");
    fprintf(ofile, "Samples %u\n", info->sample_num);
    fprintf(ofile, "Channels %u\n", info->channels);
    fprintf(ofile, "BitDepth %u\n", info->bit_depth);
    fprintf(ofile, "SampleRate %u\n\n", info->sample_rate);
    fprintf(ofile, "StartData\n");
}

/**
 * This is a function used for writring one sample into a CS229 file
 * This function can be used as a callback in enumerator
 *
 * samples are all channel for one sample
 * data is the output file
 */
void write_to_cs229(int *samples, const struct soundfile *info, void *data)
{
    FILE *ofile = (FILE *) data;

    fprintf(ofile, "%d", samples[0]);

    int j;
    for (j=1; j<info->channels; j++)
        fprintf(ofile, " %d", samples[j]);

    fprintf(ofile, "\n");
}

/**
 * Fetch file information addording to meta data
 */
struct soundfile cs229_fileinfo(FILE *file)
{
    char buffer[128];
    struct soundfile fileinfo;
    memset(&fileinfo, 0, sizeof(fileinfo));
    fileinfo.format = CS229;

    rewind(file);

    int samples_found = 0;

    while (fgets(buffer, sizeof(buffer), file))
    {
        buffer[strlen(buffer) - 1] = '\0';

        if (buffer[0] == '\0' || buffer[0] == '#')
        {
            continue;
        }

        if (strstr(buffer, "SampleRate"))
        {
            sscanf(buffer, "SampleRate %d", &fileinfo.sample_rate);
        }
        
        if (strstr(buffer, "Channels"))
        {
            sscanf(buffer, "Channels %hd", &fileinfo.channels);
        }

        if (strstr(buffer, "BitDepth"))
        {
            sscanf(buffer, "BitDepth %hd", &fileinfo.bit_depth);
        }

        if (strstr(buffer, "Samples"))
        {
            samples_found = sscanf(buffer, "Samples %d", &fileinfo.sample_num);
        }

    }

    if (!samples_found) /* If Samples meta not found, count by ourselves */
    {
        int start_count = 0;
        rewind(file);
        while (fgets(buffer, sizeof(buffer), file))
        {
            buffer[strlen(buffer) - 1] = '\0';

            if (buffer[0] == '\0' || buffer[0] == '#')
            {
                continue;
            }

            if (start_count)
            {
                fileinfo.sample_num++;
            }

            if (!strcmp(buffer, "StartData"))
            {
                start_count = 1;
            }
        }
    }

    return fileinfo;
}

/**
 * This function enumerates a CS229 file and call the callback for each sample
 *
 * cb is the callback
 * data is the data being passed to callback
 */
int cs229_enumerate(FILE *file, const struct soundfile *info, sample_cb cb, void *data)
{
    char buffer[128];

    rewind(file);
    int start_convert = 0;
    int samples[info->channels];

    while (fgets(buffer, sizeof(buffer), file))
    {
        buffer[strlen(buffer) - 1] = '\0';

        if (buffer[0] == '\0' || buffer[0] == '#')
        {
            continue;
        }

        if (start_convert)
        {
            char *result;

            int i = 0;

            result = strtok(buffer, " "); /* In case there are multipul spaces between channel */

            do
            {
                if (result[0] == '\0') /* If token is empty string, means we are encountering multipul spaces */
                    continue; /* Just skip it */

                samples[i] = atoi(result);
                i++;
            }
            while ((result = strtok(NULL, " ")) && i<info->channels);
            /* Keep calling strtok until no more channels are available or reached max channel */

            if (i < info->channels) /* We are missing channel */
                FATAL("Invalid CS229 file, missing channel");

            cb(samples, info, data); /* Call the callback */

        }

        if (!strcmp(buffer, "StartData"))
        {
            start_convert = 1;
        }
    }
    return 0;
}

