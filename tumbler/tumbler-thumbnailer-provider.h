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

#if !defined (_TUMBLER_INSIDE_TUMBLER_H) && !defined (TUMBLER_COMPILATION)
#error "Only <tumbler/tumbler.h> may be included directly. This file might disappear or change contents."
#endif

#ifndef __TUMBLER_THUMBNAILER_PROVIDER_H__
#define __TUMBLER_THUMBNAILER_PROVIDER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_THUMBNAILER_PROVIDER (tumbler_thumbnailer_provider_get_type ())
G_DECLARE_INTERFACE (TumblerThumbnailerProvider, tumbler_thumbnailer_provider, TUMBLER, THUMBNAILER_PROVIDER, GObject)

typedef struct _TumblerThumbnailerProviderInterface TumblerThumbnailerProviderIface;

struct _TumblerThumbnailerProviderInterface
{
  GTypeInterface __parent__;

  /* signals */

  /* virtual methods */
  GList *(*get_thumbnailers) (TumblerThumbnailerProvider *provider);
};

GList *tumbler_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__TUMBLER_THUMBNAILER_PROVIDER_H__ */
