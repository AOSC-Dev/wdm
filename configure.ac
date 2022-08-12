dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT([wdm],[1.99],[raorn@raorn.name])
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip silent-rules subdir-objects])
AC_CONFIG_SRCDIR([src/wdm/dm.c])
AC_CONFIG_HEADERS([include/config.h])

AC_PROG_INSTALL
AC_PROG_CC

AM_PROG_CC_C_O
AC_PROG_RANLIB
AC_PROG_SED
PKG_PROG_PKG_CONFIG

AC_DEFINE([_GNU_SOURCE], [1], [Enable GNU extensions if present])
AC_DEFINE([_XOPEN_SOURCE], [600], [Enable POSIX.1-2001 extensions if present])

AC_PROG_CC_C99
AS_IF([test "x$ac_cv_prog_cc_c99" = "xno"],
	  [AC_MSG_FAILURE([Yuor compiler does not support C99 standard])])

AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h sys/param.h syslog.h])
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_PID_T


AC_CHECK_FUNC([mkstemp],
			  [AC_DEFINE([HAVE_MKSTEMP], [1],
						 [Define if you have mkstemp function to use it instead of mktemp.])])

AC_SEARCH_LIBS([getifaddrs], [socket nsl])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.18.1)
AS_IF([test "x$USE_NLS" = "xyes"],
	[AC_DEFINE(I18N, 1, [Define if you want internationalization support])])


AC_ARG_ENABLE([auth-scheme],
			  [AS_HELP_STRING([--enable-auth-scheme=@<:@pam/crypt/shadow@:>@],
							  [Choose a specific authentication scheme @<:@default=auto@:>@])],
			  [],
			  [enable_auth_scheme=auto])

AC_ARG_ENABLE([exitlogin],
			  [AS_HELP_STRING([--enable-exitlogin],
							  [enable ExitLogin command in wdmLogin])],
			  [AS_IF([test "x$enable_exitlogin" = "xyes"],
					 [AC_DEFINE([WITH_EXITLOGIN], [1], [Define if you want to have ExitLogin])
					 ])])


AC_ARG_WITH([selinux],
			[AS_HELP_STRING([--with-selinux],
							[Add SELinux support])],
			[],
			[with_selinux=auto])

AC_ARG_WITH([console-kit],
			[AS_HELP_STRING([--with-console-kit],
							[Add ConsoleKit support @<:@default=auto@:>@])],
			[],
			[with_console_kit=auto])

AC_ARG_WITH([systemd],
			[AS_HELP_STRING([--with-systemd],
							[Add systemd support @<:@default=auto@:>@])],
			[],
			[with_systemd=auto])

AC_ARG_WITH([xinerama],
			[AS_HELP_STRING([--without-xinerama],
							[Explicit disable use of Xinerama])],
			[],
			[with_xinerama=yes])

AC_ARG_WITH([libaudit],
			[AS_HELP_STRING([--with-libaudit],
							[Add Linux audit support @<:@default=auto@:>@])],
			[],
			[with_libaudit=auto])

AC_ARG_WITH([runlockdir],
			[AS_HELP_STRING([--with-runlockdir=PATH],
							[specify directory where run lock goes @<:@default=/var/run@:>@])],
			[],
			[with_runlockdir="/var/run"])

AC_ARG_WITH([logdir],
			[AS_HELP_STRING([--with-logdir=PATH],
							[specify directory where logfile goes @<:@default=/var/log@:>@])],
			[],
			[with_logdir="/var/log"])

AC_ARG_WITH([wdmdir],
			[AS_HELP_STRING([--with-wdmdir=PATH],
							[specify path for wdm directory @<:@default=SYSCONFDIR/wdm@:>@])],
			[],
			[with_wdmdir="$sysconfdir/wdm"])

AC_ARG_WITH([gfxdir],
			[AS_HELP_STRING([--with-gfxdir=PATH],
							[specify path for pixmaps directory @<:@default=DATADIR/pixmaps/wdm@:>@])],
			[],
			[with_gfxdir="$datadir/pixmaps/wdm"])

AC_ARG_WITH([Logo],
			[AS_HELP_STRING([--with-Logo=FILENAME],
							[specify file in gfxdir for Logo])])

AC_ARG_WITH([fakehome],
			[AS_HELP_STRING([--with-fakehome=PATH],
							[path to use as home when no HOME is present @<:@default=RUNDIR/wdm@:>@])],
			[],
			[with_fakehome="$with_runlockdir/wdm"])

AC_ARG_WITH([defuserpath],
			[AS_HELP_STRING([--with-defuserpath=PATHlist],
							[specify list of paths for user sessions])],
			[],
			[with_defuserpath="/usr/local/bin:/bin:/usr/bin"])

AC_ARG_WITH([defsystempath],
			[AS_HELP_STRING([--with-defsystempath=PATHlist],[specify list of paths for superuser sessions])],
			[],
			[with_defsystempath="/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin"])


PKG_CHECK_MODULES([XLIB], [x11 xau xmu], [],
				  [AC_PATH_XTRA
				   AS_IF([test "x$no_x" != "xyes"],
						 [AC_MSG_ERROR([no (requires X development libraries)])
						 ],
						 [XLIB_CFLAGS="$X_CFLAGS"
						  XLIB_LIBS="$X_PRE_LIBS $X_LIBS -lX11 -lm $X_EXTRA_LIBS"])
				   ])
AC_SUBST([XLIB_CFLAGS])
AC_SUBST([XLIB_LIBS])


PKG_CHECK_MODULES([WINGS], [WINGs], [], [AC_MSG_ERROR([Modern WINGs is required])])
AC_SUBST([WINGS_CFLAGS])
AC_SUBST([WINGS_LIBS])


PKG_CHECK_MODULES([WRLIB], [wrlib], [], [AC_MSG_ERROR([Modern wraster is required])])
AC_SUBST([WRLIB_CFLAGS])
AC_SUBST([WRLIB_LIBS])


AS_IF([test "x$with_selinux" != "xno"],
	  [PKG_CHECK_MODULES([SELINUX], [libselinux],
						 [AC_DEFINE([WITH_SELINUX], [1], [Define if you want wdm to be compiled with SELinux support])
						 with_selinux=yes
						 ],
						 [AS_IF([test "x$with_selinux" != "xauto"],
								[AC_MSG_FAILURE([--with-selinux given, but libselinux was not found])
								],
								[AC_MSG_WARN([libselinux not found, compiling without SELinux support])
								])
						 ])
	  ])
AC_MSG_CHECKING([for SELinux support])
AC_SUBST([SELINUX_LIBS])
AC_SUBST([SELINUX_CFLAGS])
AC_MSG_RESULT([$with_selinux])


AS_IF([test "x$with_console_kit" != "xno"],
	  [AC_DEFINE([WITH_CONSOLE_KIT], [1], [Define if you want wdm to be compiled with ConsoleKit support])
	  with_console_kit=yes
	  ])
AC_MSG_CHECKING([for ConsoleKit support])
AC_MSG_RESULT([$with_console_kit])

AS_IF([test "x$with_systemd" != "xno"],
	  [PKG_CHECK_MODULES([SYSTEMD], [libsystemd],
						 [AC_DEFINE([WITH_SYSTEMD], [1], [Define if you wand wdm to be compiled with systemd support])
						 with_systemd=yes
						 ],
						 [AS_IF([test "x$with_systemd" != "xauto"],
								[AC_MSG_FAILURE([--with-systemd was given, but test for systemd failed])],
								[with_systemd=no])])
	   ])
AC_MSG_CHECKING([for systemd support])
AC_SUBST([SYSTEMD_LIBS])
AC_SUBST([SYSTEMD_CFLAGS])
AC_MSG_RESULT([$with_systemd])


AS_IF([test "x$with_console_kit" != "xno" -o "x$with_systemd" != "xno"],
	  [PKG_CHECK_MODULES([DBUS], [dbus-1])])
AC_SUBST([DBUS_CFLAGS])
AC_SUBST([DBUS_LIBS])


AS_IF([test "x$with_xinerama" != "no"],
	  [PKG_CHECK_MODULES([XINERAMA], [xinerama],
						 [AC_DEFINE([HAVE_XINERAMA], [1], [Define if Xinerama library is installed])
						 with_xinerama=yes
						 ])
	  ])
AC_MSG_CHECKING([for Xinerama support])
AC_SUBST([XINERAMA_CFLAGS])
AC_SUBST([XINERAMA_LIBS])
AC_MSG_RESULT([$with_xinerama])


AS_IF([test "x$with_libaudit" != "xno"],
    [AC_CHECK_LIB([audit], [audit_log_user_message],
				  [AUDIT_LIBS="-laudit"
				   AC_DEFINE([HAVE_LIBAUDIT], [1], [Linux audit support])
				   with_libaudit=yes
				   ],
				   [AS_IF([test "x$with_libaudit" != "xauto"],
						  [AC_MSG_FAILURE([--with-libaudit was given, but test for libaudit failed])])
				   ])
	])
AC_MSG_CHECKING([for Linux audit support])
AC_SUBST([AUDIT_LIBS])
AC_MSG_RESULT([$with_libaudit])


AC_CHECK_LIB([WUtil], [wstrdup])


PASSWDLIB="-ldl"
AS_IF([test "x$enable_auth_scheme" = "xpam" -o "x$enable_auth_scheme" = "xauto"],
	  [AC_ARG_WITH([pamdir],
				   [AS_HELP_STRING([--with-pamdir],
								   [Specify where PAM files go @<:@default=SYSCONFDIR/pam.d@:>@])],
				   [],
				   [with_pamdir="${sysconfdir}/pam.d"])
	  AC_SUBST([PAMDIR], ["$with_pamdir"])
	  AC_CHECK_LIB([pam], [pam_start],
				   [PASSWDLIB="-lpam $PASSWDLIB"
					enable_auth_scheme=pam
					AC_DEFINE([USE_PAM], [1], [Define if you want wdm to use pam for auth])
					],
					[AS_IF([test "x$enable_auth_scheme" != "xauto"],
						   [AC_MSG_FAILURE([--enable-auth-scheme=pam given, but check for PAM failed])])])
	  ])
AS_IF([test "x$enable_auth_scheme" = "xshadow" -o "x$enable_auth_scheme" = "xauto"],
	  [AC_CHECK_HEADERS([shadow.h])
	  AC_CHECK_LIB([shadow], [getspnam],
				   [PASSWDLIB="-lshadow $PASSWDLIB"
					enable_auth_scheme=shadow
					AC_DEFINE([USE_SHADOW], [1], [Define if you want wdm to use shadow passwords]),
				   ],
				   [AS_IF([test "x$enable_auth_scheme" != "xauto"],
						  [AC_MSG_FAILURE([--enable-auth-scheme=shadow given, but check for libshadow failed])])])
	  ])
AS_IF([test "x$enable_auth_scheme" = "xcrypt" -o "x$enable_auth_scheme" = "xauto"],
	  [AC_CHECK_LIB([crypt], [crypt],
					[PASSWDLIB="-lcrypt"
					 enable_auth_scheme=crypt
					 AC_DEFINE([USE_CRYPT], [1], [Define if you want wdm to use crypt(3) for auth]),
					],
					[AS_IF([test "x$enable_auth_scheme" != "xauto"],
						   [AC_MSG_FAILURE([--enable-auth-scheme=crypt given, but check for libcrypt failed])])])
	  ])
AS_IF([test "x$enable_auth_scheme" != "xpam" -a "x$enable_auth_scheme" != "xshadow" -a "x$enable_auth_scheme" != "xcrypt" ],
	  [AC_MSG_FAILURE([Unable to find appropriate authentication scheme])])

AC_MSG_CHECKING(for authentication scheme)
AC_SUBST([PASSWDLIB])
AM_CONDITIONAL([PAM], [test "x$enable_auth_scheme" = "pam"])
AC_MSG_RESULT([$enable_auth_scheme])


AC_MSG_CHECKING([for runlockdir])
RUNDIR="$with_runlockdir"
AC_SUBST([RUNDIR])
AC_MSG_RESULT([$RUNDIR])

AC_MSG_CHECKING([for logdir])
LOGDIR="$with_logdir"
AC_SUBST([LOGDIR])
AC_MSG_RESULT([$LOGDIR])

AC_MSG_CHECKING([for fake HOME])
FAKEHOME="$with_fakehome"
AC_DEFINE_UNQUOTED([FAKEHOME], ["$FAKEHOME"], [what to use as HOME value])
AC_SUBST([FAKEHOME])
AC_MSG_RESULT([$FAKEHOME])

AC_MSG_CHECKING([for default user PATH])
AC_DEFINE_UNQUOTED([DEF_USER_PATH], ["$with_defuserpath"], ["Value is a list of paths for user sessions"])
AC_MSG_RESULT([$with_defuserpath])

AC_MSG_CHECKING([for default system PATH])
AC_DEFINE_UNQUOTED([DEF_SYSTEM_PATH], ["$with_defsystempath"], ["Value is a list of paths for superuser sessions"])
AC_MSG_RESULT([$with_defsystempath])

AC_MSG_CHECKING([for wdm config directory])
DMDIR="$with_wdmdir"
AC_DEFINE_UNQUOTED([DEF_WDM_CONFIG], ["$DMDIR/wdm-config"], [Value is a path to default wdm config file])
AC_DEFINE_UNQUOTED([DEF_WDMLOGIN_CONFIG], ["$DMDIR/wdmLogin-config"], [Value is a path to default wdmLogin config file])
AC_DEFINE_UNQUOTED([DEF_CHOOSER], ["$DMDIR/chooser"], [Value is a path to default chooser program])
AC_DEFINE_UNQUOTED([DEF_AUTH_DIR], ["$DMDIR"], [Value is a path to default authdir])
AC_SUBST([DMDIR])
AC_MSG_RESULT([$DMDIR])

AC_MSG_CHECKING([for pixmaps directory])
GFXDIR="$with_gfxdir"
AC_DEFINE_UNQUOTED([WGFXDIR], ["$GFXDIR"], [Value is a path to the pixmaps directory])
AC_SUBST([GFXDIR])
AC_MSG_RESULT([$GFXDIR])

AC_MSG_CHECKING(if Logo pixmap specified)
AS_IF([test "x$with_Logo" != "x"],
	  [PIXMAPFILE="$with_Logo"
	  PIXMAPCOMMENT=""
	  AC_MSG_RESULT([$with_Logo])
	  ],
	  [PIXMAPFILE="Eagle.xpm"
	  PIXMAPCOMMENT="! "
	  AC_MSG_RESULT(no)
	  ])
AC_SUBST(PIXMAPFILE)
AC_SUBST(PIXMAPCOMMENT)


AC_ARG_VAR([XRDB_PATH], [Path to xrdb program from X])
AC_PATH_PROG([XRDB_PATH], [xrdb], [/bin/true])
AC_DEFINE_UNQUOTED([XRDB_PROGRAM], ["$XRDB_PATH"], [Value is a path for xrdb program])

AC_ARG_VAR([SHUTDOWN], [Path to shutdown program])
AC_PATH_PROG([SHUTDOWN], [shutdown], [/bin/false], [$PATH:/sbin:/usr/sbin])
AC_SUBST([SHUTDOWN])

AC_ARG_VAR([FAILSAFE_PATH], [Path to program, that is used as failsafe client])
AC_PATH_PROG([FAILSAFE_PATH], [xterm])
AC_DEFINE_UNQUOTED([DEF_FAILSAFE_CLIENT], ["$FAILSAFE_PATH"], [Value is a path for program used as failsafe client])
AC_DEFINE_UNQUOTED([DEF_SESSION], ["$FAILSAFE_PATH -ls"], [Value is a command for default session])
AC_SUBST([FAILSAFE_PATH])

AC_ARG_VAR([DEF_SERVER], [Path to default X server])
AC_PATH_PROG([DEF_SERVER], [X])
AC_DEFINE_UNQUOTED([DEF_SERVER_LINE], [":0 local $DEF_SERVER :0"], [Value is default command to start X server])
AC_SUBST([DEF_SERVER])





AC_ARG_VAR(WMAKER_PATH, [Path to wmaker program])
AC_PATH_PROG(WMAKER_PATH,wmaker,,$PATH:/usr/local/bin)
if test "x$WMAKER_PATH" != "x"; then
	WMLIST=WindowMaker
else
	WMAKER_PATH=wmaker
fi
AC_SUBST(WMAKER_PATH)


AC_ARG_VAR(AFTERSTEP_PATH, [Path to afterstep program])
AC_PATH_PROG(AFTERSTEP_PATH,afterstep,,$PATH:/usr/local/bin)
if test "x$AFTERSTEP_PATH" != "x"; then
	WMLIST=$WMLIST:AfterStep
else
	AFTERSTEP_PATH=afterstep
fi
AC_SUBST(AFTERSTEP_PATH)


AC_ARG_VAR(BLACKBOX_PATH, [Path to blackbox program])
AC_PATH_PROG(BLACKBOX_PATH,blackbox,,$PATH:/usr/local/bin)
if test "x$BLACKBOX_PATH" != "x"; then
	WMLIST=$WMLIST:BlackBox
else
	BLACKBOX_PATH=blackbox
fi
AC_SUBST(BLACKBOX_PATH)


AC_ARG_VAR(ICEWM_PATH, [Path to icewm program])
AC_PATH_PROG(ICEWM_PATH,icewm,,$PATH:/usr/local/bin)
if test "x$ICEWM_PATH" != "x"; then
	WMLIST=$WMLIST:IceWm
else
	ICEWM_PATH=icewm
fi
AC_SUBST(ICEWM_PATH)


AC_ARG_VAR(XSESSION_PATH, [Path to xsession program])
AC_PATH_PROG(XSESSION_PATH,xsession,,$PATH:/usr/local/bin)
if test "x$XSESSION_PATH" != "x"; then
	WMLIST=$WMLIST:Xsession
else
	XSESSION_PATH=xsession
fi
AC_SUBST(XSESSION_PATH)


AC_PATH_PROG(STARTKDE_PATH,startkde,,$PATH:/usr/local/bin)
if test "x$STARTKDE_PATH" != "x"; then
	WMLIST=$WMLIST:KDE
else
	STARTKDE_PATH=startkde
fi
AC_SUBST(STARTKDE_PATH)


AC_ARG_WITH(winmgr,
	AC_HELP_STRING([--with-winmgr=NAME_OF_WM], [specify an arbitrary window manager]))
if test "x$with_winmgr" != "x"; then
	WINMGR_NAME=$with_winmgr
	AC_PATH_PROG(WINMGR_PATH,$WINMGR_NAME,,$PATH:/usr/local/bin)
	if test "x$WINMGR_PATH" = "x"; then
	AC_ARG_WITH(winmgr_path,
		AC_HELP_STRING([--with-winmgr_path=WINMGR_PATH], [specify path to your window manager]))
		if test "x$with_winmgr_path" != "x"; then
			WINMGR_PATH=$with_winmgr_path
		fi
	fi
	AC_MSG_CHECKING(for winmgr_path)
	if test "x$WINMGR_PATH" = "x"; then
		AC_MSG_ERROR([unable to locate $WINMGR_NAME, use --with-winmgr_path])
	fi
	AC_MSG_RESULT($WINMGR_PATH)
	WMLIST=$WMLIST:$WINMGR_NAME
fi
AC_SUBST(WINMGR_PATH)
AC_SUBST(WINMGR_NAME)


AC_MSG_CHECKING(if  window manager list specified)
AC_ARG_WITH(wmlist,
	AC_HELP_STRING([--with-wmlist=LIST_OF_WM],[specify list of window managers]))
if test "x$with_wmlist" != "x"; then
	WMLIST="$with_wmlist"
	AC_MSG_RESULT($with_wmlist)
	AC_MSG_WARN(make sure Xsession and Xclients support the above list)
else
	AC_MSG_RESULT([no, default to $WMLIST])
fi
AC_SUBST(WMLIST)


dnl this is hackaround for libfontconfig bug

AC_ARG_ENABLE(aafont,
	AC_HELP_STRING([--enable-aafont],[enable support for antialiased fonts.]))
if test "$enable_aafont" = "yes"; then
	AC_DEFINE(USE_AA, 1, [Define if you want support for antialiased fonts])
fi

AC_CONFIG_FILES([Makefile
				 po/Makefile.in])
AC_OUTPUT
