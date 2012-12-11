/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2012 Nick Schermer <nick@xfce.org>
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

#ifndef __COVER_THUMBNAILER_H__
#define __COVER_THUMBNAILER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TYPE_COVER_THUMBNAILER            (cover_thumbnailer_get_type ())
#define COVER_THUMBNAILER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_COVER_THUMBNAILER, CoverThumbnailer))
#define COVER_THUMBNAILER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_COVER_THUMBNAILER, CoverThumbnailerClass))
#define IS_COVER_THUMBNAILER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_COVER_THUMBNAILER))
#define IS_COVER_THUMBNAILER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_COVER_THUMBNAILER)
#define COVER_THUMBNAILER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_COVER_THUMBNAILER, CoverThumbnailerClass))

typedef struct _CoverThumbnailerClass   CoverThumbnailerClass;
typedef struct _CoverThumbnailer        CoverThumbnailer;

GType cover_thumbnailer_get_type (void) G_GNUC_CONST;
void  cover_thumbnailer_register (TumblerProviderPlugin *plugin);

G_END_DECLS

#endif /* !__COVER_THUMBNAILER_H__ */
