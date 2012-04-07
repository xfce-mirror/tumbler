/* vi:set et ai sw=2 sts=2 ts=2: */
/*
 * Originally from Bickley - a meta data management framework.
 * Copyright © 2008, 2011 Intel Corporation.
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

#ifndef __GST_HELPER_H__
#define __GST_HELPER_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gst/gst.h>
#include <tumbler/tumbler.h>

G_BEGIN_DECLS

GdkPixbuf *gst_helper_convert_buffer_to_pixbuf (GstBuffer              *buffer,
                                                GCancellable           *cancellable,
                                                TumblerThumbnailFlavor *flavour);

G_END_DECLS

#endif
