# StealthPort

## Tentang

*StealthPort* adalah pemindai port IPv4 sederhana yang memungkinkan pemindaian port terbuka dengan dukungan multithreading, sehingga mampu memindai hingga seratus port sekaligus.

## Cara Build

Untuk membangun *StealthPort*, Anda memerlukan:
- Sistem operasi berbasis UNIX
- Kompiler C
- `libc` dengan dukungan TCP socket
- BSD Make atau GNU Make

Setelah semua persyaratan terpenuhi, jalankan perintah `make` di direktori root proyek.

## Penggunaan

Untuk menggunakan *StealthPort*, berikan rentang IP dengan parameter `-0` dan `-1`, serta port yang ingin dipindai dengan parameter `-p`. Jangan lupa gunakan `-t` untuk jumlah thread dan `-T` untuk timeout.

## **Contoh penggunaan:**

```sh
./StealthPort -t 100 -T 1 -0 1.0.0.0 -1 126.255.255.255 -p 80 -l found_servers.txt
```

Untuk menjalankan *StealthPort* dengan proxy SOCKS5, tambahkan parameter `-P` dan `-v`:

```sh
./StealthPort -t 100 -T 1 -0 1.0.0.0 -1 126.255.255.255 -p 80 -l found_servers.txt -P <proxy-ip> -v <proxy-port>
```

## Lisensi
Lisensi proyek ini masih dalam pertimbangan dan sementara ini dilisensikan sebagai All Rights Reserved.
