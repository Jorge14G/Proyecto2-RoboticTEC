// nodo_optimizado.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <mpi.h>

#define BUFFER_SIZE 4096
#define MAX_WORD_LEN 10
#define MAX_WORDS 10000
#define SHIFT 3

typedef struct {
    char word[MAX_WORD_LEN];
    int count;
} WordCount;

// Función para descifrar un carácter
char descifrar_caracter(char c) {
    if (c >= 'a' && c <= 'z')
        return ((c - 'a' - SHIFT + 26) % 26) + 'a';
    else if (c >= 'A' && c <= 'Z')
        return ((c - 'A' - SHIFT + 26) % 26) + 'A';
    else
        return c;
}

// Función para agregar/incrementar palabra en el array
void agregar_incremento(WordCount *array, int *n, const char *word) {
    // Ignorar palabras muy cortas (menos de 2 caracteres) para optimizar
    if (strlen(word) < 2) return;
    
    for (int i = 0; i < *n; i++) {
        if (strcmp(array[i].word, word) == 0) {
            array[i].count++;
            return;
        }
    }
    
    if (*n < MAX_WORDS) {
        strncpy(array[*n].word, word, MAX_WORD_LEN - 1);
        array[*n].word[MAX_WORD_LEN - 1] = '\0';
        array[*n].count = 1;
        (*n)++;
    }
}

// Función OPTIMIZADA para procesar segmento con MPI
void procesar_segmento_mpi(char* segmento, int tam_segmento, WordCount* palabras, int* num_palabras) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Dividir trabajo entre procesos MPI de manera más inteligente
    int palabras_estimadas = tam_segmento / 5; // Estimación: 1 palabra cada 5 caracteres
    int chars_por_proceso = tam_segmento / size;
    int resto = tam_segmento % size;
    
    int inicio = rank * chars_por_proceso + (rank < resto ? rank : resto);
    int fin = inicio + chars_por_proceso + (rank < resto ? 1 : 0);
    
    // MEJORADO: Ajustar límites para no cortar palabras (más eficiente)
    if (rank > 0) {
        // Buscar hacia atrás hasta encontrar un separador
        while (inicio > 0 && isalpha(segmento[inicio - 1])) {
            inicio--;
        }
        // Saltar espacios iniciales
        while (inicio < tam_segmento && isspace(segmento[inicio])) {
            inicio++;
        }
    }
    
    if (rank < size - 1) {
        // Buscar hacia adelante hasta completar la palabra actual
        while (fin < tam_segmento && isalpha(segmento[fin])) {
            fin++;
        }
    }
    
    printf("Proceso MPI %d procesando caracteres %d-%d (%d chars)\n", 
           rank, inicio, fin - 1, fin - inicio);
    
    // Procesar la porción asignada de manera más eficiente
    WordCount palabras_locales[MAX_WORDS];
    int num_palabras_locales = 0;
    
    char buffer[MAX_WORD_LEN];
    int index = 0;
    int palabras_procesadas = 0;
    
    for (int i = inicio; i < fin; i++) {
        char c_descifrado = descifrar_caracter(segmento[i]);
        
        if (isalpha(c_descifrado)) {
            if (index < MAX_WORD_LEN - 1) {
                buffer[index++] = tolower(c_descifrado);
            }
        } else if (isspace(c_descifrado) || ispunct(c_descifrado)) {
            if (index > 0) {
                buffer[index] = '\0';
                agregar_incremento(palabras_locales, &num_palabras_locales, buffer);
                palabras_procesadas++;
                index = 0;
            }
        }
    }
    
    // Procesar última palabra si existe
    if (index > 0) {
        buffer[index] = '\0';
        agregar_incremento(palabras_locales, &num_palabras_locales, buffer);
        palabras_procesadas++;
    }
    
    printf("Proceso MPI %d: %d palabras procesadas, %d únicas encontradas\n", 
           rank, palabras_procesadas, num_palabras_locales);
    
    // Recopilar resultados en el proceso 0 (OPTIMIZADO)
    if (rank == 0) {
        // Copiar palabras locales del proceso 0
        for (int i = 0; i < num_palabras_locales; i++) {
            strcpy(palabras[i].word, palabras_locales[i].word);
            palabras[i].count = palabras_locales[i].count;
        }
        *num_palabras = num_palabras_locales;
        
        // Recibir de otros procesos
        for (int src = 1; src < size; src++) {
            int num_recibidas;
            MPI_Recv(&num_recibidas, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            if (num_recibidas > 0) {
                WordCount palabras_recibidas[MAX_WORDS];
                MPI_Recv(palabras_recibidas, num_recibidas * sizeof(WordCount), MPI_BYTE, 
                        src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Combinar con las palabras existentes de manera más eficiente
                for (int i = 0; i < num_recibidas; i++) {
                    int encontrado = 0;
                    // Buscar si ya existe
                    for (int j = 0; j < *num_palabras; j++) {
                        if (strcmp(palabras[j].word, palabras_recibidas[i].word) == 0) {
                            palabras[j].count += palabras_recibidas[i].count;
                            encontrado = 1;
                            break;
                        }
                    }
                    
                    // Si no existe y hay espacio, agregarla
                    if (!encontrado && *num_palabras < MAX_WORDS) {
                        strcpy(palabras[*num_palabras].word, palabras_recibidas[i].word);
                        palabras[*num_palabras].count = palabras_recibidas[i].count;
                        (*num_palabras)++;
                    }
                }
            }
        }
        
        printf("Proceso 0: Combinación completada - %d palabras únicas totales\n", *num_palabras);
        
    } else {
        // Enviar al proceso 0
        MPI_Send(&num_palabras_locales, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        if (num_palabras_locales > 0) {
            MPI_Send(palabras_locales, num_palabras_locales * sizeof(WordCount), MPI_BYTE, 
                    0, 1, MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <puerto>\n", argv[0]);
        return 1;
    }
    
    // Inicializar MPI
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int puerto = atoi(argv[1]);
    char* segmento = NULL;
    int tam_segmento = 0;
    
    if (rank == 0) {
        printf("Nodo OPTIMIZADO iniciado en puerto %d con %d procesos MPI\n", puerto, size);
        
        // Crear socket servidor
        int servidor_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (servidor_sock < 0) {
            perror("Error creando socket");
            MPI_Finalize();
            return 1;
        }
        
        // Configurar reutilización de puerto
        int opt = 1;
        setsockopt(servidor_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in direccion;
        direccion.sin_family = AF_INET;
        direccion.sin_addr.s_addr = INADDR_ANY;
        direccion.sin_port = htons(puerto);
        
        if (bind(servidor_sock, (struct sockaddr*)&direccion, sizeof(direccion)) < 0) {
            perror("Error en bind");
            close(servidor_sock);
            MPI_Finalize();
            return 1;
        }
        
        if (listen(servidor_sock, 1) < 0) {
            perror("Error en listen");
            close(servidor_sock);
            MPI_Finalize();
            return 1;
        }
        
        printf("Nodo escuchando en puerto %d...\n", puerto);
        
        // Aceptar conexión del servidor
        int cliente_sock = accept(servidor_sock, NULL, NULL);
        if (cliente_sock < 0) {
            perror("Error aceptando conexión");
            close(servidor_sock);
            MPI_Finalize();
            return 1;
        }
        
        printf("Conexión aceptada del servidor principal\n");
        
        // Recibir tamaño del segmento
        recv(cliente_sock, &tam_segmento, sizeof(int), 0);
        
        // Recibir segmento
        segmento = malloc(tam_segmento + 1);
        recv(cliente_sock, segmento, tam_segmento, 0);
        segmento[tam_segmento] = '\0';
        
        printf("Recibido segmento de %d caracteres para procesamiento distribuido\n", tam_segmento);
        
        // Broadcast del tamaño a todos los procesos
        MPI_Bcast(&tam_segmento, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        // Broadcast del segmento a todos los procesos
        MPI_Bcast(segmento, tam_segmento + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
        
        // Procesar con MPI OPTIMIZADO
        WordCount palabras[MAX_WORDS];
        int num_palabras = 0;
        
        double tiempo_inicio = MPI_Wtime();
        procesar_segmento_mpi(segmento, tam_segmento, palabras, &num_palabras);
        double tiempo_fin = MPI_Wtime();
        
        printf("Procesamiento MPI completado en %.3f segundos: %d palabras únicas encontradas\n", 
               tiempo_fin - tiempo_inicio, num_palabras);
        
        // Enviar resultados de vuelta al servidor
        send(cliente_sock, &num_palabras, sizeof(int), 0);
        if (num_palabras > 0) {
            send(cliente_sock, palabras, sizeof(WordCount) * num_palabras, 0);
        }
        
        printf("Resultados enviados al servidor principal\n");
        
        close(cliente_sock);
        close(servidor_sock);
        free(segmento);
        
    } else {
        // Los procesos MPI no-rank-0 reciben los datos via broadcast
        
        // Recibir tamaño del segmento
        MPI_Bcast(&tam_segmento, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        // Allocar memoria y recibir segmento
        segmento = malloc(tam_segmento + 1);
        MPI_Bcast(segmento, tam_segmento + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
        
        printf("Proceso MPI %d recibió segmento de %d caracteres\n", rank, tam_segmento);
        
        // Procesar con MPI (solo participar, no manejar resultados)
        WordCount palabras[MAX_WORDS];
        int num_palabras = 0;
        procesar_segmento_mpi(segmento, tam_segmento, palabras, &num_palabras);
        
        free(segmento);
    }
    
    // Sincronizar todos los procesos antes de finalizar
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Finalizar MPI
    MPI_Finalize();
    return 0;
}