/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2011 Nick Schermer <nick@xfce.org>
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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <odf-thumbnailer/odf-thumbnailer-provider.h>
#include <odf-thumbnailer/odf-thumbnailer.h>



static void   odf_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *odf_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider      *provider);



struct _OdfThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (OdfThumbnailerProvider,
                                odf_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                               odf_thumbnailer_provider_thumbnailer_provider_init));



void
odf_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  odf_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
odf_thumbnailer_provider_class_init (OdfThumbnailerProviderClass *klass)
{
}



static void
odf_thumbnailer_provider_class_finalize (OdfThumbnailerProviderClass *klass)
{
}



static void
odf_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface)
{
  iface->get_thumbnailers = odf_thumbnailer_provider_get_thumbnailers;
}



static void
odf_thumbnailer_provider_init (OdfThumbnailerProvider *provider)
{
}



static GList *
odf_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  static const gchar *mime_types[] =
  {
    "application/vnd.ms-powerpoint",
    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
    "application/vnd.ms-excel",
    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    "application/msword",
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    "application/vnd.oasis.opendocument.presentation-template",
    "application/vnd.oasis.opendocument.presentation",
    "application/vnd.oasis.opendocument.spreadsheet-template",
    "application/vnd.oasis.opendocument.spreadsheet",
    "application/vnd.oasis.opendocument.text-template",
    "application/vnd.oasis.opendocument.text-master",
    "application/vnd.oasis.opendocument.text",
    "application/vnd.oasis.opendocument.graphics-template",
    "application/vnd.oasis.opendocument.graphics",
    "application/vnd.oasis.opendocument.chart",
    "application/vnd.oasis.opendocument.image",
    "application/vnd.oasis.opendocument.formula",
    "application/vnd.sun.xml.impress.template",
    "application/vnd.sun.xml.impress.template",
    "application/vnd.sun.xml.impress",
    "application/vnd.sun.xml.calc.template",
    "application/vnd.sun.xml.calc",
    "application/vnd.sun.xml.writer.global",
    "application/vnd.sun.xml.writer.template",
    "application/vnd.sun.xml.writer",
    "application/vnd.sun.xml.draw.template",
    "application/vnd.sun.xml.draw",
    "application/vnd.sun.xml.math",
    "image/openraster",
    NULL
  };
  OdfThumbnailer    *thumbnailer;
  GList             *thumbnailers = NULL;
  GStrv              uri_schemes;

  /* determine the URI schemes supported by GIO */
  uri_schemes = tumbler_util_get_supported_uri_schemes ();

  /* create the pixbuf thumbnailer */
  thumbnailer = g_object_new (ODF_TYPE_THUMBNAILER,
                              "uri-schemes", uri_schemes,
                              "mime-types", mime_types,
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
