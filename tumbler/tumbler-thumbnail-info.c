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

#include <tumbler/tumbler-enum-types.h>
#include <tumbler/tumbler-png-thumbnail-info.h>
#include <tumbler/tumbler-thumbnail-info.h>



static void tumbler_thumbnail_info_class_init (TumblerThumbnailInfoIface *klass);



static TumblerThumbnailFormat tumbler_thumbnail_info_default_format = TUMBLER_THUMBNAIL_FORMAT_PNG;



GType
tumbler_thumbnail_info_get_type (void)
{
  static GType type = G_TYPE_INVALID;
  
  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                            "TumblerThumbnailInfo",
                                            sizeof (TumblerThumbnailInfoIface),
                                            (GClassInitFunc) tumbler_thumbnail_info_class_init,
                                            0,
                                            NULL,
                                            0);

      g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

  return type;
}



static void
tumbler_thumbnail_info_class_init (TumblerThumbnailInfoIface *klass)
{
  g_object_interface_install_property (klass,
                                       g_param_spec_enum ("format",
                                                          "format",
                                                          "format",
                                                          TUMBLER_TYPE_THUMBNAIL_FORMAT,
                                                          TUMBLER_THUMBNAIL_FORMAT_INVALID,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_string ("hash",
                                                            "hash",
                                                            "hash",
                                                            NULL,
                                                            G_PARAM_READABLE));

  g_object_interface_install_property (klass,
                                       g_param_spec_uint64 ("mtime",
                                                            "mtime",
                                                            "mtime",
                                                            0, G_MAXUINT64, 0,
                                                            G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_string ("uri",
                                                            "uri",
                                                            "uri",
                                                            NULL,
                                                            G_PARAM_CONSTRUCT_ONLY |
                                                            G_PARAM_READWRITE));
}



TumblerThumbnailInfo *
tumbler_thumbnail_info_new (const gchar *uri)
{
  TumblerThumbnailFormat default_format = tumbler_thumbnail_info_default_format;

  g_return_val_if_fail (uri != NULL, NULL);

  return tumbler_thumbnail_info_new_for_format (uri, default_format);
}



TumblerThumbnailInfo *
tumbler_thumbnail_info_new_for_format (const gchar            *uri,
                                       TumblerThumbnailFormat  format)
{
  TumblerThumbnailInfo *info = NULL;

  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (format != TUMBLER_THUMBNAIL_FORMAT_INVALID, NULL);

  switch (format)
    {
    case TUMBLER_THUMBNAIL_FORMAT_PNG:
      info = g_object_new (TUMBLER_TYPE_PNG_THUMBNAIL_INFO, "uri", uri,
                           "format", format, NULL);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return info;
}



TumblerThumbnailFormat
tumbler_thumbnail_info_get_format (TumblerThumbnailInfo *info)
{
  TumblerThumbnailFormat format = TUMBLER_THUMBNAIL_FORMAT_INVALID;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), TUMBLER_THUMBNAIL_FORMAT_INVALID);
  
  g_object_get (info, "format", &format, NULL);
  return format;
}



gchar *
tumbler_thumbnail_info_get_uri (TumblerThumbnailInfo *info)
{
  gchar *uri = NULL;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), NULL);
  
  g_object_get (info, "uri", &uri, NULL);
  return uri;
}



guint64
tumbler_thumbnail_info_get_mtime (TumblerThumbnailInfo *info)
{
  guint64 mtime = 0;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), 0);
  
  g_object_get (info, "mtime", &mtime, NULL);
  return mtime;
}



gboolean
tumbler_thumbnail_info_needs_update (TumblerThumbnailInfo *info,
                                     GCancellable         *cancellable)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), FALSE);
  g_return_val_if_fail (TUMBLER_THUMBNAIL_INFO_GET_IFACE (info)->needs_update != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  
  return (TUMBLER_THUMBNAIL_INFO_GET_IFACE (info)->needs_update) (info, cancellable);
}



TumblerThumbnailFlavor *
tumbler_thumbnail_info_get_invalid_flavors (TumblerThumbnailInfo *info,
                                            GCancellable         *cancellable)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), FALSE);
  g_return_val_if_fail (TUMBLER_THUMBNAIL_INFO_GET_IFACE (info)->get_invalid_flavors != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  
  return (TUMBLER_THUMBNAIL_INFO_GET_IFACE (info)->get_invalid_flavors) (info, cancellable);
}



gboolean
tumbler_thumbnail_info_generate_flavor (TumblerThumbnailInfo  *info,
                                        TumblerThumbnailFlavor flavor,
                                        GdkPixbuf             *pixbuf,
                                        GCancellable          *cancellable,
                                        GError               **error)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), FALSE);
  g_return_val_if_fail (flavor != TUMBLER_THUMBNAIL_FLAVOR_INVALID, FALSE);
  g_return_val_if_fail (TUMBLER_THUMBNAIL_INFO_GET_IFACE (info)->generate_flavor != NULL, FALSE);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return (TUMBLER_THUMBNAIL_INFO_GET_IFACE (info)->generate_flavor) (info, flavor, pixbuf,
                                                                     cancellable, error);
}



void
tumbler_thumbnail_info_generate_fail (TumblerThumbnailInfo *info,
                                      GCancellable         *cancellable)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info));
  g_return_if_fail (TUMBLER_THUMBNAIL_INFO_GET_IFACE (info)->generate_fail != NULL);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  (TUMBLER_THUMBNAIL_INFO_GET_IFACE (info)->generate_fail) (info, cancellable);
}



GFile *
tumbler_thumbnail_info_temp_fail_file_new (TumblerThumbnailInfo *info)
{
  const gchar *home;
  GTimeVal     current_time = { 0, 0 };
  GFile       *file;
  gchar       *basename;
  gchar       *hash;
  gchar       *path;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), NULL);

  /* compute the current time */
  g_get_current_time (&current_time);

  /* determine the home directory */
  home = g_getenv ("HOME") ? g_getenv ("HOME") : g_get_home_dir ();

  /* determine the URI hash */
  g_object_get (info, "hash", &hash, NULL);

  /* build the fail file basename using the hash and timestamp */
  basename = g_strdup_printf ("%s-%ld-%ld.png", hash, current_time.tv_sec, 
                              current_time.tv_usec);

  /* build the fail file path and create a GFile for it */
  path = g_build_filename (home, ".thumbnails", "fail", "tumbler", basename, NULL);
  file = g_file_new_for_path (path);

  /* free strings */
  g_free (path);
  g_free (basename);

  return file;
}
  


GFile *
tumbler_thumbnail_info_fail_file_new (TumblerThumbnailInfo *info)
{
  const gchar *home;
  GTimeVal     current_time = { 0, 0 };
  GFile       *file;
  gchar       *basename;
  gchar       *hash;
  gchar       *path;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), NULL);

  /* compute the current time */
  g_get_current_time (&current_time);

  /* determine the home directory */
  home = g_getenv ("HOME") ? g_getenv ("HOME") : g_get_home_dir ();

  /* determine the URI hash */
  g_object_get (info, "hash", &hash, NULL);

  /* build the fail file basename using the hash and timestamp */
  basename = g_strdup_printf ("%s-.png", hash);

  /* build the fail file path and create a GFile for it */
  path = g_build_filename (home, ".thumbnails", "fail", "tumbler", basename, NULL);
  file = g_file_new_for_path (path);

  /* free strings */
  g_free (path);
  g_free (basename);

  return file;
}



GFile *
tumbler_thumbnail_info_flavor_file_new (TumblerThumbnailInfo  *info,
                                        TumblerThumbnailFlavor flavor)
{
  const gchar *home;
  const gchar *dir_basename = NULL;
  GFile       *file;
  gchar       *file_basename;
  gchar       *hash;
  gchar       *path;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), NULL);
  g_return_val_if_fail (flavor != TUMBLER_THUMBNAIL_FORMAT_INVALID, NULL);

  /* determine the user's home directory */
  home = g_getenv ("HOME") ? g_getenv ("HOME") : g_get_home_dir ();

  /* determine the correct .thumbnails/ subdir for the flavor */
  switch (flavor)
    {
    case TUMBLER_THUMBNAIL_FLAVOR_NORMAL:
      dir_basename = "normal";
      break;
    case TUMBLER_THUMBNAIL_FLAVOR_LARGE:
      dir_basename = "large";
      break;
    case TUMBLER_THUMBNAIL_FLAVOR_CROPPED:
      dir_basename = "cropped";
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /* determine the URI hash and thumbnail basename */
  g_object_get (info, "hash", &hash, NULL);
  file_basename = g_strdup_printf ("%s.png", hash);

  /* build the path for the thumbnail flavor file and create a GFile for it */
  path = g_build_filename (home, ".thumbnails", dir_basename, file_basename, NULL);
  file = g_file_new_for_path (path);

  /* free strings */
  g_free (path);
  g_free (file_basename);
  g_free (hash);

  return file;
}



GFile *
tumbler_thumbnail_info_temp_flavor_file_new (TumblerThumbnailInfo  *info,
                                             TumblerThumbnailFlavor flavor)
{
  const gchar *home;
  const gchar *dir_basename = NULL;
  GTimeVal     current_time = { 0, 0 };
  GFile       *file;
  gchar       *file_basename;
  gchar       *hash;
  gchar       *path;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), NULL);
  g_return_val_if_fail (flavor != TUMBLER_THUMBNAIL_FORMAT_INVALID, NULL);

  /* determine the user's home directory */
  home = g_getenv ("HOME") ? g_getenv ("HOME") : g_get_home_dir ();

  /* determine the correct .thumbnails/ subdir for the flavor */
  switch (flavor)
    {
    case TUMBLER_THUMBNAIL_FLAVOR_NORMAL:
      dir_basename = "normal";
      break;
    case TUMBLER_THUMBNAIL_FLAVOR_LARGE:
      dir_basename = "large";
      break;
    case TUMBLER_THUMBNAIL_FLAVOR_CROPPED:
      dir_basename = "cropped";
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /* compute the current time */
  g_get_current_time (&current_time);

  /* determine the URI hash */
  g_object_get (info, "hash", &hash, NULL);

  /* build the fail file basename using the hash and timestamp */
  file_basename = g_strdup_printf ("%s-%ld-%ld.png", hash, current_time.tv_sec, 
                                   current_time.tv_usec);

  /* build the path for the thumbnail flavor file and create a GFile for it */
  path = g_build_filename (home, ".thumbnails", dir_basename, file_basename, NULL);
  file = g_file_new_for_path (path);

  /* free strings */
  g_free (path);
  g_free (file_basename);
  g_free (hash);

  return file;
}



gint 
tumbler_thumbnail_info_get_flavor_size (TumblerThumbnailFlavor flavor)
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



gboolean
tumbler_thumbnail_info_load_mtime (TumblerThumbnailInfo *info,
                                   GCancellable         *cancellable)
{
  GFileInfo *file_info;
  guint64    mtime;
  GFile     *file;
  gchar     *uri;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_INFO (info), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);

  /* determine the source file URI and create a GFile for it */
  g_object_get (info, "uri", &uri, NULL);
  file = g_file_new_for_uri (uri);
  g_free (uri);

  /* query the modified timestamp of the file */
  file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED, 
                                 G_FILE_QUERY_INFO_NONE, cancellable, NULL);

  /* abort if the file information couldn't be queried */
  if (file_info == NULL)
    {
      g_object_unref (file);
      return FALSE;
    }

  /* set the mtime property of the thumbnail info */
  mtime = g_file_info_get_attribute_uint64 (file_info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
  g_object_set (info, "mtime", mtime, NULL);

  /* destroy the file and its info */
  g_object_unref (file_info);
  g_object_unref (file);

  return TRUE;
}



TumblerThumbnailFlavor *
tumbler_thumbnail_info_get_flavors (void)
{
  static TumblerThumbnailFlavor flavors[] = 
  {
#ifdef ENABLE_NORMAL_THUMBNAILS
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



TumblerThumbnailFormat
tumbler_thumbnail_info_get_default_format (void)
{
  return tumbler_thumbnail_info_default_format;
}



void
tumbler_thumbnail_info_set_default_format (TumblerThumbnailFormat format)
{
  g_return_if_fail (format != TUMBLER_THUMBNAIL_FORMAT_INVALID);
  tumbler_thumbnail_info_default_format = format;
}
