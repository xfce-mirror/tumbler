/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2011 Tam Merlant <tam.ille@free.fr>
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
#include <sys/types.h>
#include <fcntl.h>
#include <memory.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <tumbler/tumbler.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libopenraw-gnome/gdkpixbuf.h>

#include "raw-thumbnailer.h"

static void raw_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                                    GCancellable               *cancellable,
                                    TumblerFileInfo            *info);



struct _RawThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _RawThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (RawThumbnailer,
                       raw_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
raw_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  raw_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
raw_thumbnailer_class_init (RawThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = raw_thumbnailer_create;
}



static void
raw_thumbnailer_class_finalize (RawThumbnailerClass *klass)
{
}



static void
raw_thumbnailer_init (RawThumbnailer *thumbnailer)
{
}



static void
raw_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                        GCancellable               *cancellable,
                        TumblerFileInfo            *info)
{
  TumblerThumbnailFlavor *flavor;
  TumblerImageData        data;
  TumblerThumbnail       *thumbnail;
  const gchar            *uri;
  gchar                  *path;
  GdkPixbuf              *pixbuf = NULL;
  GError                 *error = NULL;
  GFile                  *file;
  gint                    height;
  gint                    width;
  GdkPixbuf              *scaled;

  g_return_if_fail (IS_RAW_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);
  file = g_file_new_for_uri (uri);

  thumbnail = tumbler_file_info_get_thumbnail (info);
  g_assert (thumbnail != NULL);

  /* get destination size */
  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  g_assert (flavor != NULL);
  tumbler_thumbnail_flavor_get_size (flavor, &width, &height);
  g_object_unref (flavor);

  /* libopenraw only handles local IO */
  path = g_file_get_path (file);
  if (path != NULL && g_path_is_absolute (path))
    {
      pixbuf = or_gdkpixbuf_extract_rotated_thumbnail (path, MIN (width, height));

      if (pixbuf == NULL)
        {
          g_set_error_literal (&error, TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                               TUMBLER_ERROR_MESSAGE_CREATION_FAILED);
        }
    }
  else
    {
      g_set_error_literal (&error, TUMBLER_ERROR, TUMBLER_ERROR_UNSUPPORTED,
                           TUMBLER_ERROR_MESSAGE_LOCAL_ONLY);
    }

  g_free (path);
  g_object_unref (file);

  if (pixbuf != NULL)
    {
      scaled = thumbler_util_scale_pixbuf (pixbuf, width, height);
      g_object_unref (pixbuf);
      pixbuf = scaled;

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
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", uri);
      g_object_unref (pixbuf);
    }

  g_object_unref (thumbnail);
}
