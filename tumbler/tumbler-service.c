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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <tumbler/tumbler-threshold-scheduler.h>
#include <tumbler/tumbler-scheduler.h>
#include <tumbler/tumbler-service-dbus-bindings.h>
#include <tumbler/tumbler-service.h>



#define TUMBLER_SERVICE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_SERVICE, TumblerServicePrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_REGISTRY,
};



static void tumbler_service_class_init   (TumblerServiceClass *klass);
static void tumbler_service_init         (TumblerService      *service);
static void tumbler_service_constructed  (GObject                *object);
static void tumbler_service_finalize     (GObject                *object);
static void tumbler_service_get_property (GObject                *object,
                                          guint                   prop_id,
                                          GValue                 *value,
                                          GParamSpec             *pspec);
static void tumbler_service_set_property (GObject                *object,
                                          guint                   prop_id,
                                          const GValue           *value,
                                          GParamSpec             *pspec);



struct _TumblerServiceClass
{
  GObjectClass __parent__;
};

struct _TumblerService
{
  GObject __parent__;

  TumblerServicePrivate *priv;
};

struct _TumblerServicePrivate
{
  DBusGConnection  *connection;
  TumblerRegistry  *registry;
  TumblerScheduler *scheduler;
  GMutex           *mutex;
};



static GObjectClass *tumbler_service_parent_class = NULL;



GType
tumbler_service_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerService",
                                            sizeof (TumblerServiceClass),
                                            (GClassInitFunc) tumbler_service_class_init,
                                            sizeof (TumblerService),
                                            (GInstanceInitFunc) tumbler_service_init,
                                            0);
    }

  return type;
}



static void
tumbler_service_class_init (TumblerServiceClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerServicePrivate));

  /* Determine the parent type class */
  tumbler_service_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_service_constructed; 
  gobject_class->finalize = tumbler_service_finalize; 
  gobject_class->get_property = tumbler_service_get_property;
  gobject_class->set_property = tumbler_service_set_property;

  g_object_class_install_property (gobject_class, PROP_CONNECTION,
                                   g_param_spec_pointer ("connection",
                                                         "connection",
                                                         "connection",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_REGISTRY,
                                   g_param_spec_object ("registry",
                                                        "registry",
                                                        "registry",
                                                        TUMBLER_TYPE_REGISTRY,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}



static void
tumbler_service_init (TumblerService *service)
{
  service->priv = TUMBLER_SERVICE_GET_PRIVATE (service);
  service->priv->mutex = g_mutex_new ();
}



static void
tumbler_service_constructed (GObject *object)
{
  TumblerService *service = TUMBLER_SERVICE (object);
  service->priv->scheduler = tumbler_threshold_scheduler_new ();
}



static void
tumbler_service_finalize (GObject *object)
{
  TumblerService *service = TUMBLER_SERVICE (object);

  g_object_unref (service->priv->scheduler);
  g_object_unref (service->priv->registry);

  dbus_g_connection_unref (service->priv->connection);

  g_mutex_free (service->priv->mutex);

  (*G_OBJECT_CLASS (tumbler_service_parent_class)->finalize) (object);
}



static void
tumbler_service_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  TumblerService *service = TUMBLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_pointer (value, service->priv->connection);
      break;
    case PROP_REGISTRY:
      g_value_set_object (value, service->priv->registry);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_service_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  TumblerService *service = TUMBLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->priv->connection = dbus_g_connection_ref (g_value_get_pointer (value));
      break;
    case PROP_REGISTRY:
      value, service->priv->registry = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



TumblerService *
tumbler_service_new (DBusGConnection *connection,
                     TumblerRegistry *registry)
{
  return g_object_new (TUMBLER_TYPE_SERVICE, "connection", connection, 
                       "registry", registry, NULL);
}



gboolean
tumbler_service_start (TumblerService *service,
                       GError        **error)
{
  DBusConnection *connection;
  DBusError       dbus_error;
  gint            result;

  g_return_val_if_fail (TUMBLER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_mutex_lock (service->priv->mutex);

  dbus_error_init (&dbus_error);

  /* get the native D-Bus connection */
  connection = dbus_g_connection_get_connection (service->priv->connection);

  /* request ownership for the generic thumbnailer interface */
  result = dbus_bus_request_name (connection, "org.freedesktop.thumbnailer.Generic",
                                  DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);

  /* check if that failed */
  if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
      /* propagate the D-Bus error */
      if (dbus_error_is_set (&dbus_error))
        {
          if (error != NULL)
            dbus_set_g_error (error, &dbus_error);

          dbus_error_free (&dbus_error);
        }
      else if (error != NULL)
        {
          g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                       _("Another generic thumbnailer is already running"));
        }

      g_mutex_unlock (service->priv->mutex);

      return FALSE;
    }

  /* everything is fine, install the generic thumbnailer D-Bus info */
  dbus_g_object_type_install_info (G_OBJECT_TYPE (service),
                                   &dbus_glib_tumbler_service_object_info);

  /* register the service instance as a handler of this interface */
  dbus_g_connection_register_g_object (service->priv->connection, 
                                       "/org/freedesktop/Thumbnailer",
                                       G_OBJECT (service));
  
  g_mutex_unlock (service->priv->mutex);

  return TRUE;
}
