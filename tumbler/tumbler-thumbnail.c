/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
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

#include <glib/gi18n.h>
#include <gio/gio.h>

#include <tumbler/tumbler-enum-types.h>
#include <tumbler/tumbler-error.h>
#include <tumbler/tumbler-thumbnail.h>



TumblerThumbnailFlavor *
tumbler_thumbnail_get_flavors (void)
{
  static TumblerThumbnailFlavor flavors[] = 
  {
#ifdef ENABLE_LARGE_THUMBNAILS
    TUMBLER_THUMBNAIL_FLAVOR_NORMAL,
#endif
#ifdef ENABLE_LARGE_THUMBNAILS
    TUMBLER_THUMBNAIL_FLAVOR_LARGE,
#endif
#ifdef ENABLE_CROPPED_THUMBNAILS
    TUMBLER_THUMBNAIL_FLAVOR_CROPPED,
#endif
    TUMBLER_THUMBNAIL_FLAVOR_INVALID, /* this always has to come last */
  };

  return flavors;
}



gint 
tumbler_thumbnail_flavor_get_size (TumblerThumbnailFlavor flavor)
{
  g_return_val_if_fail (flavor != TUMBLER_THUMBNAIL_FLAVOR_INVALID, 0);

  switch (flavor)
    {
    case TUMBLER_THUMBNAIL_FLAVOR_NORMAL:
      return 128;
      break;
    case TUMBLER_THUMBNAIL_FLAVOR_LARGE:
      return 256;
      break;
    case TUMBLER_THUMBNAIL_FLAVOR_CROPPED:
      return 124;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return 0;
}



GFile *
tumbler_thumbnail_flavor_get_directory (TumblerThumbnailFlavor flavor)
{
  const gchar *home;
  const gchar *basename = NULL;
  GFile       *file;
  gchar       *path;

  g_return_val_if_fail (flavor != TUMBLER_THUMBNAIL_FLAVOR_INVALID, NULL);

  /* determine the correct .thumbnails/ subdir for the flavor */
  switch (flavor)
    {
    case TUMBLER_THUMBNAIL_FLAVOR_NORMAL:
      basename = "normal";
      break;
    case TUMBLER_THUMBNAIL_FLAVOR_LARGE:
      basename = "large";
      break;
    case TUMBLER_THUMBNAIL_FLAVOR_CROPPED:
      basename = "cropped";
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /* determine the user's home directory */
  home = g_getenv ("HOME") ? g_getenv ("HOME") : g_get_home_dir ();

  /* build the flavor directory path and create a file for it */
  path = g_build_filename (home, ".thumbnails", basename, NULL);
  file = g_file_new_for_path (path);
  g_free (path);

  return file;
}



GFile *
tumbler_thumbnail_get_file (const gchar           *uri,
                            TumblerThumbnailFlavor flavor)
{
  GFile       *file;
  GFile       *parent;
  gchar       *basename;
  gchar       *md5_hash;

  g_return_val_if_fail (uri != NULL, NULL);

  /* compute the MD5 hash of the URI and use it for the basename */
  md5_hash = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
  basename = g_strdup_printf ("%s.png", md5_hash);
  g_free (md5_hash);

  /* determine the thumbnail directory for this flavor */
  parent = tumbler_thumbnail_flavor_get_directory (flavor);

  /* create a file for the thumbnail destination */
  file = g_file_resolve_relative_path (parent, basename);

  /* destroy the directory file and basename */
  g_object_unref (parent);
  g_free (basename);

  return file;
}



GFileOutputStream *
tumbler_thumbnail_create_and_open_file (GFile   *file,
                                        GError **error)
{
  GFileOutputStream *output_stream = NULL;
  GError            *err = NULL;
  GFile             *parent;
  gchar             *path;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  /* determine the directory of the file */
  parent = g_file_get_parent (file);

  if (parent == NULL)
    {
      path = g_file_get_path (file);

      /* cache files reside in $HOME/.thumbnails/ and thus always have a parent
       * directory. if the input file doesn't it's invalid */
      g_set_error (error, TUMBLER_ERROR, TUMBLER_ERROR_FAILED, 
                   _("Invalid cache filename: \"%s\""), path);

      g_free (path);

      return NULL;
    }

  /* create the directory if it doesn't exist yet */
  if (!g_file_query_exists (parent, NULL))
    {
      if (!g_file_make_directory_with_parents (parent, NULL, &err))
        {
          g_propagate_error (error, err);
          return NULL;
        }
    }

  /* open the file for writing */
  output_stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, &err);

  /* propagate error if opening the file failed */
  if (err != NULL)
    {
      g_propagate_error (error, err);
      return NULL;
    }
  
  return output_stream;
}
