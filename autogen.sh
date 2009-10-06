which xdt-autogen

if test x$? = x"0"; then
  echo "Picked XFCE development environment"
  .  ./autogen-xfce.sh
  exit 0
fi

which gnome-autogen.sh

if test x$? = x"0"; then
  echo "Picked GNOME development environment"
  .  ./autogen-gnome.sh
  exit 0
fi

echo "You need to install either gnome-common or xfce4-dev-tools"
exit 1

