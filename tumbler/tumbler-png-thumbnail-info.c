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

#include <fcntl.h>
#include <sys/stat.h>

#include <png.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include <gio/gio.h>

#include <tumbler/tumbler-error.h>
#include <tumbler/tumbler-enum-types.h>
#include <tumbler/tumbler-png-thumbnail-info.h>
#include <tumbler/tumbler-thumbnail-info.h>



#define TUMBLER_PNG_THUMBNAIL_INFO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_PNG_THUMBNAIL_INFO, TumblerPNGThumbnailInfoPrivate))



/* property identifiers */
enum
{
  PROP_0,
  PROP_FORMAT,
  PROP_HASH,
  PROP_MTIME,
  PROP_URI,
};



static void                    tumbler_png_thumbnail_info_class_init           (TumblerPNGThumbnailInfoClass *klass);
static void                    tumbler_png_thumbnail_info_iface_init           (TumblerThumbnailInfoIface    *iface);
static void                    tumbler_png_thumbnail_info_init                 (TumblerPNGThumbnailInfo      *info);
static void                    tumbler_png_thumbnail_info_constructed          (GObject                      *object);
static void                    tumbler_png_thumbnail_info_finalize             (GObject                      *object);
static void                    tumbler_png_thumbnail_info_get_property         (GObject                      *object,
                                                                                guint                         prop_id,
                                                                                GValue                       *value,
                                                                                GParamSpec                   *pspec);
static void                    tumbler_png_thumbnail_info_set_property         (GObject                      *object,
                                                                                guint                         prop_id,
                                                                                const GValue                 *value,
                                                                                GParamSpec                   *pspec);
static gboolean                tumbler_png_thumbnail_info_generate_flavor      (TumblerThumbnailInfo         *info,
                                                                                TumblerThumbnailFlavor        flavor,
                                                                                GdkPixbuf                    *pixbuf,
                                                                                GCancellable                 *cancellable,
                                                                                GError                      **error);
static void                    tumbler_png_thumbnail_info_generate_fail        (TumblerThumbnailInfo         *info,
                                                                                GCancellable                 *cancel);
static gboolean                tumbler_png_thumbnail_info_needs_update         (TumblerThumbnailInfo         *info,
                                                                                GCancellable                 *cancellable);
static TumblerThumbnailFlavor *tumbler_png_thumbnail_info_get_invalid_flavors  (TumblerThumbnailInfo         *info,
                                                                                GCancellable                 *cancellable);
static void                    tumbler_png_thumbnail_info_load_invalid_flavors (TumblerPNGThumbnailInfo      *info,
                                                                                GCancellable                 *cancellable);



struct _TumblerPNGThumbnailInfoClass
{
  GObjectClass __parent__;
};

struct _TumblerPNGThumbnailInfo
{
  GObject __parent__;

  TumblerPNGThumbnailInfoPrivate *priv;
};

struct _TumblerPNGThumbnailInfoPrivate
{
  TumblerThumbnailFlavor *invalid_flavors;
  TumblerThumbnailFormat  format;
  guint64                 mtime;
  gchar                  *uri;
  gchar                  *hash;
};



static GObjectClass *tumbler_png_thumbnail_info_parent_class = NULL;



GType
tumbler_png_thumbnail_info_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GInterfaceInfo info = 
      {
        (GClassInitFunc) tumbler_png_thumbnail_info_iface_init,
        NULL,
        NULL,
      };

      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerPNGThumbnailInfo",
                                            sizeof (TumblerPNGThumbnailInfoClass),
                                            (GClassInitFunc) tumbler_png_thumbnail_info_class_init,
                                            sizeof (TumblerPNGThumbnailInfo),
                                            (GInstanceInitFunc) tumbler_png_thumbnail_info_init,
                                            0);

      g_type_add_interface_static (type, TUMBLER_TYPE_THUMBNAIL_INFO, &info);
    }

  return type;
}



static void
tumbler_png_thumbnail_info_class_init (TumblerPNGThumbnailInfoClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerPNGThumbnailInfoPrivate));

  /* Determine the parent type class */
  tumbler_png_thumbnail_info_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_png_thumbnail_info_constructed; 
  gobject_class->finalize = tumbler_png_thumbnail_info_finalize; 
  gobject_class->get_property = tumbler_png_thumbnail_info_get_property;
  gobject_class->set_property = tumbler_png_thumbnail_info_set_property;

  g_object_class_override_property (gobject_class, PROP_FORMAT, "format");
  g_object_class_override_property (gobject_class, PROP_HASH, "hash");
  g_object_class_override_property (gobject_class, PROP_MTIME, "mtime");
  g_object_class_override_property (gobject_class, PROP_URI, "uri");
}



static void
tumbler_png_thumbnail_info_iface_init (TumblerThumbnailInfoIface *iface)
{
  iface->generate_flavor = tumbler_png_thumbnail_info_generate_flavor;
  iface->generate_fail = tumbler_png_thumbnail_info_generate_fail;
  iface->needs_update = tumbler_png_thumbnail_info_needs_update;
  iface->get_invalid_flavors = tumbler_png_thumbnail_info_get_invalid_flavors;
}



static void
tumbler_png_thumbnail_info_init (TumblerPNGThumbnailInfo *info)
{
  info->priv = TUMBLER_PNG_THUMBNAIL_INFO_GET_PRIVATE (info);
  info->priv->uri = NULL;
  info->priv->mtime = 0;
  info->priv->invalid_flavors = NULL;
}



static void
tumbler_png_thumbnail_info_constructed (GObject *object)
{
  TumblerPNGThumbnailInfo *info = TUMBLER_PNG_THUMBNAIL_INFO (object);

  /* generate the URI hash used for thumbnail filenames */
  info->priv->hash = g_compute_checksum_for_string (G_CHECKSUM_MD5, info->priv->uri, -1);
}



static void
tumbler_png_thumbnail_info_finalize (GObject *object)
{
  TumblerPNGThumbnailInfo *info = TUMBLER_PNG_THUMBNAIL_INFO (object);

  g_free (info->priv->hash);
  g_free (info->priv->uri);
  g_free (info->priv->invalid_flavors);

  (*G_OBJECT_CLASS (tumbler_png_thumbnail_info_parent_class)->finalize) (object);
}



static void
tumbler_png_thumbnail_info_get_property (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  TumblerPNGThumbnailInfo *info = TUMBLER_PNG_THUMBNAIL_INFO (object);

  switch (prop_id)
    {
    case PROP_FORMAT:
      g_value_set_enum (value, info->priv->format);
      break;
    case PROP_HASH:
      g_value_set_string (value, info->priv->hash);
      break;
    case PROP_MTIME:
      g_value_set_uint64 (value, info->priv->mtime);
      break;
    case PROP_URI:
      g_value_set_string (value, info->priv->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_png_thumbnail_info_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  TumblerPNGThumbnailInfo *info = TUMBLER_PNG_THUMBNAIL_INFO (object);

  switch (prop_id)
    {
    case PROP_FORMAT:
      info->priv->format = g_value_get_enum (value);
      break;
    case PROP_HASH:
      info->priv->hash = g_value_dup_string (value);
      break;
    case PROP_MTIME:
      info->priv->mtime = g_value_get_uint64 (value);
      break;
    case PROP_URI:
      info->priv->uri = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
tumbler_png_thumbnail_info_generate_flavor (TumblerThumbnailInfo  *info,
                                            TumblerThumbnailFlavor flavor,
                                            GdkPixbuf             *pixbuf,
                                            GCancellable          *cancellable,
                                            GError               **error)
{
  TumblerPNGThumbnailInfo *pnginfo = TUMBLER_PNG_THUMBNAIL_INFO (info);
  GFileOutputStream       *stream;
  GdkPixbuf               *dest_pixbuf;
  GError                  *err = NULL;
  GFile                   *dest_file;
  GFile                   *flavor_dir;
  GFile                   *temp_file;
  gchar                   *dest_path;
  gchar                   *flavor_dir_path;
  gchar                   *temp_path;
  gchar                   *mtime_str;
  gint                     width;
  gint                     height;

  g_return_val_if_fail (TUMBLER_IS_PNG_THUMBNAIL_INFO (info), FALSE);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* load the modified time of the source URI on demand */
  if (pnginfo->priv->mtime == 0)
    tumbler_thumbnail_info_load_mtime (info, cancellable);

  /* abort if cancelled */
  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  /* determine dimensions of the thumbnail pixbuf */
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  /* generate a new pixbuf that is guranteed to follow the thumbnail spec */
  dest_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  
  /* copy the thumbnail pixbuf into the destination pixbuf */
  gdk_pixbuf_copy_area (pixbuf, 0, 0, width, height, dest_pixbuf, 0, 0);

  /* determine the URI of the temporary file to write to */
  temp_file = tumbler_thumbnail_info_temp_flavor_file_new (info, flavor);
  
  /* determine the flavor directory and its path */
  flavor_dir = g_file_get_parent (temp_file);
  flavor_dir_path = g_file_get_path (flavor_dir);

  /* create the flavor directory with user-only read/write/execute permissions */
  g_mkdir_with_parents (flavor_dir_path, S_IRWXU);

  /* free the flavor dir path and GFile */
  g_free (flavor_dir_path);
  g_object_unref (flavor_dir);

  /* open a stream to write to (and possibly replace) the temp file */
  stream = g_file_replace (temp_file, NULL, FALSE, G_FILE_CREATE_NONE, cancellable,
                           &err);

  if (stream != NULL)
    {
      /* convert the modified time of the source URI to a string */
      mtime_str = g_strdup_printf ("%lld", pnginfo->priv->mtime);

      /* try to save the pixbuf */
      if (gdk_pixbuf_save_to_stream (dest_pixbuf, G_OUTPUT_STREAM (stream), "png",
                                     cancellable, &err, 
                                     "tEXt::Thumb::URI", pnginfo->priv->uri,
                                     "tEXt::Thumb::MTime", mtime_str,
                                     NULL))
        {
          /* saving succeeded, termine the final destination of the thumbnail */
          dest_file = tumbler_thumbnail_info_flavor_file_new (info, flavor);

          /* determine temp and destination paths */
          temp_path = g_file_get_path (temp_file);
          dest_path = g_file_get_path (dest_file);

          /* try to rename the thumbnail */
          if (g_rename (temp_path, dest_path) == -1)
            {
              g_set_error (&err, TUMBLER_ERROR, TUMBLER_ERROR_FAILED,
                           _("Could not save thumbnail to \"%s\""), dest_path);
            }

          /* free strings */
          g_free (dest_path);
          g_free (temp_path);

          /* destroy the destination GFile */
          g_object_unref (dest_file);
        }

      /* free the modified time string */
      g_free (mtime_str);

      /* close and destroy the output stream */
      g_object_unref (stream);
    }

  /* destroy the destination pixbuf and temporary GFile */
  g_object_unref (dest_pixbuf);
  g_object_unref (temp_file);

  if (err != NULL)
    {
      g_propagate_error (error, err);
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}



static void
tumbler_png_thumbnail_info_generate_fail (TumblerThumbnailInfo *info,
                                          GCancellable         *cancellable)
{
  TumblerPNGThumbnailInfo *pnginfo = TUMBLER_PNG_THUMBNAIL_INFO (info);
  GFileOutputStream       *stream;
  GdkPixbuf               *pixbuf;
  GFile                   *fail_dir;
  GFile                   *fail_file;
  GFile                   *temp_file;
  gchar                   *fail_dir_path;
  gchar                   *fail_path;
  gchar                   *temp_path;

  g_return_if_fail (TUMBLER_IS_PNG_THUMBNAIL_INFO (info));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  /* load the modified time of the source URI on demand */
  if (pnginfo->priv->mtime == 0)
    tumbler_thumbnail_info_load_mtime (info, cancellable);

  /* abort if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  /* determine the GFile referring to the temporary fail file */
  temp_file = tumbler_thumbnail_info_temp_fail_file_new (info);

  /* determine the fail directory and its path */
  fail_dir = g_file_get_parent (temp_file);
  fail_dir_path = g_file_get_path (fail_dir);

  /* create the fail directory with user-only read/write/execute permissions */
  g_mkdir_with_parents (fail_dir_path, S_IRWXU);

  /* free fail dir path and GFile */
  g_free (fail_dir_path);
  g_object_unref (fail_dir);

  /* open a stream to write to (and possibly replace) the temp file */
  stream = g_file_replace (temp_file, NULL, FALSE, G_FILE_CREATE_NONE, 
                           cancellable, NULL);

  /* abort on errors */
  if (stream == NULL)
    {
      g_object_unref (temp_file);
      return;
    }

  /* create an empty pixbuf */
  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);

  /* write the pixbuf with thumbnail information to the stream */
  gdk_pixbuf_save_to_stream (pixbuf, G_OUTPUT_STREAM (stream), "png", cancellable, NULL,
                             "tEXt::Thumb::URI", pnginfo->priv->uri,
                             "tEXt::Thumb::MTime", pnginfo->priv->mtime,
                             NULL);

  /* destroy the pixbuf and the stream */
  g_object_unref (pixbuf);
  g_object_unref (stream);

  /* determine the final location of the fail file */
  fail_file = tumbler_thumbnail_info_fail_file_new (info);

  temp_path = g_file_get_path (temp_file);
  fail_path = g_file_get_path (fail_file);

  g_rename (temp_path, fail_path);

  g_free (fail_path);
  g_free (temp_path);

  /* destroy the file objects */
  g_object_unref (fail_file);
  g_object_unref (temp_file);
}



static gboolean
tumbler_png_thumbnail_info_needs_update (TumblerThumbnailInfo *info,
                                         GCancellable         *cancellable)
{
  TumblerPNGThumbnailInfo *pnginfo = TUMBLER_PNG_THUMBNAIL_INFO (info);

  g_return_val_if_fail (TUMBLER_IS_PNG_THUMBNAIL_INFO (info), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);

  if (g_cancellable_is_cancelled (cancellable))
    return FALSE;

  if (pnginfo->priv->mtime == 0)
    tumbler_thumbnail_info_load_mtime (info, cancellable);

  if (g_cancellable_is_cancelled (cancellable))
    return FALSE;

  if (pnginfo->priv->invalid_flavors == NULL)
    tumbler_png_thumbnail_info_load_invalid_flavors (pnginfo, cancellable);

  if (g_cancellable_is_cancelled (cancellable))
    return FALSE;

  if (pnginfo->priv->invalid_flavors == NULL)
    return FALSE;

  return pnginfo->priv->invalid_flavors[0] != TUMBLER_THUMBNAIL_FLAVOR_INVALID;
}



static TumblerThumbnailFlavor *
tumbler_png_thumbnail_info_get_invalid_flavors (TumblerThumbnailInfo *info,
                                                GCancellable         *cancellable)
{
  TumblerPNGThumbnailInfo *pnginfo = TUMBLER_PNG_THUMBNAIL_INFO (info);

  g_return_val_if_fail (TUMBLER_IS_PNG_THUMBNAIL_INFO (info), NULL);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

  if (pnginfo->priv->invalid_flavors == NULL)
    tumbler_png_thumbnail_info_load_invalid_flavors (pnginfo, cancellable);

  return pnginfo->priv->invalid_flavors;
}



static void
tumbler_png_thumbnail_info_load_invalid_flavors (TumblerPNGThumbnailInfo *info,
                                                 GCancellable            *cancellable)
{
  TumblerThumbnailFlavor *flavors;
  png_structp             png_ptr;
  png_infop               info_ptr;
  png_textp               text_ptr;
  gboolean                flavor_needs_update;
  GFile                  *file;
  GList                  *invalid_flavors = NULL;
  GList                  *lp;
  gchar                  *flavor_mtime_str = NULL;
  gchar                  *flavor_uri = NULL;
  gchar                  *mtime_str;
  gchar                  *path;
  FILE                   *png;
  gint                    num_flavors;
  gint                    num_text;
  gint                    n;
  gint                    i;

  g_return_if_fail (TUMBLER_IS_PNG_THUMBNAIL_INFO (info));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  if (g_cancellable_is_cancelled (cancellable))
    return;

  /* get all supported thumbnail flavors */
  flavors = tumbler_thumbnail_info_get_flavors ();

  for (n = 0; flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
    {
      if (g_cancellable_is_cancelled (cancellable))
        {
          g_list_free (invalid_flavors);
          return;
        }

      /* we assume all flavors are outdated/missing */
      flavor_needs_update = TRUE;

      /* determine the thumbnail path for the current thumbnail flavor */
      file = tumbler_thumbnail_info_flavor_file_new (TUMBLER_THUMBNAIL_INFO (info), 
                                                     flavors[n]);
      path = g_file_get_path (file);
      g_object_unref (file);

      /* try to open the file for reading */
      if ((png = g_fopen (path, "r")) != NULL)
        {
          /* initialize the PNG reader */
          png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

          if (png_ptr)
            {
              /* initialize the info structure */
              info_ptr = png_create_info_struct (png_ptr);

              if (info_ptr)
                {
                  /* initialize reading from the file and read the file info */
                  png_init_io (png_ptr, png);
                  png_read_info (png_ptr, info_ptr);

                  /* check if there is embedded text information */
                  if (png_get_text (png_ptr, info_ptr, &text_ptr, &num_text) > 0)
                    {
                      /* iterate over all text keys */
                      for (i = 0; i < num_text; ++i)
                        {
                          if (!text_ptr[i].key)
                            continue;
                          else if (g_utf8_collate ("Thumb::URI", text_ptr[i].key) == 0)
                            {
                              /* remember the Thumb::URI value */
                              flavor_uri = g_strdup (text_ptr[i].text);
                            }
                          else if (g_utf8_collate ("Thumb::MTime", text_ptr[i].key) == 0)
                            {
                              /* remember the Thumb::MTime value */
                              flavor_mtime_str = g_strdup (text_ptr[i].text);
                            }
                        }
                    }
                }

              /* finalize the PNG reader */
              png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
            }

          /* close the PNG file handle */
          fclose (png);
        }

      /* free the filename */
      g_free (path);

      /* generate an mtime string for the source URI */
      mtime_str = g_strdup_printf ("%lld", info->priv->mtime);

      /* compare information in the thumbnail with those of the source URI */
      if (flavor_uri != NULL && flavor_mtime_str != NULL 
          && g_utf8_collate (flavor_uri, info->priv->uri) == 0
          && g_utf8_collate (flavor_mtime_str, mtime_str) == 0)
        {
          /* information is up to date, no need to regenerate the thumbnail */
          flavor_needs_update = FALSE;
        }

      /* free strings */
      g_free (mtime_str);
      g_free (flavor_uri);
      g_free (flavor_mtime_str);

      if (flavor_needs_update)
        {
          invalid_flavors = g_list_prepend (invalid_flavors, 
                                            GUINT_TO_POINTER (flavors[n]));
        }
    }

  /* determine the number of invalid flavors */
  num_flavors = g_list_length (invalid_flavors);

  /* allocate flavor array */
  info->priv->invalid_flavors = g_new (TumblerThumbnailFlavor, num_flavors + 1);

  /* fill flavor array */
  for (n = 0, lp = invalid_flavors; lp != NULL && n < num_flavors; lp = lp->next, ++n)
    info->priv->invalid_flavors[n] = GPOINTER_TO_UINT (lp->data);

  /* terminate the array with an invalid flavor */
  info->priv->invalid_flavors[n] = TUMBLER_THUMBNAIL_FLAVOR_INVALID;

  /* free the temporary flavor list */
  g_list_free (invalid_flavors);
}
