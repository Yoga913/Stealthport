// Secara keseluruhan, kode program ini digunakan untuk menjalankan pemindaian atau pemeriksaan terhadap dua alamat IP menggunakan beberapa thread,
// dengan opsi untuk mengatur timeout, port, dan pengaturan lainnya, serta opsi untuk mencatat hasil ke file log dan menggunakan proxy jika diperlukan.

#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "ipv4.h"
#include "scanner.h"
#include "address.h"

// Jumlah thread yang akan digunakan
int thread_count = 5;
// Timeout dalam detik
int timeout = 1;
// Alamat IP yang pertama
char * opt_ip0 = NULL;
// Alamat IP yang kedua
char * opt_ip1 = NULL;
// Nama file log
char * log_filename = NULL;
// Port yang akan digunakan
int port = 80;
// Flag untuk menentukan mode umum
int in_general = 0;

// Alamat IP proxy
char * proxy_ip = NULL;
// Port proxy
int proxy_port = -1;

int main(int argc, char * argv[])
{
    // Memeriksa apakah argumen yang diberikan kurang dari 2
    if(argc < 2)
    {
        goto usage_err;
    }

    int opt;
    // Memproses argumen baris perintah
    while ((opt = getopt(argc, argv, "t:T:0:1:p:l:P:v:g:")) != -1) {
        switch (opt) {
            case 't':
                // Menetapkan jumlah thread
                thread_count = atoi(optarg);
                break;
            case 'T':
                // Menetapkan timeout
                timeout = atoi(optarg);
                break;
            case '0':
                // Menetapkan alamat IP yang pertama
                opt_ip0 = optarg;
                break;
            case '1':
                // Menetapkan alamat IP yang kedua
                opt_ip1 = optarg;
                break;
            case 'p':
                // Menetapkan port
                port = atoi(optarg);
                break;
            case 'l':
                // Menetapkan nama file log
                log_filename = optarg;
                break;
            case 'P':
                // Menetapkan alamat IP proxy
                proxy_ip = optarg;
                break;
            case 'v':
                // Menetapkan port proxy
                proxy_port = atoi(optarg);
                break;
            case 'g':
                // Menetapkan mode umum
                in_general = atoi(optarg);
                break;
            case '?':
                // Menampilkan pesan kesalahan jika argumen tidak valid
                goto usage_err;
                break;
            default:
usage_err:
                // Menampilkan pesan penggunaan dan keluar dengan status gagal
                fprintf(stderr, "Usage: %s -t <thread_count> -T <timeout> -0 <ip0> -1 <ip1> -p <port>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Memeriksa apakah alamat IP yang pertama telah ditetapkan
    if(opt_ip0 == NULL)
    {
        goto usage_err;
    }

    // Memeriksa apakah alamat IP yang kedua telah ditetapkan
    if(opt_ip1 == NULL)
    {
        goto usage_err;
    }

    ipv4_t * ip0 = NULL;
    ipv4_t * ip1 = NULL;

    // Mengonversi alamat IP dari string ke format ipv4_t
    ip0 = string_to_ipv4(opt_ip0);
    ip1 = string_to_ipv4(opt_ip1);

    // Memanggil fungsi scanner dengan parameter yang telah ditetapkan
    scanner(ip0, ip1, timeout, thread_count, port);

    exit(EXIT_SUCCESS);
}
