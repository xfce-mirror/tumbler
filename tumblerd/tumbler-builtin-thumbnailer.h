/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __TUMBLER_BUILTIN_THUMBNAILER_H__
#define __TUMBLER_BUILTIN_THUMBNAILER_H__

#include <tumblerd/tumbler-thumbnailer.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_BUILTIN_THUMBNAILER            (tumbler_builtin_thumbnailer_get_type ())
#define TUMBLER_BUILTIN_THUMBNAILER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_BUILTIN_THUMBNAILER, TumblerBuiltinThumbnailer))
#define TUMBLER_BUILTIN_THUMBNAILER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_BUILTIN_THUMBNAILER, TumblerBuiltinThumbnailerClass))
#define TUMBLER_IS_BUILTIN_THUMBNAILER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_BUILTIN_THUMBNAILER))
#define TUMBLER_IS_BUILTIN_THUMBNAILER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_BUILTIN_THUMBNAILER)
#define TUMBLER_BUILTIN_THUMBNAILER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_BUILTIN_THUMBNAILER, TumblerBuiltinThumbnailerClass))

typedef struct _TumblerBuiltinThumbnailerPrivate TumblerBuiltinThumbnailerPrivate;
typedef struct _TumblerBuiltinThumbnailerClass   TumblerBuiltinThumbnailerClass;
typedef struct _TumblerBuiltinThumbnailer        TumblerBuiltinThumbnailer;

typedef gboolean (*TumblerBuiltinThumbnailerFunc) (TumblerBuiltinThumbnailer *thumbnailer,
                                                   const gchar               *uri,
                                                   const gchar               *mime_hint,
                                                   GError                   **error);

GType               tumbler_builtin_thumbnailer_get_type (void) G_GNUC_CONST;

TumblerThumbnailer *tumbler_builtin_thumbnailer_new      (TumblerBuiltinThumbnailerFunc func,
                                                          const GStrv                   mime_types,
                                                          const GStrv                   uri_schemes) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__TUMBLER_BUILTIN_THUMBNAILER_H__ */
