# libssh2 against ESP-IDF's mbedTLS.
#
# WHY VENDORED. There is no SSH client in the ESP component registry - checked
# espressif/libssh2, espressif/wolfssh and the obvious community names, all 404.
# So libssh2 is pinned at a release tag as a plain subdirectory: a tag rather
# than a submodule, so a checkout of this repository builds without a second
# fetch step, and pinned so a build a year from now is the build that was tested.
#
# mbedTLS is the crypto backend because IDF already ships and configures it -
# adding OpenSSL or wolfSSL would mean two crypto stacks in a 512 KB device.
set(LS2 ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

file(GLOB LS2_SRCS ${LS2}/src/*.c)
# The OpenSSL, wolfSSL and libgcrypt backends are excluded; mbedTLS is the one.
list(FILTER LS2_SRCS EXCLUDE REGEX "/(openssl|wincng|libgcrypt|os400qc3|mbedtls_backend)\\.c$")

idf_component_register(SRCS "ssh.c" ${LS2_SRCS}
                       INCLUDE_DIRS "include" "${LS2}/include"
                       PRIV_INCLUDE_DIRS "${LS2}/src" "priv"
                       REQUIRES mbedtls lwip esp_timer
                       PRIV_REQUIRES docstore)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LIBSSH2_MBEDTLS=1
    LIBSSH2_NO_ZLIB=1
    HAVE_UNISTD_H=1 HAVE_INTTYPES_H=1 HAVE_STDLIB_H=1 HAVE_SYS_SELECT_H=1
    HAVE_SYS_SOCKET_H=1 HAVE_SYS_TIME_H=1 HAVE_ARPA_INET_H=1
    HAVE_NETINET_IN_H=1 HAVE_SELECT=1 HAVE_SNPRINTF=1 HAVE_GETTIMEOFDAY=1
    HAVE_LONGLONG=1)
# libssh2's own sources are third-party; its warnings are not ours to fix and
# -Werror on them would make every upstream bump a blocking task.
set_source_files_properties(${LS2_SRCS} PROPERTIES COMPILE_FLAGS
    "-Wno-error -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare -Wno-type-limits")
