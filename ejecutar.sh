#!/bin/bash

# Script para ejecutar el sistema distribuido servidor-nodos con MPI

echo "=== Sistema Distribuido de Procesamiento de Texto ==="
echo "Servidor + 3 Nodos con MPI en localhost"
echo ""

# Verificar si los ejecutables existen
if [[ ! -f "servidor" || ! -f "nodo" || ! -f "cliente" ]]; then
    echo "Los ejecutables no existen. Compilando..."
    make clean
    make all
    if [ $? -ne 0 ]; then
        echo "Error en la compilación. Saliendo."
        exit 1
    fi
fi

# Crear archivo de prueba si no existe
if [ ! -f "entrada.txt" ]; then
    echo "Esta es una prueba para la verificación del cifrado de archivos, además prueba prueba prueba" > entrada.txt
    echo "Archivo de prueba creado: entrada.txt"
fi

# Cifrar archivo
echo "=== Paso 1: Cifrando archivo ==="
./cliente entrada.txt archivo_cifrado.enc
if [ $? -ne 0 ]; then
    echo "Error cifrando archivo"
    exit 1
fi

echo ""
echo "=== Paso 2: Iniciando nodos MPI ==="

# Iniciar nodos en background
echo "Iniciando Nodo 1 (puerto 8081) con 2 procesos MPI..."
mpirun -np 2 ./nodo 8081 &
NODO1_PID=$!

echo "Iniciando Nodo 2 (puerto 8082) con 2 procesos MPI..."
mpirun -np 2 ./nodo 8082 &
NODO2_PID=$!

echo "Iniciando Nodo 3 (puerto 8083) con 2 procesos MPI..."
mpirun -np 2 ./nodo 8083 &
NODO3_PID=$!

# Esperar a que los nodos se inicien
echo "Esperando a que los nodos se inicien..."
sleep 3

echo ""
echo "=== Paso 3: Ejecutando servidor principal ==="
./servidor archivo_cifrado.enc

# Función para limpiar procesos al terminar
cleanup() {
    echo ""
    echo "=== Terminando nodos ==="
    kill $NODO1_PID $NODO2_PID $NODO3_PID 2>/dev/null
    pkill -f "./nodo" 2>/dev/null
    wait 2>/dev/null
    echo "Procesos terminados."
}

# Configurar limpieza al recibir señales
trap cleanup EXIT INT TERM

echo ""
echo "=== Verificando archivos generados ==="
if [ -f "archivo_descifrado.txt" ]; then
    echo "Archivo descifrado generado exitosamente:"
    echo "Contenido:"
    cat archivo_descifrado.txt
else
    echo "No se generó el archivo descifrado"
fi

echo ""
echo "=== Procesamiento completado ==="

# La limpieza se ejecutará automáticamente por el trap