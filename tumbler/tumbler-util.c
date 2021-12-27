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

#include <math.h>

#include <glib.h>
#include <gio/gio.h>

#include <sys/stat.h>

#include <tumbler/tumbler-util.h>

/* Float block size used in the stat struct */
#define TUMBLER_STAT_BLKSIZE 512.



/* that's what `! g_log_writer_default_would_drop (G_LOG_LEVEL_DEBUG, log_domain)` leads to:
 * it is defined only from GLib 2.68, and it might be better to keep this anyway */
gboolean
tumbler_util_is_debug_logging_enabled (const gchar *log_domain)
{
  const gchar *domains;

  domains = g_getenv ("G_MESSAGES_DEBUG");
  if (domains == NULL)
    return FALSE;

  if (strcmp (domains, "all") == 0 || (log_domain != NULL && strstr (domains, log_domain)))
    return TRUE;

  return FALSE;
}



void
tumbler_util_dump_strv (const gchar *log_domain,
                        const gchar *label,
                        const gchar *const *strv)
{
  GString *string;
  const gchar *const *p;

  g_return_if_fail (label != NULL && strv != NULL);

  if (! tumbler_util_is_debug_logging_enabled (log_domain))
    return;

  string = g_string_new (label);
  g_string_append (string, ":\n");

  for (p = strv; *p != NULL; p++)
    g_string_append_printf (string, "  %s\n", *p);

  g_string_truncate (string, string->len - 1);
  g_log (log_domain, G_LOG_LEVEL_DEBUG, "%s", string->str);
  g_string_free (string, TRUE);
}



/*
 * This is intended to be used around too verbose third-party APIs we can't silence by
 * another means:
 *   tumbler_util_toggle_stderr (G_LOG_DOMAIN);
 *   … = too_verbose_api (…);
 *   tumbler_util_toggle_stderr (G_LOG_DOMAIN);
 * When debug logging is enabled, it does nothing.
 */
void
tumbler_util_toggle_stderr (const gchar *log_domain)
{
  static gint stderr_save = -2;

  /* do nothing in case of previous error or if debug logging is enabled */
  if (stderr_save == -1 || tumbler_util_is_debug_logging_enabled (log_domain))
    return;

  /* redirect stderr to /dev/null */
  if (stderr_save == -2)
    {
      fflush (stderr);
      stderr_save = dup (STDERR_FILENO);
      if (stderr_save != -1 && freopen ("/dev/null", "a", stderr) == NULL)
        stderr_save = -1;
    }
  /* restore stderr to stderr_save */
  else
    {
      fflush (stderr);
      stderr_save = dup2 (stderr_save, STDERR_FILENO);
      if (stderr_save != -1)
        stderr_save = -2;
    }
}



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


gboolean  tumbler_util_guess_is_sparse (TumblerFileInfo *info)
{
  gchar *filename;
  struct stat sb;
  gboolean ret_val = FALSE;

  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), FALSE);

  filename = g_filename_from_uri (tumbler_file_info_get_uri (info), NULL, NULL);

  if (G_LIKELY(filename))
  {
    stat (filename, &sb);

    g_free (filename);

    /* Test sparse files on regular ones */
    if (S_ISREG (sb.st_mode))
    {
      if (((TUMBLER_STAT_BLKSIZE * sb.st_blocks) / sb.st_size) < 0.8)
      {
        ret_val = TRUE;
      }
    }
  }

  return ret_val;
}



void
tumbler_util_size_prepared (GdkPixbufLoader *loader,
                            gint source_width,
                            gint source_height,
                            TumblerThumbnail *thumbnail)
{
  TumblerThumbnailFlavor *flavor;
  gdouble hratio, wratio;
  gint dest_width, dest_height;

  g_return_if_fail (GDK_IS_PIXBUF_LOADER (loader));
  g_return_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail));

  /* get the destination size */
  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  tumbler_thumbnail_flavor_get_size (flavor, &dest_width, &dest_height);
  g_object_unref (flavor);

  if (source_width <= dest_width && source_height <= dest_height)
    {
      /* do not scale the image */
      dest_width = source_width;
      dest_height = source_height;
    }
  else
    {
      /* determine which axis needs to be scaled down more */
      wratio = (gdouble) source_width / (gdouble) dest_width;
      hratio = (gdouble) source_height / (gdouble) dest_height;

      /* adjust the other axis */
      if (hratio > wratio)
        dest_width = rint (source_width / hratio);
     else
        dest_height = rint (source_height / wratio);
    }

  gdk_pixbuf_loader_set_size (loader, MAX (dest_width, 1), MAX (dest_height, 1));
}



GdkPixbuf *
tumbler_util_scale_pixbuf (GdkPixbuf *source,
                           gint dest_width,
                           gint dest_height)
{
  gdouble hratio, wratio;
  gint source_width, source_height;

  g_return_val_if_fail (GDK_IS_PIXBUF (source), NULL);

  /* determine the source pixbuf dimensions */
  source_width  = gdk_pixbuf_get_width (source);
  source_height = gdk_pixbuf_get_height (source);

  /* return the same pixbuf if no scaling is required */
  if (source_width <= dest_width && source_height <= dest_height)
    return g_object_ref (source);

  /* determine which axis needs to be scaled down more */
  wratio = (gdouble) source_width / (gdouble) dest_width;
  hratio = (gdouble) source_height / (gdouble) dest_height;

  /* adjust the other axis */
  if (hratio > wratio)
    dest_width = rint (source_width / hratio);
  else
    dest_height = rint (source_height / wratio);

  /* scale the pixbuf down to the desired size */
  return gdk_pixbuf_scale_simple (source, MAX (dest_width, 1), MAX (dest_height, 1),
                                  GDK_INTERP_BILINEAR);
}
