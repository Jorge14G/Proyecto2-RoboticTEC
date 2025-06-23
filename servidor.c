// servidor_optimizado.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define PUERTO_SERVIDOR 8080
#define PUERTO_NODO1 8081
#define PUERTO_NODO2 8082
#define PUERTO_NODO3 8083
#define MAX_NODOS 3
#define BUFFER_SIZE 4096
#define MAX_WORD_LEN 10
#define MAX_WORDS 10000
#define SHIFT 3
#define TOP_RESULTS 10  // Solo mostrar top 10 resultados

typedef struct {
    char word[MAX_WORD_LEN];
    int count;
} WordCount;

typedef struct {
    int puerto;
    char *segmento;
    int tam_segmento;
    WordCount resultado[MAX_WORDS];
    int num_palabras;
} InfoNodo;

// Función para conectar con un nodo
int conectar_nodo(int puerto) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creando socket");
        return -1;
    }
    
    struct sockaddr_in direccion;
    direccion.sin_family = AF_INET;
    direccion.sin_port = htons(puerto);
    direccion.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Intentar conectar varias veces (los nodos pueden tardar en iniciarse)
    int intentos = 0;
    while (connect(sock, (struct sockaddr*)&direccion, sizeof(direccion)) < 0 && intentos < 10) {
        printf("Intentando conectar con nodo en puerto %d... (intento %d)\n", puerto, intentos + 1);
        sleep(1);
        intentos++;
    }
    
    if (intentos >= 10) {
        printf("No se pudo conectar con nodo en puerto %d\n", puerto);
        close(sock);
        return -1;
    }
    
    printf("Conectado con nodo en puerto %d\n", puerto);
    return sock;
}

// Función para enviar segmento a un nodo y recibir resultado
void* procesar_nodo(void* arg) {
    InfoNodo* info = (InfoNodo*)arg;
    
    int sock = conectar_nodo(info->puerto);
    if (sock < 0) {
        info->num_palabras = 0;
        return NULL;
    }
    
    // Enviar tamaño del segmento
    send(sock, &info->tam_segmento, sizeof(int), 0);
    
    // Enviar segmento
    send(sock, info->segmento, info->tam_segmento, 0);
    
    // Recibir número de palabras encontradas
    recv(sock, &info->num_palabras, sizeof(int), 0);
    
    // Recibir array de palabras
    if (info->num_palabras > 0) {
        recv(sock, info->resultado, sizeof(WordCount) * info->num_palabras, 0);
    }
    
    printf("Nodo puerto %d procesó %d palabras únicas\n", info->puerto, info->num_palabras);
    
    close(sock);
    return NULL;
}

// Función MEJORADA para dividir el texto por palabras completas
void dividir_texto_por_palabras(char* texto, int tam_total, char** segmentos, int* tam_segmentos) {
    // Encontrar aproximadamente dónde dividir
    int tam_aproximado = tam_total / MAX_NODOS;
    int posiciones[MAX_NODOS + 1];
    posiciones[0] = 0;
    posiciones[MAX_NODOS] = tam_total;
    
    // Encontrar puntos de división que no corten palabras
    for (int i = 1; i < MAX_NODOS; i++) {
        int pos_inicial = i * tam_aproximado;
        
        // Buscar hacia atrás hasta encontrar un espacio o inicio
        while (pos_inicial > posiciones[i-1] && !isspace(texto[pos_inicial]) && pos_inicial > 0) {
            pos_inicial--;
        }
        
        // Si llegamos al inicio del segmento anterior, buscar hacia adelante
        if (pos_inicial <= posiciones[i-1]) {
            pos_inicial = i * tam_aproximado;
            while (pos_inicial < tam_total && !isspace(texto[pos_inicial])) {
                pos_inicial++;
            }
        }
        
        posiciones[i] = pos_inicial;
    }
    
    // Crear los segmentos
    for (int i = 0; i < MAX_NODOS; i++) {
        int inicio = posiciones[i];
        int fin = posiciones[i + 1];
        int tam_segmento = fin - inicio;
        
        segmentos[i] = malloc(tam_segmento + 1);
        memcpy(segmentos[i], texto + inicio, tam_segmento);
        segmentos[i][tam_segmento] = '\0';
        tam_segmentos[i] = tam_segmento;
        
        printf("Segmento %d: posición %d-%d (%d caracteres)\n", i + 1, inicio, fin-1, tam_segmento);
    }
}

// Función para combinar resultados de todos los nodos
void combinar_resultados(InfoNodo nodos[MAX_NODOS], WordCount* resultado_final, int* total_palabras) {
    *total_palabras = 0;
    
    for (int i = 0; i < MAX_NODOS; i++) {
        for (int j = 0; j < nodos[i].num_palabras; j++) {
            char* palabra = nodos[i].resultado[j].word;
            int count = nodos[i].resultado[j].count;
            
            // Buscar si la palabra ya existe en el resultado final
            int encontrado = 0;
            for (int k = 0; k < *total_palabras; k++) {
                if (strcmp(resultado_final[k].word, palabra) == 0) {
                    resultado_final[k].count += count;
                    encontrado = 1;
                    break;
                }
            }
            
            // Si no existe, agregarla
            if (!encontrado && *total_palabras < MAX_WORDS) {
                strcpy(resultado_final[*total_palabras].word, palabra);
                resultado_final[*total_palabras].count = count;
                (*total_palabras)++;
            }
        }
    }
}

// Función para ordenar resultados por frecuencia (mayor a menor)
void ordenar_por_frecuencia(WordCount* palabras, int num_palabras) {
    for (int i = 0; i < num_palabras - 1; i++) {
        for (int j = 0; j < num_palabras - i - 1; j++) {
            if (palabras[j].count < palabras[j + 1].count) {
                WordCount temp = palabras[j];
                palabras[j] = palabras[j + 1];
                palabras[j + 1] = temp;
            }
        }
    }
}

// Función para guardar archivo descifrado
void guardar_archivo_descifrado(char* texto_cifrado, int tam_texto, const char* nombre_archivo) {
    FILE* archivo = fopen(nombre_archivo, "w");
    if (!archivo) {
        perror("Error creando archivo descifrado");
        return;
    }
    
    for (int i = 0; i < tam_texto; i++) {
        char c = texto_cifrado[i];
        char descifrado;
        
        if (c >= 'a' && c <= 'z')
            descifrado = ((c - 'a' - SHIFT + 26) % 26) + 'a';
        else if (c >= 'A' && c <= 'Z')
            descifrado = ((c - 'A' - SHIFT + 26) % 26) + 'A';
        else
            descifrado = c;
            
        fputc(descifrado, archivo);
    }
    
    fclose(archivo);
    printf("Archivo descifrado guardado como: %s\n", nombre_archivo);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <archivo_cifrado>\n", argv[0]);
        return 1;
    }
    
    // Leer archivo cifrado
    FILE* archivo = fopen(argv[1], "r");
    if (!archivo) {
        perror("Error abriendo archivo cifrado");
        return 1;
    }
    
    // Obtener tamaño del archivo
    fseek(archivo, 0, SEEK_END);
    int tam_archivo = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);
    
    // Leer contenido
    char* contenido = malloc(tam_archivo + 1);
    fread(contenido, 1, tam_archivo, archivo);
    contenido[tam_archivo] = '\0';
    fclose(archivo);
    
    printf("Archivo leído: %d caracteres\n", tam_archivo);
    printf("Contenido cifrado: %.100s%s\n", contenido, tam_archivo > 100 ? "..." : "");
    
    // Guardar archivo descifrado
    guardar_archivo_descifrado(contenido, tam_archivo, "archivo_descifrado.txt");
    
    // Dividir en segmentos POR PALABRAS COMPLETAS
    char* segmentos[MAX_NODOS];
    int tam_segmentos[MAX_NODOS];
    dividir_texto_por_palabras(contenido, tam_archivo, segmentos, tam_segmentos);
    
    // Configurar información de nodos
    InfoNodo nodos[MAX_NODOS];
    int puertos[] = {PUERTO_NODO1, PUERTO_NODO2, PUERTO_NODO3};
    
    for (int i = 0; i < MAX_NODOS; i++) {
        nodos[i].puerto = puertos[i];
        nodos[i].segmento = segmentos[i];
        nodos[i].tam_segmento = tam_segmentos[i];
        nodos[i].num_palabras = 0;
    }
    
    // Crear threads para procesar en paralelo
    pthread_t threads[MAX_NODOS];
    printf("\nIniciando procesamiento distribuido...\n");
    
    for (int i = 0; i < MAX_NODOS; i++) {
        if (pthread_create(&threads[i], NULL, procesar_nodo, &nodos[i]) != 0) {
            perror("Error creando thread");
            return 1;
        }
    }
    
    // Esperar a que terminen todos los threads
    for (int i = 0; i < MAX_NODOS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Combinar resultados
    WordCount resultado_final[MAX_WORDS];
    int total_palabras;
    combinar_resultados(nodos, resultado_final, &total_palabras);
    
    // ORDENAR por frecuencia
    ordenar_por_frecuencia(resultado_final, total_palabras);
    
    // Mostrar resultados OPTIMIZADOS
    if (total_palabras > 0) {
        printf("\n=== RESULTADO FINAL ===\n");
        printf("🏆 PALABRA MÁS REPETIDA: \"%s\" (%d veces)\n", 
               resultado_final[0].word, resultado_final[0].count);
        
        // Mostrar solo TOP resultados para textos grandes
        int resultados_a_mostrar = (total_palabras > TOP_RESULTS) ? TOP_RESULTS : total_palabras;
        
        printf("\n📊 TOP %d PALABRAS MÁS FRECUENTES:\n", resultados_a_mostrar);
        for (int i = 0; i < resultados_a_mostrar; i++) {
            printf("  %2d. %-15s: %d veces\n", i+1, resultado_final[i].word, resultado_final[i].count);
        }
        
        if (total_palabras > TOP_RESULTS) {
            printf("  ... y %d palabras más (total: %d palabras únicas)\n", 
                   total_palabras - TOP_RESULTS, total_palabras);
        }
        
        printf("\n📈 ESTADÍSTICAS:\n");
        printf("  • Total palabras únicas: %d\n", total_palabras);
        printf("  • Palabra más frecuente: %s (%d apariciones)\n", 
               resultado_final[0].word, resultado_final[0].count);
        if (total_palabras > 1) {
            printf("  • Palabra menos frecuente: %s (%d apariciones)\n", 
                   resultado_final[total_palabras-1].word, resultado_final[total_palabras-1].count);
        }
    } else {
        printf("No se encontraron palabras en el texto.\n");
    }
    
    // Liberar memoria
    for (int i = 0; i < MAX_NODOS; i++) {
        free(segmentos[i]);
    }
    free(contenido);
    
    printf("\nProcesamiento completado exitosamente.\n");
    return 0;
}