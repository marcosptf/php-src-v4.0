/* Copyright Abandoned 1996 TCX DataKonsult AB & Monty Program KB & Detron HB 
This file is public domain and comes with NO WARRANTY of any kind */

/* Version numbers for protocol & mysqld */

#ifdef _CUSTOMCONFIG_
#include <custom_conf.h>
#else
#define PROTOCOL_VERSION		10
#define MYSQL_SERVER_VERSION		"3.23.39"
#define MYSQL_SERVER_SUFFIX		""
#define FRM_VER				6
#define MYSQL_VERSION_ID		32339

#ifndef MYSQL_PORT
#define MYSQL_PORT			3306
#endif

#ifndef MYSQL_UNIX_ADDR
#define MYSQL_UNIX_ADDR			"/tmp/mysql.sock"
#endif

/* mysqld compile time options */
#ifndef MYSQL_CHARSET
#define MYSQL_CHARSET			"latin1"
#endif
#endif
