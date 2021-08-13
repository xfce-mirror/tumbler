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
#include <glib-object.h>

#include <tumbler/tumbler.h>

#include "gepub-thumbnailer-provider.h"
#include "gepub-thumbnailer.h"



static void   gepub_thumbnailer_provider_thumbnailer_provider_init (gpointer                    g_iface,
                                                                    gpointer                    iface_data);
static GList *gepub_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider *provider);



struct _GepubThumbnailerProviderClass
{
  GObjectClass __parent__;
};

struct _GepubThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (GepubThumbnailerProvider,
                                gepub_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                TUMBLER_ADD_INTERFACE (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                       gepub_thumbnailer_provider_thumbnailer_provider_init));



void
gepub_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  gepub_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
gepub_thumbnailer_provider_class_init (GepubThumbnailerProviderClass *klass)
{
}



static void
gepub_thumbnailer_provider_class_finalize (GepubThumbnailerProviderClass *klass)
{
}



static void
gepub_thumbnailer_provider_thumbnailer_provider_init (gpointer g_iface,
                                                      gpointer iface_data)
{
  TumblerThumbnailerProviderIface *iface = g_iface;

  iface->get_thumbnailers = gepub_thumbnailer_provider_get_thumbnailers;
}



static void
gepub_thumbnailer_provider_init (GepubThumbnailerProvider *provider)
{
}



static GList *
gepub_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  GepubThumbnailer   *thumbnailer;
  GList              *thumbnailers = NULL;
  static const gchar *mime_types[] = 
  {
    "application/epub",
    "application/epub+zip",
    NULL
  };
  GStrv               uri_schemes;

  /* determine which URI schemes are supported by GIO */
  uri_schemes = tumbler_util_get_supported_uri_schemes ();

  /* create the pixbuf thumbnailer */
  thumbnailer = g_object_new (TYPE_GEPUB_THUMBNAILER, 
                              "uri-schemes", uri_schemes,
                              "mime-types", mime_types, 
                              NULL);

  /* add the thumbnailer to the list */
  thumbnailers = g_list_append (thumbnailers, thumbnailer);

  /* free URI schemes and MIME types */
  g_strfreev (uri_schemes);

  return thumbnailers;
}
