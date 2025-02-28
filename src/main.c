#include <stdio.h>
#include "../include/toralizer.h"

int main(int argc, char *argv[]) {
    printf("Toralizer v0.1\n");
    
    toralizer_init();
    
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            toralizer_process(argv[i]);
        }
    } else {
        printf("Usage: %s [input]\n", argv[0]);
    }
    
    toralizer_cleanup();
    
    return 0;
}