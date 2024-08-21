// kode program ini digunakan untuk memeriksa konektivitas jaringan ke alamat IP dan port tertentu, dengan opsi untuk menggunakan proxy SOCKS5.
// Program ini mengatur koneksi dengan mode non-blocking dan menangani timeout serta kesalahan koneksi yang mungkin terjadi.

#include <stdio.h>          // Header untuk fungsi input/output standar
#include <stdbool.h>        // Header untuk tipe data boolean
#include <string.h>         // Header untuk fungsi manipulasi string
#include <unistd.h>         // Header untuk fungsi POSIX seperti close()
#include <sys/socket.h>     // Header untuk fungsi socket
#include <netinet/in.h>     // Header untuk struktur data sockaddr_in
#include <arpa/inet.h>      // Header untuk fungsi inet_pton
#include <fcntl.h>          // Header untuk fungsi fcntl
#include <errno.h>          // Header untuk manajemen error
#include <netdb.h>          // Header untuk fungsi dan struktur data jaringan
#include <sys/select.h>     // Header untuk fungsi select()

#include "check.h"          // Header untuk deklarasi fungsi check()

// Fungsi untuk memeriksa koneksi ke alamat IP dan port tertentu, dengan atau tanpa menggunakan proxy SOCKS5
int check(const char* ip_address, int port, int timeout_seconds, const char* socks5_proxy, int proxy_port, int use_proxy)
{
    // Membuat socket baru
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1; // Gagal membuat socket
    }

    // Mengatur socket menjadi non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // Menginisialisasi struktur data untuk alamat server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Mengisi struktur dengan nol
    server_addr.sin_family = AF_INET;             // Mengatur tipe alamat menjadi IPv4
    server_addr.sin_port = htons(port);           // Mengatur port, dengan konversi ke format network byte order

    // Mengonversi alamat IP dari format teks ke format biner
    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0)
    {
        close(sockfd); // Menutup socket jika terjadi kesalahan
        return -2;     // Alamat IP tidak valid
    }

    // Jika menggunakan proxy
    if (use_proxy && socks5_proxy != NULL) {
        // Menginisialisasi struktur data untuk alamat proxy
        struct sockaddr_in proxy_addr;
        memset(&proxy_addr, 0, sizeof(proxy_addr));
        proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_port = htons(proxy_port);
        if (inet_pton(AF_INET, socks5_proxy, &proxy_addr.sin_addr) <= 0) {
            close(sockfd);
            return -6; // Alamat proxy tidak valid
        }

        // Mencoba menghubungkan ke proxy
        int connect_proxy_result = connect(sockfd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr));
        if (connect_proxy_result < 0) {
            if (errno == EINPROGRESS) {
                // Menyiapkan set file descriptor untuk fungsi select()
                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(sockfd, &fdset);

                // Mengatur batas waktu untuk fungsi select()
                struct timeval timeout;
                timeout.tv_sec = timeout_seconds;
                timeout.tv_usec = 0;

                // Memeriksa apakah koneksi berhasil dalam batas waktu
                int select_result = select(sockfd + 1, NULL, &fdset, NULL, &timeout);
                if (select_result == 1) {
                    int so_error;
                    socklen_t len = sizeof(so_error);
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
                    if (so_error == 0) {
                        /* Tidak ada error, koneksi berhasil */
                    } else {
                        close(sockfd);
                        return -7; // Kesalahan pada proxy
                    }
                } else if (select_result == 0) {
                    close(sockfd);
                    return -3; // Koneksi time out
                } else {
                    close(sockfd);
                    return -4; // Kesalahan saat select()
                }
            } else {
                close(sockfd);
                return -5; // Kesalahan saat menghubungkan ke proxy
            }
        }
    }

    // Mencoba menghubungkan ke server
    int connect_result = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (connect_result < 0)
    {
        if (errno == EINPROGRESS)
        {
            // Menyiapkan set file descriptor untuk fungsi select()
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(sockfd, &fdset);

            // Mengatur batas waktu untuk fungsi select()
            struct timeval timeout;
            timeout.tv_sec = timeout_seconds;
            timeout.tv_usec = 0;

            // Memeriksa apakah koneksi berhasil dalam batas waktu
            int select_result = select(sockfd + 1, NULL, &fdset, NULL, &timeout);
            if (select_result == 1)
            {
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error == 0) {
                    close(sockfd);
                    return 0; // Koneksi berhasil
                }
            }
            else if (select_result == 0)
            {
                close(sockfd);
                return -3; // Koneksi time out
            }
            else
            {
                close(sockfd);
                return -4; // Kesalahan saat select()
            }
        }
        else
        {
            close(sockfd);
            return -5; // Kesalahan saat menghubungkan ke server
        }
    }
    else
    {
        close(sockfd);
        return 0; // Koneksi berhasil
    }

    close(sockfd);
    return -5; // Koneksi gagal
}
