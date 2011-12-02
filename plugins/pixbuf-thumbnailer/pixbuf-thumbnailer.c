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

#include <math.h>

#include <glib.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <pixbuf-thumbnailer/pixbuf-thumbnailer.h>



static void pixbuf_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                                       GCancellable               *cancellable,
                                       TumblerFileInfo            *info);



struct _PixbufThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _PixbufThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (PixbufThumbnailer,
                       pixbuf_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
pixbuf_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  pixbuf_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
pixbuf_thumbnailer_class_init (PixbufThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = pixbuf_thumbnailer_create;
}



static void
pixbuf_thumbnailer_class_finalize (PixbufThumbnailerClass *klass)
{
}



static void
pixbuf_thumbnailer_init (PixbufThumbnailer *thumbnailer)
{
}



static void
pixbuf_thumbnailer_size_prepared (GdkPixbufLoader  *loader,
                                  gint              source_width,
                                  gint              source_height,
                                  TumblerThumbnail *thumbnail)
{
  TumblerThumbnailFlavor *flavor;
  gint                    dest_width;
  gint                    dest_height;
  gdouble                 hratio;
  gdouble                 wratio;

  g_return_if_fail (GDK_IS_PIXBUF_LOADER (loader));
  g_return_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail));

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



static GdkPixbuf *
pixbuf_thumbnailer_new_from_stream (GInputStream      *stream,
                                    TumblerThumbnail  *thumbnail,
                                    GCancellable      *cancellable,
                                    GError           **error)
{
  GdkPixbufLoader *loader;
  gssize           n_read;
  gboolean         result;
  GdkPixbuf       *pixbuf = NULL;
  guchar           buffer[65536];

  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), NULL);

  /* prepare the loader */
  loader = gdk_pixbuf_loader_new ();
  g_signal_connect (loader, "size-prepared",
      G_CALLBACK (pixbuf_thumbnailer_size_prepared), thumbnail);

  result = TRUE;
  for (;;)
    {
      n_read = g_input_stream_read (stream, buffer, sizeof (buffer),
                                    cancellable, error);

      if (n_read < 0)
        {
          result = FALSE;
          error = NULL;
          break;
        }

      if (n_read == 0)
        break;

      if (!gdk_pixbuf_loader_write (loader, buffer, n_read, error))
        {
          result = FALSE;
          error = NULL;
          break;
        }
    }

  if (!gdk_pixbuf_loader_close (loader, error))
    {
      result = FALSE;
      error = NULL;
    }

  if (result)
    {
      pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
      if (G_LIKELY (pixbuf != NULL))
        g_object_ref (pixbuf);
    }

  g_object_unref (loader);

  return pixbuf;
}



static void
pixbuf_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                           GCancellable               *cancellable,
                           TumblerFileInfo            *info)
{

  GFileInputStream *stream;
  TumblerImageData  data;
  TumblerThumbnail *thumbnail;
  const gchar      *uri;
  GdkPixbuf        *pixbuf;
  GError           *error = NULL;
  GFile            *file;

  g_return_if_fail (IS_PIXBUF_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);

  /* try to open the source file for reading */
  file = g_file_new_for_uri (uri);
  stream = g_file_read (file, NULL, &error);
  g_object_unref (file);

  if (stream == NULL)
    {
      if (error != NULL)
        {
          g_signal_emit_by_name (thumbnailer, "error", uri, error->code,
                                 error->message);
          g_error_free (error);
        }
      else
        {
          g_signal_emit_by_name (thumbnailer, "error", uri, 0,
                                 "Failed to open pixbuf stream");
        }

      return;
    }

  thumbnail = tumbler_file_info_get_thumbnail (info);
  g_assert (thumbnail != NULL);

  /* load the scaled pixbuf from the stream. this works like
   * gdk_pixbuf_new_from_file_at_scale(), but without increasing the
   * pixbuf size. */
  pixbuf = pixbuf_thumbnailer_new_from_stream (G_INPUT_STREAM (stream), thumbnail,
                                               cancellable, &error);

  g_object_unref (stream);

  if (pixbuf == NULL)
    {
      if (error != NULL)
        {
          g_signal_emit_by_name (thumbnailer, "error", uri, error->code,
                                 error->message);
          g_error_free (error);
        }
      else
        {
          g_signal_emit_by_name (thumbnailer, "error", uri, 0,
                                 "Failed to create pixbuf from stream");
        }

      g_object_unref (thumbnail);
      return;
    }

  g_assert (pixbuf != NULL);

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

  if (error != NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", uri);
    }

  g_object_unref (pixbuf);
  g_object_unref (thumbnail);
}
