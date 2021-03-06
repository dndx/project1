#include "sound.h"

int is_aiff_file(FILE *file);
struct soundfile aiff_fileinfo(FILE *file);
void write_aiff_header(FILE *ofile, const struct soundfile *info);
void write_to_aiff(int *samples, const struct soundfile *info, void *data);
int aiff_enumerate(FILE *file, const struct soundfile *info, sample_cb cb, void *data);

