#include "sound.h"

int is_cs229_file(FILE *file);
struct soundfile cs229_fileinfo(FILE *file);
void write_cs229_header(FILE *ofile, const struct soundfile *info);
void write_to_cs229(int *samples, const struct soundfile *info, void *data);
int cs229_enumerate(FILE *file, const struct soundfile *info, sample_cb cb, void *data);

