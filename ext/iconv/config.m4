dnl $Id: config.m4,v 1.4.2.2 2001/09/07 00:28:27 sniper Exp $
dnl config.m4 for extension iconv

PHP_ARG_WITH(iconv, for iconv support,
[  --with-iconv[=DIR]      Include iconv support])

if test "$PHP_ICONV" != "no"; then

  for i in /usr /usr/local $PHP_ICONV; do
    test -r $i/include/iconv.h && ICONV_DIR=$i
  done

  if test -z "$ICONV_DIR"; then
    AC_MSG_ERROR(Please reinstall the iconv library.)
  fi
  
  if test -f $ICONV_DIR/lib/libconv.a -o -f $ICONV_DIR/lib/libiconv.$SHLIB_SUFFIX_NAME ; then
    PHP_ADD_LIBRARY_WITH_PATH(iconv, $ICONV_DIR/lib, ICONV_SHARED_LIBADD)
    AC_CHECK_LIB(iconv, libiconv_open, [
    	AC_DEFINE(HAVE_ICONV, 1, [ ])
    	AC_DEFINE(HAVE_LIBICONV, 1, [ ])
    ])
  else
    AC_CHECK_LIB(c, iconv_open, AC_DEFINE(HAVE_ICONV, 1, [ ]))
  fi

  PHP_ADD_INCLUDE($ICONV_DIR/include)
  PHP_EXTENSION(iconv, $ext_shared)
  PHP_SUBST(ICONV_SHARED_LIBADD)
fi
