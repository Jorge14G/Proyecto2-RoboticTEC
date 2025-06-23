// driver/uartSender.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#define ROBOT_DEVICE "/dev/robot"
#define UART_DEVICE  "/dev/ttyACM0"

int main() {
    char buffer[128] = {0};

    // Abrir /dev/robot
    int robot_fd = open(ROBOT_DEVICE, O_RDONLY);
    if (robot_fd < 0) {
        perror("Error abriendo /dev/robot");
        return 1;
    }

    // Leer mensaje del driver
    ssize_t bytes_read = read(robot_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        perror("Error leyendo desde /dev/robot");
        close(robot_fd);
        return 1;
    }
    buffer[bytes_read] = '\0';
    close(robot_fd);

    // Abrir /dev/ttyACM0
    int uart_fd = open(UART_DEVICE, O_WRONLY | O_NOCTTY);
    if (uart_fd < 0) {
        perror("Error abriendo /dev/ttyACM0");
        return 1;
    }

    // Configurar UART
    struct termios tty;
    if (tcgetattr(uart_fd, &tty) != 0) {
        perror("Error obteniendo configuración UART");
        close(uart_fd);
        return 1;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;  // 8 bits
    tty.c_iflag = 0;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    tty.c_cflag |= (CLOCAL | CREAD);

    tcsetattr(uart_fd, TCSANOW, &tty);

    // Enviar mensaje a la Pico
    ssize_t written = write(uart_fd, buffer, strlen(buffer));
    if (written < 0) {
        perror("Error escribiendo en /dev/ttyACM0");
        close(uart_fd);
        return 1;
    }

    printf("Mensaje enviado a la Pico: %s\n", buffer);
    close(uart_fd);
    return 0;
}
