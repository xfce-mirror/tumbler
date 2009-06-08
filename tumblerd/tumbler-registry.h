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

#ifndef __TUMBLER_REGISTRY_H__
#define __TUMBLER_REGISTRY_H__

#include <tumbler/tumbler.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_REGISTRY            (tumbler_registry_get_type ())
#define TUMBLER_REGISTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_REGISTRY, TumblerRegistry))
#define TUMBLER_REGISTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_REGISTRY, TumblerRegistryClass))
#define TUMBLER_IS_REGISTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_REGISTRY))
#define TUMBLER_IS_REGISTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_REGISTRY)
#define TUMBLER_REGISTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_REGISTRY, TumblerRegistryClass))

typedef struct _TumblerRegistryPrivate TumblerRegistryPrivate;
typedef struct _TumblerRegistryClass   TumblerRegistryClass;
typedef struct _TumblerRegistry        TumblerRegistry;

GType                tumbler_registry_get_type              (void) G_GNUC_CONST;

TumblerRegistry     *tumbler_registry_new                   (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean             tumbler_registry_load                  (TumblerRegistry    *registry,
                                                             GError            **error);
void                 tumbler_registry_add                   (TumblerRegistry    *registry,
                                                             TumblerThumbnailer *thumbnailer);
GList               *tumbler_registry_get_thumbnailers      (TumblerRegistry    *registry) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailer **tumbler_registry_get_thumbnailer_array (TumblerRegistry    *registry,
                                                             const GStrv         uris,
                                                             const GStrv         mime_hints,
                                                             gint               *length) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__TUMBLER_REGISTRY_H__ */
