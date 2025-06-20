//cliente/client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHIFT 3  // Valor del cifrado César

char cifrarCaracter(char c) {
    if (c >= 'a' && c <= 'z')
        return ((c - 'a' + SHIFT) % 26) + 'a';
    else if (c >= 'A' && c <= 'Z')
        return ((c - 'A' + SHIFT) % 26) + 'A';
    else
        return c;  // No se debe cifrar
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Ejemplo: %s <archivo_entrada.txt> <archivo_salida.enc>\n", argv[0]);
        return 1;
    }

    FILE *entrada = fopen(argv[1], "r");
    if (!entrada) {
        perror("Error abriendo archivo de entrada");
        return 1;
    }

    FILE *salida = fopen(argv[2], "w");
    if (!salida) {
        perror("Error creando archivo de salida");
        fclose(entrada);
        return 1;
    }

    int c;
    while ((c = fgetc(entrada)) != EOF) {
        fputc(cifrarCaracter(c), salida);
    }

    fclose(entrada);
    fclose(salida);

    printf("Archivo cifrado exitosamente en %s\n", argv[2]);
    return 0;
}

