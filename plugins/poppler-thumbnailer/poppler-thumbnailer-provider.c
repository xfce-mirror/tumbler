/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2010 Jannis Pohlmann <jannis@xfce.org>
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

#include <poppler-thumbnailer/poppler-thumbnailer-provider.h>
#include <poppler-thumbnailer/poppler-thumbnailer.h>



static void   poppler_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *poppler_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider      *provider);



struct _PopplerThumbnailerProviderClass
{
  GObjectClass __parent__;
};

struct _PopplerThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (PopplerThumbnailerProvider,
                                poppler_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                               poppler_thumbnailer_provider_thumbnailer_provider_init));



void
poppler_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  poppler_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
poppler_thumbnailer_provider_class_init (PopplerThumbnailerProviderClass *klass)
{
}



static void
poppler_thumbnailer_provider_class_finalize (PopplerThumbnailerProviderClass *klass)
{
}



static void
poppler_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface)
{
  iface->get_thumbnailers = poppler_thumbnailer_provider_get_thumbnailers;
}



static void
poppler_thumbnailer_provider_init (PopplerThumbnailerProvider *provider)
{
}



static GList *
poppler_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  PopplerThumbnailer *thumbnailer;
  GList              *thumbnailers = NULL;
  static const gchar *mime_types[] = 
  {
    "application/pdf",
    "application/postscript",
    NULL
  };
  GStrv               uri_schemes;

  /* determine which URI schemes are supported by GIO */
  uri_schemes = tumbler_util_get_supported_uri_schemes ();

  /* create the pixbuf thumbnailer */
  thumbnailer = g_object_new (TYPE_POPPLER_THUMBNAILER, 
                              "uri-schemes", uri_schemes, "mime-types", mime_types, 
                              NULL);

  /* add the thumbnailer to the list */
  thumbnailers = g_list_append (thumbnailers, thumbnailer);

  tumbler_util_dump_strv (G_LOG_DOMAIN, "Supported URI schemes",
                          (const gchar *const *) uri_schemes);
  tumbler_util_dump_strv (G_LOG_DOMAIN, "Supported mime types", mime_types);

  /* free URI schemes and MIME types */
  g_strfreev (uri_schemes);

  return thumbnailers;
}
