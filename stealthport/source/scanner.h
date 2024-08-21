// program ini di gunakan untuk melakukan pemindaian jaringan dengan memeriksa rentang alamat IP pada port tertentu menggunakan beberapa thread untuk meningkatkan efisiensi pemindaian.
#pragma once // Direktif ini memastikan file header hanya disertakan sekali selama proses kompilasi

#include "ipv4.h" // Mengimpor deklarasi untuk tipe data dan fungsi yang berkaitan dengan IPv4

// Fungsi untuk melakukan pemindaian terhadap rentang alamat IP pada port tertentu
void scanner(ipv4_t * ip0, ipv4_t * ip1, int timeout, int thread_count, uint16_t port);
