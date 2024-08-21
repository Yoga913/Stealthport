// Kode program ini digunakan untuk mengonversi antara representasi IP versi 4 (IPv4) dalam bentuk string dan tipe data struktur `ipv4_t`.
// Fungsi `ipv4_to_string` mengubah struktur `ipv4_t` menjadi string berformat "a.b.c.d", sedangkan `string_to_ipv4` mengubah string tersebut kembali menjadi struktur `ipv4_t`.

#include <stdio.h>   // Menyertakan pustaka standar untuk input dan output
#include <stdlib.h>  // Menyertakan pustaka standar untuk fungsi utilitas seperti malloc

#include "ipv4.h"    // Menyertakan header khusus untuk tipe data ipv4_t

// Fungsi untuk mengonversi tipe data ipv4_t ke representasi string
char * ipv4_to_string(ipv4_t * ipv4)
{
    // Alokasikan memori untuk menyimpan string hasil konversi, dengan ukuran 32 karakter
    char * string = malloc(sizeof(char) * 32);

    // Konversi setiap bagian dari ipv4_t menjadi format string "a.b.c.d"
    sprintf(string, "%d.%d.%d.%d", (int)ipv4->a, (int)ipv4->b, (int)ipv4->c, (int)ipv4->d);

    // Mengembalikan string hasil konversi
    return string;
}

// Fungsi untuk mengonversi string ke tipe data ipv4_t
ipv4_t * string_to_ipv4(char * string)
{
    // Alokasikan memori untuk tipe data ipv4_t
    ipv4_t * ipv4 = malloc(sizeof(ipv4_t));

    // Parse string dan masukkan nilai ke dalam struktur ipv4_t
    sscanf(string, "%d.%d.%d.%d", (int *)&ipv4->a, (int *)&ipv4->b, (int *)&ipv4->c, (int *)&ipv4->d);

    // Mengembalikan pointer ke ipv4_t yang berisi data yang di-parse dari string
    return ipv4;
}
