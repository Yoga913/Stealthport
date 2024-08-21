// kode program ini digunnakan untuk mengonfigurasi sistem dengan jumlah thread tertentu, menetapkan batas waktu operasi, dan menentukan rentang alamat IPv4.
#pragma once

#include "ipv4.h"

// Mendefinisikan struktur bernama 'config_t'
typedef struct
{
    // Jumlah thread yang digunakan
    int thread_count;

    // Batas waktu dalam operasi
    int timeout;

    // Substruktur yang mendefinisikan rentang IP
    struct
    {
        // Rentang bawah
        struct
        {
            // String yang merepresentasikan IP
            char * string;

            // Alamat IP versi 4
            ipv4_t * ipv4;
        } bottom;

        // Rentang atas
        struct
        {
            // String yang merepresentasikan IP
            char * string;

            // Alamat IP versi 4
            ipv4_t * ipv4;
        } top;
    } range;
} config_t;
