#!/bin/sh

set -e
set -x

[ -f autogen.sh ]

[ ! -d m4 ] && mkdir m4
[ ! -d build-aux ] && mkdir build-aux

glib-gettextize --copy --force
intltoolize --automake --copy --force
autoreconf --force --install
