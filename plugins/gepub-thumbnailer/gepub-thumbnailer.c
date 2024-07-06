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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <tumbler/tumbler.h>
#include <gepub.h>

#include "gepub-thumbnailer.h"


static void gepub_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                                      GCancellable               *cancellable,
                                      TumblerFileInfo            *info);



struct _GepubThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _GepubThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (GepubThumbnailer,
                       gepub_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
gepub_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  gepub_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
gepub_thumbnailer_class_init (GepubThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = gepub_thumbnailer_create;
}



static void
gepub_thumbnailer_class_finalize (GepubThumbnailerClass *klass)
{
}



static void
gepub_thumbnailer_init (GepubThumbnailer *thumbnailer)
{
}



static GdkPixbuf *
gepub_thumbnailer_create_from_mime (gchar             *mime_type,
                                    GBytes            *content, 
                                    TumblerThumbnail  *thumbnail,
                                    GError           **error)
{
  GdkPixbufLoader *loader;
  GdkPixbuf       *pixbuf = NULL;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, error);
  if (loader != NULL)
    {
      g_signal_connect (loader, "size-prepared",
                        G_CALLBACK (tumbler_util_size_prepared), thumbnail);
      if (gdk_pixbuf_loader_write_bytes (loader, content, error))
        pixbuf = g_object_ref (gdk_pixbuf_loader_get_pixbuf (loader));

      gdk_pixbuf_loader_close (loader, NULL);
      g_object_unref (loader);
    }

  return pixbuf;
}



static void
gepub_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                          GCancellable               *cancellable,
                          TumblerFileInfo            *info)
{
  TumblerImageData  data;
  TumblerThumbnail *thumbnail;
  const gchar      *uri;
  GFile            *file;
  GError           *error = NULL;
  GdkPixbuf        *pixbuf = NULL;
  GepubDoc         *doc;
  gchar            *cover;
  gchar            *cover_mime;
  GBytes           *content;

  g_return_if_fail (IS_GEPUB_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);
  file = g_file_new_for_uri (uri);
  g_debug ("Handling URI '%s'", uri);

  if (g_file_is_native (file))
    {
      /* try to load the EPUB file */
      doc = gepub_doc_new (g_file_peek_path (file), &error);
      if (error != NULL)
        {
          g_signal_emit_by_name (thumbnailer, "error", info,
                                 error->domain, error->code, error->message);
          g_error_free (error);

          g_object_unref (file);
          return;
        }

      /* find cover file and its mime type */
      cover = gepub_doc_get_cover (doc);
      if (cover == NULL
          || (cover_mime = gepub_doc_get_resource_mime_by_id (doc, cover)) == NULL)
        {
          g_signal_emit_by_name (thumbnailer, "error", info,
                                 TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                                 "Cover not found");

          g_free (cover);
          g_object_unref (doc);
          g_object_unref (file);
          return;
        }

      /* content of cover image */
      content = gepub_doc_get_resource_by_id (doc, cover);
      if (g_bytes_get_data (content, NULL) == NULL)
        {
          g_signal_emit_by_name (thumbnailer, "error", info,
                                 TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                                 "Cover not found");

          g_bytes_unref (content);
          g_free (cover);
          g_free (cover_mime);
          g_object_unref (doc);
          g_object_unref (file);
          return;
        }

      thumbnail = tumbler_file_info_get_thumbnail (info);

      pixbuf = gepub_thumbnailer_create_from_mime (cover_mime, content,
                                                   thumbnail,
                                                   &error);
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

          g_object_unref (pixbuf);
        }

      g_bytes_unref (content);
      g_free (cover);
      g_free (cover_mime);
      g_object_unref (doc);
      g_object_unref (thumbnail);
      g_object_unref (file);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "error", info,
                             TUMBLER_ERROR, TUMBLER_ERROR_UNSUPPORTED,
                             TUMBLER_ERROR_MESSAGE_LOCAL_ONLY);
      g_object_unref (file);
      return;
    }

  if (error != NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", info,
                             error->domain, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", info);
    }
}
