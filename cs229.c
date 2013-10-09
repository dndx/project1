#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "cs229.h"
#include "utils.h"

int is_cs229_file(FILE *file)
{
    char buffer[128];

    rewind(file);

    struct soundfile info;

    while (fgets(buffer, sizeof(buffer), file))
    {
        buffer[strlen(buffer) - 1] = '\0';
        if (buffer[0] == '\0' || buffer[0] == '#')
        {
            continue;
        }

        if (!strcmp(buffer, "CS229"))
        {
            info = cs229_fileinfo(file);
            
            if (!(info.sample_rate && info.channels && info.bit_depth))
                FATAL("Not a valid CS229 file!");

            int counter = 0;

            cs229_enumerate(file, &info, sample_count, &counter);

            if (counter != info.sample_num)
                FATAL("Not a valid CS229 file, missing samples!");

            return 1;
        }
    }
    return 0;
}

void write_cs229_header(FILE *ofile, const struct soundfile *info)
{
    fprintf(ofile, "CS229\n");
    fprintf(ofile, "Samples %u\n", info->sample_num);
    fprintf(ofile, "Channels %u\n", info->channels);
    fprintf(ofile, "BitDepth %u\n", info->bit_depth);
    fprintf(ofile, "SampleRate %u\n\n", info->sample_rate);
    fprintf(ofile, "StartData\n");
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

struct soundfile cs229_fileinfo(FILE *file)
{
    char buffer[128];
    struct soundfile fileinfo;
    memset(&fileinfo, 0, sizeof(fileinfo));
    fileinfo.format = CS229;

    rewind(file);
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
            sscanf(buffer, "Samples %d", &fileinfo.sample_num);
        }

    }

    if (!fileinfo.sample_num)
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

            result = strtok(buffer, " ");

            do
            {
                if (result[0] == '\0')
                    continue;

                samples[i] = atoi(result);
                i++;
            }
            while ((result = strtok(NULL, " ")) && i<info->channels);

            cb(samples, info, data);

        }

        if (!strcmp(buffer, "StartData"))
        {
            start_convert = 1;
        }
    }
    return 0;
}

