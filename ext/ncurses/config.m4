dnl $Id: config.m4,v 1.1 2001/06/12 21:15:49 hholzgra Exp $
dnl config.m4 for extension ncurses

PHP_ARG_WITH(ncurses, for ncurses support,
[  --with-ncurses             Include ncurses support])

if test "$PHP_NCURSES" != "no"; then
   # --with-ncurses -> check with-path
	 SEARCH_PATH="/usr/local /usr"     
   SEARCH_FOR="/include/curses.h"
   if test -r $PHP_NCURSES/; then # path given as parameter
     NCURSES_DIR=$PHP_NCURSES
   else # search default path list
     AC_MSG_CHECKING(for ncurses files in default path)
     for i in $SEARCH_PATH ; do
       if test -r $i/$SEARCH_FOR; then
         NCURSES_DIR=$i
         AC_MSG_RESULT(found in $i)
       fi
     done
   fi
  
   if test -z "$NCURSES_DIR"; then
     AC_MSG_RESULT(not found)
     AC_MSG_ERROR(Please reinstall the ncurses distribution)
   fi

   # --with-ncurses -> add include path
   PHP_ADD_INCLUDE($NCURSES_DIR/include)

   # --with-ncurses -> chech for lib and symbol presence
   LIBNAME=ncurses 
   LIBSYMBOL=initscr 
   old_LIBS=$LIBS
   LIBS="$LIBS -L$NCURSES_DIR/lib -lm -ldl"
   AC_CHECK_LIB($LIBNAME, $LIBSYMBOL, [AC_DEFINE(HAVE_NCURSESLIB,1,[ ])],
				[AC_MSG_ERROR(wrong ncurses lib version or lib not found)])
   LIBS=$old_LIBS
  
   PHP_SUBST(NCURSES_SHARED_LIBADD)
   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $NCURSES_DIR/lib, SAPRFC_SHARED_LIBADD)

  PHP_EXTENSION(ncurses, $ext_shared)
fi
