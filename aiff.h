#include "sound.h"

int is_aiff_file(FILE *file);
struct soundfile aiff_fileinfo(FILE *file);
int aiff_enumerate(FILE *file, const struct soundfile *info, sample_cb cb, void *data);

