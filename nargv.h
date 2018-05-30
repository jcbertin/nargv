#ifndef _NARGV_H
#define _NARGV_H

#include <stddef.h>

typedef struct NARGV {
    char **argv, *data, *error_message;
    size_t argc, data_length;
    off_t error_index;
    int error_code;
} NARGV;

void nargv_free(NARGV* props);
void nargv_ifs(const char *nifs);
NARGV *nargv_parse(const char *input);

#endif /* _NARGV_H */

