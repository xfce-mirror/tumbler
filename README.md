[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/xfce/tumbler/-/blob/master/COPYING)

# tumbler


Tumbler is a D-Bus service for applications to request thumbnails for
various URI schemes and MIME types. It is an implementations of the 
[thumbnail management D-Bus specification](https://wiki.gnome.org/Attic/DraftThumbnailerSpec).
Tumbler provides plugin interfaces for extending the URI schemes and MIME types
for which thumbnails can be generated as well as for replacing the storage
backend that is used to store the thumbnails on disk.

----

### Homepage

[Tumbler documentation](https://docs.xfce.org/xfce/tumbler/start)

### Changelog

See [NEWS](https://gitlab.xfce.org/xfce/tumbler/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Source Code Repository

[Tumbler source code](https://gitlab.xfce.org/xfce/tumbler)

### Download a Release Tarball

[Tumbler archive](https://archive.xfce.org/src/xfce/tumbler)
    or
[Tumbler tags](https://gitlab.xfce.org/xfce/tumbler/-/tags)

### Installation

From source: 

    % cd tumbler
    % meson setup build
    % meson compile -C build
    % meson install -C build

From release tarball:

    % tar xf tumbler-<version>.tar.xz
    % cd tumbler-<version>
    % meson setup build
    % meson compile -C build
    % meson install -C build

### Reporting Bugs

Visit the [reporting bugs](https://docs.xfce.org/xfce/tumbler/bugs) page to view currently open bug reports and instructions on reporting new bugs or submitting bugfixes.

