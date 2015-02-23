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

test -d m4 || mkdir m4

which xdt-autogen
if test x"$?" = x"0"; then
  echo "Building using the Xfce development environment"
  ./autogen-xfce.sh $@
  exit $?
fi

which gnome-autogen.sh
if test x"$?" = x"0"; then
  echo "Building using the GNOME development environment"
  ./autogen-gnome.sh $@
  exit $?
fi

cat >&2 <<EOF
You need to have either the Xfce or the GNOME development enviroment
installed. Check for xfce4-dev-tools or gnome-autogen.sh in your 
package manager.
EOF
exit 1
