dnl $Id: config.m4,v 1.5 2001/05/17 00:32:45 vlad Exp $

PHP_ARG_WITH(pspell,whether to include pspell support,
[  --with-pspell[=DIR]     Include PSPELL support.])

if test "$PHP_PSPELL" != "no"; then
	PHP_EXTENSION(pspell, $ext_shared)
	if test "$PHP_PSPELL" != "yes"; then
	    PSPELL_SEARCH_DIRS=$PHP_PSPELL
	else
	    PSPELL_SEARCH_DIRS="/usr/local /usr"
	fi
	for i in $PSPELL_SEARCH_DIRS; do
		if test -f $i/include/pspell/pspell.h; then
			PSPELL_DIR=$i
			PSPELL_INCDIR=$i/include/pspell
		elif test -f $i/include/pspell.h; then
			PSPELL_DIR=$i
			PSPELL_INCDIR=$i/include
		fi
	done

	if test -z "$PSPELL_DIR"; then
		AC_MSG_ERROR(Cannot find pspell)
	fi

	PSPELL_LIBDIR=$PSPELL_DIR/lib

	AC_DEFINE(HAVE_PSPELL,1,[ ])
	PHP_SUBST(PSPELL_SHARED_LIBADD)
	PHP_ADD_LIBRARY_WITH_PATH(pspell, $PSPELL_LIBDIR, PSPELL_SHARED_LIBADD)
	PHP_ADD_INCLUDE($PSPELL_INCDIR)
fi
