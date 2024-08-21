/*Kode program ini bertujuan untuk mengonversi alamat dalam bentuk string yang berformat IPv4 dan nomor port menjadi objek `address_t`,
dan sebaliknya, mengonversi objek `address_t` kembali menjadi string yang berformat IPv4 dan nomor port.*/

#include <stdlib.h>  // Library untuk fungsi alokasi memori dan konversi lainnya
#include <stdio.h>   // Library untuk fungsi input/output standar

#include "address.h" // Header file yang mendefinisikan tipe data dan fungsi terkait alamat
#include "ipv4.h"    // Header file yang mendefinisikan tipe data dan fungsi terkait IPv4

// Fungsi untuk mengonversi string menjadi alamat tipe address_t
address_t * string_to_address(char * string)
{
    // Alokasikan memori untuk address_t baru
    address_t * address = malloc(sizeof(address_t));
    // Alokasikan memori untuk string IPv4 sementara
    char * ipv4_string = malloc(sizeof(char) * 128);
    int port;  // Variabel untuk menyimpan nomor port

    // Parse string masukan untuk mendapatkan IPv4 dan nomor port
    sscanf(string, "%s:%d", ipv4_string, &port);
    // Konversi string IPv4 menjadi tipe ipv4_t
    ipv4_t *ipv4 = string_to_ipv4(ipv4_string);

    // Set port dan IPv4 ke dalam struktur address_t
    address->port = (uint16_t)port;
    address->ipv4 = ipv4;

    // Bebaskan memori yang dialokasikan untuk string IPv4 sementara
    free(ipv4_string);

    // Kembalikan pointer ke struktur address_t yang baru dibuat
    return address;
}

// Fungsi untuk mengonversi address_t menjadi string
char * address_to_string(address_t * address)
{
    // Alokasikan memori untuk string hasil konversi
    char * string0 = malloc(sizeof(char) * 128);
    // Konversi IPv4 dalam address_t menjadi string
    char * string1 = ipv4_to_string(address->ipv4);

    // Format string hasil akhir dengan menambahkan port
    sprintf(string0, "%s:%d", string1, address->port);

    // Bebaskan memori yang dialokasikan untuk string IPv4
    free(string1);

    // Kembalikan string hasil konversi
    return string0;
}
