/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2010 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2019-2020, Olivier Duchateau <duchateau.olivier@gmail.com>
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

#ifndef __RSVG_THUMBNAILER_H__
#define __RSVG_THUMBNAILER_H__

#include <glib-object.h>

G_BEGIN_DECLS;

#define TYPE_RSVG_THUMBNAILER            (rsvg_thumbnailer_get_type ())
#define RSVG_THUMBNAILER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_RSVG_THUMBNAILER, RsvgThumbnailer))
#define RSVG_THUMBNAILER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_RSVG_THUMBNAILER, RsvgThumbnailerClass))
#define IS_RSVG_THUMBNAILER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_RSVG_THUMBNAILER))
#define IS_RSVG_THUMBNAILER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_RSVG_THUMBNAILER))
#define RSVG_THUMBNAILER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_RSVG_THUMBNAILER, RsvgThumbnailerClass))

typedef struct _RsvgThumbnailerClass   RsvgThumbnailerClass;
typedef struct _RsvgThumbnailer        RsvgThumbnailer;

GType rsvg_thumbnailer_get_type (void) G_GNUC_CONST;
void  rsvg_thumbnailer_register (TumblerProviderPlugin *plugin);

G_END_DECLS;

#endif /* !__RSVG_THUMBNAILER_H__ */
