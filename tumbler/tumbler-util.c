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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include <tumbler/tumbler-util.h>



gchar **
tumbler_util_get_supported_uri_schemes (void)
{
  const gchar *const *vfs_schemes;
  gchar             **uri_schemes;
  guint               length;
  guint               n = 0;
  guint               i;
  GVfs               *vfs;

  /* determine the URI schemes supported by GIO */
  vfs = g_vfs_get_default ();
  vfs_schemes = g_vfs_get_supported_uri_schemes (vfs);

  if (G_LIKELY (vfs_schemes != NULL))
    length = g_strv_length ((gchar **) vfs_schemes);
  else
    length = 0;

  /* always start with file */
  uri_schemes = g_new0 (gchar *, length + 2);
  uri_schemes[n++] = g_strdup ("file");

  if (G_LIKELY (vfs_schemes != NULL))
    {
      for (i = 0; vfs_schemes[i] != NULL; ++i)
        {
          /* skip unneeded schemes */
          if (strcmp ("file", vfs_schemes[i]) != 0
              && strcmp ("computer", vfs_schemes[i]) != 0
              && strcmp ("localtest", vfs_schemes[i]) != 0
              && strcmp ("network", vfs_schemes[i]) != 0)
            uri_schemes[n++] = g_strdup (vfs_schemes[i]);
        }
    }

  uri_schemes[n++] = NULL;

  return uri_schemes;
}
