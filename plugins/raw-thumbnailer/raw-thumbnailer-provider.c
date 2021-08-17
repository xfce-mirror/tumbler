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
#include <glib-object.h>

/* #include <gdk-pixbuf/gdk-pixbuf.h> */

#include <tumbler/tumbler.h>

#include "raw-thumbnailer-provider.h"
#include "raw-thumbnailer.h"



static void   raw_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *raw_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider      *provider);



struct _RawThumbnailerProviderClass
{
  GObjectClass __parent__;
};

struct _RawThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (RawThumbnailerProvider,
                                raw_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                               raw_thumbnailer_provider_thumbnailer_provider_init));



void
raw_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  raw_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
raw_thumbnailer_provider_class_init (RawThumbnailerProviderClass *klass)
{
}



static void
raw_thumbnailer_provider_class_finalize (RawThumbnailerProviderClass *klass)
{
}



static void
raw_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface)
{
  iface->get_thumbnailers = raw_thumbnailer_provider_get_thumbnailers;
}



static void
raw_thumbnailer_provider_init (RawThumbnailerProvider *provider)
{
}



static GList *
raw_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  RawThumbnailer     *thumbnailer;
  GList              *thumbnailers = NULL;
  static const gchar *uri_schemes[] = { "file", NULL };
  const gchar        *mime_types[] =
  {
    "image/x-adobe-dng",
    "image/x-canon-cr2",
    "image/x-canon-crw",
    "image/x-epson-erf",
    "image/x-nikon-nef",
    "image/x-nikon-nrw",
    "image/x-olympus-orf",
    "image/x-panasonic-raw",
    "image/x-panasonic-rw2",
    "image/x-pentax-pef",
    "image/x-sony-arw",
    "image/x-minolta-mrw",
    "image/x-fuji-raf",
    NULL
  };

  /* create the raw thumbnailer */
  thumbnailer = g_object_new (TYPE_RAW_THUMBNAILER,
                              "uri-schemes", uri_schemes,
                              "mime-types", mime_types,
                              NULL);

  /* add the thumbnailer to the list */
  thumbnailers = g_list_append (thumbnailers, thumbnailer);

  return thumbnailers;
}
