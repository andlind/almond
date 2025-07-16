AC_INIT([almond], [0.9.9.5], [andreas.lindell@almondmonitor.com])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_PROG_CC

AS_MKDIR_P(/etc/almond)
AS_MKDIR_P(/opt/almond/data)
# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h sys/socket.h sys/types.h sys/stat.h math.h signal.h pthread.h time.h netdb.h unistd.h ctype.h string.h errno.h dirent.h stdlib.h stdio.h json-c/json.h librdkafka/rdkafka.h openssl/ssl.h openssl/x509.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([memmove])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

