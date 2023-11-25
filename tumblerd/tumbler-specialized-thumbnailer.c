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
#include <glib/gprintf.h>
#include <glib-object.h>

#include <tumbler/tumbler.h>
#include <tumbler/tumbler-marshal.h>

#include <tumblerd/tumbler-specialized-thumbnailer.h>
#include <tumblerd/tumbler-utils.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_NAME,
  PROP_OBJECT_PATH,
  PROP_CONNECTION,
  PROP_PROXY,
  PROP_FOREIGN,
  PROP_MODIFIED,
};



typedef struct _SpecializedInfo SpecializedInfo; 

 

static void tumbler_specialized_thumbnailer_iface_init      (TumblerThumbnailerIface       *iface);
static void tumbler_specialized_thumbnailer_constructed     (GObject                       *object);
static void tumbler_specialized_thumbnailer_finalize        (GObject                       *object);
static void tumbler_specialized_thumbnailer_get_property    (GObject                       *object,
                                                             guint                          prop_id,
                                                             GValue                        *value,
                                                             GParamSpec                    *pspec);
static void tumbler_specialized_thumbnailer_set_property    (GObject                       *object,
                                                             guint                          prop_id,
                                                             const GValue                  *value,
                                                             GParamSpec                    *pspec);
static void tumbler_specialized_thumbnailer_create          (TumblerThumbnailer            *thumbnailer,
                                                             GCancellable                  *cancellable,
                                                             TumblerFileInfo               *info);

static void tumbler_specialized_thumbnailer_proxy_name_owner_changed (GDBusProxy                    *proxy,
                                                                      GParamSpec                    *spec,
                                                                      TumblerSpecializedThumbnailer *thumbnailer);


struct _TumblerSpecializedThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _TumblerSpecializedThumbnailer
{
  TumblerAbstractThumbnailer __parent__;

  GDBusConnection *connection;
  GDBusProxy      *proxy;

  gboolean         foreign;
  guint64          modified;

  gchar           *name;
  gchar           *object_path;
};

struct _SpecializedInfo
{
  TumblerThumbnailer *thumbnailer;
  GCond               condition;
  TUMBLER_MUTEX       (mutex);
  TumblerFileInfo    *info;
  gboolean            had_callback;
  guint32             handle;
};



G_DEFINE_TYPE_WITH_CODE (TumblerSpecializedThumbnailer, 
                         tumbler_specialized_thumbnailer,
                         TUMBLER_TYPE_ABSTRACT_THUMBNAILER,
                         G_IMPLEMENT_INTERFACE (TUMBLER_TYPE_THUMBNAILER,
                                                tumbler_specialized_thumbnailer_iface_init));



static void
tumbler_specialized_thumbnailer_class_init (TumblerSpecializedThumbnailerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_specialized_thumbnailer_constructed;
  gobject_class->finalize = tumbler_specialized_thumbnailer_finalize; 
  gobject_class->get_property = tumbler_specialized_thumbnailer_get_property;
  gobject_class->set_property = tumbler_specialized_thumbnailer_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "name",
                                                        "name",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        "object-path",
                                                        "object-path",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_PROXY,
                                   g_param_spec_object ("proxy",
                                                        "proxy",
                                                        "proxy",
                                                        G_TYPE_DBUS_PROXY,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
                                   PROP_FOREIGN,
                                   g_param_spec_boolean ("foreign",
                                                         "foreign",
                                                         "foreign",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_MODIFIED,
                                   g_param_spec_uint64 ("modified",
                                                        "modified",
                                                        "modified",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));
}



static void
tumbler_specialized_thumbnailer_iface_init (TumblerThumbnailerIface *iface)
{
  iface->create = tumbler_specialized_thumbnailer_create;
}



static void
tumbler_specialized_thumbnailer_init (TumblerSpecializedThumbnailer *thumbnailer)
{
}



static void
tumbler_specialized_thumbnailer_constructed (GObject *object)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);

  g_return_if_fail (TUMBLER_SPECIALIZED_THUMBNAILER (thumbnailer));

  /* chain up to parent classes */
  if (G_OBJECT_CLASS (tumbler_specialized_thumbnailer_parent_class)->constructed != NULL)
    (G_OBJECT_CLASS (tumbler_specialized_thumbnailer_parent_class)->constructed) (object);
  
  thumbnailer->proxy = 
    g_dbus_proxy_new_sync (thumbnailer->connection,
                           G_DBUS_PROXY_FLAGS_NONE,
                           NULL,
                           thumbnailer->name,
                           thumbnailer->object_path,
                           TUMBLER_SERVICE_NAME_PREFIX ".SpecializedThumbnailer1",
                           NULL,
                           NULL);
    
  if (thumbnailer->foreign) {
    g_signal_connect (thumbnailer->proxy, "notify::g-name-owner",
                      G_CALLBACK (tumbler_specialized_thumbnailer_proxy_name_owner_changed),
                      thumbnailer);
  }
}



static void
tumbler_specialized_thumbnailer_finalize (GObject *object)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);

  g_free (thumbnailer->name);
  g_free (thumbnailer->object_path);

  g_signal_handlers_disconnect_matched (thumbnailer->proxy, G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, thumbnailer);

  g_object_unref (thumbnailer->proxy);

  g_object_unref (thumbnailer->connection);
  
  (*G_OBJECT_CLASS (tumbler_specialized_thumbnailer_parent_class)->finalize) (object);
}



static void
tumbler_specialized_thumbnailer_get_property (GObject    *object,
                                              guint       prop_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, g_object_ref (thumbnailer->connection));
      break;
    case PROP_NAME:
      g_value_set_string (value, thumbnailer->name);
      break;
    case PROP_OBJECT_PATH:
      g_value_set_string (value, thumbnailer->object_path);
      break;
    case PROP_PROXY:
      g_value_set_object (value, thumbnailer->proxy);
      break;
    case PROP_FOREIGN:
      g_value_set_boolean (value, thumbnailer->foreign);
      break;
    case PROP_MODIFIED:
      g_value_set_uint64 (value, thumbnailer->modified);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_specialized_thumbnailer_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      thumbnailer->connection = g_object_ref (g_value_get_object (value));
      break;
    case PROP_NAME:
      thumbnailer->name = g_value_dup_string (value);
      break;
    case PROP_OBJECT_PATH:
      thumbnailer->object_path = g_value_dup_string (value);
      break;
    case PROP_FOREIGN:
      thumbnailer->foreign = g_value_get_boolean (value);
      break;
    case PROP_MODIFIED:
      thumbnailer->modified = g_value_get_uint64 (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}




static void 
thumbnailer_proxy_g_signal_cb (GDBusProxy *proxy, 
                               gchar *sender_name, 
                               gchar *signal_name, 
                               GVariant *parameters, 
                               gpointer user_data) 
{
  SpecializedInfo *info = user_data;  
  guint32 handle;
  
  if (strcmp (signal_name, "Finished") == 0) 
    {
      if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(u)"))) 
        {
          g_variant_get (parameters, "(u)", &handle);
          if (info->handle == handle) 
            {
              tumbler_mutex_lock (info->mutex);
              g_cond_broadcast (&info->condition);
              info->had_callback = TRUE;
              tumbler_mutex_unlock (info->mutex);
            }  
        }
    }
  else if (strcmp (signal_name, "Ready") == 0)
    {
       if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(us)"))) 
        {
          const gchar *uri;
          g_variant_get (parameters, "(u&s)", &handle, &uri);
          if (info->handle == handle) 
            {
               g_signal_emit_by_name (info->thumbnailer, "ready", info->info);
            }
        }
    }
  else if (strcmp (signal_name, "Error") == 0)
    {
       if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(usis)"))) 
        {
          const gchar *uri, *error_msg;
          gint error_code;
          
          g_variant_get (parameters, "(u&si&s)", &handle, &uri, &error_code, &error_msg);
          if (info->handle == handle) 
            {
              g_signal_emit_by_name (info->thumbnailer, "error", info->info,
                                     TUMBLER_ERROR, error_code, error_msg);
            }
        }
    }
}

  

static void
tumbler_specialized_thumbnailer_create (TumblerThumbnailer *thumbnailer,
                                        GCancellable       *cancellable,
                                        TumblerFileInfo    *info)
{
  TumblerSpecializedThumbnailer *s;
  SpecializedInfo                sinfo;
  gint64                         end_time;
  TumblerThumbnail              *thumbnail;
  TumblerThumbnailFlavor        *flavor;
  GVariant                      *result;
  const gchar                   *flavor_name;
  const gchar                   *uri;
  GError                        *error = NULL;
  gchar                         *message;
  int                            handler_id;
  
  g_return_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer));

  uri = tumbler_file_info_get_uri (info);
  thumbnail = tumbler_file_info_get_thumbnail (info);
  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  flavor_name = tumbler_thumbnail_flavor_get_name (flavor);

  s = TUMBLER_SPECIALIZED_THUMBNAILER (thumbnailer);

  g_cond_init (&sinfo.condition);
  sinfo.had_callback = FALSE;
  tumbler_mutex_create (sinfo.mutex);
  sinfo.info = info;
  sinfo.thumbnailer = thumbnailer;

  handler_id = g_signal_connect (s->proxy, "g-signal", 
                                 G_CALLBACK(thumbnailer_proxy_g_signal_cb), &info);

  result = g_dbus_proxy_call_sync (s->proxy, 
                                   "Queue", 
                                   g_variant_new("(sssb)",
                                                 uri,
                                                 tumbler_file_info_get_mime_type (info),
                                                 flavor_name,
                                                 /* TODO: Get this bool from scheduler type */
                                                 FALSE),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   100000000, /* 100 seconds worth of timeout */
                                   NULL,
                                   &error);

  if (error == NULL)
    {
      /*Get the return handle */
      g_variant_get (result, "(u)", &sinfo.handle);
      g_variant_unref (result);
        
      /* 100 seconds worth of timeout */
     end_time = g_get_monotonic_time () + 100 * G_TIME_SPAN_SECOND;

      tumbler_mutex_lock (sinfo.mutex);

      /* we are a thread, so the mainloop will still be
       * be running to receive the error and ready signals */
      if (!sinfo.had_callback)
        {
          if (!g_cond_wait_until (&sinfo.condition, &sinfo.mutex, end_time))
            {
              message = g_strdup (_("Failed to call the specialized thumbnailer: timeout"));
              g_signal_emit_by_name (thumbnailer, "error", info,
                                     TUMBLER_ERROR, TUMBLER_ERROR_CONNECTION_ERROR, message);
              g_free (message);
            }
        }
      tumbler_mutex_unlock (sinfo.mutex);
    }
  else
    {
      message = g_strdup_printf (_("Failed to call the specialized thumbnailer: %s"),
                                 error->message);
      g_signal_emit_by_name (thumbnailer, "error", info,
                             TUMBLER_ERROR, TUMBLER_ERROR_CONNECTION_ERROR, message);
      g_free (message);
      g_clear_error (&error);
    }
  
  g_signal_handler_disconnect (s->proxy, handler_id);

  g_cond_clear (&sinfo.condition);
}

static void
tumbler_specialized_thumbnailer_proxy_name_owner_changed (GDBusProxy                    *proxy,
                                                          GParamSpec                    *spec,
                                                          TumblerSpecializedThumbnailer *thumbnailer)
{
  gchar *name_owner;
  g_return_if_fail (G_IS_DBUS_PROXY (proxy));
  g_return_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer));
  
  name_owner = g_dbus_proxy_get_name_owner (proxy);
  if (name_owner == NULL) 
    {
      g_signal_emit_by_name (thumbnailer, "unregister");
    }
  g_free(name_owner);
}



TumblerThumbnailer *
tumbler_specialized_thumbnailer_new (GDBusConnection    *connection,
                                     const gchar        *name,
                                     const gchar        *object_path,
                                     const gchar *const *uri_schemes,
                                     const gchar *const *mime_types,
                                     guint64             modified)
{
  TumblerSpecializedThumbnailer *thumbnailer;

  g_return_val_if_fail (connection != NULL, NULL);
  g_return_val_if_fail (object_path != NULL && *object_path != '\0', NULL);
  g_return_val_if_fail (name != NULL && *name != '\0', NULL);
  g_return_val_if_fail (uri_schemes != NULL, NULL);
  g_return_val_if_fail (mime_types != NULL, NULL);

  thumbnailer = g_object_new (TUMBLER_TYPE_SPECIALIZED_THUMBNAILER, 
                              "connection", connection, "foreign", FALSE, "name", name, 
                              "object-path", object_path, "uri-schemes", uri_schemes, 
                              "mime-types", mime_types,
                              "modified", modified, NULL);

  return TUMBLER_THUMBNAILER (thumbnailer);
}



TumblerThumbnailer *
tumbler_specialized_thumbnailer_new_foreign (GDBusConnection    *connection,
                                             const gchar        *name,
                                             const gchar *const *uri_schemes,
                                             const gchar *const *mime_types)
{
  TumblerSpecializedThumbnailer *thumbnailer;
  gint64                         current_time;

  g_return_val_if_fail (connection != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (uri_schemes != NULL, NULL);
  g_return_val_if_fail (mime_types != NULL, NULL);

  current_time = g_get_real_time ();

  thumbnailer = g_object_new (TUMBLER_TYPE_SPECIALIZED_THUMBNAILER, 
                              "connection", connection, "foreign", TRUE, "name", name, 
                              "uri-schemes", uri_schemes, "mime-types", mime_types,
                              "modified", current_time / G_USEC_PER_SEC,
                              NULL);

  return TUMBLER_THUMBNAILER (thumbnailer);
}



const gchar *
tumbler_specialized_thumbnailer_get_name (TumblerSpecializedThumbnailer *thumbnailer)
{
  g_return_val_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer), FALSE);
  return thumbnailer->name;
}



gboolean
tumbler_specialized_thumbnailer_get_foreign (TumblerSpecializedThumbnailer *thumbnailer)
{
  g_return_val_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer), FALSE);
  return thumbnailer->foreign;
}



guint64
tumbler_specialized_thumbnailer_get_modified (TumblerSpecializedThumbnailer *thumbnailer)
{
  g_return_val_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer), 0);
  return thumbnailer->modified;
}
