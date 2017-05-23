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

#include <glib.h>
#include <glib-object.h>


#include <tumbler/tumbler.h>

#include <desktop-thumbnailer/desktop-thumbnailer-provider.h>
#include <desktop-thumbnailer/desktop-thumbnailer.h>



static void   desktop_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *desktop_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider      *provider);



struct _DesktopThumbnailerProviderClass
{
  GObjectClass __parent__;
};

struct _DesktopThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (DesktopThumbnailerProvider,
                                desktop_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                TUMBLER_ADD_INTERFACE (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                       desktop_thumbnailer_provider_thumbnailer_provider_init));



void
desktop_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  desktop_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
desktop_thumbnailer_provider_class_init (DesktopThumbnailerProviderClass *klass)
{
}



static void
desktop_thumbnailer_provider_class_finalize (DesktopThumbnailerProviderClass *klass)
{
}



static void
desktop_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface)
{
  iface->get_thumbnailers = desktop_thumbnailer_provider_get_thumbnailers;
}



static void
desktop_thumbnailer_provider_init (DesktopThumbnailerProvider *provider)
{
}



static GList *
desktop_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  /* This list is mainly from Totem. Generating a list from
   * GStreamer isn't realistic, so we have to hardcode it. */
  /* See https://git.gnome.org/browse/totem/tree/data/mime-type-list.txt */
  static const char *mime_types[] = {
	  "dummy",
	  NULL
  };

  DesktopThumbnailer    *thumbnailer;
  GError                *error = NULL;
  GStrv                  uri_schemes;

  uri_schemes = tumbler_util_get_supported_uri_schemes ();

  thumbnailer = g_object_new (TYPE_DESKTOP_THUMBNAILER,
                              "uri-schemes", uri_schemes,
                              "mime-types", mime_types,
                              NULL);

  g_strfreev (uri_schemes);

  return g_list_append (NULL, thumbnailer);
}
