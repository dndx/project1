#include <stdio.h>
#include <string.h>
#include <math.h>
#include "utils.h"
#include "cs229.h"
#include "aiff.h"

#define DASHES "------------------------------------------------------------"

int main(int argc, char *argv[])
{
    char filename[128];
    char *nl;
    FILE *file;
    struct soundfile fileinfo;

    fprintf(stderr, "Enter the pathname of a sound file: \n");
    if (!fgets(filename, sizeof(filename), stdin))
    {
        FATAL("fgets failed");
        return 1;
    }
    nl = strchr(filename, '\n');
    if (nl)
    {
        *nl = '\0';
    }

    file = fopen(filename, "r");
    if (!file)
    {
        FATAL("Could not open file");
    }

    puts(DASHES);

    printf("%12s %s\n", "Filename:", filename);

    if (is_cs229_file(file))
    {
        printf("%12s %s\n", "Format:", "CS229");
        fileinfo = cs229_fileinfo(file);
    }
    else if (is_aiff_file(file))
    {
        printf("%12s %s\n", "Format:", "AIFF");
        fileinfo = aiff_fileinfo(file);
    }
    else
        FATAL("Unknown file format");


    printf("%12s %d\n", "Sample Rate:", fileinfo.sample_rate);
    printf("%12s %hd\n", "Bit Depth:", fileinfo.bit_depth);
    printf("%12s %hd\n", "Channels:", fileinfo.channels);
    printf("%12s %d\n", "Samples:", fileinfo.sample_num);

    double duration = (double)fileinfo.sample_num / fileinfo.sample_rate;

    printf("%12s %.0f:%02.0f:%05.2f\n", "Duration:", floor(duration / 3600), floor(fmod(duration, 3600) / 60), fmod(fmod(duration, 3600), 60));

    puts(DASHES);

    fclose(file);

    /*
    if (is_cs229_file(file))
    {
        LOGI("CS229 File!");
    }
    */
    return EXIT_SUCCESS;
}

