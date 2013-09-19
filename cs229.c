#include <stdio.h>
#include <string.h>
#include "cs229.h"

int is_cs229_file(FILE *file)
{
    char buffer[128];

    rewind(file);

    while (fgets(buffer, sizeof(buffer), file))
    {
        buffer[strlen(buffer) - 1] = '\0';
        if (buffer[0] == '\0')
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
