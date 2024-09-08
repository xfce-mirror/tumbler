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

#ifndef __TUMBLER_THUMBNAIL_H__
#define __TUMBLER_THUMBNAIL_H__

#include <gio/gio.h>
#include <glib-object.h>
#include <tumbler/tumbler-enum-types.h>
#include <tumbler/tumbler-thumbnail-flavor.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_THUMBNAIL (tumbler_thumbnail_get_type ())
G_DECLARE_INTERFACE (TumblerThumbnail, tumbler_thumbnail, TUMBLER, THUMBNAIL, GObject)

typedef struct _TumblerImageData TumblerImageData;
typedef struct _TumblerThumbnailInterface TumblerThumbnailIface;

struct _TumblerImageData
{
  TumblerColorspace colorspace;
  const guchar     *data;
  gboolean          has_alpha;
  gint              bits_per_sample;
  gint              width;
  gint              height;
  gint              rowstride;
};

struct _TumblerThumbnailInterface
{
  GTypeInterface __parent__;

  /* signals */

  /* virtual methods */
  gboolean (*load)            (TumblerThumbnail *thumbnail,
                               GCancellable     *cancellable,
                               GError          **error);
  gboolean (*needs_update)    (TumblerThumbnail *thumbnail,
                               const gchar      *uri,
                               gdouble           mtime);
  gboolean (*save_image_data) (TumblerThumbnail *thumbnail,
                               TumblerImageData *data,
                               gdouble           mtime,
                               GCancellable     *cancellable,
                               GError          **error);
  gboolean (*save_file)       (TumblerThumbnail *thumbnail,
                               GFile            *file,
                               gdouble           mtime,
                               GCancellable     *cancellable,
                               GError          **error);
};

gboolean                tumbler_thumbnail_load            (TumblerThumbnail      *thumbnail,
                                                           GCancellable          *cancellable,
                                                           GError               **error);
gboolean                tumbler_thumbnail_needs_update    (TumblerThumbnail      *thumbnail,
                                                           const gchar           *uri,
                                                           gdouble                mtime);
gboolean                tumbler_thumbnail_save_image_data (TumblerThumbnail      *thumbnail,
                                                           TumblerImageData      *data,
                                                           gdouble                mtime,
                                                           GCancellable          *cancellable,
                                                           GError               **error);
gboolean                tumbler_thumbnail_save_file       (TumblerThumbnail      *thumbnail,
                                                           GFile                 *file,
                                                           gdouble                mtime,
                                                           GCancellable          *cancellable,
                                                           GError               **error);
TumblerThumbnailFlavor *tumbler_thumbnail_get_flavor      (TumblerThumbnail      *thumbnail);

G_END_DECLS

#endif /* !__TUMBLER_THUMBNAIL_H__ */
