/* vi:set et ai sw=2 sts=2 ts=2: */
/*
 * Copyright (c) 2011 Intel Corporation
 *
 * Author: Ross Burton <ross@linux.intel.com>
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
#include <gst/gst.h>

#include <tumbler/tumbler.h>

#include <gst-thumbnailer/gst-thumbnailer-provider.h>
#include <gst-thumbnailer/gst-thumbnailer.h>



static void   gst_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *gst_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider      *provider);



struct _GstThumbnailerProviderClass
{
  GObjectClass __parent__;
};

struct _GstThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (GstThumbnailerProvider,
                                gst_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                               gst_thumbnailer_provider_thumbnailer_provider_init));



void
gst_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  gst_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
gst_thumbnailer_provider_class_init (GstThumbnailerProviderClass *klass)
{
}



static void
gst_thumbnailer_provider_class_finalize (GstThumbnailerProviderClass *klass)
{
}



static void
gst_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface)
{
  iface->get_thumbnailers = gst_thumbnailer_provider_get_thumbnailers;
}



static void
gst_thumbnailer_provider_init (GstThumbnailerProvider *provider)
{
}



static GList *
gst_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  /* This list is mainly from Totem. Generating a list from 
   * GStreamer isn't realistic, so we have to hardcode it. */
  /* See https://git.gnome.org/browse/totem/tree/data/mime-type-list.txt */
  static const char *mime_types[] = {
    "application/mxf",
    "application/ogg",
    "application/ram",
    "application/sdp",
    "application/smil",
    "application/smil+xml",
    "application/vnd.apple.mpegurl",
    "application/vnd.ms-wpl",
    "application/vnd.rn-realmedia",
    "application/x-extension-m4a",
    "application/x-extension-mp4",
    "application/x-flac",
    "application/x-flash-video",
    "application/x-matroska",
    "application/x-netshow-channel",
    "application/x-ogg",
    "application/x-quicktime-media-link",
    "application/x-quicktimeplayer",
    "application/x-shorten",
    "application/x-smil",
    "application/xspf+xml",
    "audio/3gpp",
    "audio/ac3",
    "audio/AMR",
    "audio/AMR-WB",
    "audio/basic",
    "audio/flac",
    "audio/midi",
    "audio/mp2",
    "audio/mp4",
    "audio/mpeg",
    "audio/mpegurl",
    "audio/ogg",
    "audio/prs.sid",
    "audio/vnd.rn-realaudio",
    "audio/x-aiff",
    "audio/x-ape",
    "audio/x-flac",
    "audio/x-gsm",
    "audio/x-it",
    "audio/x-m4a",
    "audio/x-matroska",
    "audio/x-mod",
    "audio/x-mp3",
    "audio/x-mpeg",
    "audio/x-mpegurl",
    "audio/x-ms-asf",
    "audio/x-ms-asx",
    "audio/x-ms-wax",
    "audio/x-ms-wma",
    "audio/x-musepack",
    "audio/x-pn-aiff",
    "audio/x-pn-au",
    "audio/x-pn-realaudio",
    "audio/x-pn-realaudio-plugin",
    "audio/x-pn-wav",
    "audio/x-pn-windows-acm",
    "audio/x-realaudio",
    "audio/x-real-audio",
    "audio/x-s3m",
    "audio/x-sbc",
    "audio/x-scpls",
    "audio/x-speex",
    "audio/x-stm",
    "audio/x-tta",
    "audio/x-wav",
    "audio/x-wavpack",
    "audio/x-vorbis",
    "audio/x-vorbis+ogg",
    "audio/x-xm",
    "image/vnd.rn-realpix",
    "image/x-pict",
    "misc/ultravox",
    "text/google-video-pointer",
    "text/x-google-video-pointer",
    "video/3gp",
    "video/3gpp",
    "video/dv",
    "video/divx",
    "video/fli",
    "video/flv",
    "video/mp2t",
    "video/mp4",
    "video/mp4v-es",
    "video/mpeg",
    "video/msvideo",
    "video/ogg",
    "video/quicktime",
    "video/vivo",
    "video/vnd.divx",
    "video/vnd.mpegurl",
    "video/vnd.rn-realvideo",
    "video/vnd.vivo",
    "video/webm",
    "video/x-anim",
    "video/x-avi",
    "video/x-flc",
    "video/x-fli",
    "video/x-flic",
    "video/x-flv",
    "video/x-m4v",
    "video/x-matroska",
    "video/x-mpeg",
    "video/x-mpeg2",
    "video/x-ms-asf",
    "video/x-ms-asx",
    "video/x-msvideo",
    "video/x-ms-wm",
    "video/x-ms-wmv",
    "video/x-ms-wmx",
    "video/x-ms-wvx",
    "video/x-nsv",
    "video/x-ogm+ogg",
    "video/x-theora+ogg",
    "video/x-totem-stream",
    "x-content/video-dvd",
    "x-content/video-vcd",
    "x-content/video-svcd",
    NULL
  };
  GstThumbnailer    *thumbnailer;
  GError            *error = NULL;
  GStrv              uri_schemes;

  if (!gst_init_check (0, NULL, &error))
    {
      g_warning ("Cannot initialize GStreamer, thumbnailer not loaded: %s", 
                 error->message);
      return NULL;
    }

  uri_schemes = tumbler_util_get_supported_uri_schemes ();

  thumbnailer = g_object_new (TYPE_GST_THUMBNAILER,
                              "uri-schemes", uri_schemes,
                              "mime-types", mime_types,
                              NULL);

  g_strfreev (uri_schemes);

  return g_list_append (NULL, thumbnailer);
}
