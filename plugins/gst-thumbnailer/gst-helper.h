/* vi:set et ai sw=2 sts=2 ts=2: */
/*
 * Originally from Bickley - a meta data management framework.
 * Copyright © 2008, 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA
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
