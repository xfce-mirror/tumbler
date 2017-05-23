/*
 * Copyright (c) 2017 Ali Abdallah <ali@xfce.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gio/gio.h>


#include <tumbler/tumbler.h>

#include <desktop-thumbnailer/desktop-thumbnailer.h>



static void desktop_thumbnailer_create   (TumblerAbstractThumbnailer *thumbnailer,
										  GCancellable               *cancellable,
										  TumblerFileInfo            *info);



struct _DesktopThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _DesktopThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (DesktopThumbnailer,
                       desktop_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
desktop_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  desktop_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
desktop_thumbnailer_class_init (DesktopThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = desktop_thumbnailer_create;
}



static void
desktop_thumbnailer_class_finalize (DesktopThumbnailerClass *klass)
{
}



static void
desktop_thumbnailer_init (DesktopThumbnailer *thumbnailer)
{
}


static void
desktop_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                        GCancellable               *cancellable,
                        TumblerFileInfo            *info)
{
  TumblerThumbnail *thumbnail;
  const gchar      *uri;
  GFile            *file;
  GError           *error = NULL;
  gchar            *path;
  GdkPixbuf        *pixbuf = NULL;
  TumblerImageData  data;

  g_return_if_fail (IS_DESKTOP_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);
  file = g_file_new_for_uri (uri);


  thumbnail = tumbler_file_info_get_thumbnail (info);

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

  if (error != NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", uri);
    }

  g_object_unref (thumbnail);
}
