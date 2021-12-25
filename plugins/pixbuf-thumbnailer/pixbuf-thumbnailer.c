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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <pixbuf-thumbnailer/pixbuf-thumbnailer.h>



/*
 * A buffer size of 1 MiB makes it possible to load a lot of current images in only
 * a few iterations, which can significantly improve performance for some formats
 * like GIF, without being too big, which on the contrary would degrade performance.
 * On the other hand, it ensures that a cancelled load will end fairly quickly at
 * the end of the current iteration, instead of continuing almost indefinitely as
 * can be the case for some images, again in GIF format.
 * See https://gitlab.xfce.org/apps/ristretto/-/issues/16 for an example of such an
 * image.
 */
#define LOADER_BUFFER_SIZE 1048576



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



static GdkPixbuf *
pixbuf_thumbnailer_new_from_stream (GInputStream      *stream,
                                    TumblerThumbnail  *thumbnail,
                                    const gchar       *mime_type,
                                    GCancellable      *cancellable,
                                    GError           **error)
{
  GdkPixbufLoader *loader;
  GdkPixbufAnimation *animation;
  GdkPixbufAnimationIter *iter;
  gssize           n_read;
  GdkPixbuf       *pixbuf = NULL;
  guchar          *buffer;
  GError          *err = NULL;
  gboolean         has_frame;

  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* try to use a pixbuf loader specific to the mime type, falling back on the
   * generic loader in case of error */
  loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, NULL);
  if (loader == NULL)
    loader = gdk_pixbuf_loader_new ();

  g_signal_connect (loader, "size-prepared",
                    G_CALLBACK (tumbler_util_size_prepared), thumbnail);

  buffer = g_new (guchar, LOADER_BUFFER_SIZE);
  for (;;)
    {
      n_read = g_input_stream_read (stream, buffer, LOADER_BUFFER_SIZE,
                                    cancellable, &err);

      /* read error (< 0), read finished (== 0) or write error */
      if (n_read <= 0 || ! gdk_pixbuf_loader_write (loader, buffer, n_read, &err))
        break;

      /* we only need the first frame to generate a thumbnail in case of an animation */
      animation = gdk_pixbuf_loader_get_animation (loader);
      if (animation != NULL)
        {
          iter = gdk_pixbuf_animation_get_iter (animation, NULL);
          has_frame = ! gdk_pixbuf_animation_iter_on_currently_loading_frame (iter);
          g_object_unref (iter);

          if (has_frame)
            break;
        }
    }

  /* close the pixbuf loader, ignoring errors if there has been one before */
  gdk_pixbuf_loader_close (loader, err == NULL ? &err : NULL);

  /* some images reported as corrupt are still displayable and we don't care about
   * the rest of the animation */
  if (err == NULL || g_error_matches (err, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE)
      || g_error_matches (err, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INCOMPLETE_ANIMATION))
    {
      pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
      if (G_LIKELY (pixbuf != NULL))
        {
          g_clear_error (&err);
          pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
        }
      /* should not happend, just in case */
      else if (err == NULL)
        g_set_error (&err, TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                     TUMBLER_ERROR_MESSAGE_CREATION_FAILED);
    }

  g_object_unref (loader);
  g_free (buffer);
  if (err != NULL)
    g_propagate_error (error, err);

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
  g_debug ("Handling URI '%s'", uri);

  /* try to open the source file for reading */
  file = g_file_new_for_uri (uri);
  stream = g_file_read (file, cancellable, &error);
  g_object_unref (file);

  if (stream == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code,
                             error->message);
      g_error_free (error);

      return;
    }

  thumbnail = tumbler_file_info_get_thumbnail (info);
  g_assert (thumbnail != NULL);

  /* load the scaled pixbuf from the stream. this works like
   * gdk_pixbuf_new_from_file_at_scale(), but without increasing the
   * pixbuf size. */
  pixbuf = pixbuf_thumbnailer_new_from_stream (G_INPUT_STREAM (stream), thumbnail,
                                               tumbler_file_info_get_mime_type (info),
                                               cancellable, &error);

  g_object_unref (stream);

  if (pixbuf == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code,
                             error->message);
      g_error_free (error);
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
