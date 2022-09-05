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

#ifndef __TUMBLER_ABSTRACT_THUMBNAILER_H__
#define __TUMBLER_ABSTRACT_THUMBNAILER_H__

#include <glib-object.h>
#include <gio/gio.h>

#include <tumbler/tumbler-file-info.h>

G_BEGIN_DECLS;

#define TUMBLER_TYPE_ABSTRACT_THUMBNAILER            (tumbler_abstract_thumbnailer_get_type ())
#define TUMBLER_ABSTRACT_THUMBNAILER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_ABSTRACT_THUMBNAILER, TumblerAbstractThumbnailer))
#define TUMBLER_ABSTRACT_THUMBNAILER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_ABSTRACT_THUMBNAILER, TumblerAbstractThumbnailerClass))
#define TUMBLER_IS_ABSTRACT_THUMBNAILER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_ABSTRACT_THUMBNAILER))
#define TUMBLER_IS_ABSTRACT_THUMBNAILER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_ABSTRACT_THUMBNAILER)
#define TUMBLER_ABSTRACT_THUMBNAILER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_ABSTRACT_THUMBNAILER, TumblerAbstractThumbnailerClass))

typedef struct _TumblerAbstractThumbnailerPrivate TumblerAbstractThumbnailerPrivate;
typedef struct _TumblerAbstractThumbnailerClass   TumblerAbstractThumbnailerClass;
typedef struct _TumblerAbstractThumbnailer        TumblerAbstractThumbnailer;

struct _TumblerAbstractThumbnailerClass
{
  GObjectClass __parent__;

  /* virtual methods */
  void (*create) (TumblerAbstractThumbnailer *thumbnailer,
                  GCancellable               *cancellable,
                  TumblerFileInfo            *info);
};

struct _TumblerAbstractThumbnailer
{
  GObject __parent__;

  TumblerAbstractThumbnailerPrivate *priv;
};

GType tumbler_abstract_thumbnailer_get_type (void) G_GNUC_CONST;

G_END_DECLS;

#endif /* !__TUMBLER_ABSTRACT_THUMBNAILER_H__ */
