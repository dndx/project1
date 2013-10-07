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

    while (fgets(buffer, sizeof(buffer), file))
    {
        buffer[strlen(buffer) - 1] = '\0';
        if (buffer[0] == '\0' || buffer[0] == '#')
        {
            continue;
        }
        if (!strcmp(buffer, "CS229"))
        {
            return 1;
        }
    }
    return 0;
}

struct soundfile cs229_fileinfo(FILE *file)
{
    char buffer[128];
    struct soundfile fileinfo;
    memset(&fileinfo, 0, sizeof(fileinfo));

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

