#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    int ret;
    ret = validargs(argc, argv);
    debug("global_options %d\n", global_options);

    if(ret == -1){
        debug("EXIT_FAILURE\n");
        USAGE(*argv, EXIT_FAILURE);
    }
    else if(ret == 0){
        // help
        if(global_options == 1){
            USAGE(*argv, EXIT_SUCCESS);
        }
        //serializ
        else if(global_options == 2){
            int status = serialize();

            if(status == 0){
                debug("ZUCC\n");
                USAGE(*argv, EXIT_SUCCESS);
            }else{
                debug("F\n");
                USAGE(*argv, EXIT_FAILURE);
            }
        }
        // deserialization
        else if(global_options == 4 || global_options == 12){
            int status = deserialize();

            if(status == 0){
                debug("ZUCC\n");
                USAGE(*argv, EXIT_SUCCESS);
            }else{
                debug("F\n");
                USAGE(*argv, EXIT_FAILURE);
            }
        }
        else{
            debug("Something went wrong 1");
            USAGE(*argv, EXIT_FAILURE);
        }
            // USAGE(*argv, EXIT_SUCCESS);

    }
    else{
        debug("Something went wrong 2");
        USAGE(*argv, EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
