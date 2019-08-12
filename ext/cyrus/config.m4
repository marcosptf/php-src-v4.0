dnl config.m4 for extension cyrus

PHP_ARG_WITH(cyrus, for cyrus imap support,
[  --with-cyrus             Include cyrus imap support])

if test "$PHP_CYRUS" != "no"; then
	found_cyrus=no
	found_sasl=no
	found_openssl=no
	for i in /usr /usr/local $PHP_CYRUS; do
		if test -r $i/include/cyrus/imclient.h && test "$found_cyrus" = "no"; then
			PHP_ADD_INCLUDE($i/include)
			PHP_SUBST(CYRUS_SHARED_LIBADD)
			PHP_ADD_LIBRARY_WITH_PATH(cyrus, $i/lib, CYRUS_SHARED_LIBADD)
			found_cyrus=yes
		fi
		if test -r $i/include/sasl.h && test "$found_sasl" = "no"; then
			PHP_ADD_INCLUDE($i/include)
			PHP_SUBST(SASL_SHARED_LIBADD)
			PHP_ADD_LIBRARY_WITH_PATH(sasl, $i/lib, SASL_SHARED_LIBADD)
			found_sasl=yes
		fi
		if test -r $i/include/openssl/ssl.h && test "$found_openssl" = "no" && test "$PHP_OPENSSL" = "no"; then
			PHP_SUBST(OPENSSL_SHARED_LIBADD)
			PHP_SUBST(CRYPTO_SHARED_LIBADD)
			PHP_ADD_LIBRARY_WITH_PATH(ssl, $i/lib, OPENSSL_SHARED_LIBADD)
			PHP_ADD_LIBRARY_WITH_PATH(crypto, $i/lib, CRYPTO_SHARED_LIBADD)
			found_openssl=yes
		fi
	done

	if test "$found_cyrus" = "no"; then
		AC_MSG_RESULT(not found)
		AC_MSG_ERROR(Please Re-install the cyrus distribution)
	fi

	if test "$found_sasl" = "no"; then
		AC_MSG_RESULT(sasl not found)
		AC_MSG_ERROR(Please Re-install the cyrus distribution)
	fi

	AC_DEFINE(HAVE_CYRUS,1,[ ])

	PHP_EXTENSION(cyrus, $ext_shared)
fi
