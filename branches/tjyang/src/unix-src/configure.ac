#  configure.ac for hobbitmon
# 
# Process this file with autoconf to produce a configure script.
# We need autoconfig 2.59 version
AC_PREREQ(2.59)
# hobbit's name 
AC_INIT(hobbitmon, 4.4, tj_yang@hotmail.com)
#
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
#
AC_CONFIG_SRCDIR([bbdisplay/bbconvert.c])
# config.sh will be generated.
AC_CONFIG_HEADER([config.h])

# Checks for programs.
# We need tar.
# AC_CHECK_PROGS([TAR], [ggtar gggtar], [:])
# if test "$TAR" = :; then
# AC_MSG_ERROR([This package needs tar.])
# AC_MSG_WARN([This package needs tar.])
# fi


#############################################
# Define Some Variables
#############################################

AC_DEFINE(VARIABLE, VALUE, DESCRIPTION)

AC_PROG_CC
# AC_PROG_CXX
#AC_PROG_AWK
#AC_PROG_INSTALL
#AC_PROG_LN_S
#AC_PROG_MAKE_SET
#AC_PROG_RANLIB
#############################################
# Checks for libraries.
#############################################

#############################################
# Checks for header files.
#############################################
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdint.h stdlib.h string.h strings.h sys/param.h sys/socket.h sys/time.h unistd.h utime.h])

#############################################
# Checks for typedefs, structures, and compiler characteristics.
#############################################
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_VOLATILE
#############################################
# Checks for library functions.
#############################################
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
# AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_FUNC_WAIT3
AC_CHECK_FUNCS([alarm dup2 getcwd gethostbyname gethostname gettimeofday inet_ntoa memmove memset mkdir pathconf pstat_getdynamic putenv regcomp rmdir select socket strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strspn strstr strtol utime])

AC_SUBST(VARIABLE1,[VALUE1])
# AC_CONFIG_FILES(FILES)
# Create FILE for all FILE.in

#                 bbdisplay/Makefile
#                 bbnet/Makefile
#                 build/Makefile
#                 client/Makefile
#                 common/Makefile
#                 contrib/netware/Makefile
#                 demotool/Makefile
#                 docs/Makefile
#                 docs/autotww/FAQ-en/Makefile
#                 docs/autotww/en/Makefile
#                 docs/book/en/Makefile
#                 docs/hobbit-developer-guide/en/Makefile
#                 docs/hobbit-developer-guide/en/old/Makefile
#                 docs/hobbit-faq-howto/FAQ-en/Makefile
#                 docs/hobbit-faq-howto/en/Makefile
#                 docs/hobbit-reference-manual/en/Makefile
#                 docs/hobbit-sysadmin-guide/en/Makefile
#                 docs/hobbit-user-guide/en/Makefile
#                 docs/slides/Makefile
#                 hobbitd/Makefile
#                 lib/Makefile
#                 web/Makefile
AC_CONFIG_FILES([ bbproxy/Makefile ])
AC_OUTPUT
