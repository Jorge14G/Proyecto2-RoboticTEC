#!/bin/bash
echo "========================================"
echo "🟢 NODO 1 - Puerto 8081"  
echo "========================================"
echo "Iniciando nodo 1 con 2 procesos MPI..."
echo "Esperando conexión del servidor..."
echo ""

# Verificar que el ejecutable existe
if [ ! -f "./nodo" ]; then
    echo "❌ Error: Ejecutable 'nodo' no encontrado"
    echo "   Ejecuta 'make all' primero"
    exit 1
fi

# Ejecutar nodo con MPI
mpirun -np 2 ./nodo 8081

echo ""
echo "🔴 Nodo 1 terminado"