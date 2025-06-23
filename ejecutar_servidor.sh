#!/bin/bash
echo "========================================"
echo "🚀 SERVIDOR PRINCIPAL"
echo "========================================"
echo ""

# Verificar ejecutables
echo "🔍 Verificando programas..."
if [[ ! -f "servidor" || ! -f "nodo" || ! -f "cliente" ]]; then
    echo "⚠️  Algunos ejecutables no existen. Compilando..."
    make clean
    make all
    if [ $? -ne 0 ]; then
        echo "❌ Error en compilación"
        exit 1
    fi
    echo "✅ Programas compilados exitosamente"
else
    echo "✅ Todos los ejecutables encontrados"
fi

echo ""
echo "📝 Preparando archivo de prueba..."

# Crear archivo de entrada si no existe
if [ ! -f "entrada.txt" ]; then
    echo "Esta es una prueba para la verificación del cifrado de archivos, además prueba prueba prueba" > entrada.txt
    echo "✅ Archivo entrada.txt creado"
else
    echo "✅ Archivo entrada.txt ya existe"
fi

echo ""
echo "🔐 Cifrando archivo..."
./cliente entrada.txt archivo_cifrado.enc
if [ $? -ne 0 ]; then
    echo "❌ Error cifrando archivo"
    exit 1
fi
echo "✅ Archivo cifrado creado: archivo_cifrado.enc"
echo ""
echo ""
echo ""

echo "⏳ Esperando a que los nodos estén listos..."
echo "   (Asegúrate de que los 3 nodos estén ejecutándose)"

# Countdown
for i in {5..1}; do
    echo "   Iniciando en $i segundos..."
    sleep 1
done

echo ""
echo "🚀 INICIANDO PROCESAMIENTO DISTRIBUIDO"
echo "========================================"
echo "Conectando con nodos en puertos:"
echo "  • Puerto 8081 (Nodo 1)"
echo "  • Puerto 8082 (Nodo 2)" 
echo "  • Puerto 8083 (Nodo 3)"
echo ""

# Ejecutar servidor principal
./servidor archivo_cifrado.enc
EXIT_CODE=$?

echo ""
echo "========================================"
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ PROCESAMIENTO COMPLETADO EXITOSAMENTE"
    echo ""
    
    echo ""
    echo "📁 Archivos generados:"
    echo "  • archivo_cifrado.enc"
    echo "  • archivo_descifrado.txt "
    
else
    echo "❌ Error en el procesamiento"
    echo "   Verifica que los 3 nodos estén ejecutándose"
fi

echo ""
echo "🏁 Servidor terminado"