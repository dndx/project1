#include "sound.h"

int is_cs229_file(FILE *file);
struct soundfile cs229_fileinfo(FILE *file);
int cs229_enumerate(FILE *file, const struct soundfile *info, sample_cb cb, void *data);

