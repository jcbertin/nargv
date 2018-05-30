//============================================================================
// Name         : nargv.c
// Author       : Triston J. Taylor (pc.wiz.tt@gmail.com)
// Version      : 2.0
// Copyright    : (C) Triston J. Taylor 2012. All Rights Reserved
//============================================================================
// Description  : New Argument Vectors
//
//                Parse a shell style string into an argument vector table.
//
//============================================================================
// Public Calls : nargv_free, nargv_parse, nargv_ifs
// Public Type Structs: NARGV
// Public Data: None

//============================================================================

#include <stddef.h>
#include <stdlib.h>

#include "nargv.h"

static const char * const NARGV_DEFAULT_IFS = " \t\n";
static const char *NARGV_IFS = NARGV_DEFAULT_IFS;

void nargv_free(NARGV* props) {
    free(props->data); free(props->argv);
    free(props);
}

void nargv_ifs(const char *nifs) {
    if (! nifs) {
        if (NARGV_IFS != NARGV_DEFAULT_IFS)
            free((void *)NARGV_IFS);
        NARGV_IFS = NARGV_DEFAULT_IFS;
    } else if (NARGV_IFS != nifs) {
        if (NARGV_IFS != NARGV_DEFAULT_IFS)
            free((void *)NARGV_IFS);
        NARGV_IFS = strdup(nifs);
    }
}

static int nargv_field_seperator(char seperator) {
    const char *list = NARGV_IFS;
    if (seperator) {
        while (*list) {
            if (*list++ == seperator) return 1;
        }
        return 0;
    }
    return 1; // null is always a field seperator
}

NARGV *nargv_parse(const char *input) {

    NARGV *nvp = calloc(1, sizeof(NARGV));

    if (! input) {
        nvp->error_code = 1;
        nvp->error_message = "cannot parse null pointer";
        return nvp;
    }

    /* Get the input length */
    size_t input_length = strlen(input);
    if (! input_length) {
        nvp->error_code = 2;
        nvp->error_message = "cannot parse empty input";
        return nvp;
    }

    int composing_argument = 0;
    off_t quote = 0;
    size_t index;
    char look_ahead;

    // FIRST PASS
    // discover how many elements we have, and discover how large a data buffer we need
    for (index = 0; index <= input_length; index++) {

        if (nargv_field_seperator(input[index])) {
            if (composing_argument) {
                // close the argument
                composing_argument = 0;
                nvp->data_length++;
            }
            continue;
        }

        if (! composing_argument) {
            nvp->argc++;
            composing_argument = 1;
        }        

        switch (input[index]) {

            /* back slash */
            case '\\':
                // If the sequence is not \' or \" or seperator copy the back slash, and
                // the data
                look_ahead = *(input+index+1);
                if (look_ahead == '"' || look_ahead == '\'' || nargv_field_seperator(look_ahead)) {
                    index++;    
                } else {
                    index++;
                    nvp->data_length++;
                }
            break;

            /* double quote */
            case '"':
                quote = index;
                while (input[++index] != '"') {
                    switch (input[index]) {
                        case '\0':
                            nvp->error_index = quote + 1;
                            nvp->error_code = 3;
                            nvp->error_message = "unterminated double quote";
                            return nvp;                        
                        case '\\':
                            look_ahead = *(input + index + 1);
                            if (look_ahead == '"') {
                                index++;
                            } else {
                                index++;
                                nvp->data_length++;                                     
                            }
                        break;
                    }
                    nvp->data_length++;
                }

                continue;

            /* single quote */
            case '\'':
                /* copy single quoted data */
                quote = index; // QT used as temp here...
                while (input[++index] != '\'') {
                    if (! input[index]) {
                        // unterminated double quote @ input
                        nvp->error_index = quote + 1;
                        nvp->error_code = 4;
                        nvp->error_message = "unterminated single quote";
                        return nvp;                        
                    }
                    nvp->data_length++;
                }
                continue;

        }

        // "record" the data        
        nvp->data_length++;

    }

    // +1 for extra NULL pointer required by execv() and friends
    nvp->argv = calloc(nvp->argc + 1, sizeof(char *));
    nvp->data = calloc(nvp->data_length, 1);

    // SECOND PASS
    composing_argument = 0;
    quote = 0;

    int data_index = 0;
    int arg_index = 0;

    for (index = 0; index <= input_length; index++) {

        if (nargv_field_seperator(input[index])) {
            if (composing_argument) {
                composing_argument = 0;
                nvp->data[data_index++] = '\0';
            }
            continue;
        }

        if (! composing_argument) {
            nvp->argv[arg_index++] = (nvp->data + data_index);
            composing_argument = 1;
        }        

        switch (input[index]) {

            /* back slash */
            case '\\':
                // If the sequence is not \' or \" or field seperator copy the backslash
                look_ahead = *(input+index+1);
                if (look_ahead == '"' || look_ahead == '\'' || nargv_field_seperator(look_ahead)) {
                    index++;    
                } else {
                    nvp->data[data_index++] = input[index++];
                }
            break;

            /* double quote */
            case '"':
                while (input[++index] != '"') {
                    if (input[index] == '\\') {
                        look_ahead = *(input + index + 1);
                        if (look_ahead == '"') {
                            index++;
                        } else {
                            nvp->data[data_index++] = input[index++];                        
                        }
                    }
                    nvp->data[data_index++] = input[index];
                }                
                continue;

            /* single quote */
            case '\'':
                /* copy single quoted data */
                while (input[++index] != '\'') {
                    nvp->data[data_index++] = input[index];
                } 
                continue;

        }

        // "record" the data        
        nvp->data[data_index++] = input[index];

    }

    return nvp;
}

#ifdef NARGVTEST
#include <stdio.h>
int main(int argc, char *argv[]) {

    size_t arg;
    char line_in[4096];

    while (fgets(line_in, 4096, stdin) == line_in) {
        NARGV *nargv = nargv_parse(line_in);
        if (nargv->error_code) {
            printf("\nnargv parse error: %i: %s: at input column %i\n",
            nargv->error_code, nargv->error_message, nargv->error_index);
        } else {
            printf("\nNARGV Argument Count: %i\n", nargv->argc);
            printf("NARGV Data Length: %i\n\n", nargv->data_length);
            for (arg = 0; arg < nargv->argc; arg++) {
                printf("argument %i: %s\n", arg, nargv->argv[arg]);
            }
        }
        nargv_free(nargv);
    }

}
#endif
