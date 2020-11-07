/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2010 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2019-2020, Olivier Duchateau <duchateau.olivier@gmail.com>
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

#include <math.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <tumbler/tumbler.h>
#include <cairo.h>
#include <librsvg/rsvg.h>
#include <gdk/gdk.h>

#include "rsvg-thumbnailer.h"


static void rsvg_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                                     GCancellable               *cancellable,
                                     TumblerFileInfo            *info);



struct _RsvgThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _RsvgThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (RsvgThumbnailer,
                       rsvg_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
rsvg_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  rsvg_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
rsvg_thumbnailer_class_init (RsvgThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = rsvg_thumbnailer_create;
}



static void
rsvg_thumbnailer_class_finalize (RsvgThumbnailerClass *klass)
{
}



static void
rsvg_thumbnailer_init (RsvgThumbnailer *thumbnailer)
{
}



static GdkPixbuf *
rsvg_thumbnailer_scale_pixbuf (GdkPixbuf        *source,
                               TumblerThumbnail *thumbnail)
{
  TumblerThumbnailFlavor *flavor;
  gint                    dest_width, width;
  gint                    dest_height, height;
  gdouble                 wratio, hratio;

  g_return_val_if_fail (GDK_IS_PIXBUF (source), NULL);
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), NULL);

  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  tumbler_thumbnail_flavor_get_size (flavor, &dest_width,
                                     &dest_height);
  g_object_unref (flavor);

  /* width and height of original pixbuf */
  width = gdk_pixbuf_get_width (source);
  height = gdk_pixbuf_get_height (source);

  if (width <= dest_width && height <= dest_height)
    {
      /* do not scale the image */
      dest_width = width;
      dest_height = height;
    }
  else
    {
      /* determine which axis needs to be scaled down more */
      wratio = (gdouble) width / (gdouble) dest_width;
      hratio = (gdouble) height / (gdouble) dest_height;

      /* adjust the other axis */
      if (hratio > wratio)
        dest_width = rint (width / hratio);
      else
        dest_height = rint (height / wratio);
    }
  /* scale the pixbuf down to the desired size */
  return gdk_pixbuf_scale_simple (source,
                                  MAX (dest_width, 1),
                                  MAX (dest_height, 1),
                                  GDK_INTERP_BILINEAR);
}



static GdkPixbuf *
rsvg_thumbnailer_pixbuf_from_handle (RsvgHandle       *handle,
                                     TumblerThumbnail *thumbnail)
{
  RsvgDimensionData  dimensions;
  cairo_surface_t   *surface;
  cairo_t           *cr;
  GdkPixbuf         *pixbuf = NULL;

  g_return_val_if_fail (RSVG_IS_HANDLE (handle), NULL);
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), NULL);

  /* get (original) dimensions of file */
  rsvg_handle_get_dimensions (handle, &dimensions);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        dimensions.width,
                                        dimensions.height);
  cr = cairo_create (surface);

  cairo_save (cr);
  if (cairo_status (cr) == CAIRO_STATUS_SUCCESS)
    {
      if (rsvg_handle_render_cairo (handle, cr))
        {
          cairo_restore (cr);

          pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                                dimensions.width,
                                                dimensions.height);
        }
    }

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  if (pixbuf != NULL)
    return rsvg_thumbnailer_scale_pixbuf (pixbuf, thumbnail);
  else
    return pixbuf;
}



static void
rsvg_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                         GCancellable               *cancellable,
                         TumblerFileInfo            *info)
{
  TumblerImageData  data;
  TumblerThumbnail *thumbnail;
  const gchar      *uri;
  GFile            *file;
  GError           *error = NULL;
  GdkPixbuf        *pixbuf = NULL;
  RsvgHandle       *handle;
  GInputStream     *stream;
  GBytes           *bytes;

  g_return_if_fail (IS_RSVG_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);

  /* try to load the SVG/SVGZ file based on the URI */
  file = g_file_new_for_uri (uri);
  bytes = g_file_load_bytes (file, NULL, NULL, &error);
  if (bytes == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri,
                             error->code, error->message);
      g_error_free (error);

      g_clear_object (&file);
      return;
    }
  stream = g_memory_input_stream_new_from_bytes (bytes);

  handle = rsvg_handle_new_from_stream_sync (G_INPUT_STREAM (stream),
                                             file,
                                             RSVG_HANDLE_FLAG_KEEP_IMAGE_DATA,
                                             NULL, &error);
  if (handle == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri,
                             error->code, error->message);
      g_error_free (error);

      g_bytes_unref (bytes);
      g_clear_object (&file);
      return;
    }
  g_bytes_unref (bytes);

  rsvg_handle_set_dpi (handle, 90.0);

  thumbnail = tumbler_file_info_get_thumbnail (info);

  pixbuf = rsvg_thumbnailer_pixbuf_from_handle (handle, thumbnail);

  if (pixbuf != NULL)
    {
      data.data = gdk_pixbuf_get_pixels (pixbuf);
      data.has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
      data.bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
      data.width = gdk_pixbuf_get_width (pixbuf);
      data.height = gdk_pixbuf_get_height (pixbuf);
      data.rowstride = gdk_pixbuf_get_rowstride (pixbuf);
      data.colorspace = (TumblerColorspace) gdk_pixbuf_get_colorspace (pixbuf);

      tumbler_thumbnail_save_image_data (thumbnail, &data,
                                         tumbler_file_info_get_mtime (info),
                                         NULL, &error);
    }

  if (error != NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code,
                             error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", uri);
    }

  g_object_unref (handle);
  g_object_unref (thumbnail);
  g_object_unref (pixbuf);
  g_clear_object (&file);
}
