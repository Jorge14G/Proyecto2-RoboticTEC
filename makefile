# Makefile para el proyecto servidor-nodos con MPI

CC = gcc
MPICC = mpicc
CFLAGS = -Wall -g -std=c99
LDFLAGS = -pthread

# Archivos fuente
SERVIDOR_SRC = servidor.c
NODO_SRC = nodo.c
CLIENTE_SRC = cliente.c

# Archivos ejecutables
SERVIDOR_BIN = servidor
NODO_BIN = nodo
CLIENTE_BIN = cliente

.PHONY: all clean install-deps run-demo

all: $(SERVIDOR_BIN) $(NODO_BIN) $(CLIENTE_BIN)

# Compilar servidor
$(SERVIDOR_BIN): $(SERVIDOR_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Compilar nodo con MPI
$(NODO_BIN): $(NODO_SRC)
	$(MPICC) $(CFLAGS) -o $@ $<

# Compilar cliente (tu código existente)
$(CLIENTE_BIN): $(CLIENTE_SRC)
	$(CC) $(CFLAGS) -o $@ $<

# Instalar dependencias en Ubuntu
install-deps:
	sudo apt update
	sudo apt install -y build-essential
	sudo apt install -y openmpi-bin openmpi-common libopenmpi-dev
	sudo apt install -y gcc make

# Crear archivo de prueba
test-file:
	echo "Esta es una prueba para la verificación del cifrado de archivos, además prueba prueba prueba" > entrada.txt

# Ejecutar demostración completa
run-demo: all test-file
	@echo "=== Creando archivo cifrado ==="
	./$(CLIENTE_BIN) entrada.txt archivo_cifrado.enc
	@echo ""
	@echo "=== Iniciando nodos en background ==="
	mpirun -np 2 ./$(NODO_BIN) 8081 &
	sleep 1
	mpirun -np 2 ./$(NODO_BIN) 8082 &
	sleep 1
	mpirun -np 2 ./$(NODO_BIN) 8083 &
	sleep 2
	@echo ""
	@echo "=== Ejecutando servidor principal ==="
	./$(SERVIDOR_BIN) archivo_cifrado.enc
	@echo ""
	@echo "=== Terminando procesos background ==="
	pkill -f "./$(NODO_BIN)" || true

# Limpiar archivos compilados
clean:
	rm -f $(SERVIDOR_BIN) $(NODO_BIN) $(CLIENTE_BIN)
	rm -f entrada.txt archivo_cifrado.enc archivo_descifrado.txt

# Mostrar ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make                 - Compilar todos los programas"
	@echo "  make install-deps    - Instalar dependencias MPI en Ubuntu"
	@echo "  make run-demo        - Ejecutar demostración completa"
	@echo "  make clean           - Limpiar archivos compilados"
	@echo "  make help            - Mostrar esta ayuda"