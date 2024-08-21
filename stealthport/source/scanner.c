// kode program ini digunakan untuk memindai alamat IP dan port dalam rentang tertentu untuk memeriksa apakah mereka online dan dapat diakses.
// Program ini menggunakan multithreading untuk melakukan pemeriksaan secara paralel dan dapat melakukan pengecekan dengan atau tanpa proxy.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

// Header files yang diperlukan untuk berbagai fungsi
#include "scanner.h"
#include "ipv4.h"
#include "check.h"
#include "address.h"
#include "ping.h"

// Deklarasi variabel global
extern char * log_filename;

extern char * proxy_ip;
extern int proxy_port;
extern int in_general;

// Struktur data untuk menyimpan argumen thread
typedef struct
{
    char *ip_address; // Alamat IP yang akan diperiksa
    int port;         // Port yang akan diperiksa
    int timeout;      // Waktu tunggu untuk koneksi
} threadargs_t;

// Fungsi yang dijalankan oleh setiap thread untuk memeriksa status alamat IP dan port
void *scanner_thread(void *arg)
{
    threadargs_t *args = (threadargs_t *)arg;

    char *ip_address = args->ip_address;
    int port = args->port;
    int timeout = args->timeout;

    int proxy_or_not = 1;

    // Menentukan apakah proxy digunakan atau tidak
    if(proxy_ip == NULL && proxy_port == -1)
    {
        proxy_or_not = 0;
    }

    if(in_general == 0)
    {
        // Memeriksa status alamat IP dan port
        switch(check(ip_address, port, timeout, proxy_ip, proxy_port, proxy_or_not))
        {
        case 0:
            // Jika alamat IP dan port online
            printf("%s:%d => \x1b[32;1monline\x1b[0m\n", ip_address, port);

            if(log_filename != NULL)
            {
                // Menyimpan hasil ke file log jika log_filename diatur
                FILE * log_file = fopen(log_filename, "a");
                if(log_file == NULL)
                {
                    fprintf(stderr, "\x1b[31;1mfatal error: error opening %s\x1b[0m\n", log_filename);
                    fclose(log_file);
                    exit(EXIT_FAILURE);
                }
                fprintf(log_file, "%s:%d\n", ip_address, port);
                fclose(log_file);
            }
            break;
        case -1:
            // Jika gagal membuat socket
            printf("%s:%d => \x1b[31;1mfailed to create socket\x1b[0m\n", ip_address, port);
            break;
        case -2:
            // Jika gagal parsing alamat IP
            printf("%s:%d => \x1b[31;1mfailed to parse IP address\x1b[0m\n", ip_address, port);
            break;
        case -3:
            /*
             *  Jika waktu tunggu habis, tidak dapat dipastikan apakah alamat IP
             *  aktif atau tidak, kecuali jika waktu tunggu yang lebih besar diatur.
             */
            printf("%s:%d => \x1b[31;1munreachable\x1b[0m\n", ip_address, port);
            break;
        case -4:
            // Jika gagal memeriksa status socket
            printf("%s:%d => \x1b[31;1mfailed to check socket status\x1b[0m\n", ip_address, port);
            break;
        case -5:
            // Jika koneksi gagal atau ditolak
            printf("%s:%d => \x1b[31;1mrejected\x1b[0m\n", ip_address, port);
            break;
        }
    }
    else if(in_general == 1)
    {
        // Jika dalam mode umum, periksa status alamat IP dengan ping
        switch(ping(ip_address))
        {
        case true:
            printf("%s => \x1b[32;1monline\x1b[0m\n", ip_address);
            break;
        case false:
            printf("%s => \x1b[31;1moffline\x1b[0m\n", ip_address);
            break;
        }
    }

    // Membebaskan memori yang dialokasikan
    free(ip_address);
    free(args);
    pthread_exit(NULL);
}

// Fungsi utama untuk memindai alamat IP dalam rentang tertentu dengan multithreading
void scanner(ipv4_t *ip0, ipv4_t *ip1, int timeout, int thread_count, uint16_t port)
{
    pthread_t threads[thread_count]; // Array untuk menyimpan ID thread
    int current_thread = 0;

    // Loop untuk setiap alamat IP dalam rentang yang diberikan
    for (int a = (int)ip0->a; a <= (int)ip1->a; a++)
    {
        for (int b = (int)ip0->b; b <= (int)ip1->b; b++)
        {
            for (int c = (int)ip0->c; c <= (int)ip1->c; c++)
            {
                for (int d = (int)ip0->d; d <= (int)ip1->d; d++)
                {
                    /*
                     *  Membuat thread baru untuk memeriksa apakah alamat IP aktif atau tersedia.
                     *  Jika jumlah thread mencapai batas maksimum, tunggu hingga semua thread selesai
                     *  sebelum membuat set thread berikutnya.
                     */
                    ipv4_t *ipv4 = malloc(sizeof(ipv4_t));
                    ipv4->a = a;
                    ipv4->b = b;
                    ipv4->c = c;
                    ipv4->d = d;
                    char *ip_address = ipv4_to_string(ipv4);

                    threadargs_t *args = malloc(sizeof(threadargs_t));
                    args->ip_address = ip_address;
                    args->port = port;
                    args->timeout = timeout;

                    pthread_create(&threads[current_thread], NULL, scanner_thread, args);
                    current_thread = (current_thread + 1) % thread_count;

                    // Tunggu semua thread selesai jika jumlah thread mencapai batas maksimum
                    if (current_thread == 0)
                    {
                        for (int i = 0; i < thread_count; i++)
                        {
                            pthread_join(threads[i], NULL);
                        }
                    }
                }
            }
        }
    }

    // Tunggu semua thread yang tersisa untuk selesai
    for (int i = 0; i < current_thread; i++)
    {
        pthread_join(threads[i], NULL);
    }
}
