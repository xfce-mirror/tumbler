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

#ifndef __TUMBLER_THUMBNAILER_H__
#define __TUMBLER_THUMBNAILER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_THUMBNAILER           (tumbler_thumbnailer_get_type ())
#define TUMBLER_THUMBNAILER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_THUMBNAILER, TumblerThumbnailer))
#define TUMBLER_IS_THUMBNAILER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_THUMBNAILER))
#define TUMBLER_THUMBNAILER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TUMBLER_TYPE_THUMBNAILER, TumblerThumbnailerIface))

typedef struct _TumblerThumbnailer      TumblerThumbnailer;
typedef struct _TumblerThumbnailerIface TumblerThumbnailerIface;

struct _TumblerThumbnailerIface
{
  GTypeInterface __parent__;

  /* signals */
  void (*ready)      (TumblerThumbnailer *thumbnailer,
                      const gchar        *uri);
  void (*error)      (TumblerThumbnailer *thumbnailer,
                      const gchar        *failed_uri,
                      gint                error_code,
                      const gchar        *message);
  void (*unregister) (TumblerThumbnailer *thumbnailer);

  /* virtual methods */
  void (*create) (TumblerThumbnailer *thumbnailer,
                  const gchar        *uri,
                  const gchar        *mime_hint);
};

GType                tumbler_thumbnailer_get_type        (void) G_GNUC_CONST;

void                 tumbler_thumbnailer_create          (TumblerThumbnailer  *thumbnailer,
                                                          const gchar         *uri,
                                                          const gchar         *mime_hint);

GStrv                tumbler_thumbnailer_get_hash_keys   (TumblerThumbnailer  *thumbnailer);
GStrv                tumbler_thumbnailer_get_mime_types  (TumblerThumbnailer  *thumbnailer);
GStrv                tumbler_thumbnailer_get_uri_schemes (TumblerThumbnailer  *thumbnailer);

TumblerThumbnailer **tumbler_thumbnailer_array_copy      (TumblerThumbnailer **thumbnailers);
void                 tumbler_thumbnailer_array_free      (TumblerThumbnailer **thumbnailers);

G_END_DECLS

#endif /* !__TUMBLER_THUMBNAILER_H__ */
