# digunakan untuk mengelola proses kompilasi kode sumber C, mengubahnya menjadi file objek, dan akhirnya membangun executable bernama `StealthPort`.
# Target `clean` digunakan untuk membersihkan file objek dan executable yang dihasilkan.

# Compiler yang digunakan
CC = gcc

# Flag kompilasi yang digunakan
CFLAGS = -Wall -Wextra -std=c99 -g

# Daftar semua file sumber C yang ada di folder source
SOURCES = $(wildcard source/*.c)

# Daftar semua file objek (.o) yang dihasilkan dari file sumber
OBJECTS = $(SOURCES:.c=.o)

# Target phony, yang berarti target ini tidak berhubungan dengan file
.PHONY: all clean

# Target default, yang akan dibangun ketika perintah `make` dipanggil tanpa argumen
all: StealthPort

# Target untuk membangun executable `StealthPort` dari file objek
StealthPort: $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ -lpthread -lrt

# Aturan untuk membangun file objek (.o) dari file sumber (.c)
source/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Target untuk membersihkan file objek dan executable yang dihasilkan
clean:
	rm -f $(OBJECTS) StealthPort


