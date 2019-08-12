dnl $Id: config.m4,v 1.3 2001/07/28 07:09:42 sasha Exp $

PHP_ARG_WITH(crack, whether to include crack support,
[  --with-crack[=DIR]      Include crack support.])

if test "$PHP_CRACK" != "no"; then
	for i in /usr/local/lib /usr/lib $PHP_CRACK $PHP_CRACK/cracklib; do
		if test -f $i/libcrack.a; then
			CRACK_LIBDIR=$i
		fi
	done
	for i in /usr/local/include /usr/include $PHP_CRACK $PHP_CRACK/cracklib; do
		if test -f $i/packer.h; then
			CRACK_INCLUDEDIR=$i
		fi
	done
  
	if test -z "$CRACK_LIBDIR"; then
		AC_MSG_ERROR(Cannot find the cracklib library file)
	fi
	if test -z "$CRACK_INCLUDEDIR"; then
		AC_MSG_ERROR(Cannot find a cracklib header file)
	fi

	PHP_ADD_INCLUDE($CRACK_INCLUDEDIR)
	PHP_ADD_LIBRARY_WITH_PATH(crack, $CRACK_LIBDIR, CRACK_SHARED_LIBADD)

	PHP_EXTENSION(crack, $ext_shared)

	AC_DEFINE(HAVE_CRACK, 1, [ ])
fi

PHP_SUBST(CRACK_SHARED_LIBADD)
