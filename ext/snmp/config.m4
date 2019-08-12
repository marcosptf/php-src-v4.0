dnl
dnl $Id: config.m4,v 1.20.2.2 2001/09/07 00:28:27 sniper Exp $
dnl

PHP_ARG_WITH(snmp,for SNMP support,
[  --with-snmp[=DIR]       Include SNMP support.  DIR is the SNMP base
                          install directory, defaults to searching through
                          a number of common locations for the snmp install.
                          Set DIR to "shared" to build as a dl, or "shared,DIR"
                          to build as a dl and still specify DIR.])

  if test "$PHP_SNMP" != "no"; then

    if test "$PHP_SNMP" = "yes"; then
      for i in /usr/include /usr/local/include; do
        test -f $i/snmp.h                       && SNMP_INCDIR=$i
        test -f $i/ucd-snmp/snmp.h              && SNMP_INCDIR=$i/ucd-snmp
        test -f $i/snmp/snmp.h                  && SNMP_INCDIR=$i/snmp
        test -f $i/snmp/include/ucd-snmp/snmp.h && SNMP_INCDIR=$i/snmp/include/ucd-snmp
      done
      for i in /usr /usr/snmp /usr/local /usr/local/snmp; do
        test -f $i/lib/libsnmp.a -o -f $i/lib/libsnmp.$SHLIB_SUFFIX_NAME && SNMP_LIBDIR=$i/lib
      done
    else
      SNMP_INCDIR=$PHP_SNMP/include
      test -d $PHP_SNMP/include/ucd-snmp && SNMP_INCDIR=$PHP_SNMP/include/ucd-snmp
      SNMP_LIBDIR=$PHP_SNMP/lib
    fi

    if test -z "$SNMP_INCDIR"; then
      AC_MSG_ERROR(snmp.h not found. Check your SNMP installation.)
    elif test -z "$SNMP_LIBDIR"; then
      AC_MSG_ERROR(libsnmp not found. Check your SNMP installation.)
    fi

    old_CPPFLAGS=$CPPFLAGS
    CPPFLAGS=-I$SNMP_INCDIR
    AC_CHECK_HEADERS(default_store.h)
    if test "$ac_cv_header_default_store_h" = "yes"; then
      AC_MSG_CHECKING(for OpenSSL support in SNMP libraries)
      AC_EGREP_CPP(yes,[
        #include <ucd-snmp-config.h>
        #if USE_OPENSSL
        yes
        #endif
      ],[
        SNMP_SSL=yes
      ],[
        SNMP_SSL=no
      ])
    fi
    CPPFLAGS=$old_CPPFLAGS
    AC_MSG_RESULT($SNMP_SSL)
  
    if test "$SNMP_SSL" = "yes"; then
      if test "$PHP_OPENSSL" != "no"; then
        PHP_ADD_LIBRARY(ssl,   1, SNMP_SHARED_LIBADD)
        PHP_ADD_LIBRARY(crypto,1, SNMP_SHARED_LIBADD)
      else
        AC_MSG_ERROR(The UCD-SNMP in this system is build with SSL support. 

        Add --with-openssl<=DIR> to your configure line.)
      fi
    fi

    AC_CHECK_LIB(kstat, kstat_read, [ PHP_ADD_LIBRARY(kstat,,SNMP_SHARED_LIBADD) ])

    AC_DEFINE(HAVE_SNMP,1,[ ])
    PHP_ADD_INCLUDE($SNMP_INCDIR)
    PHP_ADD_LIBRARY_WITH_PATH(snmp, $SNMP_LIBDIR, SNMP_SHARED_LIBADD)

    PHP_EXTENSION(snmp, $ext_shared)
    PHP_SUBST(SNMP_SHARED_LIBADD)
  fi


AC_MSG_CHECKING(whether to enable UCD SNMP hack)
AC_ARG_ENABLE(ucd-snmp-hack,
[  --enable-ucd-snmp-hack  Enable UCD SNMP hack],[
  if test "$enableval" = "yes" ; then
    AC_DEFINE(UCD_SNMP_HACK, 1, [ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
