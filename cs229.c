#include <stdio.h>
#include <string.h>
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

int cs229_to_aiff(FILE *file, FILE *ofile, struct soundfile *info)
{
    unsigned int temp_32;
    unsigned short temp_16;
    char buffer[128];

    fputs("FORM", ofile);
    
    temp_32 = 4 + 4 + 4 + 18 + 4 + 4 + 4 + 4
              + (info->bit_depth / 8) * info->channels * info-> sample_num;
    temp_32 = htonl(temp_32); /* File SIze */
    fwrite(&temp_32, 4, 1, ofile);

    fputs("AIFFCOMM", ofile);

    temp_32 = htonl(18); /* COMM Chunk size */
    fwrite(&temp_32, 4, 1, ofile);

    temp_16 = htons(info->channels); /* NumChannels  */
    fwrite(&temp_16, 2, 1, ofile);

    temp_32 = htonl(info->sample_num); /* NumSampleFrames */
    fwrite(&temp_32, 4, 1, ofile);

    temp_16 = htons(info->bit_depth); /* SampleSize */
    fwrite(&temp_16, 2, 1, ofile);

    unsigned long fraction;
    fraction = info->sample_rate;
    int i;

    for (i=63; i>=0; i--)
    {
        if (fraction & (0x01UL << i)) /* Found the integer */
            break; 
    }

    fraction <<= 63 - i;
    fraction = byte_swap_64(fraction);

    temp_16 = htons(i + 16383);

    fwrite(&temp_16, 2, 1, ofile);
    fwrite(&fraction, 8, 1, ofile);

    fputs("SSND", ofile);

    temp_32 = (info->bit_depth / 8) * info->channels * info-> sample_num + 8;
    temp_32 = htonl(temp_32);
    /* Chunk size */
    fwrite(&temp_32, 4, 1, ofile);

    temp_32 = 0;
    fwrite(&temp_32, 4, 1, ofile); /* Offset */
    fwrite(&temp_32, 4, 1, ofile); /* BlockSize */

    rewind(file);
    int start_convert = 0;
    int data[info->channels];

    while (fgets(buffer, sizeof(buffer), file))
    {
        buffer[strlen(buffer) - 1] = '\0';

        if (buffer[0] == '\0' || buffer[0] == '#')
        {
            continue;
        }

        if (start_convert)
        {
            for (i=0; i<info->channels; i++)
            {
                sscanf(buffer, "%d", &data[i]);
                data[i] <<= info->bit_depth;
                data[i] = htonl(data[i]);
                fwrite(&data[i], info->bit_depth / 8, 1, ofile);
            }
        }

        if (!strcmp(buffer, "StartData"))
        {
            start_convert = 1;
        }
    }


    return 0;
}

