AC_DEFUN([AC_LUSTRE],
[
  AC_ARG_WITH([lustre],
    [AS_HELP_STRING([--with-lustre],
      [support the lustre file system @<:@default=check@:>@])],
    [],
    [with_lustre=check])
          
    LIBLUSTREAPI=
    AS_IF([test "x$with_lustre" != xno],
      [AC_CHECK_LIB([lustreapi], [llapi_quotactl],
        [AC_SUBST([LIBLUSTREAPI], ["-llustreapi"])
         AC_DEFINE([HAVE_LIBLUSTREAPI], [1],
                   [Define if you have liblustreapi])
        ],
        [if test "x$with_liblustre" != xcheck; then
           AC_MSG_FAILURE(
             [--with-lustre was given, but test for liblustreapi failed])
         fi
        ], )])
])
