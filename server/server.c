#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD_LEN 100
#define SHIFT 3
#define MAX_WORDS 1000

typedef struct {
    char word[MAX_WORD_LEN];
    int count;
} WordCount;

// Declaración de funciones
char descifrarCaracter(char c);
void agregarIncremento(WordCount *array, int *n, const char *word);

char descifrarCaracter(char c) {
    if (c >= 'a' && c <= 'z')
        return ((c - 'a' - SHIFT + 26) % 26) + 'a';
    else if (c >= 'A' && c <= 'Z')
        return ((c - 'A' - SHIFT + 26) % 26) + 'A';
    else
        return c;
}

void agregarIncremento(WordCount *array, int *n, const char *word) {
    for (int i = 0; i < *n; i++) {
        if (strcmp(array[i].word, word) == 0) {
            array[i].count++;
            return;
        }
    }
    if (*n < MAX_WORDS) {
        snprintf(array[*n].word, MAX_WORD_LEN, "%s", word);
        array[*n].count = 1;
        (*n)++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Ejemplo: %s <archivo_cifrado.enc>\n", argv[0]);
        return 1;
    }

    FILE *entrada = fopen(argv[1], "r");
    if (!entrada) {
        perror("Error abriendo archivo cifrado");
        return 1;
    }

    WordCount palabras[MAX_WORDS];
    int total_palabras = 0;

    char buffer[MAX_WORD_LEN];
    int index = 0;
    int c;

    while ((c = fgetc(entrada)) != EOF) {
        char d = descifrarCaracter((char)c);

        if (isalpha(d)) {
            buffer[index++] = tolower(d);
            if (index >= MAX_WORD_LEN - 1) index = 0; // prevención
        } else if (isspace(d) || ispunct(d)) {
            if (index > 0) {
                buffer[index] = '\0';
                agregarIncremento(palabras, &total_palabras, buffer);
                index = 0;
            }
        }
    }

    if (index > 0) {
        buffer[index] = '\0';
        agregarIncremento(palabras, &total_palabras, buffer);
    }

    fclose(entrada);

    // Buscar la palabra más repetida
    int max_idx = 0;
    for (int i = 1; i < total_palabras; i++) {
        if (palabras[i].count > palabras[max_idx].count) {
            max_idx = i;
        }
    }

    printf("Palabra más repetida: \"%s\" (%d veces)\n",
           palabras[max_idx].word, palabras[max_idx].count);

    return 0;
}
