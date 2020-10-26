# This is taken from curl/lib/Makefile.inc
set(CURL_SRC_DIR ${ANDROID_QEMU2_TOP_DIR}/../curl/lib)

android_add_library(
  TARGET curl
  LIBNAME
    libcurl
    URL
    "https://android.googlesource.com/platform/external/curl/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../curl/"
  NOTICE "REPO/NOTICE"
  LICENSE curl
  SRC # cmake-format: sortable
      ${CURL_SRC_DIR}/amigaos.c
      ${CURL_SRC_DIR}/asyn-ares.c
      ${CURL_SRC_DIR}/asyn-thread.c
      ${CURL_SRC_DIR}/base64.c
      ${CURL_SRC_DIR}/conncache.c
      ${CURL_SRC_DIR}/connect.c
      ${CURL_SRC_DIR}/content_encoding.c
      ${CURL_SRC_DIR}/cookie.c
      ${CURL_SRC_DIR}/curl_addrinfo.c
      ${CURL_SRC_DIR}/curl_ctype.c
      ${CURL_SRC_DIR}/curl_des.c
      ${CURL_SRC_DIR}/curl_endian.c
      ${CURL_SRC_DIR}/curl_fnmatch.c
      ${CURL_SRC_DIR}/curl_gethostname.c
      ${CURL_SRC_DIR}/curl_gssapi.c
      ${CURL_SRC_DIR}/curl_memrchr.c
      ${CURL_SRC_DIR}/curl_multibyte.c
      ${CURL_SRC_DIR}/curl_ntlm_core.c
      ${CURL_SRC_DIR}/curl_ntlm_wb.c
      ${CURL_SRC_DIR}/curl_path.c
      ${CURL_SRC_DIR}/curl_range.c
      ${CURL_SRC_DIR}/curl_rtmp.c
      ${CURL_SRC_DIR}/curl_sasl.c
      ${CURL_SRC_DIR}/curl_sspi.c
      ${CURL_SRC_DIR}/curl_threads.c
      ${CURL_SRC_DIR}/dict.c
      ${CURL_SRC_DIR}/doh.c
      ${CURL_SRC_DIR}/dotdot.c
      ${CURL_SRC_DIR}/easy.c
      ${CURL_SRC_DIR}/escape.c
      ${CURL_SRC_DIR}/file.c
      ${CURL_SRC_DIR}/fileinfo.c
      ${CURL_SRC_DIR}/formdata.c
      ${CURL_SRC_DIR}/ftp.c
      ${CURL_SRC_DIR}/ftplistparser.c
      ${CURL_SRC_DIR}/getenv.c
      ${CURL_SRC_DIR}/getinfo.c
      ${CURL_SRC_DIR}/gopher.c
      ${CURL_SRC_DIR}/hash.c
      ${CURL_SRC_DIR}/hmac.c
      ${CURL_SRC_DIR}/hostasyn.c
      ${CURL_SRC_DIR}/hostcheck.c
      ${CURL_SRC_DIR}/hostip.c
      ${CURL_SRC_DIR}/hostip4.c
      ${CURL_SRC_DIR}/hostip6.c
      ${CURL_SRC_DIR}/hostsyn.c
      ${CURL_SRC_DIR}/http.c
      ${CURL_SRC_DIR}/http2.c
      ${CURL_SRC_DIR}/http_chunks.c
      ${CURL_SRC_DIR}/http_digest.c
      ${CURL_SRC_DIR}/http_negotiate.c
      ${CURL_SRC_DIR}/http_ntlm.c
      ${CURL_SRC_DIR}/http_proxy.c
      ${CURL_SRC_DIR}/idn_win32.c
      ${CURL_SRC_DIR}/if2ip.c
      ${CURL_SRC_DIR}/imap.c
      ${CURL_SRC_DIR}/inet_ntop.c
      ${CURL_SRC_DIR}/inet_pton.c
      ${CURL_SRC_DIR}/krb5.c
      ${CURL_SRC_DIR}/ldap.c
      ${CURL_SRC_DIR}/llist.c
      ${CURL_SRC_DIR}/md4.c
      ${CURL_SRC_DIR}/md5.c
      ${CURL_SRC_DIR}/memdebug.c
      ${CURL_SRC_DIR}/mime.c
      ${CURL_SRC_DIR}/mprintf.c
      ${CURL_SRC_DIR}/multi.c
      ${CURL_SRC_DIR}/netrc.c
      ${CURL_SRC_DIR}/non-ascii.c
      ${CURL_SRC_DIR}/nonblock.c
      ${CURL_SRC_DIR}/openldap.c
      ${CURL_SRC_DIR}/parsedate.c
      ${CURL_SRC_DIR}/pingpong.c
      ${CURL_SRC_DIR}/pipeline.c
      ${CURL_SRC_DIR}/pop3.c
      ${CURL_SRC_DIR}/progress.c
      ${CURL_SRC_DIR}/psl.c
      ${CURL_SRC_DIR}/rand.c
      ${CURL_SRC_DIR}/rtsp.c
      ${CURL_SRC_DIR}/security.c
      ${CURL_SRC_DIR}/select.c
      ${CURL_SRC_DIR}/sendf.c
      ${CURL_SRC_DIR}/setopt.c
      ${CURL_SRC_DIR}/sha256.c
      ${CURL_SRC_DIR}/share.c
      ${CURL_SRC_DIR}/slist.c
      ${CURL_SRC_DIR}/smb.c
      ${CURL_SRC_DIR}/smtp.c
      ${CURL_SRC_DIR}/socks.c
      ${CURL_SRC_DIR}/socks_gssapi.c
      ${CURL_SRC_DIR}/socks_sspi.c
      ${CURL_SRC_DIR}/speedcheck.c
      ${CURL_SRC_DIR}/splay.c
      ${CURL_SRC_DIR}/ssh-libssh.c
      ${CURL_SRC_DIR}/ssh.c
      ${CURL_SRC_DIR}/strcase.c
      ${CURL_SRC_DIR}/strdup.c
      ${CURL_SRC_DIR}/strerror.c
      ${CURL_SRC_DIR}/strtok.c
      ${CURL_SRC_DIR}/strtoofft.c
      ${CURL_SRC_DIR}/system_win32.c
      ${CURL_SRC_DIR}/telnet.c
      ${CURL_SRC_DIR}/tftp.c
      ${CURL_SRC_DIR}/timeval.c
      ${CURL_SRC_DIR}/transfer.c
      ${CURL_SRC_DIR}/url.c
      ${CURL_SRC_DIR}/urlapi.c
      ${CURL_SRC_DIR}/vauth/cleartext.c
      ${CURL_SRC_DIR}/vauth/cram.c
      ${CURL_SRC_DIR}/vauth/digest.c
      ${CURL_SRC_DIR}/vauth/digest_sspi.c
      ${CURL_SRC_DIR}/vauth/krb5_gssapi.c
      ${CURL_SRC_DIR}/vauth/krb5_sspi.c
      ${CURL_SRC_DIR}/vauth/ntlm.c
      ${CURL_SRC_DIR}/vauth/ntlm_sspi.c
      ${CURL_SRC_DIR}/vauth/oauth2.c
      ${CURL_SRC_DIR}/vauth/spnego_gssapi.c
      ${CURL_SRC_DIR}/vauth/spnego_sspi.c
      # LIB_VTLS_CFILES =
      ${CURL_SRC_DIR}/vauth/vauth.c
      ${CURL_SRC_DIR}/version.c
      ${CURL_SRC_DIR}/vtls/cyassl.c
      ${CURL_SRC_DIR}/vtls/darwinssl.c
      ${CURL_SRC_DIR}/vtls/gskit.c
      ${CURL_SRC_DIR}/vtls/gtls.c
      ${CURL_SRC_DIR}/vtls/mbedtls.c
      ${CURL_SRC_DIR}/vtls/mesalink.c
      # LIB_CFILES =
      ${CURL_SRC_DIR}/vtls/nss.c
      ${CURL_SRC_DIR}/vtls/openssl.c
      ${CURL_SRC_DIR}/vtls/polarssl.c
      ${CURL_SRC_DIR}/vtls/polarssl_threadlock.c
      ${CURL_SRC_DIR}/vtls/schannel.c
      ${CURL_SRC_DIR}/vtls/schannel_verify.c
      ${CURL_SRC_DIR}/vtls/vtls.c
      ${CURL_SRC_DIR}/warnless.c
      ${CURL_SRC_DIR}/wildcard.c
      ${CURL_SRC_DIR}/x509asn1.c)
target_compile_definitions(
  curl PRIVATE -DBUILDING_LIBCURL -DCURL_HIDDEN_SYMBOLS
               -DOS=\"${ANDROID_TARGET_TAG}\")
target_compile_definitions(curl PUBLIC -DCURL_STATICLIB)
target_compile_options(curl PRIVATE -Wno-macro-redefined)
android_target_compile_definitions(curl darwin-x86_64 PRIVATE -DHAVE_CONFIG_H)
android_target_compile_definitions(curl linux-x86_64 PRIVATE -DHAVE_CONFIG_H)
android_target_compile_definitions(curl linux-aarch64 PRIVATE -DHAVE_CONFIG_H)
android_target_compile_definitions(
  curl
  windows_msvc-x86_64
  PRIVATE
  -DWIN32
  -DBUILDING_LIBCURL
  -DUSE_SCHANNEL # For WINSSL support
  -DHAVE_ZLIB_H
  -DHAVE_ZLIB
  -DHAVE_LIBZ
  -DHAVE_WINDOWS_H
  -DUSE_WIN32_IDN
  -DWANT_IDN_PROTOTYPES
  -DUSE_IPV6
  -DUSE_WINDOWS_SSPI
  -DWIN64)
target_compile_options(curl PRIVATE -fvisibility=default)
target_include_directories(
  curl
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/${ANDROID_TARGET_TAG} # Make sure this one is
                                                    # first, so we can pick up
                                                    # the right config..
    ${CURL_SRC_DIR}/..
    ${CURL_SRC_DIR})
target_include_directories(curl PUBLIC ${CURL_SRC_DIR}/../include)
target_link_libraries(curl PRIVATE ssl zlib)
android_target_link_libraries(
  curl windows_msvc-x86_64 PRIVATE wldap32::wldap32 gdi32::gdi32
                                   normaliz::normaliz crypt32::crypt32)
