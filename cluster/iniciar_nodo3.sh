#!/bin/bash
echo "========================================"
echo "🔵 NODO 3 - Puerto 8083"
echo "========================================"
echo "Iniciando nodo 3 con 2 procesos MPI..."
echo "Esperando conexión del servidor..."
echo ""

# Verificar que el ejecutable existe
if [ ! -f "./nodo" ]; then
    echo "❌ Error: Ejecutable 'nodo' no encontrado"
    echo "   Ejecuta 'make all' primero"
    exit 1
fi

# Ejecutar nodo con MPI
mpirun -np 2 ./nodo 8083

echo ""
echo "🔴 Nodo 3 terminado"