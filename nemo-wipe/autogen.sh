#!/bin/bash

mkdir -p build/aux                    || exit 1
mkdir -p build/m4                     || exit 1
#gtkdocize --flavour no-tmpl || exit 1
echo "no" | glib-gettextize -f -c     || exit 1
intltoolize -c -f --automake          || exit 1
gnome-doc-prepare -c -f               || exit 1
autoreconf -vfi                       || exit 1

#~ ./configure "$@"
