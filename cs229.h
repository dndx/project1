#include "sound.h"

int is_cs229_file(FILE *file);
struct soundfile cs229_fileinfo(FILE *file);
int cs229_to_aiff(FILE *file, FILE *ofile, struct soundfile *info);
