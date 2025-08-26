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
#include "config.h"
#endif

#include "ffmpeg-thumbnailer-provider.h"
#include "ffmpeg-thumbnailer.h"

#include <gdk-pixbuf/gdk-pixbuf.h>



static void
ffmpeg_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *
ffmpeg_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider);



struct _FfmpegThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (FfmpegThumbnailerProvider,
                                ffmpeg_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
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
ffmpeg_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface)
{
  iface->get_thumbnailers = ffmpeg_thumbnailer_provider_get_thumbnailers;
}



static void
ffmpeg_thumbnailer_provider_init (FfmpegThumbnailerProvider *provider)
{
}



static GList *
ffmpeg_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  static const gchar *mime_types[] = {
    "video/3gpp",
    "video/3gpp2",
    "video/annodex",
    "video/dv",
    "video/isivideo",
    "video/mj2",
    "video/mp2t",
    "video/mp4",
    "video/mpeg",
    "video/ogg",
    "video/quicktime",
    "video/vnd.avi",
    "video/vnd.mpegurl",
    "video/vnd.radgamettools.bink",
    "video/vnd.radgamettools.smacker",
    "video/vnd.rn-realvideo",
    "video/vnd.vivo",
    "video/vnd.youtube.yt",
    "video/wavelet",
    "video/webm",
    "video/x-anim",
    "video/x-flic",
    "video/x-flv",
    "video/x-javafx",
    "video/x-matroska",
    "video/x-matroska-3d",
    "video/x-mjpeg",
    "video/x-mng",
    "video/x-ms-wmv",
    "video/x-nsv",
    "video/x-ogm+ogg",
    "video/x-sgi-movie",
    "video/x-theora+ogg",
    "application/mxf",
    "application/vnd.ms-asf",
    "application/vnd.rn-realmedia",
    "application/x-matroska",
    "application/ogg",
    NULL
  };
  FfmpegThumbnailer *thumbnailer;
  GList *thumbnailers = NULL;
  GStrv uri_schemes;

  /* determine the URI schemes supported by GIO */
  uri_schemes = tumbler_util_get_supported_uri_schemes ();

  /* create the pixbuf thumbnailer */
  thumbnailer = g_object_new (FFMPEG_TYPE_THUMBNAILER,
                              "uri-schemes", uri_schemes, "mime-types", mime_types,
                              NULL);

  /* add the thumbnailer to the list */
  thumbnailers = g_list_append (thumbnailers, thumbnailer);

  tumbler_util_dump_strv (G_LOG_DOMAIN, "Supported URI schemes",
                          (const gchar *const *) uri_schemes);
  tumbler_util_dump_strv (G_LOG_DOMAIN, "Supported mime types", mime_types);

  /* free URI schemes */
  g_strfreev (uri_schemes);

  return thumbnailers;
}
