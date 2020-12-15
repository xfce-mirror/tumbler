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

#ifndef __TUMBLER_UTILS_H__
#define __TUMBLER_UTILS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define g_dbus_async_return_if_fail(expr, invocation)                                   \
  G_STMT_START{                                                                         \
    if (G_UNLIKELY (!(expr)))                                                           \
      {                                                                                 \
        GError *dbus_async_return_if_fail_error = NULL;                                 \
                                                                                        \
        g_set_error (&dbus_async_return_if_fail_error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, \
                     "Assertion \"%s\" failed", #expr);                                 \
        g_dbus_method_invocation_return_gerror (invocation, dbus_async_return_if_fail_error);\
        g_clear_error (&dbus_async_return_if_fail_error);                               \
                                                                                        \
        return;                                                                         \
      }                                                                                 \
  }G_STMT_END

#define g_dbus_async_return_val_if_fail(expr, invocation,val)                           \
  G_STMT_START{                                                                         \
    if (G_UNLIKELY (!(expr)))                                                           \
      {                                                                                 \
        GError *dbus_async_return_if_fail_error = NULL;                                 \
                                                                                        \
        g_set_error (&dbus_async_return_if_fail_error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, \
                     "Assertion \"%s\" failed", #expr);                                 \
        g_dbus_method_invocation_return_gerror (invocation, dbus_async_return_if_fail_error);\
        g_clear_error (&dbus_async_return_if_fail_error);                               \
                                                                                        \
        return (val);                                                                   \
      }                                                                                 \
  }G_STMT_END

#define TUMBLER_MUTEX(mtx)        GMutex mtx
#define tumbler_mutex_free(mtx)   g_mutex_clear (&(mtx))
#define tumbler_mutex_lock(mtx)   g_mutex_lock (&(mtx))
#define tumbler_mutex_unlock(mtx) g_mutex_unlock (&(mtx))
#define tumbler_mutex_create(mtx) g_mutex_init (&(mtx))

G_END_DECLS

#endif /* !__TUMBLER_UTILS_H__ */
