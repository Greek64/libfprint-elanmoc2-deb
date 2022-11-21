/*
 * Virtual driver for image device debugging
 *
 * Copyright (C) 2019 Benjamin Berg <bberg@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * This is a virtual driver to debug the image based drivers. A small
 * python script is provided to connect to it via a socket, allowing
 * prints to be sent to this device programmatically.
 * Using this it is possible to test libfprint and fprintd.
 */

#define FP_COMPONENT "virtual_image"

#include "fpi-log.h"

#include "../fpi-image.h"
#include "../fpi-image-device.h"

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>

struct _FpDeviceVirtualImage
{
  FpImageDevice      parent;

  GSocketListener   *listener;
  GSocketConnection *connection;
  GCancellable      *listen_cancellable;
  GCancellable      *cancellable;

  gint               socket_fd;
  gint               client_fd;

  gboolean           automatic_finger;
  FpImage           *recv_img;
  gint               recv_img_hdr[2];
};

G_DECLARE_FINAL_TYPE (FpDeviceVirtualImage, fpi_device_virtual_image, FPI, DEVICE_VIRTUAL_IMAGE, FpImageDevice)
G_DEFINE_TYPE (FpDeviceVirtualImage, fpi_device_virtual_image, FP_TYPE_IMAGE_DEVICE)

static void start_listen (FpDeviceVirtualImage *dev);
static void recv_image (FpDeviceVirtualImage *dev,
                        GInputStream         *stream);

static void
recv_image_img_recv_cb (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  g_autoptr(GError) error = NULL;
  FpDeviceVirtualImage *self;
  FpImageDevice *device;
  gboolean success;
  gsize bytes = 0;

  success = g_input_stream_read_all_finish (G_INPUT_STREAM (source_object), res, &bytes, &error);

  /* Can't use self if the operation was cancelled. */
  if (!success && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return;

  self = FPI_DEVICE_VIRTUAL_IMAGE (user_data);
  device = FP_IMAGE_DEVICE (self);

  /* Consider success if we received the right amount of data, otherwise
   * an error must have happened. */
  if (bytes < self->recv_img->width * self->recv_img->height)
    {
      if (!success)
        g_warning ("Error receiving image data: %s", error->message);
      else
        g_warning ("Error receiving image data: end of stream before all data was read");

      self = FPI_DEVICE_VIRTUAL_IMAGE (user_data);
      g_io_stream_close (G_IO_STREAM (self->connection), NULL, NULL);
      g_clear_object (&self->connection);
      return;
    }

  if (self->automatic_finger)
    fpi_image_device_report_finger_status (device, TRUE);
  fpi_image_device_image_captured (device, g_steal_pointer (&self->recv_img));
  if (self->automatic_finger)
    fpi_image_device_report_finger_status (device, FALSE);

  /* And, listen for more images from the same client. */
  recv_image (self, G_INPUT_STREAM (source_object));
}

static void
recv_image_hdr_recv_cb (GObject      *source_object,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  g_autoptr(GError) error = NULL;
  FpDeviceVirtualImage *self;
  gboolean success;
  gsize bytes;

  success = g_input_stream_read_all_finish (G_INPUT_STREAM (source_object), res, &bytes, &error);

  if (!success || bytes != sizeof (self->recv_img_hdr))
    {
      if (!success)
        {
          if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) ||
              g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CLOSED))
            return;
          g_warning ("Error receiving header for image data: %s", error->message);
        }
      else if (bytes != 0)
        {
          g_warning ("Received incomplete header before end of stream.");
        }

      self = FPI_DEVICE_VIRTUAL_IMAGE (user_data);
      g_io_stream_close (G_IO_STREAM (self->connection), NULL, NULL);
      g_clear_object (&self->connection);
      return;
    }

  self = FPI_DEVICE_VIRTUAL_IMAGE (user_data);
  if (self->recv_img_hdr[0] > 5000 || self->recv_img_hdr[1] > 5000)
    {
      g_warning ("Image header suggests an unrealistically large image, disconnecting client.");
      g_io_stream_close (G_IO_STREAM (self->connection), NULL, NULL);
      g_clear_object (&self->connection);
    }

  if (self->recv_img_hdr[0] < 0 || self->recv_img_hdr[1] < 0)
    {
      switch (self->recv_img_hdr[0])
        {
        case -1:
          /* -1 is a retry error, just pass it through */
          fpi_image_device_retry_scan (FP_IMAGE_DEVICE (self), self->recv_img_hdr[1]);
          break;

        case -2:
          /* -2 is a fatal error, just pass it through*/
          fpi_image_device_session_error (FP_IMAGE_DEVICE (self),
                                          fpi_device_error_new (self->recv_img_hdr[1]));
          break;

        case -3:
          /* -3 sets/clears automatic finger detection for images */
          self->automatic_finger = !!self->recv_img_hdr[1];
          break;

        case -4:
          /* -4 submits a finger detection report */
          fpi_image_device_report_finger_status (FP_IMAGE_DEVICE (self),
                                                 !!self->recv_img_hdr[1]);
          break;

        case -5:
          /* -5 causes the device to disappear (no further data) */
          fpi_device_remove (FP_DEVICE (self));
          break;

        default:
          /* disconnect client, it didn't play fair */
          g_io_stream_close (G_IO_STREAM (self->connection), NULL, NULL);
          g_clear_object (&self->connection);
        }

      /* And, listen for more images from the same client. */
      recv_image (self, G_INPUT_STREAM (source_object));
      return;
    }

  self->recv_img = fp_image_new (self->recv_img_hdr[0], self->recv_img_hdr[1]);
  g_debug ("image data: %p", self->recv_img->data);
  g_input_stream_read_all_async (G_INPUT_STREAM (source_object),
                                 (guint8 *) self->recv_img->data,
                                 self->recv_img->width * self->recv_img->height,
                                 G_PRIORITY_DEFAULT,
                                 self->cancellable,
                                 recv_image_img_recv_cb,
                                 self);
}

static void
recv_image (FpDeviceVirtualImage *self, GInputStream *stream)
{
  FpiImageDeviceState state;

  g_object_get (self, "fpi-image-device-state", &state, NULL);

  g_debug ("Starting image receive (if active), state is: %i", state);

  /* Only register if the state is active. */
  if (state >= FPI_IMAGE_DEVICE_STATE_IDLE)
    {
      g_input_stream_read_all_async (stream,
                                     self->recv_img_hdr,
                                     sizeof (self->recv_img_hdr),
                                     G_PRIORITY_DEFAULT,
                                     self->cancellable,
                                     recv_image_hdr_recv_cb,
                                     self);
    }
}

static void
new_connection_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  GSocketConnection *connection;
  GInputStream *stream;
  FpDeviceVirtualImage *dev = user_data;

  connection = g_socket_listener_accept_finish (G_SOCKET_LISTENER (source_object),
                                                res,
                                                NULL,
                                                &error);
  if (!connection)
    {
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

      g_warning ("Error accepting a new connection: %s", error->message);
      start_listen (dev);
    }

  /* Always accept further connections (but we disconnect them immediately
   * if we already have a connection). */
  start_listen (dev);

  if (dev->connection)
    {
      /* We may not have noticed that the stream was closed,
       * if the device is deactivated.
       * Cancel any ongoing operation on the old connection. */
      g_cancellable_cancel (dev->cancellable);
      g_clear_object (&dev->cancellable);
      dev->cancellable = g_cancellable_new ();

      g_clear_object (&dev->connection);
    }

  if (dev->connection)
    {
      g_warning ("Rejecting new connection");
      g_io_stream_close (G_IO_STREAM (connection), NULL, NULL);
      g_object_unref (connection);
      return;
    }

  dev->connection = connection;
  dev->automatic_finger = TRUE;
  stream = g_io_stream_get_input_stream (G_IO_STREAM (connection));

  fp_dbg ("Got a new connection!");
  recv_image (dev, stream);
}

static void
start_listen (FpDeviceVirtualImage *dev)
{
  g_socket_listener_accept_async (dev->listener,
                                  dev->listen_cancellable,
                                  new_connection_cb,
                                  dev);
}

static void
dev_init (FpImageDevice *dev)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GSocketListener) listener = NULL;
  FpDeviceVirtualImage *self = FPI_DEVICE_VIRTUAL_IMAGE (dev);
  const char *env;

  g_autoptr(GSocketAddress) addr = NULL;
  G_DEBUG_HERE ();

  self->client_fd = -1;

  env = fpi_device_get_virtual_env (FP_DEVICE (self));

  listener = g_socket_listener_new ();
  g_socket_listener_set_backlog (listener, 1);

  /* Remove any left over socket. */
  g_unlink (env);

  addr = g_unix_socket_address_new (env);

  if (!g_socket_listener_add_address (listener,
                                      addr,
                                      G_SOCKET_TYPE_STREAM,
                                      G_SOCKET_PROTOCOL_DEFAULT,
                                      NULL,
                                      NULL,
                                      &error))
    {
      g_warning ("Could not listen on unix socket: %s", error->message);

      fpi_image_device_open_complete (FP_IMAGE_DEVICE (dev), g_steal_pointer (&error));

      return;
    }

  self->listener = g_steal_pointer (&listener);
  self->cancellable = g_cancellable_new ();
  self->listen_cancellable = g_cancellable_new ();

  start_listen (self);

  /* Delay result to open up the possibility of testing race conditions. */
  fpi_device_add_timeout (FP_DEVICE (dev), 100, (FpTimeoutFunc) fpi_image_device_open_complete, NULL, NULL);
}

static void
dev_deinit (FpImageDevice *dev)
{
  FpDeviceVirtualImage *self = FPI_DEVICE_VIRTUAL_IMAGE (dev);

  G_DEBUG_HERE ();

  g_cancellable_cancel (self->cancellable);
  g_cancellable_cancel (self->listen_cancellable);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->listen_cancellable);
  g_clear_object (&self->listener);
  g_clear_object (&self->connection);

  /* Delay result to open up the possibility of testing race conditions. */
  fpi_device_add_timeout (FP_DEVICE (dev), 100, (FpTimeoutFunc) fpi_image_device_close_complete, NULL, NULL);
}

static void
dev_activate (FpImageDevice *dev)
{
  FpDeviceVirtualImage *self = FPI_DEVICE_VIRTUAL_IMAGE (dev);

  fpi_image_device_activate_complete (dev, NULL);

  if (self->connection)
    recv_image (self, g_io_stream_get_input_stream (G_IO_STREAM (self->connection)));
}

static void
dev_deactivate (FpImageDevice *dev)
{
  FpDeviceVirtualImage *self = FPI_DEVICE_VIRTUAL_IMAGE (dev);

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  self->cancellable = g_cancellable_new ();

  /* XXX: Need to wait for the operation to be cancelled. */
  fpi_device_add_timeout (FP_DEVICE (dev), 10, (FpTimeoutFunc) fpi_image_device_deactivate_complete, NULL, NULL);
}

static void
dev_notify_removed_cb (FpDevice *dev)
{
  FpiImageDeviceState state;
  gboolean removed;

  g_object_get (dev,
                "fpi-image-device-state", &state,
                "removed", &removed,
                NULL);

  if (!removed || state == FPI_IMAGE_DEVICE_STATE_INACTIVE)
    return;

  /* This error will be converted to an FP_DEVICE_ERROR_REMOVED by the
   * surrounding layers. */
  fpi_image_device_session_error (FP_IMAGE_DEVICE (dev),
                                  fpi_device_error_new (FP_DEVICE_ERROR_PROTO));
}

static void
fpi_device_virtual_image_init (FpDeviceVirtualImage *self)
{
  /* NOTE: This is not nice, but we can generally rely on the underlying
   *       system to throw errors on the transport layer.
   */
  g_signal_connect (self,
                    "notify::removed",
                    G_CALLBACK (dev_notify_removed_cb),
                    NULL);
}

static const FpIdEntry driver_ids[] = {
  { .virtual_envvar = "FP_VIRTUAL_IMAGE" },
  { .virtual_envvar = NULL }
};

static void
fpi_device_virtual_image_class_init (FpDeviceVirtualImageClass *klass)
{
  FpDeviceClass *dev_class = FP_DEVICE_CLASS (klass);
  FpImageDeviceClass *img_class = FP_IMAGE_DEVICE_CLASS (klass);

  dev_class->id = FP_COMPONENT;
  dev_class->full_name = "Virtual image device for debugging";
  dev_class->type = FP_DEVICE_TYPE_VIRTUAL;
  dev_class->id_table = driver_ids;

  img_class->img_open = dev_init;
  img_class->img_close = dev_deinit;

  img_class->activate = dev_activate;
  img_class->deactivate = dev_deactivate;
}
