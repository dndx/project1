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
    FILE *file, *ofile;
    struct soundfile fileinfo;

    fprintf(stderr, "Enter the pathname of a input sound file: \n");
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

    fprintf(stderr, "Enter the pathname of the output file: \n");
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

    ofile = fopen(filename, "w");
    if (!file)
    {
        FATAL("Could not open output file");
    }

    puts(DASHES);

    if (is_aiff_file(file))
    {
        LOGI("Input file is AIFF file, converting to CS229...\n");
        fileinfo = aiff_fileinfo(file);

        fprintf(ofile, "CS229\n");
        fprintf(ofile, "Samples %u\n", fileinfo.sample_num);
        fprintf(ofile, "Channels %u\n", fileinfo.channels);
        fprintf(ofile, "BitDepth %u\n", fileinfo.bit_depth);
        fprintf(ofile, "SampleRate %u\n\n", fileinfo.sample_rate);
        fprintf(ofile, "StartData\n");
        
        aiff_to_cs229(file, ofile, &fileinfo);
    }
    else if (is_cs229_file(file))
    {
        LOGI("Input file is CS229 file, converting to AIFF...\n");
        fileinfo = cs229_fileinfo(file);

        cs229_to_aiff(file, ofile, &fileinfo);
    }
    else
    {
        LOGE("Unrecognized file %s, terminating...", filename);
        return EXIT_FAILURE;
    }
    
    LOGI("Done. \n");

    puts(DASHES);

    fclose(file);
    fclose(ofile);

    return EXIT_SUCCESS;
}

