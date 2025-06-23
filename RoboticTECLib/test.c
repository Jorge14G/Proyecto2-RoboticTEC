// RoboticLib/test.c

#include "roboticLib.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <PALABRA>\n", argv[0]);
        return 1;
    }

    escribir_palabra(argv[1]);
    return 0;
}
