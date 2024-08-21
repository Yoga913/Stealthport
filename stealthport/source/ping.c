// Kode program ini digunakan untuk memeriksa konektivitas jaringan ke alamat IP tertentu dengan menggunakan perintah ping dari sistem operasi.
// Fungsi `ping` mencoba mengirimkan satu paket ping ke alamat IP yang diberikan dan mengembalikan `true` jika ping berhasil, atau `false` jika tidak.

#include <stdarg.h>   // Menyertakan header untuk dukungan variadic functions.
#include <stdbool.h>  // Menyertakan header untuk tipe data boolean (true/false).
#include <stdlib.h>   // Menyertakan header untuk fungsi alokasi memori dan fungsi utilitas lainnya.
#include <stdio.h>    // Menyertakan header untuk input/output standar.

#include "ping.h"     // Menyertakan header "ping.h", mungkin berisi deklarasi fungsi atau definisi yang relevan dengan operasi ping.
#include "ipv4.h"     // Menyertakan header "ipv4.h", mungkin berisi deklarasi atau definisi yang relevan dengan IP versi 4.

bool ping(char * ipv4)
{
    // Mengalokasikan memori sebesar 128 byte untuk menyimpan perintah ping yang akan dibentuk.
    char * command = malloc(sizeof(char) * 128);
    
    // Membentuk string perintah ping dengan argumen yang diberikan, mengalihkan output ke /dev/null (tidak terlihat oleh pengguna).
    sprintf(command, "ping -c 1 %s >/dev/null 2>&1", ipv4);
    
    // Menjalankan perintah yang telah dibentuk dan memeriksa apakah perintah berhasil dieksekusi (kode return 0).
    if(system(command) == 0)
    {
        free(command);  // Membebaskan memori yang telah dialokasikan untuk string perintah.
        return true;    // Mengembalikan nilai true jika ping berhasil (perintah berhasil dijalankan).
    }
    
    free(command);  // Membebaskan memori yang telah dialokasikan untuk string perintah jika ping gagal.
    return false;   // Mengembalikan nilai false jika ping gagal (perintah tidak berhasil dijalankan).
}

