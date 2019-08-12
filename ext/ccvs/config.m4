dnl $Id: config.m4,v 1.9 2001/05/22 21:33:17 sniper Exp $
dnl config.m4 for PHP4 CCVS Extension

PHP_ARG_WITH(ccvs, for CCVS support,
[  --with-ccvs[=DIR]       Include CCVS support])

if test "$PHP_CCVS" != "no"; then
  if test -r $PHP_CCVS/include/cv_api.h; then
     CCVS_DIR=$PHP_CCVS
  else 
    for i in /usr /usr/local /usr/local/ccvs ; do
      if test -r $i/include/cv_api.h; then
        CCVS_DIR=$i
      fi
    done
  fi

  if test -z "$CCVS_DIR"; then
    AC_MSG_ERROR(not found. Please check your CCVS installation; cv_api.h should be in <ccvs_dir>/include/)
  fi
  
  PHP_ADD_INCLUDE($CCVS_DIR/include)
  PHP_ADD_LIBRARY_WITH_PATH(ccvs, $CCVS_DIR/lib, CCVS_SHARED_LIBADD)
  PHP_EXTENSION(ccvs, $ext_shared)
  PHP_SUBST(CCVS_SHARED_LIBADD)
fi
