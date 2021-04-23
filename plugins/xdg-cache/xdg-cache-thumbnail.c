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

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <xdg-cache/xdg-cache-cache.h>
#include <xdg-cache/xdg-cache-thumbnail.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_CACHE,
  PROP_URI,
  PROP_FLAVOR,
};



static void     xdg_cache_thumbnail_thumbnail_init  (gpointer                g_iface,
                                                     gpointer                iface_data);
static void     xdg_cache_thumbnail_finalize        (GObject                *object);
static void     xdg_cache_thumbnail_get_property    (GObject                *object,
                                                     guint                   prop_id,
                                                     GValue                 *value,
                                                     GParamSpec             *pspec);
static void     xdg_cache_thumbnail_set_property    (GObject                *object,
                                                     guint                   prop_id,
                                                     const GValue           *value,
                                                     GParamSpec             *pspec);
static gboolean xdg_cache_thumbnail_load            (TumblerThumbnail       *thumbnail,
                                                     GCancellable           *cancellable,
                                                     GError                **error);
static gboolean xdg_cache_thumbnail_needs_update    (TumblerThumbnail       *thumbnail,
                                                     const gchar            *uri,
                                                     guint64                 mtime);
static gboolean xdg_cache_thumbnail_save_image_data (TumblerThumbnail       *thumbnail,
                                                     TumblerImageData       *data,
                                                     guint64                 mtime,
                                                     GCancellable           *cancellable,
                                                     GError                **error);



struct _XDGCacheThumbnailClass
{
  GObjectClass __parent__;
};

struct _XDGCacheThumbnail
{
  GObject __parent__;

  TumblerThumbnailFlavor *flavor;
  XDGCacheCache          *cache;
  gchar                  *uri;
  gchar                  *cached_uri;
  guint64                 cached_mtime;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (XDGCacheThumbnail,
                                xdg_cache_thumbnail,
                                G_TYPE_OBJECT,
                                0,
                                TUMBLER_ADD_INTERFACE (TUMBLER_TYPE_THUMBNAIL,
                                                       xdg_cache_thumbnail_thumbnail_init));



void
xdg_cache_thumbnail_register (TumblerCachePlugin *plugin)
{
  xdg_cache_thumbnail_register_type (G_TYPE_MODULE (plugin));
}



static void
xdg_cache_thumbnail_class_init (XDGCacheThumbnailClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xdg_cache_thumbnail_finalize; 
  gobject_class->get_property = xdg_cache_thumbnail_get_property;
  gobject_class->set_property = xdg_cache_thumbnail_set_property;

  g_object_class_override_property (gobject_class, PROP_CACHE, "cache");
  g_object_class_override_property (gobject_class, PROP_URI, "uri");
  g_object_class_override_property (gobject_class, PROP_FLAVOR, "flavor");
}



static void
xdg_cache_thumbnail_class_finalize (XDGCacheThumbnailClass *klass)
{
}



static void
xdg_cache_thumbnail_thumbnail_init (gpointer g_iface,
                                    gpointer iface_data)
{
  TumblerThumbnailIface *iface = g_iface;

  iface->load = xdg_cache_thumbnail_load;
  iface->needs_update = xdg_cache_thumbnail_needs_update;
  iface->save_image_data = xdg_cache_thumbnail_save_image_data;
}



static void
xdg_cache_thumbnail_init (XDGCacheThumbnail *thumbnail)
{
}



static void
xdg_cache_thumbnail_finalize (GObject *object)
{
  XDGCacheThumbnail *thumbnail = XDG_CACHE_THUMBNAIL (object);
  
  g_free (thumbnail->uri);
  g_free (thumbnail->cached_uri);

  g_object_unref (thumbnail->cache);

  (*G_OBJECT_CLASS (xdg_cache_thumbnail_parent_class)->finalize) (object);
}



static void
xdg_cache_thumbnail_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  XDGCacheThumbnail *thumbnail = XDG_CACHE_THUMBNAIL (object);

  switch (prop_id)
    {
    case PROP_CACHE:
      g_value_set_object (value, TUMBLER_CACHE (thumbnail->cache));
      break;
    case PROP_URI:
      g_value_set_string (value, thumbnail->uri);
      break;
    case PROP_FLAVOR:
      g_value_set_object (value, thumbnail->flavor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xdg_cache_thumbnail_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  XDGCacheThumbnail *thumbnail = XDG_CACHE_THUMBNAIL (object);

  switch (prop_id)
    {
    case PROP_CACHE:
      thumbnail->cache = XDG_CACHE_CACHE (g_value_dup_object (value));
      break;
    case PROP_URI:
      thumbnail->uri = g_value_dup_string (value);
      break;
    case PROP_FLAVOR:
      thumbnail->flavor = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
xdg_cache_thumbnail_load (TumblerThumbnail *thumbnail,
                          GCancellable     *cancellable,
                          GError          **error)
{
  XDGCacheThumbnail *cache_thumbnail = XDG_CACHE_THUMBNAIL (thumbnail);
  GError            *err = NULL;
  GFile             *file;
  gchar             *path;

  g_return_val_if_fail (XDG_CACHE_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (cache_thumbnail->uri != NULL, FALSE);
  g_return_val_if_fail (XDG_CACHE_IS_CACHE (cache_thumbnail->cache), FALSE);

  file = xdg_cache_cache_get_file (cache_thumbnail->uri, 
                                   cache_thumbnail->flavor);
  path = g_file_get_path (file);
  g_object_unref (file);

  g_free (cache_thumbnail->cached_uri);
  cache_thumbnail->cached_uri = NULL;
  cache_thumbnail->cached_mtime = 0;

  xdg_cache_cache_read_thumbnail_info (path, 
                                       &cache_thumbnail->cached_uri,
                                       &cache_thumbnail->cached_mtime,
                                       cancellable, &err);

  /* free the filename */
  g_free (path);

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



static gboolean
xdg_cache_thumbnail_needs_update (TumblerThumbnail *thumbnail,
                                  const gchar      *uri,
                                  guint64           mtime)
{
  XDGCacheThumbnail *cache_thumbnail = XDG_CACHE_THUMBNAIL (thumbnail);

  g_return_val_if_fail (XDG_CACHE_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (uri != NULL && *uri != '\0', FALSE);

  if (cache_thumbnail->cached_uri == NULL)
    return TRUE;

  if (cache_thumbnail->cached_mtime == 0)
    return TRUE;

  return strcmp (cache_thumbnail->uri, uri) != 0
    || cache_thumbnail->cached_mtime != mtime;
}



static gboolean
xdg_cache_thumbnail_save_image_data (TumblerThumbnail *thumbnail,
                                     TumblerImageData *data,
                                     guint64           mtime,
                                     GCancellable     *cancellable,
                                     GError          **error)
{
  XDGCacheThumbnail *cache_thumbnail = XDG_CACHE_THUMBNAIL (thumbnail);
  GFileOutputStream *stream;
  GdkPixbuf         *dest_pixbuf;
  GdkPixbuf         *src_pixbuf;
  GError            *err = NULL;
  GFile             *dest_file;
  GFile             *flavor_dir;
  GFile             *temp_file;
  gchar             *dest_path;
  gchar             *flavor_dir_path;
  gchar             *temp_path;
  gchar             *mtime_str;
  gint               width;
  gint               height;

  g_return_val_if_fail (XDG_CACHE_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* abort if cancelled */
  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  /* determine dimensions of the thumbnail pixbuf */
  width = data->width;
  height = data->height;

  src_pixbuf = gdk_pixbuf_new_from_data (data->data,
                                         (GdkColorspace) data->colorspace,
                                         data->has_alpha,
                                         data->bits_per_sample,
                                         data->width,
                                         data->height,
                                         data->rowstride,
                                         NULL, NULL);

  /* generate a new pixbuf that is guranteed to follow the thumbnail spec */
  dest_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  
  /* copy the thumbnail pixbuf into the destination pixbuf */
  gdk_pixbuf_copy_area (src_pixbuf, 0, 0, width, height, dest_pixbuf, 0, 0);

  /* determine the URI of the temporary file to write to */
  temp_file = xdg_cache_cache_get_temp_file (cache_thumbnail->uri,
                                             cache_thumbnail->flavor);
  
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
      mtime_str = g_strdup_printf ("%" G_GUINT64_FORMAT, mtime);

      /* try to save the pixbuf */
      if (gdk_pixbuf_save_to_stream (dest_pixbuf, G_OUTPUT_STREAM (stream), "png",
                                     cancellable, &err, 
                                     "tEXt::Thumb::URI", cache_thumbnail->uri,
                                     "tEXt::Thumb::MTime", mtime_str,
                                     NULL))
        {
          /* saving succeeded, termine the final destination of the thumbnail */
          dest_file = xdg_cache_cache_get_file (cache_thumbnail->uri, 
                                                cache_thumbnail->flavor);

          /* determine temp and destination paths */
          temp_path = g_file_get_path (temp_file);
          dest_path = g_file_get_path (dest_file);

          /* try to rename the thumbnail */
          if (g_rename (temp_path, dest_path) == -1)
            {
              g_set_error (&err, TUMBLER_ERROR, TUMBLER_ERROR_SAVE_FAILED,
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

  /* destroy the source pixbuf, destination pixbuf and temporary GFile */
  g_object_unref (dest_pixbuf);
  g_object_unref (src_pixbuf);
  g_object_unref (temp_file);

  if (err != NULL)
    {
      g_propagate_error (error, err);
      return FALSE;
    }
  else
    {
      g_free (cache_thumbnail->cached_uri);
      cache_thumbnail->cached_uri = g_strdup (cache_thumbnail->uri);
      cache_thumbnail->cached_mtime = mtime;
      return TRUE;
    }
}
