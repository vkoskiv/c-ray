//
//  errorhandler.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 14/09/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "errorhandler.h"

//Prototype
void print_trace(void);

void errorHandler(renderError error) {
    switch (error) {
        case threadMallocFailed:
            printf("Failed to allocate memory for thread arguments, aborting.\n");
            print_trace();
            break;
        case imageMallocFailed:
            printf("Failed to allocate memory for image data, aborting.\n");
            print_trace();
            break;
        case sceneBuildFailed:
            printf("Scene builder failed. (Missing scene file.) Aborting.\n");
            print_trace();
            break;
        case invalidThreadCount:
            printf("Render sections and thread count are not even. Render will be corrupted (likely). Aborting.\n");
            print_trace();
            break;
        case threadFrozen:
            printf("A thread has frozen. Aborting.\n");
            print_trace();
            break;
        case defaultError:
            printf("Something went wrong. Aborting.\n");
            print_trace();
            break;
        default:
            printf("Something went wrong. Aborting.\n");
            print_trace();
            break;
    }
}

void print_trace(void) {
    void *array[10];
    size_t size;
    char **strings;
    size_t i;
    
    size = backtrace (array, 10);
    strings = backtrace_symbols (array, (int)size);
    
    printf ("Obtained %zd stack frames.\n", size);
    
    for (i = 0; i < size; i++)
        printf ("%s\n", strings[i]);
    
    free (strings);
    abort();
}