/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General 
 * Public License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include <tumbler/tumbler-util.h>



gchar **
tumbler_util_get_supported_uri_schemes (void)
{
  const gchar *const *vfs_schemes;
  gchar             **uri_schemes;
  gboolean            file_scheme_found = FALSE;
  guint               length;
  guint               n;
  GVfs               *vfs;

  /* determine the URI schemes supported by GIO */
  vfs = g_vfs_get_default ();
  vfs_schemes = g_vfs_get_supported_uri_schemes (vfs);

  /* search for the "file" scheme */
  for (n = 0; !file_scheme_found && vfs_schemes[n] != NULL; ++n)
    if (g_strcmp0 (vfs_schemes[n], "file") == 0)
      file_scheme_found = TRUE;

  /* check if the "file" scheme is included */
  if (file_scheme_found)
    {
      /* it is, so simply copy the array */
      uri_schemes = g_strdupv ((gchar **)vfs_schemes);
    }
  else
    {
      /* it is not, so we need to copy the array and add "file" */
      length = g_strv_length ((gchar **)vfs_schemes);
      uri_schemes = g_new0 (gchar *, length + 2);
      uri_schemes[0] = g_strdup ("file");
      for (n = 1; n <= length; ++n)
        uri_schemes[n] = g_strdup (vfs_schemes[n-1]);
      uri_schemes[n] = NULL;
    }

  return uri_schemes;
}
