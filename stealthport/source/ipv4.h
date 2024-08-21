// Kode program ini digunakan untuk mendefinisikan dan mengelola alamat IPv4 dalam format struktural.
// Ini termasuk mengonversi alamat IPv4 dari dan ke representasi string.

#pragma once

#include <stdint.h>

/* 
 * Struktur ini mendefinisikan alamat IPv4.
 * Alamat IPv4 terdiri dari 4 bagian, masing-masing berukuran 8-bit (1 byte).
 */
typedef struct
{
    uint8_t a, b, c, d;  // Bagian-bagian dari alamat IPv4
} ipv4_t;

/*
 * Fungsi ini mengonversi alamat IPv4 yang disimpan dalam struktur ipv4_t
 * menjadi string representasi dari alamat IPv4 tersebut.
 *
 * @param ipv4: Pointer ke struktur ipv4_t yang berisi alamat IPv4.
 * @return: Pointer ke string yang berisi representasi alamat IPv4.
 */
char * ipv4_to_string(ipv4_t * ipv4);

/*
 * Fungsi ini mengonversi string representasi alamat IPv4
 * menjadi struktur ipv4_t.
 *
 * @param string: Pointer ke string yang berisi representasi alamat IPv4.
 * @return: Pointer ke struktur ipv4_t yang berisi alamat IPv4 yang diambil dari string.
 */
ipv4_t * string_to_ipv4(char * string);
