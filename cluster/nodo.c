// nodo_simplificado_corregido.c
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
    if (strlen(word) < 1) return;  // Cambié de 2 a 1 para permitir palabras de 1 letra
    
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

// Función SIMPLIFICADA para encontrar palabras en un rango sin solapamiento
void extraer_palabras_rango(char* segmento, int inicio, int fin, WordCount* palabras, int* num_palabras, int rank) {
    char buffer[MAX_WORD_LEN];
    int index = 0;
    int palabras_encontradas = 0;
    
    // printf("Proceso %d analizando rango [%d,%d): '", rank, inicio, fin);
    for (int i = inicio; i < fin; i++) {
        printf("%c", segmento[i]);
    }
    printf("'\n");
    
    for (int i = inicio; i < fin; i++) {
        char c_descifrado = descifrar_caracter(segmento[i]);
        
        if (isalpha(c_descifrado)) {
            if (index < MAX_WORD_LEN - 1) {
                buffer[index++] = tolower(c_descifrado);
            }
        } else {
            // Encontramos un separador
            if (index > 0) {
                buffer[index] = '\0';
                // printf("Proceso %d encontró palabra: '%s'\n", rank, buffer);
                agregar_incremento(palabras, num_palabras, buffer);
                palabras_encontradas++;
                index = 0;
            }
        }
    }
    
    // Procesar última palabra si el rango termina en el final del segmento
    if (index > 0 && fin == strlen(segmento)) {
        buffer[index] = '\0';
        // printf("Proceso %d encontró palabra final: '%s'\n", rank, buffer);
        agregar_incremento(palabras, num_palabras, buffer);
        palabras_encontradas++;
    }
    
    printf("Proceso %d: %d palabras procesadas, %d únicas\n", rank, palabras_encontradas, *num_palabras);
}

// Función para dividir el trabajo por PALABRAS, no por caracteres
void procesar_segmento_mpi(char* segmento, int tam_segmento, WordCount* palabras, int* num_palabras) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // NUEVO ENFOQUE: Primero extraer TODAS las palabras en el proceso 0
    if (rank == 0) {
        // Extraer todas las palabras del segmento
        char buffer[MAX_WORD_LEN];
        int index = 0;
        char todas_palabras[MAX_WORDS][MAX_WORD_LEN];
        int total_palabras_extraidas = 0;
        
        // printf("Proceso 0: Extrayendo todas las palabras del segmento: '%s'\n", segmento);
        
        for (int i = 0; i < tam_segmento; i++) {
            char c_descifrado = descifrar_caracter(segmento[i]);
            
            if (isalpha(c_descifrado)) {
                if (index < MAX_WORD_LEN - 1) {
                    buffer[index++] = tolower(c_descifrado);
                }
            } else {
                if (index > 0) {
                    buffer[index] = '\0';
                    if (total_palabras_extraidas < MAX_WORDS) {
                        strcpy(todas_palabras[total_palabras_extraidas], buffer);
                        total_palabras_extraidas++;
                        //printf("Extraída palabra %d: '%s'\n", total_palabras_extraidas, buffer);
                    }
                    index = 0;
                }
            }
        }
        
        // Procesar última palabra si existe
        if (index > 0 && total_palabras_extraidas < MAX_WORDS) {
            buffer[index] = '\0';
            strcpy(todas_palabras[total_palabras_extraidas], buffer);
            total_palabras_extraidas++;
            //printf("Extraída palabra final %d: '%s'\n", total_palabras_extraidas, buffer);
        }
        
        printf("Total de palabras extraídas: %d\n", total_palabras_extraidas);
        
        // Broadcast del número total de palabras
        MPI_Bcast(&total_palabras_extraidas, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        // Broadcast de todas las palabras
        MPI_Bcast(todas_palabras, total_palabras_extraidas * MAX_WORD_LEN, MPI_CHAR, 0, MPI_COMM_WORLD);
        
        // Dividir las palabras entre procesos
        int palabras_por_proceso = total_palabras_extraidas / size;
        int resto = total_palabras_extraidas % size;
        
        int inicio = rank * palabras_por_proceso + (rank < resto ? rank : resto);
        int fin = inicio + palabras_por_proceso + (rank < resto ? 1 : 0);
        
        //printf("Proceso %d procesará palabras desde índice %d hasta %d\n", rank, inicio, fin-1);
        
        // Procesar las palabras asignadas al proceso 0
        for (int i = inicio; i < fin; i++) {
            agregar_incremento(palabras, num_palabras, todas_palabras[i]);
        }
        
        printf("Proceso 0 procesó %d palabras, encontró %d únicas\n", fin - inicio, *num_palabras);
        
        // Recibir resultados de otros procesos
        for (int src = 1; src < size; src++) {
            int num_recibidas;
            MPI_Recv(&num_recibidas, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            if (num_recibidas > 0) {
                WordCount palabras_recibidas[MAX_WORDS];
                MPI_Recv(palabras_recibidas, num_recibidas * sizeof(WordCount), MPI_BYTE, 
                        src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Combinar palabras
                for (int i = 0; i < num_recibidas; i++) {
                    int encontrado = 0;
                    for (int j = 0; j < *num_palabras; j++) {
                        if (strcmp(palabras[j].word, palabras_recibidas[i].word) == 0) {
                            palabras[j].count += palabras_recibidas[i].count;
                            encontrado = 1;
                            break;
                        }
                    }
                    
                    if (!encontrado && *num_palabras < MAX_WORDS) {
                        strcpy(palabras[*num_palabras].word, palabras_recibidas[i].word);
                        palabras[*num_palabras].count = palabras_recibidas[i].count;
                        (*num_palabras)++;
                    }
                }
            }
        }
        
    } else {
        // Procesos no-0: recibir datos y procesar su parte
        int total_palabras_extraidas;
        char todas_palabras[MAX_WORDS][MAX_WORD_LEN];
        
        // Recibir número total de palabras
        MPI_Bcast(&total_palabras_extraidas, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        // Recibir todas las palabras
        MPI_Bcast(todas_palabras, total_palabras_extraidas * MAX_WORD_LEN, MPI_CHAR, 0, MPI_COMM_WORLD);
        
        // Calcular rango de palabras para este proceso
        int palabras_por_proceso = total_palabras_extraidas / size;
        int resto = total_palabras_extraidas % size;
        
        int inicio = rank * palabras_por_proceso + (rank < resto ? rank : resto);
        int fin = inicio + palabras_por_proceso + (rank < resto ? 1 : 0);
        
        // printf("Proceso %d procesará palabras desde índice %d hasta %d\n", rank, inicio, fin-1);
        
        // Procesar palabras asignadas
        WordCount palabras_locales[MAX_WORDS];
        int num_palabras_locales = 0;
        
        for (int i = inicio; i < fin; i++) {
            agregar_incremento(palabras_locales, &num_palabras_locales, todas_palabras[i]);
        }
        
        //  printf("Proceso %d procesó %d palabras, encontró %d únicas\n", 
        //        fin - inicio, num_palabras_locales);
        
        // Enviar resultados al proceso 0
        MPI_Send(&num_palabras_locales, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        if (num_palabras_locales > 0) {
            MPI_Send(palabras_locales, num_palabras_locales * sizeof(WordCount), MPI_BYTE, 
                    0, 1, MPI_COMM_WORLD);
        }
    }
    
    if (rank == 0) {
        // printf("Resultado final del nodo: ");
        // for (int i = 0; i < *num_palabras; i++) {
        //     printf("%s(%d) ", palabras[i].word, palabras[i].count);
        // }
        printf("\n");
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
        printf("Nodo SIMPLIFICADO iniciado en puerto %d con %d procesos MPI\n", puerto, size);
        
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
        
        printf("===== RECIBIDO SEGMENTO =====\n");
        printf("Tamaño: %d caracteres\n", tam_segmento);
        //printf("Contenido cifrado: '%s'\n", segmento);
        
        // // Mostrar contenido descifrado para debug
        // printf("Contenido descifrado: '");
        // for (int i = 0; i < tam_segmento; i++) {
        //     printf("%c", descifrar_caracter(segmento[i]));
        // }
        printf("'\n");
        printf("=============================\n");
        
        // Procesar con MPI SIMPLIFICADO
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
        // Los procesos MPI no-rank-0 solo participan en el procesamiento
        WordCount palabras[MAX_WORDS];
        int num_palabras = 0;
        procesar_segmento_mpi(segmento, tam_segmento, palabras, &num_palabras);
    }
    
    // Sincronizar todos los procesos antes de finalizar
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Finalizar MPI
    MPI_Finalize();
    return 0;
}
