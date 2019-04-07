#ifndef FS_H
#define FS_H

#include <stdbool.h>

void *acalloc(size_t nmemb, size_t size,  char *name);
void *arecalloc(void *ptr, size_t old_nmemb, size_t nmemb,
                size_t size,  char *name);

int read_file(char *path, char *buf);

// Dynamic lib resolution
bool resolvesym(char *filename, char *symbol, void **val, char **err);

#endif // of FS_H
