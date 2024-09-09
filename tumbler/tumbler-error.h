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

#if !defined(_TUMBLER_INSIDE_TUMBLER_H) && !defined(TUMBLER_COMPILATION)
#error "Only <tumbler/tumbler.h> may be included directly. This file might disappear or change contents."
#endif

#ifndef __TUMBLER_ERROR_H__
#define __TUMBLER_ERROR_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SECTION:tumbler-error
 * @title:Error Domain and Types
 * @include: tumbler/tumbler.h
 */

#define TUMBLER_ERROR_DOMAIN "Tumbler"
#define TUMBLER_ERROR (g_quark_from_static_string (TUMBLER_ERROR_DOMAIN))

#define TUMBLER_ERROR_MESSAGE_CREATION_FAILED _("Thumbnail could not be inferred from file contents")
#define TUMBLER_ERROR_MESSAGE_LOCAL_ONLY _("Only local files are supported")
#define TUMBLER_ERROR_MESSAGE_CORRUPT_THUMBNAIL _("Corrupt thumbnail PNG: '%s'")
#define TUMBLER_ERROR_MESSAGE_SAVE_FAILED _("Could not save thumbnail to \"%s\"")
#define TUMBLER_ERROR_MESSAGE_NO_THUMB_OF_THUMB _("The file \"%s\" is a thumbnail itself")
#define TUMBLER_ERROR_MESSAGE_NO_THUMBNAILER _("No thumbnailer available for \"%s\"")
#define TUMBLER_ERROR_MESSAGE_SHUT_DOWN _("The thumbnailer service is shutting down")
#define TUMBLER_ERROR_MESSAGE_UNSUPPORTED_FLAVOR _("Unsupported thumbnail flavor requested")

#define TUMBLER_WARNING_VERSION_MISMATCH "Version mismatch: %s"
#define TUMBLER_WARNING_MALFORMED_FILE "Malformed file \"%s\": %s"
#define TUMBLER_WARNING_LOAD_FILE_FAILED "Failed to load the file \"%s\": %s"
#define TUMBLER_WARNING_LOAD_PLUGIN_FAILED "Failed to load plugin \"%s\": %s"
#define TUMBLER_WARNING_PLUGIN_LACKS_SYMBOLS "Plugin \"%s\" lacks required symbols"

typedef enum /*< enum >*/
{
  TUMBLER_ERROR_UNSUPPORTED,
  TUMBLER_ERROR_CONNECTION_ERROR,
  TUMBLER_ERROR_INVALID_FORMAT,
  TUMBLER_ERROR_IS_THUMBNAIL,
  TUMBLER_ERROR_SAVE_FAILED,
  TUMBLER_ERROR_UNSUPPORTED_FLAVOR,
  TUMBLER_ERROR_NO_CONTENT,
  TUMBLER_ERROR_SHUTTING_DOWN,
  TUMBLER_ERROR_OTHER_ERROR_DOMAIN,
} TumblerErrorEnum;

G_END_DECLS

#endif /* !__TUMBLER_ERROR_H__ */
