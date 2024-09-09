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
#include "config.h"
#endif

#include "tumbler-thumbnailer-provider.h"



G_DEFINE_INTERFACE (TumblerThumbnailerProvider, tumbler_thumbnailer_provider, G_TYPE_OBJECT)



static void
tumbler_thumbnailer_provider_default_init (TumblerThumbnailerProviderIface *iface)
{
}



GList *
tumbler_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER_PROVIDER (provider), NULL);
  g_return_val_if_fail (TUMBLER_THUMBNAILER_PROVIDER_GET_IFACE (provider)->get_thumbnailers != NULL, NULL);

  return (TUMBLER_THUMBNAILER_PROVIDER_GET_IFACE (provider)->get_thumbnailers) (provider);
}
