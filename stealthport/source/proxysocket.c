#define _GNU_SOURCE    // Memperbaiki peringatan tentang vasprintf
#include "proxysocket.h"

#ifdef __WIN32__
//#include <ws2tcpip.h>
#else
#include <sys/socket.h>     // Header untuk fungsi socket di sistem Unix
#include <netinet/in.h>     // Header untuk struktur alamat Internet
#include <arpa/inet.h>      // Header untuk fungsi konversi alamat IP
#include <netdb.h>          // Header untuk fungsi pengambilan informasi host dan layanan
#include <unistd.h>         // Header untuk fungsi-fungsi sistem Unix seperti read dan write
#include <errno.h>          // Header untuk mendefinisikan nomor error
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1    // Mendefinisikan SOCKET_ERROR jika belum didefinisikan
#endif
#endif

#include <stdio.h>          // Header untuk input/output standar
#include <stdlib.h>         // Header untuk fungsi alokasi memori dan fungsi utilitas lainnya
#include <string.h>         // Header untuk fungsi manipulasi string
#include <ctype.h>          // Header untuk fungsi karakter seperti isdigit, isalpha, dll.
#include <stdarg.h>         // Header untuk variadic functions

#ifdef _MSC_VER
#define va_copy(dst, src) ((dst) = (src))  // Definisi ulang va_copy untuk kompatibilitas dengan Microsoft C
#endif

#if defined(_WIN32) && !defined(__MINGW64_VERSION_MAJOR)
#define strcasecmp stricmp       // Definisi ulang fungsi untuk perbandingan string tanpa memperhatikan huruf kapital pada Windows
#define strncasecmp strnicmp     // Definisi ulang fungsi untuk perbandingan string tanpa memperhatikan huruf kapital dengan batasan panjang
#endif

#ifndef _WIN32
#define HAVE_VASPRINTF 1        // Mendefinisikan macro untuk menggunakan vasprintf di sistem non-Windows
#define HAVE_ASPRINTF 1         // Mendefinisikan macro untuk menggunakan asprintf di sistem non-Windows
#endif

#define PROXYSOCKET_VERSION_STRINGIZE_(major, minor, micro) #major"."#minor"."#micro
#define PROXYSOCKET_VERSION_STRINGIZE(major, minor, micro) PROXYSOCKET_VERSION_STRINGIZE_(major, minor, micro)

#define PROXYSOCKET_VERSION_STRING PROXYSOCKET_VERSION_STRINGIZE(PROXYSOCKET_VERSION_MAJOR, PROXYSOCKET_VERSION_MINOR, PROXYSOCKET_VERSION_MICRO)

#define USE_CLIENT_DNS  0       // Mendefinisikan opsi untuk tidak menggunakan DNS client
#define USE_PROXY_DNS   1       // Mendefinisikan opsi untuk menggunakan DNS proxy

struct proxysocketconfig_struct {
  struct proxyinfo_struct* proxyinfolist;  // Daftar informasi proxy
  proxysocketconfig_log_fn log_function;   // Fungsi logging
  void* log_data;                          // Data untuk fungsi logging
  int8_t proxy_dns;                        // Opsi untuk menggunakan DNS proxy
  uint32_t sendtimeout;                    // Waktu tunggu untuk pengiriman
  uint32_t recvtimeout;                    // Waktu tunggu untuk penerimaan
};

struct proxyinfo_struct {
  int proxytype;       // Tipe proxy
  char* proxyhost;     // Host proxy
  uint16_t proxyport;  // Port proxy
  char* proxyuser;     // Username proxy
  char* proxypass;     // Password proxy
  struct proxyinfo_struct* next;  // Pointer ke informasi proxy berikutnya
};

#ifndef HAVE_VASPRINTF
int vasprintf(char** strp, const char* fmt, va_list ap)
{
  va_list aq;
  int len;
  // Mengalokasikan buffer untuk format data
  va_copy(aq, ap);
  len = vsnprintf(NULL, 0, fmt, aq);   // Menghitung panjang string yang dibutuhkan
  if ((*strp = (char*)malloc(len + 1)) == NULL) {  // Mengalokasikan memori
    return -1;
  }
  // Format data logging
  vsnprintf(*strp, len + 1, fmt, ap);
  return len;
}
#endif

#ifndef HAVE_ASPRINTF
int asprintf(char** strp, const char* fmt, ...)
{
  int len;
  va_list ap;
  va_start(ap, fmt);
  len = vasprintf(strp, fmt, ap);   // Menggunakan vasprintf untuk format string
  va_end(ap);
  return len;
}
#endif

int appendsprintf(char** dststrp, int dststrlen, const char* fmt, ...)
{
  int len;
  char* str;
  va_list ap;
  va_start(ap, fmt);
  len = vasprintf(&str, fmt, ap);   // Format string baru
  va_end(ap);
  // Menambahkan hasil format ke string tujuan
  if (len >= 0) {
    if (dststrlen < 0)
      dststrlen = (*dststrp ? strlen(*dststrp) : 0);  // Mengatur panjang string tujuan
    len = dststrlen + len;
    if ((*dststrp = realloc(*dststrp, len + 1)) == NULL)  // Mengalokasikan memori baru
      len = -1;
    else
      strcpy(*dststrp + dststrlen, str);  // Menyalin string baru ke string tujuan
    free(str);
  }
  return len;
}

// pesan error untuk alokasi memori
static const char* memory_allocation_error = "Memory allocation error";

// fungsi untuk mendapatkan alamat IPV4 dari host name
uint32_t get_ipv4_address (const char* hostname)
{
  uint32_t addr;
  struct hostent* hostdata;
  // Jika hostname tidak valid, kembalikan INADDR_NONE
  if (!hostname || !*hostname)
    return INADDR_NONE;
  // Cek apakah hostname adalah alamat IP bertitik
  if ((addr = inet_addr(hostname)) != INADDR_NONE)
    return addr;
  // Cari hostname (proses ini akan memblokir), catatan: gethostbyname() tidak thread-safe
  if ((hostdata = gethostbyname(hostname)) != NULL)
    memcpy(&addr, hostdata->h_addr_list[0], sizeof(addr));
  else
    addr = INADDR_NONE;
  return addr;
}

// Fungsi untuk membuat string Base64 dari string input
char* make_base64_string (const char* str)
{
  int done = 0;
  char base64_table[256];
  int i;
  // Alokasikan buffer untuk hasil encoding Base64
  char* buf = (char*)malloc((strlen(str) + 2) / 3 * 4 + 1);
  char* dst = buf;
  const char* src = str;
  // Isi base64_table dengan encoding karakter
  for (i = 0; i < 26; i++) {
    base64_table[i] = (char)('A' + i);
    base64_table[26 + i] = (char)('a' + i);
  }
  for (i = 0; i < 10; i++) {
    base64_table[52 + i] = (char)('0' + i);
  }
  base64_table[62] = '+';
  base64_table[63] = '/';
  // Encode data ke dalam format Base64
  while (!done) {
    char igroup[3];
    int n;

    //baca input
    igroup[0] = igroup[1] = igroup[2] = 0;
    for (n = 0; n < 3; n++) {
      if (!*src) {
        done++;
        break;
      }
      igroup[n] = *src++;
    }
    //encode bagian data yang di baca
    if (n > 0) {
      *dst++ = base64_table[igroup[0] >> 2];
      *dst++ = base64_table[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
      *dst++ = (n < 2 ? '=' : base64_table[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)]);
      *dst++ = (n < 3 ? '=' : base64_table[igroup[2] & 0x3F]);
    }
  }
  *dst = 0;
  return buf;
}

// Fungsi untuk mengirimkan request/permintaan HTTP dan mendapatkan respons
int send_http_request (SOCKET sock, const char* request, char** response)
{
  char* responseline;
  int responselinelen;
  int responselen = 0;
  int resultcode = -1;
  char* p;
  // Jika response tidak null, inisialisasi dengan NULL
  if (response)
    *response = NULL;
  // Kirim request/permintaan HTTP
  if (request) {
    if (send(sock, request, strlen(request), 0) == -1)
      return -1;
  }
  //ambil kode hasil dari respons HTTP
  if ((responseline = socket_receiveline(sock)) == NULL)
    return -1;
  if (strncasecmp(responseline, "HTTP/", 5) != 0)
    return -1;
  p = responseline + 5;
  while (*p && (isdigit(*p) || *p == '.'))
    p++;
  if (!*p || *p++ != ' ')
    return -1;
  if ((resultcode = strtol(p, &p, 10)) == 0)
    return -1;
  //ambil seluruh header dan respons
  while (responseline && *responseline) {
    if (response) {
      responselinelen = strlen(responseline);
      *response = (char*)realloc(*response, responselen + responselinelen + 2);
      memcpy(*response + responselen, responseline, responselinelen);
      responselen += responselinelen;
      (*response)[responselen++] = '\n';
    }
    free(responseline);
    responseline = socket_receiveline(sock);
  }
  if (responseline)
    free(responseline);
  if (response)
    (*response)[responselen] = 0;
  return resultcode;
}

/* * * definisi yang dibutuhkan untuk klien proxy SOCKS4 * * */

#define SOCKS4_VERSION   0x04  // Versi protokol SOCKS4

#define SOCKS4_COMMAND_CONNECT   0x01  // Perintah untuk koneksi SOCKS4
//#define SOCKS4_COMMAND_BIND      0x02  // Perintah untuk bind SOCKS4 (tidak digunakan)

#define SOCKS4_STATUS_SUCCESS                    90  // Status sukses SOCKS4
#define SOCKS4_STATUS_FAILED                     91  // Status gagal SOCKS4
#define SOCKS4_STATUS_IDENT_FAILED               92  // Status identifikasi gagal SOCKS4
#define SOCKS4_STATUS_IDENT_MISMATCH             93  // Status identifikasi tidak cocok SOCKS4

// harus dikompilasi dengan -mno-ms-bitfields
#pragma pack(1)
struct __attribute__((packed, aligned(1))) socks4_connect_request {
  uint8_t socks_version;  // Versi protokol SOCKS
  uint8_t socks_command;  // Perintah SOCKS
  uint16_t dst_port;      // Port tujuan
  uint32_t dst_addr;      // Alamat IP tujuan
  uint8_t userid[1];      // ID pengguna (tidak digunakan dalam SOCKS4)
};

/* * * definisi yang dibutuhkan untuk klien proxy SOCKS5 * * */

#define SOCKS5_VERSION   0x05  // Versi protokol SOCKS5

#define SOCKS5_METHOD_NOAUTH 0x00  // Metode otentikasi tanpa otentikasi
//#define SOCKS5_METHOD_GSSAPI 0x01  // Metode otentikasi GSSAPI (tidak digunakan)
#define SOCKS5_METHOD_LOGIN  0x02  // Metode otentikasi login
#define SOCKS5_METHOD_NONE   0xFF  // Metode otentikasi tidak didukung

#define SOCKS5_COMMAND_CONNECT   0x01  // Perintah untuk koneksi SOCKS5
//#define SOCKS5_COMMAND_BIND      0x02  // Perintah untuk bind SOCKS5 (tidak digunakan)

#define SOCKS5_ADDRESSTYPE_IPV4        0x01  // Tipe alamat IPv4
#define SOCKS5_ADDRESSTYPE_DOMAINNAME  0x03  // Tipe alamat domain
#define SOCKS5_ADDRESSTYPE_IPV6        0x04  // Tipe alamat IPv6

#define SOCKS5_STATUS_SUCCESS                    0x00  // Status sukses SOCKS5
#define SOCKS5_STATUS_SOCKS_SERVER_FAILURE       0x01  // Status gagal server SOCKS5
#define SOCKS5_STATUS_DENIED                     0x02  // Status ditolak SOCKS5
#define SOCKS5_STATUS_NETWORK_UNREACHABLE        0x03  // Status jaringan tidak dapat dijangkau SOCKS5
#define SOCKS5_STATUS_HOST_UNREACHABLE           0x04  // Status host tidak dapat dijangkau SOCKS5
#define SOCKS5_STATUS_CONNECTION_REFUSED         0x05  // Status koneksi ditolak SOCKS5
#define SOCKS5_STATUS_TTL_EXPIRES                0x06  // Status TTL kadaluarsa SOCKS5
#define SOCKS5_STATUS_COMMAND_NOT_SUPPORTED      0x07  // Status perintah tidak didukung SOCKS5
#define SOCKS5_STATUS_ADDRESS_TYPE_NOT_SUPPORTED 0x08  // Status tipe alamat tidak didukung SOCKS5

// harus dikompilasi dengan -mno-ms-bitfields
#pragma pack(1)
struct __attribute__((packed, aligned(1))) socks5_connect_request_ipv4 {
  uint8_t socks_version;  // Versi protokol SOCKS
  uint8_t socks_command;  // Perintah SOCKS
  uint8_t reserved;       // Cadangan (dibiarkan 0)
  uint8_t socks_addresstype;  // Tipe alamat SOCKS5
  uint32_t dst_addr;      // Alamat IP tujuan
  uint16_t dst_port;      // Port tujuan
};

void write_log_info (proxysocketconfig proxy, int level, const char* fmt, ...)
{
  if (proxy && proxy->log_function) {
    va_list ap;
    char* msg;
    int msglen;
    va_start(ap, fmt);
    msglen = vasprintf(&msg, fmt, ap);
    va_end(ap);
    if (msglen < 0) {
      proxy->log_function(level, memory_allocation_error, proxy->log_data);
      return;
    }
    // pass data logging ke fungsi pengguna
    proxy->log_function(level, msg, proxy->log_data);
    //membersihkan
    free(msg);
    va_end(ap);
  }
}

void proxyinfolist_free (struct proxyinfo_struct* proxyinfo)
{
  struct proxyinfo_struct* next;
  struct proxyinfo_struct* current = proxyinfo;
  while (current) {
    next = current->next;
    if (current->proxyhost)
      free(current->proxyhost);
    if (current->proxyuser)
      free(current->proxyuser);
    if (current->proxypass)
      free(current->proxypass);
    free(current);
    current = next;
  }
}

DLL_EXPORT_PROXYSOCKET void proxysocket_get_version (int* pmajor, int* pminor, int* pmicro)
{
  if (pmajor)
    *pmajor = PROXYSOCKET_VERSION_MAJOR;
  if (pminor)
    *pminor = PROXYSOCKET_VERSION_MINOR;
  if (pmicro)
    *pmicro = PROXYSOCKET_VERSION_MICRO;
}

DLL_EXPORT_PROXYSOCKET const char* proxysocket_get_version_string ()
{
  return PROXYSOCKET_VERSION_STRING;
}

DLL_EXPORT_PROXYSOCKET const char* proxysocketconfig_get_type_name (int proxytype)
{
  switch (proxytype) {
    case PROXYSOCKET_TYPE_NONE:
      return "NONE"; // type proxy tidak ada
    case PROXYSOCKET_TYPE_SOCKS4:
      return "SOCKS4A"; // type proxy SOCK4A
    case PROXYSOCKET_TYPE_SOCKS5:
      return "SOCKS5"; // type proxy SOCKS5
    case PROXYSOCKET_TYPE_WEB_CONNECT:
      return "WEB"; // type proxy WEB
    default:
      return "INVALID"; // type proxy tidak valid
  }
}

DLL_EXPORT_PROXYSOCKET int proxysocketconfig_get_name_type (const char* proxytypename)
{
  if (strcasecmp(proxytypename, "NONE") == 0 || strcasecmp(proxytypename, "DIRECT") == 0)
    return PROXYSOCKET_TYPE_NONE;
  if (strcasecmp(proxytypename, "SOCKS4A") == 0 || strcasecmp(proxytypename, "SOCKS4") == 0)
    return PROXYSOCKET_TYPE_SOCKS4;
  if (strcasecmp(proxytypename, "SOCKS5") == 0)
    return PROXYSOCKET_TYPE_SOCKS5;
  if (strcasecmp(proxytypename, "WEB") == 0 || strcasecmp(proxytypename, "HTTP") == 0)
    return PROXYSOCKET_TYPE_WEB_CONNECT;
  return PROXYSOCKET_TYPE_INVALID;
}

DLL_EXPORT_PROXYSOCKET int proxysocket_initialize ()
{
#ifdef __WIN32__
  //inisialisasi WINSOCK
  static WSADATA wsaData;
  int wsaerr = WSAStartup(MAKEWORD(1, 0), &wsaData);
  if (wsaerr) {
    //fprintf(stderr, "Error calling WSAStartup\n");
    return wsaerr;
  }
  atexit((void(*)())WSACleanup);
#endif
  return 0;
}

DLL_EXPORT_PROXYSOCKET proxysocketconfig proxysocketconfig_create_direct ()
{
  struct proxysocketconfig_struct* proxy;
  if ((proxy = (struct proxysocketconfig_struct*)malloc(sizeof(struct proxysocketconfig_struct))) == NULL)
    return NULL;
  proxy->proxyinfolist = NULL;
  proxy->log_function = NULL;
  proxy->log_data = NULL;
  proxy->proxy_dns = USE_CLIENT_DNS;
  proxy->sendtimeout = 0;
  proxy->recvtimeout = 0;
  if (proxysocketconfig_add_proxy(proxy, PROXYSOCKET_TYPE_NONE, NULL, 0, NULL, NULL) != 0) {
    free(proxy);
    return NULL;
  }
  return proxy;
}

DLL_EXPORT_PROXYSOCKET proxysocketconfig proxysocketconfig_create (int proxytype, const char* proxyhost, uint16_t proxyport, const char* proxyuser, const char* proxypass)
{
  struct proxysocketconfig_struct* proxy;
  proxy = proxysocketconfig_create_direct();
  if (proxysocketconfig_add_proxy(proxy, proxytype, proxyhost, proxyport, proxyuser, proxypass) != 0) {
    free(proxy);
    return NULL;
  }
  return proxy;
}

// Fungsi untuk menambahkan konfigurasi proxy baru
DLL_EXPORT_PROXYSOCKET int proxysocketconfig_add_proxy (proxysocketconfig proxy, int proxytype, const char* proxyhost, uint16_t proxyport, const char* proxyuser, const char* proxypass)
{
  // Menentukan entri berikutnya (membersihkan dan menghapus jika koneksi langsung dimasukkan sebelumnya)
  struct proxyinfo_struct* next = proxy->proxyinfolist;
  if (proxytype == PROXYSOCKET_TYPE_NONE && next) {
    proxyinfolist_free(next);
    next = NULL;
  }
  // Menyisipkan proxy baru di depan daftar
  if (!proxy || (proxy->proxyinfolist = (struct proxyinfo_struct*)malloc(sizeof(struct proxyinfo_struct))) == NULL)
    return -1;
  proxy->proxyinfolist->proxytype = proxytype;
  proxy->proxyinfolist->proxyhost = (proxyhost ? strdup(proxyhost) : NULL);
  proxy->proxyinfolist->proxyport = proxyport;
  proxy->proxyinfolist->proxyuser = (proxyuser ? strdup(proxyuser) : NULL);
  proxy->proxyinfolist->proxypass = (proxypass ? strdup(proxypass) : NULL);
  proxy->proxyinfolist->next = next;
  return 0;
}

// Fungsi untuk mendapatkan deskripsi entri proxy
char* proxysocketconfig_get_description_entry (proxysocketconfig proxy, struct proxyinfo_struct* proxyinfo, char* desc, int desclen)
{
  if (!proxy || !proxyinfo)
    return desc;
  if (proxyinfo != proxy->proxyinfolist)
    desclen = appendsprintf(&desc, desclen, " -> ");
  switch (proxyinfo->proxytype) {
    case PROXYSOCKET_TYPE_NONE :
      desclen = appendsprintf(&desc, desclen, "direct connection");
      break;
    case PROXYSOCKET_TYPE_SOCKS4 :
      desclen = appendsprintf(&desc, desclen, "SOCKS4 proxy: %s:%u (%s%s)", proxyinfo->proxyhost, (unsigned int)proxyinfo->proxyport, (!proxyinfo->proxyuser || !*proxyinfo->proxyuser ? "no authentication" : "user: "), (!proxyinfo->proxyuser || !*proxyinfo->proxyuser ? "" : proxyinfo->proxyuser));
      break;
    case PROXYSOCKET_TYPE_SOCKS5 :
      desclen = appendsprintf(&desc, desclen, "SOCKS5 proxy: %s:%u (%s%s)", proxyinfo->proxyhost, (unsigned int)proxyinfo->proxyport, (!proxyinfo->proxyuser || !*proxyinfo->proxyuser ? "no authentication" : "user: "), (!proxyinfo->proxyuser || !*proxyinfo->proxyuser ? "" : proxyinfo->proxyuser));
      break;
    case PROXYSOCKET_TYPE_WEB_CONNECT :
      desclen = appendsprintf(&desc, desclen, "web proxy: %s:%u (%s%s)", proxyinfo->proxyhost, (unsigned int)proxyinfo->proxyport, (!proxyinfo->proxyuser || !*proxyinfo->proxyuser ? "no authentication" : "user: "), (!proxyinfo->proxyuser || !*proxyinfo->proxyuser ? "" : proxyinfo->proxyuser));
      break;
    //case PROXYSOCKET_TYPE_INVALID :
    default :
      desclen = appendsprintf(&desc, desclen, "INVALID");
      break;
  }
  if (!proxyinfo->next || proxyinfo->next->proxytype == PROXYSOCKET_TYPE_NONE)
    return desc;
  return proxysocketconfig_get_description_entry(proxy, proxyinfo->next, desc, desclen);
}

// Fungsi untuk mendapatkan deskripsi dari konfigurasi proxy
DLL_EXPORT_PROXYSOCKET char* proxysocketconfig_get_description (proxysocketconfig proxy)
{
  char* result = NULL;
  int resultlen = 0;
  if (!proxy || !proxy->proxyinfolist)
    return NULL;
  if (proxy->proxyinfolist && proxy->proxyinfolist->proxytype != PROXYSOCKET_TYPE_NONE)
  resultlen = appendsprintf(&result, 0, "(use %s DNS) ", (proxy->proxy_dns == USE_PROXY_DNS ? "server" : "client"));
  return proxysocketconfig_get_description_entry(proxy, proxy->proxyinfolist, result, resultlen);
}

// Fungsi untuk mengatur fungsi log dan data pengguna untuk log
DLL_EXPORT_PROXYSOCKET void proxysocketconfig_set_logging (proxysocketconfig proxy, proxysocketconfig_log_fn log_fn, void* userdata)
{
  proxy->log_function = log_fn;
  proxy->log_data = userdata;
}

// Fungsi untuk mengatur timeout pengiriman dan penerimaan
DLL_EXPORT_PROXYSOCKET void proxysocketconfig_set_timeout (proxysocketconfig proxy, uint32_t sendtimeout, uint32_t recvtimeout)
{
  proxy->sendtimeout = sendtimeout;
  proxy->recvtimeout = recvtimeout;
}

// Fungsi untuk menggunakan DNS proxy
DLL_EXPORT_PROXYSOCKET void proxysocketconfig_use_proxy_dns (proxysocketconfig proxy, int proxy_dns)
{
  proxy->proxy_dns = (proxy_dns == 0 ? USE_CLIENT_DNS : USE_PROXY_DNS);
}

// Fungsi untuk membebaskan memori konfigurasi proxy
DLL_EXPORT_PROXYSOCKET void proxysocketconfig_free (proxysocketconfig proxy)
{
  if (proxy) {
    proxyinfolist_free(proxy->proxyinfolist);
    free(proxy);
  }
}

// Fungsi untuk mencatat dan menyimpan pesan kesalahan
void log_and_keep_error_message (proxysocketconfig proxy, char** pmsg, const char* fmt, ...)
{
  if (fmt && (proxy->log_function || pmsg)) {
    char* msg;
    int msglen;
    //membuat pesan
    va_list ap;
    va_start(ap, fmt);
    msglen = vasprintf(&msg, fmt, ap);
    va_end(ap);
    //mencatata pesan
    if (msglen >= 0) {
      if (proxy->log_function)
        proxy->log_function(PROXYSOCKET_LOG_ERROR, msg, proxy->log_data);
      if (pmsg)
        *pmsg = msg;
      else
        free(msg);
    }
  }
}

#define ERROR_DISCONNECT_AND_ABORT(...) \
{ \
  log_and_keep_error_message(proxy, errmsg, __VA_ARGS__); \
  if (sock != INVALID_SOCKET) \
    proxysocket_disconnect(proxy, sock); \
  return INVALID_SOCKET; \
}

// Fungsi untuk menghubungkan ke server proxy
SOCKET proxyinfo_connect (proxysocketconfig proxy, struct proxyinfo_struct* proxyinfo, const char* dsthost, uint16_t dstport, char** errmsg)
{
  uint32_t hostaddr;
  uint32_t proxyaddr;
  SOCKET sock = INVALID_SOCKET;
  // Jika informasi proxy tidak ada, log error dan putuskan koneksi
  if (!proxyinfo)
    ERROR_DISCONNECT_AND_ABORT("Informasi proxy tidak ada")
  // Resolusi host tujuan jika diperlukan (ketika DNS klien digunakan atau untuk koneksi langsung)
  if (proxy->proxy_dns == USE_CLIENT_DNS || proxyinfo->proxytype == PROXYSOCKET_TYPE_NONE) {
    if ((hostaddr = get_ipv4_address(dsthost)) == INADDR_NONE)
      ERROR_DISCONNECT_AND_ABORT("Error saat mencari host: %s", dsthost)
    write_log_info(proxy, PROXYSOCKET_LOG_DEBUG, "Host %s di-resolve menjadi IP: %s", dsthost, inet_ntoa(*(struct in_addr*)&hostaddr));
  } else {
    hostaddr = INADDR_NONE;
  }
  // Resolusi alamat host proxy
  proxyaddr = INADDR_NONE;
  if (proxyinfo->proxyhost && *proxyinfo->proxyhost) {
    if ((proxyaddr = get_ipv4_address(proxyinfo->proxyhost)) == INADDR_NONE)
      ERROR_DISCONNECT_AND_ABORT("Error saat mencari host proxy: %s", proxyinfo->proxyhost)
    write_log_info(proxy, PROXYSOCKET_LOG_DEBUG, "Host proxy %s di-resolve menjadi IP: %s", proxyinfo->proxyhost, inet_ntoa(*(struct in_addr*)&proxyaddr));
  }
  // Jika alamat proxy tidak ditemukan dan tipe proxy bukan 'none', log error dan putuskan koneksi
  if (proxyaddr == INADDR_NONE && proxyinfo->proxytype != PROXYSOCKET_TYPE_NONE)
    ERROR_DISCONNECT_AND_ABORT("Host proxy tidak ada")

  // Jika tipe proxy adalah 'none', lakukan koneksi langsung tanpa proxy
  if (proxyinfo->proxytype == PROXYSOCKET_TYPE_NONE) {
    /* * * KONEKSI LANGSUNG TANPA PROXY * * */
    // Buat socket
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
      ERROR_DISCONNECT_AND_ABORT("Kesalahan saat membuat socket koneksi")

    /*
    // Atur opsi linger
    const struct linger option_linger = {~0, 3};   // linger 3 detik saat memutuskan koneksi
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&option_linger, sizeof(option_linger));
    */

    //Bind socket
    if ((proxyaddr != INADDR_NONE && proxyaddr != INADDR_ANY) || proxyinfo->proxyport) {
      struct sockaddr_in local_sock_addr;
      local_sock_addr.sin_family = AF_INET;
      local_sock_addr.sin_port = htons(proxyinfo->proxyport);
      local_sock_addr.sin_addr.s_addr = (proxyaddr == INADDR_NONE ? INADDR_ANY : proxyaddr);
      write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Binding ke: %s:%lu", inet_ntoa(*(struct in_addr*)&local_sock_addr.sin_addr.s_addr), (unsigned long)ntohs(local_sock_addr.sin_port));
      if (bind(sock, (struct sockaddr*)&local_sock_addr, sizeof(local_sock_addr)) != 0)
        ERROR_DISCONNECT_AND_ABORT("Error saat binding socket ke: %s:%lu", inet_ntoa(*(struct in_addr*)&local_sock_addr.sin_addr.s_addr), (unsigned long)ntohs(local_sock_addr.sin_port))
    }
    //atur time out koneksi
    socket_set_timeouts_milliseconds(sock, proxy->sendtimeout, proxy->recvtimeout);
    //koneksi ke host
    struct sockaddr_in remote_sock_addr;
    remote_sock_addr.sin_family = AF_INET;
    remote_sock_addr.sin_port = htons(dstport);
    remote_sock_addr.sin_addr.s_addr = hostaddr;
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Koneksi ke host: %s:%lu", inet_ntoa(*(struct in_addr*)&remote_sock_addr.sin_addr.s_addr), (unsigned long)dstport);
    if (connect(sock, (struct sockaddr*)&remote_sock_addr, sizeof(remote_sock_addr)) == SOCKET_ERROR)
      ERROR_DISCONNECT_AND_ABORT("Error saat koneksi ke host: %s:%lu", inet_ntoa(*(struct in_addr*)&hostaddr), (unsigned long)dstport)
  } else if (proxyinfo->proxytype == PROXYSOCKET_TYPE_SOCKS4) {
    /* * * KONEKSI MENGGUNAKAN PROXY SOCKS4 * * */
    // Hubungkan ke server proxy SOCKS4
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Preparing to connect to SOCKS4 proxy: %s:%lu", inet_ntoa(*(struct in_addr*)&proxyaddr), (unsigned long)proxyinfo->proxyport);
    if ((sock = proxyinfo_connect(proxy, proxyinfo->next, proxyinfo->proxyhost, proxyinfo->proxyport, errmsg)) == INVALID_SOCKET)
      //ERROR_DISCONNECT_AND_ABORT("Error connecting to SOCKS4 proxy: %s:%lu", proxyinfo->proxyhost, (unsigned long)proxyinfo->proxyport);/////
      ERROR_DISCONNECT_AND_ABORT(NULL);
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Terhubung ke proxy SOCKS4: %s:%lu", inet_ntoa(*(struct in_addr*)&proxyaddr), (unsigned long)proxyinfo->proxyport);
    //kIRIM Permintaan konesi
    struct socks4_connect_request* request;
    size_t requestlen = sizeof(struct socks4_connect_request);
    if ((request = (struct socks4_connect_request*)malloc(requestlen)) == NULL)
      ERROR_DISCONNECT_AND_ABORT(memory_allocation_error)
    request->socks_version = SOCKS4_VERSION;
    request->socks_command = SOCKS4_COMMAND_CONNECT;
    request->dst_port = htons(dstport);
    request->dst_addr = (proxy->proxy_dns == USE_CLIENT_DNS ? hostaddr : htonl(0x000000FF));
    request->userid[0] = 0;
    if (proxyinfo->proxyuser && *proxyinfo->proxyuser) {
      requestlen += strlen(proxyinfo->proxyuser);
      if ((request = (struct socks4_connect_request*)realloc(request, requestlen)) == NULL)
        ERROR_DISCONNECT_AND_ABORT(memory_allocation_error)
      strcpy((char*)&(request->userid), proxyinfo->proxyuser);
    }
    if (proxy->proxy_dns) {
      size_t dsthostlen = (dsthost ? strlen(dsthost) : 0);
      if ((request = (struct socks4_connect_request*)realloc(request, requestlen + dsthostlen + 1)) == NULL)
        ERROR_DISCONNECT_AND_ABORT(memory_allocation_error)
      strcpy(((char*)request) + requestlen, (dsthost ? dsthost : ""));
      requestlen += dsthostlen + 1;
    }
    if (!(proxyinfo->proxyuser && *proxyinfo->proxyuser))
      write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Menguhubungkan ke tujuan: %s:%lu", (proxy->proxy_dns == USE_CLIENT_DNS ? inet_ntoa(*(struct in_addr*)&hostaddr) : dsthost), (unsigned long)dstport);
    else
      write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Menghubungkan ke tujuan: %s:%lu (user-id: %s)", (proxy->proxy_dns == USE_CLIENT_DNS ? inet_ntoa(*(struct in_addr*)&hostaddr) : dsthost), (unsigned long)dstport, proxyinfo->proxyuser);
    if (send(sock, (void*)request, requestlen, 0) < requestlen)
      ERROR_DISCONNECT_AND_ABORT("Error saat mengirim perintah koneksi ke proxy SOCKS4")
    free(request);

    //terima respons
    struct socks4_connect_request response;
    if (recv(sock, (void*)&response, 8, 0) < 8)
      ERROR_DISCONNECT_AND_ABORT("Koneksi terputus saat membaca respons koneksi dari proxy SOCKS4")
    //display response information
    if (response.socks_version != 0)
      write_log_info(proxy, PROXYSOCKET_LOG_WARNING, "Invalid SOCKS4 reply code version (%u)", (unsigned int)response.socks_version);
    switch (response.socks_command) {
      case SOCKS4_STATUS_SUCCESS :
        write_log_info(proxy, PROXYSOCKET_LOG_INFO, "SOCKS4 proxy connection established to: %s:%lu", inet_ntoa(*(struct in_addr*)&hostaddr), (unsigned long)dstport);
        break;
      case SOCKS4_STATUS_FAILED :
        ERROR_DISCONNECT_AND_ABORT("SOCKS4 connection rejected or failed")
      case SOCKS4_STATUS_IDENT_FAILED :
        ERROR_DISCONNECT_AND_ABORT("SOCKS4 request rejected because SOCKS server cannot connect to identd on the client")
      case SOCKS4_STATUS_IDENT_MISMATCH :
        ERROR_DISCONNECT_AND_ABORT("SOCKS4 request rejected because the client program and identd report different user-ids")
      default :
        ERROR_DISCONNECT_AND_ABORT("Unsupported reply from SOCKS4 server (%u)", (unsigned int)response.socks_command)
    }
  } else if (proxyinfo->proxytype == PROXYSOCKET_TYPE_SOCKS5) {
    /* * * CONNECTION USING SOCKS5 PROXY * * */
    //connect to the SOCKS5 proxy server
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Preparing to connect to SOCKS5 proxy: %s:%lu", proxyinfo->proxyhost, (unsigned long)proxyinfo->proxyport);
    if ((sock = proxyinfo_connect(proxy, proxyinfo->next, proxyinfo->proxyhost, proxyinfo->proxyport, errmsg)) == INVALID_SOCKET)
      //ERROR_DISCONNECT_AND_ABORT("Error connecting to SOCKS5 proxy: %s:%lu", proxyinfo->proxyhost, (unsigned long)proxyinfo->proxyport);/////
      ERROR_DISCONNECT_AND_ABORT(NULL);
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Connected to SOCKS5 proxy: %s:%lu", proxyinfo->proxyhost, (unsigned long)proxyinfo->proxyport);
    //send initial data
    if (!(proxyinfo->proxyuser && *proxyinfo->proxyuser) && !(proxyinfo->proxypass && *proxyinfo->proxypass)) {
      uint8_t initsend[] = {SOCKS5_VERSION, 1, SOCKS5_METHOD_NOAUTH};
      if (send(sock, (void*)initsend, sizeof(initsend), 0) < sizeof(initsend))
        ERROR_DISCONNECT_AND_ABORT("Error sending data to SOCKS5 proxy")
    } else {
      uint8_t initsend[] = {SOCKS5_VERSION, 2, SOCKS5_METHOD_LOGIN, SOCKS5_METHOD_NOAUTH};
      if (send(sock, (void*)initsend, sizeof(initsend), 0) < sizeof(initsend))
        ERROR_DISCONNECT_AND_ABORT("Error sending data to SOCKS5 proxy")
    }
    //receive initial data
    uint8_t initrecv[2];
    if (recv(sock, (void*)initrecv, sizeof(initrecv), 0) < sizeof(initrecv))
      ERROR_DISCONNECT_AND_ABORT("Connection lost while reading data from SOCKS5 proxy")
    //display response information
    if (initrecv[0] != SOCKS5_VERSION)
      write_log_info(proxy, PROXYSOCKET_LOG_WARNING, "SOCKS5 proxy version mismatch (%u)", (unsigned int)initrecv[0]);
    static const char* methodmsg = "SOCKS5 proxy authentication method: %s";
    switch (initrecv[1]) {
      case SOCKS5_METHOD_NOAUTH :
        write_log_info(proxy, PROXYSOCKET_LOG_INFO, methodmsg, "no authentication required");
        break;
/////////////////////////////////////////////////////////////////////////////////////////////////////
/*
      case SOCKS5_METHOD_GSSAPI :
        write_log_info(proxy, PROXYSOCKET_LOG_INFO, methodmsg, "GSSAPI");
        break;
*/
      case SOCKS5_METHOD_LOGIN :
        write_log_info(proxy, PROXYSOCKET_LOG_INFO, methodmsg, "username/password");
        break;
      case SOCKS5_METHOD_NONE :
        write_log_info(proxy, PROXYSOCKET_LOG_ERROR, methodmsg, "no compatible methods");
        ERROR_DISCONNECT_AND_ABORT("Unable to negociate SOCKS5 proxy authentication method")
      default :
        write_log_info(proxy, PROXYSOCKET_LOG_ERROR, methodmsg, "unknown (%u)", (unsigned int)initrecv[1]);
        ERROR_DISCONNECT_AND_ABORT("Received unknown SOCKS5 proxy authentication method (%u)", (unsigned int)initrecv[1])
    }
    //authenticate if needed
    if (initrecv[1] == SOCKS5_METHOD_LOGIN) {
      char* authbuf;
      size_t proxyuserlen = (proxyinfo->proxyuser ? strlen(proxyinfo->proxyuser) : 0);
      size_t proxypasslen = (proxyinfo->proxypass ? strlen(proxyinfo->proxypass) : 0);
      size_t authbuflen = 3 + proxyuserlen + proxypasslen;
      if ((authbuf = (char*)malloc(authbuflen)) == NULL)
        ERROR_DISCONNECT_AND_ABORT(memory_allocation_error)
      authbuf[0] = 1;
      authbuf[1] = proxyuserlen;
      memcpy(authbuf + 2, proxyinfo->proxyuser, proxyuserlen);
      authbuf[2 + proxyuserlen] = proxypasslen;
      memcpy(authbuf + 3 + proxyuserlen, proxyinfo->proxypass, proxypasslen);
      write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Sending authentication (login: %s)", (proxyinfo->proxyuser ? proxyinfo->proxyuser : NULL));
      if (send(sock, (void*)authbuf, authbuflen, 0) < authbuflen) {
        free(authbuf);
        ERROR_DISCONNECT_AND_ABORT("Error sending authentication data to SOCKS5 proxy")
      }
      free(authbuf);
      if (recv(sock, (void*)initrecv, sizeof(initrecv), 0) < sizeof(initrecv))
        ERROR_DISCONNECT_AND_ABORT("Connection lost while reading authentication response from SOCKS5 proxy")
      if (initrecv[0] != 1)
        ERROR_DISCONNECT_AND_ABORT("SOCKS5 proxy subnegotiation version mismatch (%u)", (unsigned int)initrecv[0])
      if (initrecv[1] == SOCKS5_STATUS_CONNECTION_REFUSED)
        ERROR_DISCONNECT_AND_ABORT("SOCKS5 access denied")
      if (initrecv[1] != 0)
        ERROR_DISCONNECT_AND_ABORT("SOCKS5 authentication failed with status code %u (login: %s)", (unsigned int)initrecv[1], (proxyinfo->proxyuser ? proxyinfo->proxyuser : NULL))
      /////TO DO: check and report status
      //write_log_info(proxy, PROXYSOCKET_LOG_INFO, "SOCKS5 authentication succeeded (login: %s)", (proxyinfo->proxyuser ? proxyinfo->proxyuser : NULL));
    }
    //send connect command
    if (proxy->proxy_dns == USE_CLIENT_DNS) {
      struct socks5_connect_request_ipv4 request = { SOCKS5_VERSION, SOCKS5_COMMAND_CONNECT, 0, SOCKS5_ADDRESSTYPE_IPV4, hostaddr, htons(dstport) };
      write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Connecting to IPv4 destination: %s:%lu", inet_ntoa(*(struct in_addr*)&hostaddr), (unsigned long)dstport);
      if (send(sock, (void*)&request, sizeof(request), 0) < sizeof(request))
        ERROR_DISCONNECT_AND_ABORT("Error sending connect command with IPv4 address to SOCKS5 proxy")
    } else {
      struct socks5_connect_request_ipv4* request;
      size_t dsthostlen = (dsthost ? strlen(dsthost) : 0);
      int requestlen = sizeof(struct socks5_connect_request_ipv4) - sizeof(uint32_t) + 1 + dsthostlen;
      request = (struct socks5_connect_request_ipv4*)malloc(requestlen);
      request->socks_version = SOCKS5_VERSION;
      request->socks_command = SOCKS5_COMMAND_CONNECT;
      request->reserved = 0;
      request->socks_addresstype = SOCKS5_ADDRESSTYPE_DOMAINNAME;
      *(uint8_t*)&(request->dst_addr) = dsthostlen;
      if (dsthostlen > 0)
        memcpy((void*)(((uint8_t*)&(request->dst_addr)) + 1), dsthost, dsthostlen);
      *(uint16_t*)(((uint8_t*)&(request->dst_addr)) + 1 + dsthostlen) = htons(dstport);
      write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Connecting to destination host: %s:%lu", dsthost, (unsigned long)dstport);
      if (send(sock, (void*)request, requestlen, 0) < requestlen)
        ERROR_DISCONNECT_AND_ABORT("Error sending connect command with hostname to SOCKS5 proxy")
      free(request);
    }
    //receive response
    if (recv(sock, (void*)initrecv, sizeof(initrecv), 0) < sizeof(initrecv))
      ERROR_DISCONNECT_AND_ABORT("Koneksi terputus saat membaca respons koneksi dari proxy SOCKS5")
    if (initrecv[0] != SOCKS5_VERSION)
      ERROR_DISCONNECT_AND_ABORT("SOCKS5 proxy version mismatch (%u)", (unsigned int)initrecv[0])
    switch (initrecv[1]) {
      case SOCKS5_STATUS_SUCCESS :
        // menulis informasi log jika koneksi SOCK5 behasil
        write_log_info(proxy, PROXYSOCKET_LOG_INFO, "SOCKS5 proxy connection established to: %s:%lu", inet_ntoa(*(struct in_addr*)&hostaddr), (unsigned long)dstport);
        break;
      case SOCKS5_STATUS_SOCKS_SERVER_FAILURE :
        ERROR_DISCONNECT_AND_ABORT("General SOCKS5 server failure")
      case SOCKS5_STATUS_DENIED :
        ERROR_DISCONNECT_AND_ABORT("Connection denied by SOCKS5 server")
      case SOCKS5_STATUS_NETWORK_UNREACHABLE :
        ERROR_DISCONNECT_AND_ABORT("SOCKS5 server response: Network unreachable")
      case SOCKS5_STATUS_HOST_UNREACHABLE :
        ERROR_DISCONNECT_AND_ABORT("SOCKS5 server response: Host unreachable")
      case SOCKS5_STATUS_CONNECTION_REFUSED :
        ERROR_DISCONNECT_AND_ABORT("SOCKS5 server response: Connection refused")
      case SOCKS5_STATUS_TTL_EXPIRES :
        ERROR_DISCONNECT_AND_ABORT("SOCKS5 server response: TTL expired")
      case SOCKS5_STATUS_COMMAND_NOT_SUPPORTED :
        ERROR_DISCONNECT_AND_ABORT("Command not supported by SOCKS5 server")
      case SOCKS5_STATUS_ADDRESS_TYPE_NOT_SUPPORTED :
        ERROR_DISCONNECT_AND_ABORT("Address type not supported by SOCKS5 server")
      default :
        ERROR_DISCONNECT_AND_ABORT("Unsupported status code from SOCKS5 server (%u)", (unsigned int)initrecv[1])
    }
    // Menerima data dari socket dan menyimpannya ke dalam variabel initrecv
    if (recv(sock, (void*)initrecv, sizeof(initrecv), 0) < sizeof(initrecv))
      ERROR_DISCONNECT_AND_ABORT("Koneksi terputus saat membaca respons koneksi dari proxy SOCKS5")
    // Memeriksa nilai yang diterima pada byte pertama dari respons SOCKS5
    if (initrecv[0] != 0)
      write_log_info(proxy, PROXYSOCKET_LOG_WARNING, "Nilai cadangan respons SOCKS5 yang diharapkan adalah nol (%u)", (unsigned int)initrecv[0]);
    // Memproses byte kedua dari respons SOCKS5 untuk menentukan jenis alamat
    switch (initrecv[1]) {
      case 1 : // Jenis alamat IPV4
        {
          uint32_t bindaddr;
          // Menerima alamat IPv4 dari socket
          if (recv(sock, (void*)&bindaddr, sizeof(bindaddr), 0) < sizeof(bindaddr))
            ERROR_DISCONNECT_AND_ABORT("Koneksi terputus saat membaca respons koneksi dari proxy SOCKS5")
          // menulis informasi alamat IPV4 ke log
          write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Koneksi SOCKS5 terhubung ke alamat IPv4: %s", inet_ntoa(*(struct in_addr*)&bindaddr));
        }
        break;
      case 3 : // Jenis alamat nama host
        {
          char* bindaddr;
          uint8_t bindaddrlen;
          // menerima nama panjang host
          if (recv(sock, (void*)&bindaddrlen, sizeof(bindaddrlen), 0) < sizeof(bindaddrlen))
            ERROR_DISCONNECT_AND_ABORT ("Koneksi terputus saat membaca respons koneksi dari proxy SOCKS5");
        // Mengalokasikan memori untuk nama host
          if ((bindaddr = (char*)malloc(bindaddrlen + 1)) == NULL)
            ERROR_DISCONNECT_AND_ABORT(memory_allocation_error)
          // Menerima nama host dari socket
          if (recv(sock, (void*)bindaddr, bindaddrlen, 0) < bindaddrlen)
            ERROR_DISCONNECT_AND_ABORT("Koneksi terputus saat membaca respons koneksi dari proxy SOCKS5")
          bindaddr[bindaddrlen] = 0; // Menambahkan null terminator pada akhir nama host
        // Menulis informasi nama host ke log
          write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Koneksi SOCKS5 terhubung ke host:: %s", bindaddr);
          free(bindaddr); // membebaskan memory yang dialokasikan
        }
        break;
      case 4 : // jenis alamat IPV6
        {
          uint8_t bindaddr[16];
          // Menerima alamat IPv6 dari socket
          if (recv(sock, (void*)bindaddr, sizeof(bindaddr), 0) < sizeof(bindaddr))
            ERROR_DISCONNECT_AND_ABORT("Koneksi terputus saat membaca respons koneksi dari proxy SOCKS5")
          //To do log alamat IPv6
          //write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Koneksi SOCKS5 terhubung ke alamat IPv6: %s", inet_ntoa(*(struct in_addr*)&bindaddr));
        }
        break;
      default :
      // Menangani jenis alamat yang tidak didukung
        ERROR_DISCONNECT_AND_ABORT("Jenis alamat SOCKS5 tidak didukung (%u)", (unsigned int)initrecv[1])
    }

    // menerima socket dari port
    uint16_t bindport;
    if (recv(sock, (void*)&bindport, sizeof(bindport), 0) < sizeof(bindport))
      ERROR_DISCONNECT_AND_ABORT("Koneksi terputus saat membaca respons koneksi dari proxy SOCKS5")
    // menulis informasi port ke log
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Koneksi SOCKS5 terhubung ke port: %lu", (unsigned long)bindport);
  } else if (proxyinfo->proxytype == PROXYSOCKET_TYPE_WEB_CONNECT) {
    /* * * KONEKSI MENGGUNAKAN PROXY HTTP/WEB * * */
    // Menghubungkan ke server proxy HTTP
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Preparing to connect to web proxy: %s:%lu", proxyinfo->proxyhost, (unsigned long)proxyinfo->proxyport);
    if ((sock = proxyinfo_connect(proxy, proxyinfo->next, proxyinfo->proxyhost, proxyinfo->proxyport, errmsg)) == INVALID_SOCKET)
      // ERROR_DISCONNECT_AND_ABORT("Kesalahan saat menghubungkan ke proxy web: %s:%lu", proxyinfo->proxyhost, (unsigned long)proxyinfo->proxyport);/////
      ERROR_DISCONNECT_AND_ABORT(NULL);
      // Menulis informasi koneksi ke proxy web ke log
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Terhubung ke proxy web: %s:%lu", proxyinfo->proxyhost, (unsigned long)proxyinfo->proxyport);
    
    // Menyiapkan data autentikasi dasar
    char* proxyauth = NULL;
    if (proxyinfo->proxyuser && *proxyinfo->proxyuser) {
      // Menulis informasi pengguna autentikasi proxy ke log
      write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Pengguna autentikasi proxy: %s", proxyinfo->proxyuser);
      char* userpass;
      int proxyuserlen = strlen(proxyinfo->proxyuser);
      // Mengalokasikan memori untuk string pengguna dan kata sandi
      if ((userpass = (char*)malloc(proxyuserlen + (proxyinfo->proxypass ? strlen(proxyinfo->proxypass) : 0) + 2)) == NULL)
        ERROR_DISCONNECT_AND_ABORT(memory_allocation_error)
      memcpy(userpass, proxyinfo->proxyuser, proxyuserlen); // Menyalin nama pengguna ke buffer
      userpass[proxyuserlen] = ':'; // Menambahkan pemisah antara nama pengguna dan kata sandi
      if (proxyinfo->proxypass)
        strcpy(userpass + proxyuserlen + 1, proxyinfo->proxypass); // Menyalin kata sandi ke buffer
      proxyauth = make_base64_string(userpass); // Mengkodekan string autentikasi dalam base64
      free(userpass); // memebebaskan memory yang di alokasikan
    }

    // Menghubungkan proxy ke tujuan
    int result;
    char* response;
    char* proxycmd;
    const char* host = (proxy->proxy_dns == USE_CLIENT_DNS ? inet_ntoa(*(struct in_addr*)&hostaddr) : (dsthost ? dsthost : ""));
    // Menghitung panjang perintah proxy yang akan dikirim
    size_t proxycmdlen = 22 + 15 + 1 + 5 + 1 + (proxyauth ? 29 + strlen(proxyauth) : 0);
    // Mencatat informasi log bahwa perintah CONNECT dikirim
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Mengirim HTTP proxy CONNECT %s:%u", host, dstport);
    // Mengalokasikan memori untuk perintah proxy
    if ((proxycmd = (char*)malloc(proxycmdlen)) == NULL)
      ERROR_DISCONNECT_AND_ABORT(memory_allocation_error)
    // Menyusun perintah CONNECT HTTP untuk dikirim
    snprintf(proxycmd, proxycmdlen, "CONNECT %s:%u HTTP/1.0%s%s\r\n\r\n", host, dstport, (proxyauth ? "\r\nProxy-Authorization: Basic " : ""), (proxyauth ? proxyauth : ""));
    // Membebaskan memori yang digunakan untuk autentikasi proxy jika ada
    if (proxyauth)
      free(proxyauth);
    // Mengirim request/permintaan HTTP dan menerima respons
    result = send_http_request(sock, proxycmd, &response);
    // Membebaskan memori yang digunakan untuk perintah proxy
    free(proxycmd);
    // Mencatat informasi log jika kode respons HTTP proxy tidak 200
    if (result != 200)
      write_log_info(proxy, PROXYSOCKET_LOG_DEBUG, "Kode respons HTTP proxy %i, rincian:\n%s", result, response);
    // Membebaskan memori yang digunakan untuk respons
    free(response);
    response = NULL;
    // Memeriksa rentang nilai kode respons untuk memastikan validitasnya
    if (result < 100 || result >= 600)
      ERROR_DISCONNECT_AND_ABORT("Respons tidak valid, mungkin bukan dari proxy web")
    // Menangani kode respons HTTP yang spesifik
    switch (result) {
      case 400 :
        ERROR_DISCONNECT_AND_ABORT("request/permintaan buruk")
      case 401 :
        ERROR_DISCONNECT_AND_ABORT("Autentikasi diperlukan")
      case 403 :
        ERROR_DISCONNECT_AND_ABORT("Akses ditolak")
      case 404 :
        ERROR_DISCONNECT_AND_ABORT("Tidak ditemukan")
      case 405 :
        ERROR_DISCONNECT_AND_ABORT("Metode tidak diizinkan")
      case 407 :
        if (proxyinfo->proxyuser && *proxyinfo->proxyuser)
          ERROR_DISCONNECT_AND_ABORT("Autentikasi proxy gagal (pengguna: %s)", proxyinfo->proxyuser)
        else
          ERROR_DISCONNECT_AND_ABORT("Autentikasi proxy diperlukan")
      case 408 :
        ERROR_DISCONNECT_AND_ABORT("request/permintaan waktu habis")
      case 429 :
        ERROR_DISCONNECT_AND_ABORT("Terlalu banyak request/permintaan")
}
    }
    // Memeriksa rentang nilai kode respons untuk memastikan kemajuan dan kesalahan
    if (result < 100)
      ERROR_DISCONNECT_AND_ABORT("Proxy web mengembalikan respons kemajuan yang tidak terduga")
    if (result >= 500)
      ERROR_DISCONNECT_AND_ABORT("Proxy web mengembalikan kesalahan server")
    if (result >= 400)
      ERROR_DISCONNECT_AND_ABORT("Proxy web mengembalikan kesalahan klien")
    if (result >= 300)
      ERROR_DISCONNECT_AND_ABORT("Proxy web mengembalikan pengalihan yang tidak terduga")
    if (result < 100 || result >= 300)
      ERROR_DISCONNECT_AND_ABORT("eror proxy web")
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Koneksi proxy web berhasil ke: %s:%lu", host, (unsigned long)dstport);
    // Mencatat informasi log bahwa koneksi telah berhasil dibuat melalui proxy (komentar sementara)
    //write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Connected to %s:%u via proxy %s:%u", dsthost, dstport, inet_ntoa(*(struct in_addr*)&proxyaddr), (unsigned long)proxyinfo->proxyport);/////
  } else {
    /* * * INVALID PROXY TYPE SPECIFIED * * */
    ERROR_DISCONNECT_AND_ABORT("Jenis proxy yang tidak dikenal")
  }
  return sock;
}

DLL_EXPORT_PROXYSOCKET SOCKET proxysocket_connect (proxysocketconfig proxy, const char* dsthost, uint16_t dstport, char** errmsg)
{
  if (proxy) {
    // Menghubungkan menggunakan proxy yang sudah ada
    return proxyinfo_connect(proxy, proxy->proxyinfolist, dsthost, dstport, errmsg);
  } else {
    //Menggunakan proxy langsung jika proxy NULL
    SOCKET result;
    if ((proxy = proxysocketconfig_create_direct()) == NULL)
      return SOCKET_ERROR;
    result = proxyinfo_connect(proxy, proxy->proxyinfolist, dsthost, dstport, errmsg);
    proxysocketconfig_free(proxy);
    return result;
  }
}

DLL_EXPORT_PROXYSOCKET void proxysocket_disconnect (proxysocketconfig proxy, SOCKET sock)
{
  int status;
  write_log_info(proxy, PROXYSOCKET_LOG_DEBUG, "Menutup koneksi");
#if defined(__WIN32__) || defined(__DJGPP__)
  //Menutup socket diwindowsn /shutdown(sock, SD_BOTH);
  status = closesocket(sock);
  if (status != 0 && status != WSAEWOULDBLOCK)
#else
  //Menutup soket di sistem non-Windows/shutdown(sock, SHUT_RDWR);
  status = close(sock);
  if (status != 0)
#endif
// Menulis log kesalahan jika gagal menutup koneksi
    write_log_info(proxy, PROXYSOCKET_LOG_ERROR, "Gagal menutup Koneksi (error: %i)", status);
  else
  // Menulis log informasi bahwa koneksi telah ditutup
    write_log_info(proxy, PROXYSOCKET_LOG_INFO, "Koneksi ditutup");
}

DLL_EXPORT_PROXYSOCKET void socket_set_timeouts_milliseconds (SOCKET sock, uint32_t sendtimeout, uint32_t recvtimeout)
{
#ifdef __WIN32__
  DWORD timeoutdata;
  // Menetapkan waktu tunggu kirim dan terima untuk Windows
  timeoutdata = sendtimeout;
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutdata, sizeof(timeoutdata));
  timeoutdata = recvtimeout;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutdata, sizeof(timeoutdata));
#else
  struct timeval timeoutdata;
  // Menetapkan waktu tunggu kirim dan terima untuk sistem non-Windows
  timeoutdata.tv_sec = sendtimeout / 1000;
  timeoutdata.tv_usec = (sendtimeout % 1000) * 1000;
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const void*)&timeoutdata, sizeof(timeoutdata));
  timeoutdata.tv_sec = recvtimeout / 1000;
  timeoutdata.tv_usec = (recvtimeout % 1000) * 1000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const void*)&timeoutdata, sizeof(timeoutdata));
#endif
}

#define READ_BUFFER_SIZE 128

DLL_EXPORT_PROXYSOCKET char* socket_receiveline (SOCKET sock)
{
  char* buf;
  int bufpos = 0;
  int bufsize = READ_BUFFER_SIZE;
  int n;
  int i;
  // Menghentikan jika soket tidak valid
  if (sock == INVALID_SOCKET)
    return NULL;
  // Menerima data dalam buffer yang dialokasikan secara dinamis
  if ((buf = (char*)malloc(READ_BUFFER_SIZE)) == NULL)
    return NULL;
  while ((n = recv(sock, buf + bufpos, READ_BUFFER_SIZE, MSG_PEEK)) > 0) {
    //menghapus data baru dalama data yang diintip
    for (i = 0; i < n; i++) {
      if (buf[bufpos + i] == '\n') {
        n = i + 1;
        //menghapus baris baru diakhir
        if ((recv(sock, buf + bufpos, n, 0)) == n) {
          if (bufpos + i > 0 && buf[bufpos + i - 1] == '\r')
            buf[bufpos + i - 1] = 0;
          else
            buf[bufpos + i] = 0;
          return buf;
        }
      }
    }
    // Membaca data hingga dan termasuk baris baru
    if ((n = recv(sock, buf + bufpos, n, 0)) <= 0)
      break;
    bufpos += n;
    if ((buf = (char*)realloc(buf, bufpos + READ_BUFFER_SIZE)) == NULL)
      return NULL;
  }
  //mendeteksi koneksi yang terputus
  if (bufpos == 0 && n < 0) {
    free(buf);
    return NULL;
  }
  //menambhakan nol di akhir buffer
  if (bufpos >= bufsize)
    if ((buf = (char*)realloc(buf, bufpos + 1)) == NULL)
      return NULL;
  buf[bufpos] = 0;
  return buf;
}

DLL_EXPORT_PROXYSOCKET char* socket_get_error_message ()
{
  char* result = NULL;
#ifdef _WIN32
  //Mendapatkan pesan erorr pada windows
  LPSTR errmsg;
  if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errmsg, 0, NULL) > 0) {
    result = strdup(errmsg);
    LocalFree(errmsg);
  }
#else
  char* errmsg;
  // TO (todo sengaja di pisah sementata)DO: gunakan strerror_r sebagai gantinya
  if ((errmsg = strerror(errno)) != NULL) {
    result = strdup(errmsg);
  }
#endif
  return result;
}

