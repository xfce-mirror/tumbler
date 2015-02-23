#!/bin/sh
#
# vi:set et ai sw=2 sts=2 ts=2: */
# -
# Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
# 
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of 
# the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public 
# License along with this program; if not, write to the Free 
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

export ACLOCAL_FLAGS="-I `pwd`/m4macros $ACLOCAL_FLAGS"

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="tumbler"
REQUIRED_AUTOMAKE_VERSION=1.11

(test -f $srcdir/configure.ac \
  && test -f $srcdir/README) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

# Automake requires that ChangeLog exist.
touch ChangeLog

which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}
. gnome-autogen.sh
