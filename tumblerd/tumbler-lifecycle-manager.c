/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2011 Jannis Pohlmann <jannis@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-lifecycle-manager.h>
#include <tumblerd/tumbler-utils.h>



#define SHUTDOWN_TIMEOUT_SECONDS 300



/* signal identifiers */
enum
{
  SIGNAL_SHUTDOWN,
  LAST_SIGNAL,
};



static void tumbler_lifecycle_manager_finalize (GObject *object);



struct _TumblerLifecycleManager
{
  GObject __parent__;

  TUMBLER_MUTEX (lock);

  guint   timeout_id;
  guint   component_use_count;
  guint   shutdown_emitted : 1;
};



static guint lifecycle_manager_signals[LAST_SIGNAL];



G_DEFINE_TYPE (TumblerLifecycleManager, tumbler_lifecycle_manager, G_TYPE_OBJECT)



static void
tumbler_lifecycle_manager_class_init (TumblerLifecycleManagerClass *klass)
{
  GObjectClass *gobject_class;

  /* Determine the parent type class */
  tumbler_lifecycle_manager_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_lifecycle_manager_finalize; 

  lifecycle_manager_signals[SIGNAL_SHUTDOWN] =
    g_signal_new ("shutdown",
                  TUMBLER_TYPE_LIFECYCLE_MANAGER,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}



static void
tumbler_lifecycle_manager_init (TumblerLifecycleManager *manager)
{
  tumbler_mutex_create (manager->lock);
  manager->timeout_id = 0;
  manager->component_use_count = 0;
  manager->shutdown_emitted = FALSE;
}



static void
tumbler_lifecycle_manager_finalize (GObject *object)
{
  TumblerLifecycleManager *manager = TUMBLER_LIFECYCLE_MANAGER (object);

  tumbler_mutex_free (manager->lock);

  (*G_OBJECT_CLASS (tumbler_lifecycle_manager_parent_class)->finalize) (object);
}



static gboolean
tumbler_lifecycle_manager_timeout (gpointer user_data)
{
  TumblerLifecycleManager *manager = user_data;

  tumbler_mutex_lock (manager->lock);

  /* reschedule the timeout if one of the components is still in use */
  if (manager->component_use_count > 0)
    {
      tumbler_mutex_unlock (manager->lock);
      return TRUE;
    }

  /* reset the timeout id */
  manager->timeout_id = 0;

  /* emit the shutdown signal */
  g_signal_emit (manager, lifecycle_manager_signals[SIGNAL_SHUTDOWN], 0);

  /* set the shutdown emitted flag to force other threads not to 
   * reschedule the timeout */
  manager->shutdown_emitted = TRUE;

  tumbler_mutex_unlock (manager->lock);

  return FALSE;
}



TumblerLifecycleManager *
tumbler_lifecycle_manager_new (void)
{
  return g_object_new (TUMBLER_TYPE_LIFECYCLE_MANAGER, NULL);
}



void
tumbler_lifecycle_manager_start (TumblerLifecycleManager *manager)
{
  g_return_if_fail (TUMBLER_IS_LIFECYCLE_MANAGER (manager));

  tumbler_mutex_lock (manager->lock);

  /* ignore if there already is a timeout scheduled */
  if (manager->timeout_id > 0)
    {
      tumbler_mutex_unlock (manager->lock);
      return;
    }

  manager->timeout_id = 
    g_timeout_add_seconds (SHUTDOWN_TIMEOUT_SECONDS, 
                           tumbler_lifecycle_manager_timeout, 
                           manager);

  tumbler_mutex_unlock (manager->lock);
}



gboolean
tumbler_lifecycle_manager_keep_alive (TumblerLifecycleManager *manager,
                                      GError                 **error)
{
  g_return_val_if_fail (TUMBLER_IS_LIFECYCLE_MANAGER (manager), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  tumbler_mutex_lock (manager->lock);

  /* if the shutdown signal has been emitted, there's nothing 
   * we can do to prevent a shutdown anymore */
  if (manager->shutdown_emitted)
    {
      tumbler_mutex_unlock (manager->lock);

      if (error != NULL)
        {
          g_set_error (error, TUMBLER_ERROR, TUMBLER_ERROR_SHUTTING_DOWN,
                       "%s", TUMBLER_ERROR_MESSAGE_SHUT_DOWN);
        }
      return FALSE;
    }

  /* if there is an existing timeout, drop it (we are going to 
   * replace it with a new one) */
  if (manager->timeout_id > 0)
    g_source_remove (manager->timeout_id);

  /* reschedule the shutdown timeout */
  manager->timeout_id = 
    g_timeout_add_seconds (SHUTDOWN_TIMEOUT_SECONDS, 
                           tumbler_lifecycle_manager_timeout, 
                           manager);

  tumbler_mutex_unlock (manager->lock);

  return TRUE;
}



void
tumbler_lifecycle_manager_increment_use_count (TumblerLifecycleManager *manager)
{
  g_return_if_fail (TUMBLER_IS_LIFECYCLE_MANAGER (manager));

  tumbler_mutex_lock (manager->lock);

  manager->component_use_count += 1;
  
  tumbler_mutex_unlock (manager->lock);
}



void
tumbler_lifecycle_manager_decrement_use_count (TumblerLifecycleManager *manager)
{
  g_return_if_fail (TUMBLER_IS_LIFECYCLE_MANAGER (manager));

  tumbler_mutex_lock (manager->lock);
  
  /* decrement the use count, make sure not to drop below zero */
  if (manager->component_use_count > 0)
    manager->component_use_count -= 1;
  
  tumbler_mutex_unlock (manager->lock);
}
