/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tumbler/tumbler-builtin-thumbnailer.h>
#include <tumbler/tumbler-thumbnailer.h>



static gboolean
_tumbler_pixbuf_thumbnailer (TumblerBuiltinThumbnailer *thumbnailer,
                             const gchar               *uri,
                             const gchar               *mime_hint,
                             GError                   **error)
{
  return TRUE;
}



TumblerThumbnailer *
tumbler_pixbuf_thumbnailer_new (void)
{
  static const gchar *mime_types[] = {
    "image/png",
    NULL,
  };
  static const gchar *uri_schemes[] = {
    "file", 
    "sftp",
    "http",
    NULL,
  };

  return tumbler_builtin_thumbnailer_new (_tumbler_pixbuf_thumbnailer, 
                                          (const GStrv) mime_types,
                                          (const GStrv) uri_schemes);
}
