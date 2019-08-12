dnl $Id: config.m4,v 1.1 2000/11/13 19:47:19 venaas Exp $
dnl config.m4 for extension OpenSSL

if test "$OPENSSL_DIR"; then
  PHP_EXTENSION(openssl, $ext_shared)
  AC_DEFINE(HAVE_OPENSSL_EXT,1,[ ])
fi
