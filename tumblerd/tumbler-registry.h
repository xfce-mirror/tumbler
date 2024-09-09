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

#include "tumbler/tumbler.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_REGISTRY (tumbler_registry_get_type ())
G_DECLARE_FINAL_TYPE (TumblerRegistry, tumbler_registry, TUMBLER, REGISTRY, GObject)

TumblerRegistry *
tumbler_registry_new (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean
tumbler_registry_load (TumblerRegistry *registry,
                       GError **error);
void
tumbler_registry_add (TumblerRegistry *registry,
                      TumblerThumbnailer *thumbnailer);
void
tumbler_registry_remove (TumblerRegistry *registry,
                         TumblerThumbnailer *thumbnailer);
GList *
tumbler_registry_get_thumbnailers (TumblerRegistry *registry) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
GList **
tumbler_registry_get_thumbnailer_array (TumblerRegistry *registry,
                                        TumblerFileInfo **infos,
                                        guint length) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void
tumbler_registry_update_supported (TumblerRegistry *registry);
void
tumbler_registry_get_supported (TumblerRegistry *registry,
                                const gchar *const **uri_schemes,
                                const gchar *const **mime_types);
TumblerThumbnailer *
tumbler_registry_get_preferred (TumblerRegistry *registry,
                                const gchar *hash_key) G_GNUC_WARN_UNUSED_RESULT;
void
tumbler_registry_set_preferred (TumblerRegistry *registry,
                                const gchar *hash_key,
                                TumblerThumbnailer *thumbnailer);

G_END_DECLS

#endif /* !__TUMBLER_REGISTRY_H__ */
