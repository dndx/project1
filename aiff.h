#include "sound.h"

int is_aiff_file(FILE *file);
struct soundfile aiff_fileinfo(FILE *file);
int aiff_to_cs229(FILE *file, FILE *ofile, struct soundfile *info);
