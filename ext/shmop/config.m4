dnl $Id: config.m4,v 1.2 2000/10/02 17:35:58 rasmus Exp $
PHP_ARG_ENABLE(shmop, whether to enable shmop support, 
[  --enable-shmop          Enable shmop support])

if test "$PHP_SHMOP" != "no"; then
  AC_DEFINE(HAVE_SHMOP, 1, [ ])
  PHP_EXTENSION(shmop, $ext_shared)
fi
