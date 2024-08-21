/*Secara keseluruhan Kode program ini mengelola alamat jaringan dengan mengonversi antara representasi string dan struktur data yang mencakup alamat IPv4 dan port.*/

// Pastikan header file hanya disertakan sekali
#pragma once

// Sertakan tipe data integer dengan lebar tetap
#include <stdint.h>

// Sertakan header file untuk definisi tipe data IPv4
#include "ipv4.h"

// Definisikan struktur data 'address_t' untuk menyimpan alamat IP dan port
typedef struct
{
    ipv4_t *ipv4;    // Pointer ke tipe data IPv4
    uint16_t port;   // Port sebagai 16-bit unsigned integer
} address_t;

// Deklarasi fungsi untuk mengonversi string menjadi pointer 'address_t'
address_t * string_to_address(char * string);

// Deklarasi fungsi untuk mengonversi 'address_t' menjadi string
char * address_to_tring(address_t * address);

