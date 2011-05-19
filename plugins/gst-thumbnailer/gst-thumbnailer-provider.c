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
                                TUMBLER_ADD_INTERFACE (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
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
  static const char *mime_types[] = {
    "application/asx",
    "application/ogg",
    "application/x-flash-video",
    "application/x-ms-wmp",
    "application/x-ms-wms",
    "application/x-ogg",
    "video/3gpp",
    "video/divx",
    "video/flv",
    "video/jpeg",
    "video/mp4",
    "video/mpeg",
    "video/ogg",
    "video/quicktime",
    "video/x-flv",
    "video/x-m4v",
    "video/x-matroska",
    "video/x-ms-asf",
    "video/x-ms-wm",
    "video/x-ms-wmp",
    "video/x-ms-wmv",
    "video/x-ms-wvx",
    "video/x-msvideo",
    "video/x-ogg",
    "video/x-wmv",
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
