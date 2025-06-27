#include "roboticLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/robot"

void escribir_palabra(const char* palabra) {
    int fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("roboticLib: no se pudo abrir /dev/robot");
        return;
    }

    char comando[128];
    snprintf(comando, sizeof(comando), "W|%s\n", palabra);

    if (write(fd, comando, strlen(comando)) < 0) {
        perror("roboticLib: error escribiendo en /dev/robot");
    }

    close(fd);

    // Ejecutar uartSender desde ../driver/
    int status = system("../driver/uartSender");
    if (status == -1) {
        perror("roboticLib: error ejecutando uartSender");
    }
}
