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

#ifndef __GST_THUMBNAILER_H__
#define __GST_THUMBNAILER_H__

#include <tumbler/tumbler.h>

G_BEGIN_DECLS

#define GST_TYPE_THUMBNAILER (gst_thumbnailer_get_type ())
G_DECLARE_FINAL_TYPE (GstThumbnailer, gst_thumbnailer, GST, THUMBNAILER, TumblerAbstractThumbnailer)

void gst_thumbnailer_register (TumblerProviderPlugin *plugin);

G_END_DECLS

#endif /* !__GST_THUMBNAILER_H__ */
