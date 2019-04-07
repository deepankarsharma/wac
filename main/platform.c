#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "util.h"

// Assert calloc
void *acalloc(size_t nmemb, size_t size,  char *name) {
    void *res = calloc(nmemb, size);
    if (nmemb * size == 0) {
        warn("acalloc: %s requests allocating 0 bytes.\n", name);
    } else if (res == NULL) {
        FATAL("Could not allocate %ul bytes for %s", nmemb * size, name);
    }
    return res;
}

// Assert realloc/calloc
void *arecalloc(void *ptr, size_t old_nmemb, size_t nmemb,
                size_t size,  char *name) {
    void *res = realloc(ptr, nmemb * size);
    if (res == NULL) {
        FATAL("Could not allocate %ul bytes for %s", nmemb * size, name);
    }
    // Initialize new memory
    memset(res + old_nmemb * size, 0, (nmemb - old_nmemb) * size);
    return res;
}