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

#ifndef __FOLDER_THUMBNAILER_PROVIDER_H__
#define __FOLDER_THUMBNAILER_PROVIDER_H__

#include <glib-object.h>

G_BEGIN_DECLS;

#define TYPE_FOLDER_THUMBNAILER_PROVIDER            (folder_thumbnailer_provider_get_type ())
#define FOLDER_THUMBNAILER_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_FOLDER_THUMBNAILER_PROVIDER, FolderThumbnailerProvider))
#define FOLDER_THUMBNAILER_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_FOLDER_THUMBNAILER_PROVIDER, FolderThumbnailerProviderClass))
#define IS_FOLDER_THUMBNAILER_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_FOLDER_THUMBNAILER_PROVIDER))
#define IS_FOLDER_THUMBNAILER_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_FOLDER_THUMBNAILER_PROVIDER)
#define FOLDER_THUMBNAILER_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_FOLDER_THUMBNAILER_PROVIDER, FolderThumbnailerProviderClass))

typedef struct _FolderThumbnailerProviderClass FolderThumbnailerProviderClass;
typedef struct _FolderThumbnailerProvider      FolderThumbnailerProvider;

GType folder_thumbnailer_provider_get_type (void) G_GNUC_CONST;
void  folder_thumbnailer_provider_register (TumblerProviderPlugin *plugin);

G_END_DECLS;

#endif /* !__FOLDER_THUMBNAILER_PROVIDER_H__ */
