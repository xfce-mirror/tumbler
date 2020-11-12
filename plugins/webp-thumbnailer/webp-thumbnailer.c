/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2010 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2020, Olivier Duchateau <duchateau.olivier@gmail.com>
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
#include <gdk/gdk.h>
#include <cairo.h>
#include <webp/decode.h>
#include <webp/demux.h>

#include "webp-thumbnailer.h"


static void webp_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                                     GCancellable               *cancellable,
                                     TumblerFileInfo            *info);



struct _WebPThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _WebPThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (WebPThumbnailer,
                       webp_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
webp_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  webp_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
webp_thumbnailer_class_init (WebPThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = webp_thumbnailer_create;
}



static void
webp_thumbnailer_class_finalize (WebPThumbnailerClass *klass)
{
}



static void
webp_thumbnailer_init (WebPThumbnailer *thumbnailer)
{
}



static GdkPixbuf *
webp_thumbnailer_scale_pixbuf (GdkPixbuf        *source,
                               TumblerThumbnail *thumbnail)
{
  TumblerThumbnailFlavor *flavor;
  gint                    dest_width, width;
  gint                    dest_height, height;
  gdouble                 wratio, hratio;

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
webp_thumbnailer_decode_frame (const uint8_t     *data,
                               size_t             data_size,
                               WebPDecoderConfig *config,
                               GError           **error)
{
  gint             width, height;
  gint             stride;
  cairo_surface_t *surface;
  GdkPixbuf       *pixbuf = NULL;
  GError          *err = NULL;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (WebPGetFeatures (data, data_size, &config->input)
      != VP8_STATUS_OK)
    {
      g_set_error (&err, TUMBLER_ERROR,
                   TUMBLER_ERROR_NO_CONTENT,
                   _("It seems not a valid WebP image"));
      g_propagate_error (error, err);
      return NULL;
    }

  width = config->input.width;
  height = config->input.height;
  config->options.no_fancy_upsampling = 1;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        width, height);
  cairo_surface_flush (surface);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  config->output.colorspace = MODE_BGRA;
#elif G_BYTE_ORDER == G_BIG_ENDIAN
  config->output.colorspace = MODE_ARGB;
#endif
  config->output.u.RGBA.rgba = (uint8_t *) cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);
  config->output.u.RGBA.stride = stride;
  config->output.u.RGBA.size = stride * height;
  config->output.is_external_memory = 1;

  if (WebPDecode (data, data_size, config) != VP8_STATUS_OK)
    {
      g_set_error (&err, TUMBLER_ERROR,
                   TUMBLER_ERROR_NO_CONTENT,
                   _("Unable to decode WebP image"));
      g_propagate_error (error, err);

      WebPFreeDecBuffer (&config->output);
      cairo_surface_destroy (surface);
      return NULL;
    }

  cairo_surface_mark_dirty (surface);
  if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
    pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                          width, height);

  WebPFreeDecBuffer (&config->output);
  cairo_surface_destroy (surface);

  if (err != NULL)
    g_propagate_error (error, err);

  return pixbuf;
}



static GdkPixbuf *
webp_thumbnailer_get_pixbuf (GFile            *file,
                             TumblerThumbnail *thumbnail,
                             GError          **error)
{
  WebPDecoderConfig config;
  WebPData          data;
  WebPDemuxer      *demux;
  WebPIterator      iter;
  uint32_t          flags;
  GError           *err = NULL;
  uint8_t          *content;
  gsize             length;
  GdkPixbuf        *pixbuf = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! WebPInitDecoderConfig (&config))
    {
      g_set_error (&err, TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                   _("Could not initialize WebP configuration object"));
      g_propagate_error (error, err);
      return NULL;
    }

  if (! g_file_load_contents (file, NULL,
                              (gchar **)&content,
                              &length, NULL,
                              &err))
    {
      g_propagate_error (error, err);
      return NULL;
    }

  data.bytes = content;
  data.size = length;

  demux = WebPDemux (&data);
  flags = WebPDemuxGetI (demux, WEBP_FF_FORMAT_FLAGS);
  if (flags & ANIMATION_FLAG)
    {
      WebPData frame;
      /* only the first frame! */
      if (! WebPDemuxGetFrame (demux, 1, &iter))
        {
          g_set_error (&err, TUMBLER_ERROR,
                       TUMBLER_ERROR_NO_CONTENT,
                       _("Unable to retrieve WebP frame"));
          g_propagate_error (error, err);

          WebPDemuxDelete (demux);
          g_free (content);
          return NULL;
        }
      else
        {
          frame = iter.fragment;
          pixbuf = webp_thumbnailer_decode_frame (frame.bytes,
                                                  frame.size,
                                                  &config,
                                                  &err);

          WebPDemuxReleaseIterator (&iter);
        }
    }
  else
    {
      pixbuf = webp_thumbnailer_decode_frame (data.bytes,
                                              data.size,
                                              &config, &err);
    }

  WebPDemuxDelete (demux);
  g_free (content);

  if (err != NULL)
    g_propagate_error (error, err);

  if (pixbuf != NULL)
    return webp_thumbnailer_scale_pixbuf (pixbuf, thumbnail);
  else
    return pixbuf;
}



static void
webp_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                         GCancellable               *cancellable,
                         TumblerFileInfo            *info)
{
  TumblerImageData  data;
  TumblerThumbnail *thumbnail;
  const gchar      *uri;
  GFile            *file;
  GError           *error = NULL;
  GdkPixbuf        *pixbuf = NULL;

  g_return_if_fail (IS_WEBP_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);

  /* try to load the WebP file based on the URI */
  file = g_file_new_for_uri (uri);

  thumbnail = tumbler_file_info_get_thumbnail (info);

  pixbuf = webp_thumbnailer_get_pixbuf (file, thumbnail, &error);

  g_clear_object (&file);

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

  g_object_unref (thumbnail);
  g_object_unref (pixbuf);
}
