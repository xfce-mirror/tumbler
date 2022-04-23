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

#if !defined (TUMBLER_INSIDE_TUMBLER_H) && !defined (TUMBLER_COMPILATION)
#error "Only <tumbler/tumbler.h> may be included directly. This file might disappear or change contents."
#endif

#ifndef __TUMBLER_THUMBNAIL_FLAVOR_H__
#define __TUMBLER_THUMBNAIL_FLAVOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_THUMBNAIL_FLAVOR            (tumbler_thumbnail_flavor_get_type ())
#define TUMBLER_THUMBNAIL_FLAVOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_THUMBNAIL_FLAVOR, TumblerThumbnailFlavor))
#define TUMBLER_THUMBNAIL_FLAVOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_THUMBNAIL_FLAVOR, TumblerThumbnailFlavorClass))
#define TUMBLER_IS_THUMBNAIL_FLAVOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_THUMBNAIL_FLAVOR))
#define TUMBLER_IS_THUMBNAIL_FLAVOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_THUMBNAIL_FLAVOR)
#define TUMBLER_THUMBNAIL_FLAVOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_THUMBNAIL_FLAVOR, TumblerThumbnailFlavorClass))

typedef struct _TumblerThumbnailFlavorPrivate TumblerThumbnailFlavorPrivate;
typedef struct _TumblerThumbnailFlavorClass   TumblerThumbnailFlavorClass;
typedef struct _TumblerThumbnailFlavor        TumblerThumbnailFlavor;

GType                   tumbler_thumbnail_flavor_get_type     (void) G_GNUC_CONST;

TumblerThumbnailFlavor *tumbler_thumbnail_flavor_new          (const gchar            *name,
                                                               gint                    width,
                                                               gint                    height) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailFlavor *tumbler_thumbnail_flavor_new_normal   (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailFlavor *tumbler_thumbnail_flavor_new_large    (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailFlavor *tumbler_thumbnail_flavor_new_x_large  (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailFlavor *tumbler_thumbnail_flavor_new_xx_large (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
const gchar            *tumbler_thumbnail_flavor_get_name     (TumblerThumbnailFlavor *flavor);
void                    tumbler_thumbnail_flavor_get_size     (TumblerThumbnailFlavor *flavor,
                                                               gint                   *width,
                                                               gint                   *height);

G_END_DECLS

#endif /* !__TUMBLER_THUMBNAIL_FLAVOR_H__ */
