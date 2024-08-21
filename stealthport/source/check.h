// kode program ini berfungsi untuk melakukan pengecekan konektivitas ke alamat IP dan port.

#pragma once // Mencegah multiple inclusion dari file header

int check(const char* ip_address, int port, int timeout_seconds, const char* socks5_proxy, int proxy_port, int use_proxy);
// Memeriksa konektivitas ke alamat IP dan port tertentu dengan opsi menggunakan proxy SOCKS5
