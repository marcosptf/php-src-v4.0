dnl $Id: config.m4,v 1.13.2.1 2001/08/30 16:30:14 sniper Exp $

PHP_ARG_WITH(fdftk, for fdftk support,
[  --with-fdftk[=DIR]      Include fdftk support])

if test "$PHP_FDFTK" != "no"; then

  for i in /usr /usr/local $PHP_FDFTK; do
    if test -r $i/include/FdfTk.h -o -r $i/include/fdftk.h; then
      FDFTK_DIR=$i
    fi
  done

  if test -z "$FDFTK_DIR"; then
    AC_MSG_ERROR(FdfTk.h or fdftk.h not found. Please reinstall the fdftk distribution)
  fi

  PHP_ADD_INCLUDE($FDFTK_DIR/include)
  
  FDFLIBRARY=""
  for i in fdftk FdfTk; do
    AC_CHECK_LIB($i, FDFOpen, [FDFLIBRARY=$i], [], [-L$FDFTK_DIR/lib -lm])
  done
  
  if test -z "$FDFLIBRARY"; then
    AC_MSG_ERROR(fdftk module requires >= fdftk 2.0)
  fi
  
  AC_DEFINE(HAVE_FDFLIB,1,[ ])
  PHP_ADD_LIBRARY_WITH_PATH($FDFLIBRARY, $FDFTK_DIR/lib, FDFTK_SHARED_LIBADD)

  PHP_SUBST(FDFTK_SHARED_LIBADD)
  PHP_EXTENSION(fdf, $ext_shared)
fi
