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
          if (strcmp ("file", vfs_schemes[i]) != 0         /* always first scheme */
              && strcmp ("computer", vfs_schemes[i]) != 0  /* only devices here */
              && strcmp ("localtest", vfs_schemes[i]) != 0 /* test fs */
              && strcmp ("http", vfs_schemes[i]) != 0      /* not a fs you can browse */
              && strcmp ("cdda", vfs_schemes[i]) != 0      /* audio cds */
              && strcmp ("network", vfs_schemes[i]) != 0)  /* only to list remotes, not files */
            uri_schemes[n++] = g_strdup (vfs_schemes[i]);
        }
    }

  uri_schemes[n++] = NULL;

  return uri_schemes;
}


static gchar *
tumbler_util_get_settings_filename (void)
{
  gchar               *path;
  const gchar          filename[] = "tumbler" G_DIR_SEPARATOR_S "tumbler.rc";
  const gchar * const *dirs;
  guint                n;

  /* check user directory */
  path = g_build_filename (g_get_user_config_dir (), filename, NULL);
  if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
    return path;
  g_free (path);

  dirs = g_get_system_config_dirs ();
  if (G_UNLIKELY (dirs == NULL))
    return FALSE;

  /* look in system config dirs */
  for (n = 0; dirs[n] != NULL; n++)
    {
      path = g_build_filename (dirs[n], filename, NULL);
      if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
        return path;
      g_free (path);
    }

  return NULL;
}



GKeyFile *
tumbler_util_get_settings (void)
{
  GKeyFile *settings;
  GError   *err = NULL;
  gchar    *filename;

  settings = g_key_file_new ();
  filename = tumbler_util_get_settings_filename ();

  if (filename != NULL
      && !g_key_file_load_from_file (settings, filename, 0, &err))
    {
      g_critical ("Unable to load settings from \"%s\": %s", filename, err->message);
      g_error_free (err);
    }

  g_free (filename);

  return settings;
}
