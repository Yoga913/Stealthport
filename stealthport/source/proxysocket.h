/**
 * @file proxysocket.h
 * @brief proxysocket header file for creating network socket connections via different proxy methods
 * @author Brecht Sanders
 * @date 2016
 * @copyright MIT
 *
 * Include this header file to use proxysocket and link with -lproxysocket.
 * This header provides the functionality to connect network sockets
 * optionally via different proxy methods.
 */


#ifndef __INCLUDED_PROXYSOCKET_H
#define __INCLUDED_PROXYSOCKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \cond PRIVATE */
/* Menyertakan header untuk Winsock di Windows atau netinet/in.h di sistem non-Windows. */
#ifdef __WIN32__
# include <winsock2.h>
# ifdef BUILD_PROXYSOCKET_DLL
#  define DLL_EXPORT_PROXYSOCKET __declspec(dllexport)
# elif !defined(STATIC) && !defined(BUILD_PROXYSOCKET_STATIC)
#  define DLL_EXPORT_PROXYSOCKET __declspec(dllimport)
# else
#  define DLL_EXPORT_PROXYSOCKET
# endif
#else
# define DLL_EXPORT_PROXYSOCKET
# include <netinet/in.h>
# define SOCKET int
# ifndef INVALID_SOCKET
#  define INVALID_SOCKET -1
# endif
#endif
/*! \endcond */

/*! \brief Konstanta nomor versi dari proxysocket
 * \sa     proxysocket_get_version()
 * \sa     proxysocket_get_version_string()
 * \name   PROXYSOCKET_VERSION_*
 * \{
 */
/*! \brief nomor versi utama */
#define PROXYSOCKET_VERSION_MAJOR 0
/*! \brief nomor versi minor */
#define PROXYSOCKET_VERSION_MINOR 1
/*! \brief nomor versi mikro */
#define PROXYSOCKET_VERSION_MICRO 12
/*! @} */

/*! \brief Jenis-jenis proxy
 * \sa     proxysocketconfig_create()
 * \sa     proxysocketconfig_add_proxy()
 * \{
 */
/*! \brief koneksi langsung tanpa proxy */
#define PROXYSOCKET_TYPE_NONE           0x00
/*! \brief proxy SOCKS4 */
#define PROXYSOCKET_TYPE_SOCKS4         0x04
/*! \brief proxy SOCKS5 */
#define PROXYSOCKET_TYPE_SOCKS5         0x05
/*! \brief proxy HTTP */
#define PROXYSOCKET_TYPE_WEB_CONNECT    0x20
/*! \brief jenis proxy tidak valid */
#define PROXYSOCKET_TYPE_INVALID        -1
/*! @} */

/*! \brief level logging
 * \sa     proxysocketconfig_log_fn()
 * \name   PROXYSOCKET_VERSION_*
 * \{
 */
#define PROXYSOCKET_LOG_ERROR   0
#define PROXYSOCKET_LOG_WARNING 1
#define PROXYSOCKET_LOG_INFO    2
#define PROXYSOCKET_LOG_DEBUG   3
/*! @} */

// Tujuan: fungsi untuk membuat socket yang dapat digunakan oleh fungsi sistem dan menghubungkannya ke host jarak jauh, opsional melalui proxy

/*! \brief Mendapatkan versi proxysocket
 * \param  pmajor        pointer ke integer yang akan menerima nomor versi utama
 * \param  pminor        pointer ke integer yang akan menerima nomor versi minor
 * \param  pmicro        pointer ke integer yang akan menerima nomor versi mikro
 * \sa     proxysocket_get_version_string()
 */
DLL_EXPORT_PROXYSOCKET void proxysocket_get_version (int* pmajor, int* pminor, int* pmicro);

/*! \brief Mendapatkan string versi proxysocket
 * \return string versi
 * \sa     proxysocket_get_version()
 */
DLL_EXPORT_PROXYSOCKET const char* proxysocket_get_version_string ();

/*! \brief Mendapatkan nama tekstual dari jenis proxy
 * \param  proxytype       jenis proxy (salah satu dari konstanta PROXYSOCKET_TYPE_)
 * \return nama dari jenis proxy (NONE/SOCKS4A/SOCKS5/WEB/INVALID)
 * \sa     proxysocketconfig_create()
 * \sa     proxysocketconfig_get_name_type()
 */
DLL_EXPORT_PROXYSOCKET const char* proxysocketconfig_get_type_name (int proxytype);

/*! \brief Mendapatkan jenis proxy dari nama tekstualnya
 * \param  proxytypename   nama dari jenis proxy (NONE/DIRECT/SOCKS4/SOCKS4A/SOCKS5/WEB/HTTP)
 * \return jenis proxy (salah satu dari konstanta PROXYSOCKET_TYPE_) atau PROXYSOCKET_TYPE_INVALID jika gagal
 * \sa     proxysocketconfig_create()
 * \sa     proxysocketconfig_get_type_name()
 */
DLL_EXPORT_PROXYSOCKET int proxysocketconfig_get_name_type (const char* proxytypename);

/*! \brief Inisialisasi pustaka, panggil sekali di awal thread utama aplikasi
 * \return nol jika berhasil
 */
DLL_EXPORT_PROXYSOCKET int proxysocket_initialize ();

/*! \brief Tipe objek proxysocketconfig */
typedef struct proxysocketconfig_struct* proxysocketconfig;

/*! \brief Membuat struktur data informasi proxy yang diatur untuk koneksi langsung
 * \return struktur data informasi proxy jika berhasil atau NULL jika gagal
 * \sa     proxysocketconfig_create()
 * \sa     proxysocketconfig_add_proxy()
 * \sa     proxysocketconfig_free()
 */
DLL_EXPORT_PROXYSOCKET proxysocketconfig proxysocketconfig_create_direct ();

/*! \brief Membuat struktur data informasi proxy
 * \param  proxytype   jenis proxy (salah satu dari konstanta PROXYSOCKET_TYPE_)
 * \param  proxyhost   hostname atau alamat IP proxy (atau jika proxytype adalah PROXYSOCKET_TYPE_NONE, alamat untuk di-bind jika non-zero)
 * \param  proxyport   nomor port proxy (atau jika proxytype adalah PROXYSOCKET_TYPE_NONE, port untuk di-bind jika non-zero)
 * \param  proxyuser   login autentikasi proxy atau NULL jika tidak ada
 * \param  proxypass   password autentikasi proxy atau NULL jika tidak ada
 * \return struktur data informasi proxy jika berhasil atau NULL jika gagal
 * \sa     proxysocketconfig_create_direct()
 * \sa     proxysocketconfig_add_proxy()
 * \sa     proxysocketconfig_free()
 */
DLL_EXPORT_PROXYSOCKET proxysocketconfig proxysocketconfig_create (int proxytype, const char* proxyhost, uint16_t proxyport, const char* proxyuser, const char* proxypass);

/*! \brief Menambahkan metode proxy ke struktur data informasi proxy (beberapa dapat dihubungkan secara bertingkat)
 * \param  proxy       informasi proxy seperti yang dikembalikan oleh proxysocketconfig_create()
 * \param  proxytype   jenis proxy (salah satu dari konstanta PROXYSOCKET_TYPE_)
 * \param  proxyhost   hostname atau alamat IP proxy (atau jika proxytype adalah PROXYSOCKET_TYPE_NONE, alamat untuk di-bind jika non-zero)
 * \param  proxyport   nomor port proxy (atau jika proxytype adalah PROXYSOCKET_TYPE_NONE, port untuk di-bind jika non-zero)
 * \param  proxyuser   login autentikasi proxy atau NULL jika tidak ada
 * \param  proxypass   password autentikasi proxy atau NULL jika tidak ada
 * \return struktur data informasi proxy jika berhasil atau NULL jika gagal
 * \sa     proxysocketconfig_create_direct()
 * \sa     proxysocketconfig_create()
 * \sa     proxysocketconfig_free()
 */
DLL_EXPORT_PROXYSOCKET int proxysocketconfig_add_proxy (proxysocketconfig proxy, int proxytype, const char* proxyhost, uint16_t proxyport, const char* proxyuser, const char* proxypass);

/*! \brief Tipe pointer ke fungsi untuk logging
 * \param  level       level logging (salah satu dari konstanta PROXYSOCKET_LOG_)
 * \param  message     teks yang akan di-log
 * \param  userdata    data kustom yang diteruskan ke proxysocketconfig_set_logging
 * \sa     proxysocketconfig_set_logging()
 */
typedef void (*proxysocketconfig_log_fn)(int level, const char* message, void* userdata);

/*! \brief Mendapatkan deskripsi dari proxy yang dikonfigurasi
 * \param  proxy       informasi proxy seperti yang dikembalikan oleh proxysocketconfig_create()
 * \return string dengan deskripsi pengaturan proxy (pemanggil harus free) atau NULL jika gagal
 */
DLL_EXPORT_PROXYSOCKET char* proxysocketconfig_get_description (proxysocketconfig proxy);

/*! \brief Mengkonfigurasi fungsi logging
 * \param  proxy       informasi proxy seperti yang dikembalikan oleh proxysocketconfig_create()
 * \param  log_fn      fungsi logging yang akan digunakan atau NULL untuk menonaktifkan logging
 * \param  userdata    data yang ditentukan pengguna yang akan diteruskan ke fungsi logging
 * \sa     proxysocketconfig_create()
 * \sa     proxysocketconfig_log_fn
 */
DLL_EXPORT_PROXYSOCKET void proxysocketconfig_set_logging (proxysocketconfig proxy, proxysocketconfig_log_fn log_fn, void* userdata);

/*! \brief Mengkonfigurasi timeout koneksi
 * \param  proxy       informasi proxy seperti yang dikembalikan oleh proxysocketconfig_create()
 * \param  sendtimeout timeout pengiriman dalam milidetik (0 untuk tidak ada timeout)
 * \param  recvtimeout timeout penerimaan dalam milidetik (0 untuk tidak ada timeout)
 * \sa     proxysocketconfig_create()
 */
DLL_EXPORT_PROXYSOCKET void proxysocketconfig_set_timeout (proxysocketconfig proxy, uint32_t sendtimeout, uint32_t recvtimeout);

/*! \brief Menentukan tempat resolusi nama dilakukan
 * \param  proxy       informasi proxy seperti yang dikembalikan oleh proxysocketconfig_create()
 * \param  proxy_dns   lakukan pencarian DNS pada server proxy jika non-zero atau pada klien jika nol (default)
 * \sa     proxysocketconfig_create()
 */
DLL_EXPORT_PROXYSOCKET void proxysocketconfig_use_proxy_dns (proxysocketconfig proxy, int proxy_dns);

/*! \brief Membersihkan informasi proxy
 * \param  proxy       informasi proxy seperti yang dikembalikan oleh proxysocketconfig_create()
 * \sa     proxysocketconfig_create()
 */
DLL_EXPORT_PROXYSOCKET void proxysocketconfig_free (proxysocketconfig proxy);

/*! \brief Membuat koneksi TCP menggunakan proxy yang ditentukan
 * \param  proxy       informasi proxy seperti yang dikembalikan oleh proxysocketconfig_create()
 * \param  dsthost     hostname atau alamat IP tujuan
 * \param  dstport     nomor port tujuan
 * \param  errmsg      pointer ke string yang akan menerima pesan kesalahan, dapat menjadi NULL, pemanggil harus free
 * \return socket jaringan jika berhasil atau INVALID_SOCKET jika gagal
 * \sa     proxysocketconfig_create()
 */
DLL_EXPORT_PROXYSOCKET SOCKET proxysocket_connect (proxysocketconfig proxy, const char* dsthost, uint16_t dstport, char** errmsg);

/*! \brief Memutuskan koneksi socket proxy
 * \param  proxy       informasi proxy seperti yang dikembalikan oleh proxysocketconfig_create()
 * \param  sock        socket jaringan seperti yang dikembalikan oleh proxysocket_connect()
 * \sa     proxysocketconfig_create()
 * \sa     proxysocket_connect()
 */
DLL_EXPORT_PROXYSOCKET void proxysocket_disconnect (proxysocketconfig proxy, SOCKET sock);

/*! \brief Mengatur timeout socket
 * \param  sock        socket jaringan seperti yang dikembalikan oleh proxysocket_connect() atau socket()
 * \param  timeout     timeout dalam detik (0 untuk menonaktifkan)
 * \sa     proxysocket_connect()
 */
DLL_EXPORT_PROXYSOCKET void socket_set_timeouts_milliseconds (SOCKET sock, uint32_t sendtimeout, uint32_t recvtimeout);

/*! \brief Membaca sebuah baris dari socket
 * \param  sock        socket jaringan seperti yang dikembalikan oleh proxysocket_connect() atau socket()
 * \return isi baris tanpa newline trailing (pemanggil harus free) atau NULL jika gagal
 * \sa     proxysocket_connect()
 */
DLL_EXPORT_PROXYSOCKET char* socket_receiveline (SOCKET sock);

/*! \brief Mendapatkan pesan kesalahan dari kesalahan socket terakhir
 * \return pesan kesalahan atau NULL jika tidak ada kesalahan
 */
DLL_EXPORT_PROXYSOCKET char* socket_get_error_message ();

#ifdef __cplusplus
}
#endif

#endif //__INCLUDED_PROXYSOCKET_H

// Secara keseluruhan kode program ini untuk menyediakan antarmuka dalam bahasa C untuk menangani koneksi jaringan melalui proxy.
// Ini termasuk:

// 1. **Versi dan Tipe Proxy**: Menyediakan informasi tentang versi library dan tipe-tipe proxy yang didukung (misalnya SOCKS4, SOCKS5, HTTP).

// 2. **Konfigurasi Proxy**: Memungkinkan pembuatan dan pengaturan konfigurasi proxy, termasuk menambahkan beberapa proxy dalam satu konfigurasi, mengatur otentikasi, dan menentukan waktu tunggu.

// 3. **Koneksi dan Pemutusan**: Menyediakan fungsi untuk membuat koneksi TCP menggunakan proxy, serta memutuskan koneksi dan membersihkan sumber daya.

// 4. **Logging dan Pengaturan Waktu Tunggu**: Mengatur fungsi logging untuk mencatat informasi atau kesalahan, serta mengonfigurasi waktu tunggu untuk operasi jaringan.

// 5. **Fungsi Utilitas**: Menyediakan fungsi untuk mendapatkan deskripsi proxy, membaca data dari socket, dan mendapatkan pesan kesalahan.

// Ini memungkinkan aplikasi untuk terhubung ke host remote melalui berbagai jenis proxy, dengan kemampuan untuk mengelola, mencatat, dan mengonfigurasi koneksi tersebut secara fleksibel.