dnl $Id: config.m4,v 1.8 2001/05/07 08:22:34 sas Exp $
dnl config.m4 for extension wddx

PHP_ARG_ENABLE(wddx,for WDDX support,
[  --enable-wddx           Enable WDDX support])

if test "$PHP_WDDX" != "no"; then
  if test "$ext_shared" != "yes" && test "$enable_xml" = "no"; then
    AC_MSG_WARN(Activating XML)
    enable_xml=yes
  fi
  AC_DEFINE(HAVE_WDDX, 1, [ ])
  PHP_EXTENSION(wddx, $ext_shared)
fi
