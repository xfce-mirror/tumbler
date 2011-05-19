/* vi:set et ai sw=2 sts=2 ts=2: */
/*
 * Copyright (c) 2011 Intel Corporation
 *
 * Author: Ross Burton <ross@linux.intel.com>
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

#ifndef __GST_THUMBNAILER_PROVIDER_H__
#define __GST_THUMBNAILER_PROVIDER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TYPE_GST_THUMBNAILER_PROVIDER            (gst_thumbnailer_provider_get_type ())
#define GST_THUMBNAILER_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GST_THUMBNAILER_PROVIDER, GstThumbnailerProvider))
#define GST_THUMBNAILER_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GST_THUMBNAILER_PROVIDER, GstThumbnailerProviderClass))
#define IS_GST_THUMBNAILER_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GST_THUMBNAILER_PROVIDER))
#define IS_GST_THUMBNAILER_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GST_THUMBNAILER_PROVIDER)
#define GST_THUMBNAILER_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GST_THUMBNAILER_PROVIDER, GstThumbnailerProviderClass))

typedef struct _GstThumbnailerProviderClass GstThumbnailerProviderClass;
typedef struct _GstThumbnailerProvider      GstThumbnailerProvider;

GType gst_thumbnailer_provider_get_type (void) G_GNUC_CONST;
void  gst_thumbnailer_provider_register (TumblerProviderPlugin *plugin);

G_END_DECLS

#endif /* !__GST_THUMBNAILER_PROVIDER_H__ */
