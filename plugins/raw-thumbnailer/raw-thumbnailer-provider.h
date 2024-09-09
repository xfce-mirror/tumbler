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

#ifndef __RAW_THUMBNAILER_PROVIDER_H__
#define __RAW_THUMBNAILER_PROVIDER_H__

#include "tumbler/tumbler.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define RAW_TYPE_THUMBNAILER_PROVIDER (raw_thumbnailer_provider_get_type ())
G_DECLARE_FINAL_TYPE (RawThumbnailerProvider, raw_thumbnailer_provider, RAW, THUMBNAILER_PROVIDER, GObject)

void
raw_thumbnailer_provider_register (TumblerProviderPlugin *plugin);

G_END_DECLS

#endif /* !__RAW_THUMBNAILER_PROVIDER_H__ */
