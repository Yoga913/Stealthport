#pragma once // Memastikan file header ini hanya dimasukkan sekali dalam satu kompilasi

// Fungsi ping untuk memeriksa ketersediaan alamat IPv4
// Parameter: ipv4 - string yang berisi alamat IPv4 yang akan diping
// Mengembalikan: true jika ping berhasil, false jika gagal
bool ping(char * ipv4);
