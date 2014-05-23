# Nemo Actions
# A Nemo extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
#
# This Program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This Program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this Library; see the file COPYING.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place,
# Suite 330, Boston, MA 02111-1307, USA.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

# serial 2 change NACT_ prefix to NA_ (Nemo Actions)

# let the user specify an alternate nemo-extension dir
# --with-nemo-extdir=<dir>

AC_DEFUN([NA_NEMO_EXTDIR],[
	AC_REQUIRE([_AC_ARG_NA_NEMO_EXTDIR])dnl
	AC_REQUIRE([_AC_NA_CHECK_NEMO_EXTDIR])dnl
	if test "x${ac_with_nact_nemo_extdir}" = "x"; then
		AC_MSG_ERROR([Unable to determine nemo extension folder, please use --with-nemo-extdir option])
	else
		AC_MSG_NOTICE([installing plugin in ${ac_with_nact_nemo_extdir}])
		AC_SUBST([NEMO_EXTENSIONS_DIR],[${ac_with_nact_nemo_extdir}])
	fi
])

AC_DEFUN([_AC_ARG_NA_NEMO_EXTDIR],[
	AC_ARG_WITH(
		[nemo-extdir],
		AC_HELP_STRING(
			[--with-nemo-extdir=DIR],
			[nemo plugins extension directory @<:@auto@:>@]
		),
	[ac_with_nact_nemo_extdir=$withval],
	[ac_with_nact_nemo_extdir=""]
	)
])

AC_DEFUN([_AC_NA_CHECK_NEMO_EXTDIR],[
	if test "x${ac_with_nact_nemo_extdir}" = "x"; then
		if test "x${PKG_CONFIG}" != "x"; then
			ac_with_nact_nemo_extdir="`${PKG_CONFIG} --variable=extensiondir libnemo-extension`"
		fi
	fi
])
