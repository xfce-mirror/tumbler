/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
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

#ifndef __TUMBLER_THUMBNAILER_H__
#define __TUMBLER_THUMBNAILER_H__

#include <glib-object.h>
#include <gio/gio.h>

#include <tumbler/tumbler-file-info.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_THUMBNAILER           (tumbler_thumbnailer_get_type ())
#define TUMBLER_THUMBNAILER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_THUMBNAILER, TumblerThumbnailer))
#define TUMBLER_IS_THUMBNAILER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_THUMBNAILER))
#define TUMBLER_THUMBNAILER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TUMBLER_TYPE_THUMBNAILER, TumblerThumbnailerIface))

typedef struct _TumblerThumbnailer      TumblerThumbnailer;
typedef struct _TumblerThumbnailerIface TumblerThumbnailerInterface;
typedef TumblerThumbnailerInterface     TumblerThumbnailerIface;

struct _TumblerThumbnailerIface
{
  GTypeInterface __parent__;

  /* signals */
  void (*ready)      (TumblerThumbnailer *thumbnailer,
                      const gchar        *uri);
  void (*error)      (TumblerThumbnailer *thumbnailer,
                      const gchar        *failed_uri,
                      GQuark              error_domain,
                      gint                error_code,
                      const gchar        *message);
  void (*unregister) (TumblerThumbnailer *thumbnailer);

  /* virtual methods */
  void (*create) (TumblerThumbnailer *thumbnailer,
                  GCancellable       *cancellable,
                  TumblerFileInfo    *info);
};

GType                tumbler_thumbnailer_get_type          (void) G_GNUC_CONST;

void                 tumbler_thumbnailer_create            (TumblerThumbnailer     *thumbnailer,
                                                            GCancellable           *cancellable,
                                                            TumblerFileInfo        *info);

gchar              **tumbler_thumbnailer_get_hash_keys     (TumblerThumbnailer  *thumbnailer);
gchar              **tumbler_thumbnailer_get_mime_types    (TumblerThumbnailer  *thumbnailer);
gchar              **tumbler_thumbnailer_get_uri_schemes   (TumblerThumbnailer  *thumbnailer);
gint                 tumbler_thumbnailer_get_priority      (TumblerThumbnailer  *thumbnailer);
gint64               tumbler_thumbnailer_get_max_file_size (TumblerThumbnailer  *thumbnailer);

gboolean             tumbler_thumbnailer_supports_location (TumblerThumbnailer  *thumbnailer,
                                                            GFile               *file);
gboolean             tumbler_thumbnailer_supports_hash_key (TumblerThumbnailer  *thumbnailer,
                                                            const gchar         *hash_key);

GList              **tumbler_thumbnailer_array_copy        (GList              **thumbnailers,
                                                            guint                length);
void                 tumbler_thumbnailer_array_free        (GList              **thumbnailers,
                                                            guint                length);

G_END_DECLS

#endif /* !__TUMBLER_THUMBNAILER_H__ */
