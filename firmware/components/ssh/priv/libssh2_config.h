/* libssh2's build-time configuration for ESP-IDF. The definitions that matter
 * are on the command line in CMakeLists.txt; this exists because libssh2's
 * sources include it unconditionally. */
#pragma once

/* No zlib. Compression on an SSH session that carries a few hundred bytes of
 * command output is pure cost, and it would pull a second compressor into a
 * 512 KB device for nothing. */
#undef LIBSSH2_HAVE_ZLIB
#define HAVE_UNISTD_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_SELECT 1
#define HAVE_SNPRINTF 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_LONGLONG 1
