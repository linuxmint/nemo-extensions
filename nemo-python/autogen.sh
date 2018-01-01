#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

which gnome-autogen.sh || {
    echo "You need to install gnome-common from GNOME Git (or from"
    echo "your OS vendor's package manager)."
    exit 1
}
. gnome-autogen.sh
