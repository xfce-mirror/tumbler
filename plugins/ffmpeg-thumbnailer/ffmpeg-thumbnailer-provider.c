/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2010 Lionel Le Folgoc <mrpouit@ubuntu.com>
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
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <ffmpeg-thumbnailer/ffmpeg-thumbnailer-provider.h>
#include <ffmpeg-thumbnailer/ffmpeg-thumbnailer.h>



static void   ffmpeg_thumbnailer_provider_thumbnailer_provider_init (gpointer                    g_iface,
                                                                     gpointer                    iface_data);
static GList *ffmpeg_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider *provider);



struct _FfmpegThumbnailerProviderClass
{
  GObjectClass __parent__;
};

struct _FfmpegThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (FfmpegThumbnailerProvider,
                                ffmpeg_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                TUMBLER_ADD_INTERFACE (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                       ffmpeg_thumbnailer_provider_thumbnailer_provider_init));



void
ffmpeg_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  ffmpeg_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
ffmpeg_thumbnailer_provider_class_init (FfmpegThumbnailerProviderClass *klass)
{
}



static void
ffmpeg_thumbnailer_provider_class_finalize (FfmpegThumbnailerProviderClass *klass)
{
}



static void
ffmpeg_thumbnailer_provider_thumbnailer_provider_init (gpointer g_iface,
                                                       gpointer iface_data)
{
  TumblerThumbnailerProviderIface *iface = g_iface;

  iface->get_thumbnailers = ffmpeg_thumbnailer_provider_get_thumbnailers;
}



static void
ffmpeg_thumbnailer_provider_init (FfmpegThumbnailerProvider *provider)
{
}



static GList *
ffmpeg_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  static const gchar *mime_types[] =
  {
    "video/jpeg",
    "video/mp4",
    "video/mp2t",
    "video/mpeg",
    "video/quicktime",
    "video/x-ms-asf",
    "video/x-ms-wm",
    "video/x-ms-wmv",
    "video/x-msvideo",
    "video/x-flv",
    "application/x-flash-video",
    "application/vnd.rn-realmedia",
    "video/3gpp",
    "video/x-matroska",
    "video/ogg",
    "video/webm",
    NULL
  };
  FfmpegThumbnailer  *thumbnailer;
  GList              *thumbnailers = NULL;
  GStrv               uri_schemes;

  /* determine the URI schemes supported by GIO */
  uri_schemes = tumbler_util_get_supported_uri_schemes ();

  /* create the pixbuf thumbnailer */
  thumbnailer = g_object_new (TYPE_FFMPEG_THUMBNAILER,
                              "uri-schemes", uri_schemes, "mime-types", mime_types,
                              NULL);

  /* add the thumbnailer to the list */
  thumbnailers = g_list_append (thumbnailers, thumbnailer);

  /* free URI schemes */
  g_strfreev (uri_schemes);

  return thumbnailers;
}
