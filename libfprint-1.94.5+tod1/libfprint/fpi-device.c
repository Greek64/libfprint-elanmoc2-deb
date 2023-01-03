/*
 * FpDevice - A fingerprint reader device - Private APIs
 * Copyright (C) 2019 Benjamin Berg <bberg@redhat.com>
 * Copyright (C) 2019 Marco Trevisan <marco.trevisan@canonical.com>
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

#define FP_COMPONENT "device"
#include <math.h>
#include <fcntl.h>
#include <errno.h>

#include "fpi-log.h"

#include "fp-device-private.h"

/**
 * SECTION: fpi-device
 * @title: Internal FpDevice
 * @short_description: Internal device routines
 *
 * The methods that are available for drivers to manipulate a device. See
 * #FpDeviceClass for more information. Also note that most of these are
 * not relevant for image based devices, see #FpImageDeviceClass in that
 * case.
 *
 * Also see the public #FpDevice routines.
 */

/* Manually redefine what G_DEFINE_* macro does */
static inline gpointer
fp_device_get_instance_private (FpDevice *self)
{
  FpDeviceClass *dev_class = g_type_class_peek_static (FP_TYPE_DEVICE);

  return G_STRUCT_MEMBER_P (self,
                            g_type_class_get_instance_private_offset (dev_class));
}

/**
 * fpi_device_class_auto_initialize_features:
 *
 * Initializes the #FpDeviceClass @features flags checking what device vfuncs
 * are implemented.
 * Drivers should call this at the end of the class initialization.
 */
void
fpi_device_class_auto_initialize_features (FpDeviceClass *device_class)
{
  g_return_if_fail (FP_IS_DEVICE_CLASS (device_class));

  if (device_class->capture)
    device_class->features |= FP_DEVICE_FEATURE_CAPTURE;

  if (device_class->verify)
    device_class->features |= FP_DEVICE_FEATURE_VERIFY;

  if (device_class->identify)
    device_class->features |= FP_DEVICE_FEATURE_IDENTIFY;

  if (device_class->list)
    device_class->features |= FP_DEVICE_FEATURE_STORAGE_LIST;

  if (device_class->delete)
    device_class->features |= FP_DEVICE_FEATURE_STORAGE_DELETE;

  if (device_class->clear_storage)
    device_class->features |= FP_DEVICE_FEATURE_STORAGE_CLEAR;

  if (device_class->delete && (device_class->list || device_class->clear_storage))
    device_class->features |= FP_DEVICE_FEATURE_STORAGE;

  if (device_class->temp_hot_seconds < 0)
    device_class->features |= FP_DEVICE_FEATURE_ALWAYS_ON;
}

/**
 * fpi_device_retry_new:
 * @error: The #FpDeviceRetry error value describing the issue
 *
 * Create a new retry error code for use with fpi_device_verify_complete()
 * and similar calls.
 */
GError *
fpi_device_retry_new (FpDeviceRetry error)
{
  const gchar *msg;

  switch (error)
    {
    case FP_DEVICE_RETRY_GENERAL:
      msg = "Please try again.";
      break;

    case FP_DEVICE_RETRY_TOO_SHORT:
      msg = "The swipe was too short, please try again.";
      break;

    case FP_DEVICE_RETRY_CENTER_FINGER:
      msg = "The finger was not centered properly, please try again.";
      break;

    case FP_DEVICE_RETRY_REMOVE_FINGER:
      msg = "Please try again after removing the finger first.";
      break;

    default:
      g_warning ("Unsupported error, returning general error instead!");
      error = FP_DEVICE_RETRY_GENERAL;
      msg = "Please try again.";
    }

  return g_error_new_literal (FP_DEVICE_RETRY, error, msg);
}

/**
 * fpi_device_error_new:
 * @error: The #FpDeviceRetry error value describing the issue
 *
 * Create a new error code for use with fpi_device_verify_complete() and
 * similar calls.
 */
GError *
fpi_device_error_new (FpDeviceError error)
{
  const gchar *msg;

  switch (error)
    {
    case FP_DEVICE_ERROR_GENERAL:
      msg = "An unspecified error occurred!";
      break;

    case FP_DEVICE_ERROR_NOT_SUPPORTED:
      msg = "The operation is not supported on this device!";
      break;

    case FP_DEVICE_ERROR_NOT_OPEN:
      msg = "The device needs to be opened first!";
      break;

    case FP_DEVICE_ERROR_ALREADY_OPEN:
      msg = "The device has already been opened!";
      break;

    case FP_DEVICE_ERROR_BUSY:
      msg = "The device is still busy with another operation, please try again later.";
      break;

    case FP_DEVICE_ERROR_PROTO:
      msg = "The driver encountered a protocol error with the device.";
      break;

    case FP_DEVICE_ERROR_DATA_INVALID:
      msg = "Passed (print) data is not valid.";
      break;

    case FP_DEVICE_ERROR_DATA_FULL:
      msg = "On device storage space is full.";
      break;

    case FP_DEVICE_ERROR_DATA_NOT_FOUND:
      msg = "Print was not found on the devices storage.";
      break;

    case FP_DEVICE_ERROR_DATA_DUPLICATE:
      msg = "This finger has already enrolled, please try a different finger";
      break;

    case FP_DEVICE_ERROR_REMOVED:
      msg = "This device has been removed from the system.";
      break;

    case FP_DEVICE_ERROR_TOO_HOT:
      msg = "Device disabled to prevent overheating.";
      break;

    default:
      g_warning ("Unsupported error, returning general error instead!");
      error = FP_DEVICE_ERROR_GENERAL;
      msg = "An unspecified error occurred!";
    }

  return g_error_new_literal (FP_DEVICE_ERROR, error, msg);
}

/**
 * fpi_device_retry_new_msg:
 * @error: The #FpDeviceRetry error value describing the issue
 * @msg: Custom message to use with printf-style formatting
 * @...: args for @msg
 *
 * Create a new retry error code for use with fpi_device_verify_complete()
 * and similar calls.
 */
GError *
fpi_device_retry_new_msg (FpDeviceRetry device_error,
                          const gchar  *msg,
                          ...)
{
  GError *error;
  va_list args;

  va_start (args, msg);
  error = g_error_new_valist (FP_DEVICE_RETRY, device_error, msg, args);
  va_end (args);

  return error;
}

/**
 * fpi_device_error_new_msg:
 * @error: The #FpDeviceRetry error value describing the issue
 * @msg: Custom message to use with printf-style formatting
 * @...: args for @msg
 *
 * Create a new error code for use with fpi_device_verify_complete()
 * and similar calls.
 */
GError *
fpi_device_error_new_msg (FpDeviceError device_error,
                          const gchar  *msg,
                          ...)
{
  GError *error;
  va_list args;

  va_start (args, msg);
  error = g_error_new_valist (FP_DEVICE_ERROR, device_error, msg, args);
  va_end (args);

  return error;
}

/**
 * fpi_device_set_nr_enroll_stages:
 * @device: The #FpDevice
 * @enroll_stages: The number of enroll stages
 *
 * Updates the reported number of enroll stages that the device needs.
 * If all supported devices have the same number of stages, then the
 * value can simply be set in the class.
 */
void
fpi_device_set_nr_enroll_stages (FpDevice *device,
                                 gint      enroll_stages)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (enroll_stages > 0);

  priv->nr_enroll_stages = enroll_stages;
  g_object_notify (G_OBJECT (device), "nr-enroll-stages");
}

/**
 * fpi_device_set_scan_type:
 * @device: The #FpDevice
 * @scan_type: The scan type of the device
 *
 * Updates the the scan type of the device from the default.
 * If all supported devices have the same scan type, then the
 * value can simply be set in the class.
 */
void
fpi_device_set_scan_type (FpDevice  *device,
                          FpScanType scan_type)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));

  priv->scan_type = scan_type;
  g_object_notify (G_OBJECT (device), "scan-type");
}

/**
 * fpi_device_update_features:
 * @device: The #FpDevice
 * @update: The feature flags to update
 * @value: The value to set the flags to
 *
 * Updates the feature flags for the device. This can be used
 * to runtime detect features that are supported by the device.
 */
void
fpi_device_update_features (FpDevice       *device,
                            FpDeviceFeature update,
                            FpDeviceFeature value)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_PROBE);
  g_return_if_fail ((value & update) == value);

  priv->features = (priv->features & ~update) | (value & update);
}

typedef struct
{
  GSource   source;
  FpDevice *device;
} FpDeviceTimeoutSource;

static void
timeout_finalize (GSource *source)
{
  FpDeviceTimeoutSource *timeout_source = (FpDeviceTimeoutSource *) source;
  FpDevicePrivate *priv;

  priv = fp_device_get_instance_private (timeout_source->device);
  priv->sources = g_slist_remove (priv->sources, source);
}

static gboolean
timeout_dispatch (GSource *source, GSourceFunc gsource_func, gpointer user_data)
{
  FpDeviceTimeoutSource *timeout_source = (FpDeviceTimeoutSource *) source;
  FpTimeoutFunc callback = (FpTimeoutFunc) gsource_func;

  callback (timeout_source->device, user_data);

  return G_SOURCE_REMOVE;
}

static GSourceFuncs timeout_funcs = {
  NULL, /* prepare */
  NULL, /* check */
  timeout_dispatch,
  timeout_finalize,
  NULL, NULL
};

/**
 * fpi_device_add_timeout:
 * @device: The #FpDevice
 * @interval: The interval in milliseconds
 * @func: The #FpTimeoutFunc to call on timeout
 * @user_data: (nullable): User data to pass to the callback
 * @destroy_notify: (nullable): #GDestroyNotify for @user_data
 *
 * Register a timeout to run. Drivers should always make sure that timers are
 * cancelled when appropriate.
 *
 * Returns: (transfer none): A newly created and attached #GSource
 */
GSource *
fpi_device_add_timeout (FpDevice      *device,
                        gint           interval,
                        FpTimeoutFunc  func,
                        gpointer       user_data,
                        GDestroyNotify destroy_notify)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpDeviceTimeoutSource *source;
  GMainContext *context;

  source = (FpDeviceTimeoutSource *) g_source_new (&timeout_funcs,
                                                   sizeof (FpDeviceTimeoutSource));
  source->device = device;

  if (priv->current_task)
    context = g_task_get_context (priv->current_task);
  else
    context = g_main_context_get_thread_default ();

  g_source_attach (&source->source, context);
  g_source_set_callback (&source->source, (GSourceFunc) func, user_data, destroy_notify);
  g_source_set_ready_time (&source->source,
                           g_source_get_time (&source->source) + interval * (guint64) 1000);
  priv->sources = g_slist_prepend (priv->sources, source);
  g_source_unref (&source->source);

  return &source->source;
}

/**
 * fpi_device_get_usb_device:
 * @device: The #FpDevice
 *
 * Get the #GUsbDevice for this #FpDevice. Only permissible to call if the
 * #FpDevice is of type %FP_DEVICE_TYPE_USB.
 *
 * Returns: The #GUsbDevice
 */
GUsbDevice *
fpi_device_get_usb_device (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_val_if_fail (FP_IS_DEVICE (device), NULL);
  g_return_val_if_fail (priv->type == FP_DEVICE_TYPE_USB, NULL);

  return priv->usb_device;
}

/**
 * fpi_device_get_udev_data:
 * @device: The #FpDevice
 * @subtype: Which subtype to get information about
 *
 * Get a subtype-specific hardware resource for this #FpDevice. Only permissible to call if the
 * #FpDevice is of type %FP_DEVICE_TYPE_UDEV.
 *
 * Returns: Depends on @subtype; for SPIDEV/HIDRAW returns a path to the relevant device.
 */
gpointer
fpi_device_get_udev_data (FpDevice *device, FpiDeviceUdevSubtypeFlags subtype)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_val_if_fail (FP_IS_DEVICE (device), NULL);
  g_return_val_if_fail (priv->type == FP_DEVICE_TYPE_UDEV, NULL);

  switch (subtype)
    {
    case FPI_DEVICE_UDEV_SUBTYPE_HIDRAW:
      return priv->udev_data.hidraw_path;

    case FPI_DEVICE_UDEV_SUBTYPE_SPIDEV:
      return priv->udev_data.spidev_path;

    default:
      g_return_val_if_reached (NULL);
      return NULL;
    }
}

/**
 * fpi_device_get_virtual_env:
 * @device: The #FpDevice
 *
 * Get the value of the environment variable that caused the virtual #FpDevice to be
 * generated. Only permissible to call if the #FpDevice is of type %FP_DEVICE_TYPE_VIRTUAL.
 *
 * Returns: The value of the environment variable
 */
const gchar *
fpi_device_get_virtual_env (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_val_if_fail (FP_IS_DEVICE (device), NULL);
  g_return_val_if_fail (priv->type == FP_DEVICE_TYPE_VIRTUAL, NULL);

  return priv->virtual_env;
}

/**
 * fpi_device_get_current_action:
 * @device: The #FpDevice
 *
 * Get the currently ongoing action or %FPI_DEVICE_ACTION_NONE if there
 * is no operation at this time.
 *
 * This is useful for drivers that might share code paths between different
 * actions (e.g. verify and identify) and want to find out again later which
 * action was started in the beginning.
 *
 * Returns: The ongoing #FpiDeviceAction
 */
FpiDeviceAction
fpi_device_get_current_action (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_val_if_fail (FP_IS_DEVICE (device), FPI_DEVICE_ACTION_NONE);

  return priv->current_action;
}

/**
 * fpi_device_action_is_cancelled:
 * @device: The #FpDevice
 *
 * Checks whether the current action has been cancelled by the user.
 * This is equivalent to first getting the cancellable using
 * fpi_device_get_cancellable() and then checking whether it has been
 * cancelled (if it is non-NULL).
 *
 * Returns: %TRUE if action should be cancelled
 */
gboolean
fpi_device_action_is_cancelled (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_val_if_fail (FP_IS_DEVICE (device), TRUE);
  g_return_val_if_fail (priv->current_action != FPI_DEVICE_ACTION_NONE, TRUE);

  return g_cancellable_is_cancelled (priv->current_cancellable);
}

/**
 * fpi_device_get_driver_data:
 * @device: The #FpDevice
 *
 * Returns: The driver data from the #FpIdEntry table entry
 */
guint64
fpi_device_get_driver_data (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_val_if_fail (FP_IS_DEVICE (device), 0);

  return priv->driver_data;
}

/**
 * fpi_device_get_enroll_data:
 * @device: The #FpDevice
 * @print: (out) (transfer none): The user provided template print
 *
 * Get data for enrollment.
 */
void
fpi_device_get_enroll_data (FpDevice *device,
                            FpPrint **print)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpEnrollData *data;

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_ENROLL);

  data = g_task_get_task_data (priv->current_task);
  g_assert (data);

  if (print)
    *print = data->print;
}

/**
 * fpi_device_get_capture_data:
 * @device: The #FpDevice
 * @wait_for_finger: (out): Whether to wait for finger or not
 *
 * Get data for capture.
 */
void
fpi_device_get_capture_data (FpDevice *device,
                             gboolean *wait_for_finger)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_CAPTURE);

  if (wait_for_finger)
    *wait_for_finger = priv->wait_for_finger;
}

/**
 * fpi_device_get_verify_data:
 * @device: The #FpDevice
 * @print: (out) (transfer none): The enrolled print
 *
 * Get data for verify.
 */
void
fpi_device_get_verify_data (FpDevice *device,
                            FpPrint **print)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpMatchData *data;

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_VERIFY);

  data = g_task_get_task_data (priv->current_task);
  g_assert (data);

  if (print)
    *print = data->enrolled_print;
}

/**
 * fpi_device_get_identify_data:
 * @device: The #FpDevice
 * @prints: (out) (transfer none) (element-type FpPrint): The gallery of prints
 *
 * Get prints gallery for identification.
 *
 * The @prints array is always non-%NULL and may contain a list of #FpPrint's
 * that the device should match against.
 *
 * Note that @prints can be an empty array, in such case the device is expected
 * to report the scanned print matching the one in its internal storage, if any.
 *
 */
void
fpi_device_get_identify_data (FpDevice   *device,
                              GPtrArray **prints)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpMatchData *data;

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_IDENTIFY);

  data = g_task_get_task_data (priv->current_task);
  g_assert (data);

  if (prints)
    *prints = data->gallery;
}

/**
 * fpi_device_get_delete_data:
 * @device: The #FpDevice
 * @print: (out) (transfer none): The print to delete
 *
 * Get data for delete.
 */
void
fpi_device_get_delete_data (FpDevice *device,
                            FpPrint **print)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_DELETE);

  if (print)
    *print = g_task_get_task_data (priv->current_task);
}

/**
 * fpi_device_get_cancellable:
 * @device: The #FpDevice
 *
 * Retrieve the #GCancellable that may cancel the currently ongoing operation. This
 * is primarily useful to pass directly to e.g. fpi_usb_transfer_submit() for cancellable
 * transfers.
 * In many cases the cancel vfunc may be more convenient to react to cancellation in some
 * way.
 *
 * Returns: (transfer none): The #GCancellable for the current action.
 */
GCancellable *
fpi_device_get_cancellable (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_val_if_fail (FP_IS_DEVICE (device), NULL);
  g_return_val_if_fail (priv->current_action != FPI_DEVICE_ACTION_NONE, NULL);

  return priv->current_cancellable;
}

static void
emit_removed_on_task_completed (FpDevice *device)
{
  g_signal_emit_by_name (device, "removed");
}

/**
 * fpi_device_remove:
 * @device: The #FpDevice
 *
 * Called to signal to the #FpDevice that it has been unplugged (physically
 * removed from the system).
 *
 * For USB devices, this API is called automatically by #FpContext.
 */
void
fpi_device_remove (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (!priv->is_removed);

  priv->is_removed = TRUE;

  g_object_notify (G_OBJECT (device), "removed");

  /* If there is a pending action, we wait for it to fail, otherwise we
   * immediately emit the "removed" signal. */
  if (priv->current_task)
    {
      g_signal_connect_object (priv->current_task,
                               "notify::completed",
                               (GCallback) emit_removed_on_task_completed,
                               device,
                               G_CONNECT_SWAPPED);
    }
  else
    {
      g_signal_emit_by_name (device, "removed");
    }
}

/**
 * fpi_device_action_error:
 * @device: The #FpDevice
 * @error: The #GError to return
 *
 * Finish an ongoing action with an error. This is the same as calling
 * the corresponding complete function such as fpi_device_open_complete()
 * with an error set. If possible, use the correct complete function as
 * that results in improved error detection.
 */
void
fpi_device_action_error (FpDevice *device,
                         GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action != FPI_DEVICE_ACTION_NONE);

  if (error != NULL)
    {
      g_autofree char *action_str = NULL;

      action_str = g_enum_to_string (FPI_TYPE_DEVICE_ACTION, priv->current_action);
      g_debug ("Device reported generic error (%s) during action; action was: %s",
               error->message, action_str);
    }
  else
    {
      g_warning ("Device failed to pass an error to generic action error function");
      error = fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL, "Device reported error but did not provide an error condition");
    }


  switch (priv->current_action)
    {
    case FPI_DEVICE_ACTION_PROBE:
      fpi_device_probe_complete (device, NULL, NULL, error);
      break;

    case FPI_DEVICE_ACTION_OPEN:
      fpi_device_open_complete (device, error);
      break;

    case FPI_DEVICE_ACTION_CLOSE:
      fpi_device_close_complete (device, error);
      break;

    case FPI_DEVICE_ACTION_ENROLL:
      fpi_device_enroll_complete (device, NULL, error);
      break;

    case FPI_DEVICE_ACTION_VERIFY:
      fpi_device_verify_complete (device, error);
      break;

    case FPI_DEVICE_ACTION_IDENTIFY:
      fpi_device_identify_complete (device, error);
      break;

    case FPI_DEVICE_ACTION_CAPTURE:
      fpi_device_capture_complete (device, NULL, error);
      break;

    case FPI_DEVICE_ACTION_DELETE:
      fpi_device_delete_complete (device, error);
      break;

    case FPI_DEVICE_ACTION_LIST:
      fpi_device_list_complete (device, NULL, error);
      break;

    case FPI_DEVICE_ACTION_CLEAR_STORAGE:
      fpi_device_clear_storage_complete (device, error);
      break;

    default:
    case FPI_DEVICE_ACTION_NONE:
      g_return_if_reached ();
      break;
    }
}

/**
 * fpi_device_critical_enter:
 * @device: The #FpDevice
 *
 * Enter a critical section in the driver code where no outside calls from
 * libfprint should happen. Drivers can already assume that everything
 * happens from the same thread, however, that still allows e.g. the cancel
 * vfunc to be called at any point in time.
 *
 * Using this kind of critical section, the driver can assume that libfprint
 * will not forward any external requests to the driver for the time being.
 * This is for example useful to prevent cancellation while the device is being
 * set up. Or, said differently, using this feature means that the cancel
 * handler is able to make more assumptions about the current state.
 *
 * Please note that the driver is not shielded from all external changes. For
 * example the cancellable as returned by fpi_device_get_cancellable() will
 * still change immediately.
 *
 * The driver may call this function multiple times, but must also ensure that
 * fpi_device_critical_leave() is called an equal amount of times and that all
 * critical sections are left before command completion.
 */
void
fpi_device_critical_enter (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (priv->current_action != FPI_DEVICE_ACTION_NONE);

  priv->critical_section += 1;

  /* Stop flushing events if that was previously queued. */
  if (priv->critical_section_flush_source)
    g_source_destroy (priv->critical_section_flush_source);
  priv->critical_section_flush_source = NULL;
}

static gboolean
fpi_device_critical_section_flush_idle_cb (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpDeviceClass *cls = FP_DEVICE_GET_CLASS (device);

  if (priv->cancel_queued)
    {
      /* Cancellation must only happen if the driver is busy. */
      if (priv->current_action != FPI_DEVICE_ACTION_NONE &&
          priv->current_task_idle_return_source == NULL)
        cls->cancel (device);
      priv->cancel_queued = FALSE;

      return G_SOURCE_CONTINUE;
    }

  if (priv->suspend_queued)
    {
      priv->suspend_queued = FALSE;
      fpi_device_suspend (device);

      return G_SOURCE_CONTINUE;
    }

  if (priv->resume_queued)
    {
      priv->resume_queued = FALSE;
      fpi_device_resume (device);

      return G_SOURCE_CONTINUE;
    }

  priv->critical_section_flush_source = NULL;

  return G_SOURCE_REMOVE;
}

/**
 * fpi_device_critical_leave:
 * @device: The #FpDevice
 *
 * Leave a critical section started by fpi_device_critical_enter().
 *
 * Once all critical sections have been left, libfprint will start flushing
 * out the queued up requests. This is done from the mainloop and the driver
 * is protected from reentrency issues.
 */
void
fpi_device_critical_leave (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (priv->current_action != FPI_DEVICE_ACTION_NONE);
  g_return_if_fail (priv->critical_section);

  priv->critical_section -= 1;
  if (priv->critical_section)
    return;

  /* We left the critical section, make sure a flush is queued. */
  if (priv->critical_section_flush_source)
    return;

  priv->critical_section_flush_source = g_idle_source_new ();
  g_source_set_priority (priv->critical_section_flush_source, G_PRIORITY_HIGH);
  g_source_set_callback (priv->critical_section_flush_source,
                         (GSourceFunc) fpi_device_critical_section_flush_idle_cb,
                         device,
                         NULL);
  g_source_set_name (priv->critical_section_flush_source,
                     "Flush libfprint driver critical section");
  g_source_attach (priv->critical_section_flush_source,
                   g_task_get_context (priv->current_task));
  g_source_unref (priv->critical_section_flush_source);
}

static void
clear_device_cancel_action (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_clear_pointer (&priv->current_idle_cancel_source, g_source_destroy);

  if (priv->current_cancellable_id)
    {
      g_cancellable_disconnect (priv->current_cancellable,
                                priv->current_cancellable_id);
      priv->current_cancellable_id = 0;
    }

  if (priv->current_task_cancellable_id)
    {
      g_cancellable_disconnect (g_task_get_cancellable (priv->current_task),
                                priv->current_task_cancellable_id);
      priv->current_task_cancellable_id = 0;
    }
}

typedef enum _FpDeviceTaskReturnType {
  FP_DEVICE_TASK_RETURN_INT,
  FP_DEVICE_TASK_RETURN_BOOL,
  FP_DEVICE_TASK_RETURN_OBJECT,
  FP_DEVICE_TASK_RETURN_PTR_ARRAY,
  FP_DEVICE_TASK_RETURN_ERROR,
} FpDeviceTaskReturnType;

typedef struct _FpDeviceTaskReturnData
{
  FpDevice              *device;
  FpDeviceTaskReturnType type;
  gpointer               result;
} FpDeviceTaskReturnData;

static gboolean
fp_device_task_return_in_idle_cb (gpointer user_data)
{
  FpDeviceTaskReturnData *data = user_data;
  FpDevicePrivate *priv = fp_device_get_instance_private (data->device);
  g_autofree char *action_str = NULL;
  FpiDeviceAction action;

  g_autoptr(GTask) task = NULL;
  g_autoptr(GError) cancellation_reason = NULL;


  action_str = g_enum_to_string (FPI_TYPE_DEVICE_ACTION, priv->current_action);
  g_debug ("Completing action %s in idle!", action_str);

  task = g_steal_pointer (&priv->current_task);
  action = priv->current_action;
  priv->current_action = FPI_DEVICE_ACTION_NONE;
  priv->current_task_idle_return_source = NULL;
  g_clear_object (&priv->current_cancellable);
  cancellation_reason = g_steal_pointer (&priv->current_cancellation_reason);

  fpi_device_update_temp (data->device, FALSE);

  if (action == FPI_DEVICE_ACTION_OPEN &&
      data->type != FP_DEVICE_TASK_RETURN_ERROR)
    {
      priv->is_open = TRUE;
      g_object_notify (G_OBJECT (data->device), "open");
    }
  else if (action == FPI_DEVICE_ACTION_CLOSE)
    {
      /* Always consider the device closed. Drivers should try hard to close the
       * device. Generally, e.g. cancellations should be ignored.
       */
      priv->is_open = FALSE;
      g_object_notify (G_OBJECT (data->device), "open");
    }

  /* TODO: Port/use the cancellation mechanism for device removal! */

  /* Return FP_DEVICE_ERROR_REMOVED if the device is removed,
   * with the exception of a successful open, which is an odd corner case. */
  if (priv->is_removed &&
      ((action != FPI_DEVICE_ACTION_OPEN) ||
       (action == FPI_DEVICE_ACTION_OPEN && data->type == FP_DEVICE_TASK_RETURN_ERROR)))
    {
      g_task_return_error (task, fpi_device_error_new (FP_DEVICE_ERROR_REMOVED));

      /* NOTE: The removed signal will be emitted from the GTask
       *       notify::completed if that is necessary. */

      return G_SOURCE_REMOVE;
    }

  switch (data->type)
    {
    case FP_DEVICE_TASK_RETURN_INT:
      g_task_return_int (task, GPOINTER_TO_INT (data->result));
      break;

    case FP_DEVICE_TASK_RETURN_BOOL:
      g_task_return_boolean (task, GPOINTER_TO_UINT (data->result));
      break;

    case FP_DEVICE_TASK_RETURN_OBJECT:
      g_task_return_pointer (task, g_steal_pointer (&data->result),
                             g_object_unref);
      break;

    case FP_DEVICE_TASK_RETURN_PTR_ARRAY:
      g_task_return_pointer (task, g_steal_pointer (&data->result),
                             (GDestroyNotify) g_ptr_array_unref);
      break;

    case FP_DEVICE_TASK_RETURN_ERROR:
      /* Return internal cancellation reason instead if we have one.
       * Note that an external cancellation always returns G_IO_ERROR_CANCELLED
       */
      if (cancellation_reason)
        {
          g_task_set_task_data (task, NULL, NULL);
          g_task_return_error (task, g_steal_pointer (&cancellation_reason));
        }
      else
        {
          g_task_return_error (task, g_steal_pointer (&data->result));
        }
      break;

    default:
      g_assert_not_reached ();
    }

  return G_SOURCE_REMOVE;
}

static void
fpi_device_task_return_data_free (FpDeviceTaskReturnData *data)
{
  if (data->result)
    {
      switch (data->type)
        {
        case FP_DEVICE_TASK_RETURN_INT:
        case FP_DEVICE_TASK_RETURN_BOOL:
          break;

        case FP_DEVICE_TASK_RETURN_OBJECT:
          g_clear_object ((GObject **) &data->result);
          break;

        case FP_DEVICE_TASK_RETURN_PTR_ARRAY:
          g_clear_pointer ((GPtrArray **) &data->result, g_ptr_array_unref);
          break;

        case FP_DEVICE_TASK_RETURN_ERROR:
          g_clear_error ((GError **) &data->result);
          break;

        default:
          g_assert_not_reached ();
        }
    }
  g_object_unref (data->device);
  g_free (data);
}

static void
fpi_device_return_task_in_idle (FpDevice              *device,
                                FpDeviceTaskReturnType return_type,
                                gpointer               return_data)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpDeviceTaskReturnData *data;

  data = g_new0 (FpDeviceTaskReturnData, 1);
  data->device = g_object_ref (device);
  data->type = return_type;
  data->result = return_data;

  priv->current_task_idle_return_source = g_idle_source_new ();
  g_source_set_priority (priv->current_task_idle_return_source,
                         g_task_get_priority (priv->current_task));
  g_source_set_callback (priv->current_task_idle_return_source,
                         fp_device_task_return_in_idle_cb,
                         data,
                         (GDestroyNotify) fpi_device_task_return_data_free);

  g_source_attach (priv->current_task_idle_return_source,
                   g_task_get_context (priv->current_task));
  g_source_unref (priv->current_task_idle_return_source);
}

/**
 * fpi_device_probe_complete:
 * @device: The #FpDevice
 * @device_id: Unique ID for the device or %NULL
 * @device_name: Human readable name or %NULL for driver name
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing probe operation. If error is %NULL success is assumed.
 */
void
fpi_device_probe_complete (FpDevice    *device,
                           const gchar *device_id,
                           const gchar *device_name,
                           GError      *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_PROBE);

  g_debug ("Device reported probe completion");

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (!error)
    {
      if (device_id)
        {
          g_clear_pointer (&priv->device_id, g_free);
          priv->device_id = g_strdup (device_id);
          g_object_notify (G_OBJECT (device), "device-id");
        }
      if (device_name)
        {
          g_clear_pointer (&priv->device_name, g_free);
          priv->device_name = g_strdup (device_name);
          g_object_notify (G_OBJECT (device), "name");
        }
      fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_BOOL,
                                      GUINT_TO_POINTER (TRUE));
    }
  else
    {
      fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
    }
}

/**
 * fpi_device_open_complete:
 * @device: The #FpDevice
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing open operation. If error is %NULL success is assumed.
 */
void
fpi_device_open_complete (FpDevice *device, GError *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_OPEN);

  g_debug ("Device reported open completion");

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (!error)
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_BOOL,
                                    GUINT_TO_POINTER (TRUE));
  else
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
}

/**
 * fpi_device_close_complete:
 * @device: The #FpDevice
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing close operation. If error is %NULL success is assumed.
 */
void
fpi_device_close_complete (FpDevice *device, GError *error)
{
  GError *nested_error = NULL;
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_CLOSE);

  g_debug ("Device reported close completion");

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  switch (priv->type)
    {
    case FP_DEVICE_TYPE_USB:
      if (!g_usb_device_close (priv->usb_device, &nested_error))
        {
          if (error == NULL)
            error = nested_error;
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
          return;
        }
      break;

    case FP_DEVICE_TYPE_VIRTUAL:
    case FP_DEVICE_TYPE_UDEV:
      break;

    default:
      g_assert_not_reached ();
      fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR,
                                      fpi_device_error_new (FP_DEVICE_ERROR_GENERAL));
      return;
    }

  if (!error)
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_BOOL,
                                    GUINT_TO_POINTER (TRUE));
  else
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
}

/**
 * fpi_device_enroll_complete:
 * @device: The #FpDevice
 * @print: (nullable) (transfer full): The #FpPrint or %NULL on failure
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing enroll operation. The #FpPrint can be stored by the
 * caller for later verification.
 */
void
fpi_device_enroll_complete (FpDevice *device, FpPrint *print, GError *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_ENROLL);

  g_debug ("Device reported enroll completion");

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (!error)
    {
      if (FP_IS_PRINT (print))
        {
          FpiPrintType print_type;
          g_autofree char *finger_str = NULL;

          g_object_get (print, "fpi-type", &print_type, NULL);
          if (print_type == FPI_PRINT_UNDEFINED)
            {
              g_warning ("Driver did not set the type on the returned print!");
              g_clear_object (&print);

              error = fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                                "Driver provided incorrect print data!");
              fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
              return;
            }

          finger_str = g_enum_to_string (FP_TYPE_FINGER, fp_print_get_finger (print));
          g_debug ("Print for finger %s enrolled", finger_str);

          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_OBJECT, print);
        }
      else
        {
          g_warning ("Driver did not provide a valid print and failed to provide an error!");
          error = fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                            "Driver failed to provide print data!");
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
        }
    }
  else
    {
      fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
      if (FP_IS_PRINT (print))
        {
          g_warning ("Driver passed an error but also provided a print, returning error!");
          g_object_unref (print);
        }
    }
}

/**
 * fpi_device_verify_complete:
 * @device: The #FpDevice
 * @error: A #GError if result is %FPI_MATCH_ERROR
 *
 * Finish an ongoing verify operation.
 *
 * Note that @error should only be set for actual errors. In the case
 * of retry errors, report these using fpi_device_verify_report()
 * and then call this function without any error argument.
 *
 * If @error is not set, we expect that a result (and print, in case)
 * have been already reported via fpi_device_verify_report().
 */
void
fpi_device_verify_complete (FpDevice *device,
                            GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpMatchData *data;

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_VERIFY);

  g_debug ("Device reported verify completion");

  data = g_task_get_task_data (priv->current_task);

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (!error)
    {
      if (!data->result_reported)
        {
          g_warning ("Driver reported successful verify complete but did not report the result earlier. Reporting error instead");
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR,
                                          fpi_device_error_new (FP_DEVICE_ERROR_GENERAL));
        }
      else if (data->error)
        {
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, g_steal_pointer (&data->error));
        }
      else
        {
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_INT,
                                          GINT_TO_POINTER (data->match != NULL ? FPI_MATCH_SUCCESS : FPI_MATCH_FAIL));
        }
    }
  else
    {
      /* Replace a retry error with a general error, this is a driver bug. */
      if (error->domain == FP_DEVICE_RETRY)
        {
          g_warning ("Driver reported a retry error to fpi_device_verify_complete. "
                     "This is not permissible and needs to be reported using "
                     "fpi_device_verify_report, reporting general verification "
                     "failure instead.");
          g_clear_error (&error);
          error = fpi_device_error_new (FP_DEVICE_ERROR_GENERAL);
        }
      fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
    }
}

/**
 * fpi_device_identify_complete:
 * @device: The #FpDevice
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing identify operation.
 *
 * Note that @error should only be set for actual errors. In the case
 * of retry errors, report these using fpi_device_identify_report()
 * and then call this function without any error argument.
 *
 * If @error is not set, we expect that a match and / or a print have been
 * already reported via fpi_device_identify_report()
 */
void
fpi_device_identify_complete (FpDevice *device,
                              GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpMatchData *data;

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_IDENTIFY);

  g_debug ("Device reported identify completion");

  data = g_task_get_task_data (priv->current_task);

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (!error)
    {
      if (!data->result_reported)
        {
          g_warning ("Driver reported successful identify complete but did not report the result earlier. Reporting error instead");
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR,
                                          fpi_device_error_new (FP_DEVICE_ERROR_GENERAL));
        }
      else if (data->error)
        {
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, g_steal_pointer (&data->error));
        }
      else
        {
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_BOOL, GUINT_TO_POINTER (TRUE));
        }
    }
  else
    {
      /* Replace a retry error with a general error, this is a driver bug. */
      if (error->domain == FP_DEVICE_RETRY)
        {
          g_warning ("Driver reported a retry error to fpi_device_identify_complete. "
                     "This is not permissible and needs to be reported using "
                     "fpi_device_identify_report, reporting general identification "
                     "failure instead.");
          g_clear_error (&error);
          error = fpi_device_error_new (FP_DEVICE_ERROR_GENERAL);
        }
      fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
    }
}


/**
 * fpi_device_capture_complete:
 * @device: The #FpDevice
 * @image: The #FpImage, or %NULL on error
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing capture operation.
 */
void
fpi_device_capture_complete (FpDevice *device,
                             FpImage  *image,
                             GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_CAPTURE);

  g_debug ("Device reported capture completion");

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (!error)
    {
      if (image)
        {
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_OBJECT, image);
        }
      else
        {
          g_warning ("Driver did not provide an error for a failed capture operation!");
          error = fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                            "Driver failed to provide an error!");
          fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
        }
    }
  else
    {
      fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
      if (image)
        {
          g_warning ("Driver passed an error but also provided an image, returning error!");
          g_clear_object (&image);
        }
    }
}

/**
 * fpi_device_delete_complete:
 * @device: The #FpDevice
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing delete operation.
 */
void
fpi_device_delete_complete (FpDevice *device,
                            GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_DELETE);

  g_debug ("Device reported deletion completion");

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (!error)
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_BOOL,
                                    GUINT_TO_POINTER (TRUE));
  else
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
}

/**
 * fpi_device_list_complete:
 * @device: The #FpDevice
 * @prints: (element-type FpPrint) (transfer container): Possibly empty array of prints or %NULL on error
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing list operation.
 *
 * Please note that the @prints array will be free'ed using
 * g_ptr_array_unref() and the elements are destroyed automatically.
 * As such, you must use g_ptr_array_new_with_free_func() with
 * g_object_unref() as free func to create the array.
 */
void
fpi_device_list_complete (FpDevice  *device,
                          GPtrArray *prints,
                          GError    *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_LIST);

  g_debug ("Device reported listing completion");

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (prints && error)
    {
      g_warning ("Driver reported back prints and error, ignoring prints");
      g_clear_pointer (&prints, g_ptr_array_unref);
    }
  else if (!prints && !error)
    {
      g_warning ("Driver did not pass array but failed to provide an error");
      error = fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                        "Driver failed to provide a list of prints");
    }

  if (!error)
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_PTR_ARRAY, prints);
  else
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
}

static int
update_attr (const char *attr, const char *value)
{
  int fd, err;
  gssize r;
  char buf[50] = { 0 };

  fd = open (attr, O_RDONLY);
  err = -errno;
  if (fd < 0)
    return -err;

  r = read (fd, buf, sizeof (buf) - 1);
  err = errno;
  close (fd);
  if (r < 0)
    return -err;

  g_strchomp (buf);
  if (g_strcmp0 (buf, value) == 0)
    return 0;

  /* O_TRUNC makes things work in the umockdev environment */
  fd = open (attr, O_WRONLY | O_TRUNC);
  err = errno;
  if (fd < 0)
    return -err;

  r = write (fd, value, strlen (value));
  err = -errno;
  close (fd);
  if (r < 0)
    {
      /* Write failures are weird, and are worth a warning */
      g_warning ("Could not write %s to %s", value, attr);
      return -err;
    }

  return 0;
}

static void
complete_suspend_resume_task (FpDevice *device)
{
  g_autoptr(GTask) task = NULL;
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_assert (priv->suspend_resume_task);
  task = g_steal_pointer (&priv->suspend_resume_task);

  g_task_return_boolean (task, TRUE);
}

void
fpi_device_suspend (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  /* If the device is currently idle, just complete immediately.
   * For long running tasks, call the driver handler right away, for short
   * tasks, wait for completion and then return the task.
   */
  switch (priv->current_action)
    {
    case FPI_DEVICE_ACTION_NONE:
      fpi_device_suspend_complete (device, NULL);
      break;

    case FPI_DEVICE_ACTION_ENROLL:
    case FPI_DEVICE_ACTION_VERIFY:
    case FPI_DEVICE_ACTION_IDENTIFY:
    case FPI_DEVICE_ACTION_CAPTURE:
      if (FP_DEVICE_GET_CLASS (device)->suspend)
        {
          if (priv->critical_section)
            priv->suspend_queued = TRUE;
          else
            FP_DEVICE_GET_CLASS (device)->suspend (device);
        }
      else
        {
          fpi_device_suspend_complete (device, fpi_device_error_new (FP_DEVICE_ERROR_NOT_SUPPORTED));
        }
      break;

    default:
    case FPI_DEVICE_ACTION_PROBE:
    case FPI_DEVICE_ACTION_OPEN:
    case FPI_DEVICE_ACTION_CLOSE:
    case FPI_DEVICE_ACTION_DELETE:
    case FPI_DEVICE_ACTION_LIST:
    case FPI_DEVICE_ACTION_CLEAR_STORAGE:
      g_signal_connect_object (priv->current_task,
                               "notify::completed",
                               G_CALLBACK (complete_suspend_resume_task),
                               device,
                               G_CONNECT_SWAPPED);

      break;
    }
}

void
fpi_device_resume (FpDevice *device)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  switch (priv->current_action)
    {
    case FPI_DEVICE_ACTION_NONE:
      fpi_device_resume_complete (device, NULL);
      break;

    case FPI_DEVICE_ACTION_ENROLL:
    case FPI_DEVICE_ACTION_VERIFY:
    case FPI_DEVICE_ACTION_IDENTIFY:
    case FPI_DEVICE_ACTION_CAPTURE:
      if (FP_DEVICE_GET_CLASS (device)->resume)
        {
          if (priv->critical_section)
            priv->resume_queued = TRUE;
          else
            FP_DEVICE_GET_CLASS (device)->resume (device);
        }
      else
        {
          fpi_device_resume_complete (device, fpi_device_error_new (FP_DEVICE_ERROR_NOT_SUPPORTED));
        }
      break;

    default:
    case FPI_DEVICE_ACTION_PROBE:
    case FPI_DEVICE_ACTION_OPEN:
    case FPI_DEVICE_ACTION_CLOSE:
    case FPI_DEVICE_ACTION_DELETE:
    case FPI_DEVICE_ACTION_LIST:
    case FPI_DEVICE_ACTION_CLEAR_STORAGE:
      /* cannot happen as we make sure these tasks complete before suspend */
      g_assert_not_reached ();
      complete_suspend_resume_task (device);
      break;
    }
}

void
fpi_device_configure_wakeup (FpDevice *device, gboolean enabled)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  switch (priv->type)
    {
    case FP_DEVICE_TYPE_USB:
      {
        g_autoptr(GString) ports = NULL;
        g_autoptr(GUsbDevice) dev = NULL;
        const char *wakeup_command = enabled ? "enabled" : "disabled";
        guint8 bus;
        g_autofree gchar *sysfs_wakeup = NULL;
        g_autofree gchar *sysfs_persist = NULL;
        int res;

        ports = g_string_new (NULL);
        bus = g_usb_device_get_bus (priv->usb_device);

        /* Walk up, skipping the root hub. */
        g_set_object (&dev, priv->usb_device);
        while (TRUE)
          {
            g_autoptr(GUsbDevice) parent = g_usb_device_get_parent (dev);
            g_autofree gchar *port_str = NULL;
            guint8 port;

            if (!parent)
              break;

            port = g_usb_device_get_port_number (dev);
            port_str = g_strdup_printf ("%d.", port);
            g_string_prepend (ports, port_str);
            g_set_object (&dev, parent);
          }
        g_string_set_size (ports, ports->len - 1);

        sysfs_wakeup = g_strdup_printf ("/sys/bus/usb/devices/%d-%s/power/wakeup", bus, ports->str);
        res = update_attr (sysfs_wakeup, wakeup_command);
        if (res < 0)
          g_debug ("Failed to set %s to %s", sysfs_wakeup, wakeup_command);

        /* Persist means that the kernel tries to keep the USB device open
         * in case it is "replugged" due to suspend.
         * This is not helpful, as it will receive a reset and will be in a bad
         * state. Instead, seeing an unplug and a new device makes more sense.
         */
        sysfs_persist = g_strdup_printf ("/sys/bus/usb/devices/%d-%s/power/persist", bus, ports->str);
        res = update_attr (sysfs_persist, "0");
        if (res < 0)
          g_warning ("Failed to disable USB persist by writing to %s", sysfs_persist);

        break;
      }

    case FP_DEVICE_TYPE_VIRTUAL:
    case FP_DEVICE_TYPE_UDEV:
      break;

    default:
      g_assert_not_reached ();
      fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR,
                                      fpi_device_error_new (FP_DEVICE_ERROR_GENERAL));
      return;
    }
}

static void
fpi_device_suspend_completed (FpDevice *device)
{
  g_autoptr(GTask) task = NULL;
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  /* We have an ongoing operation, allow the device to wake up the machine. */
  if (priv->current_action != FPI_DEVICE_ACTION_NONE)
    fpi_device_configure_wakeup (device, TRUE);

  if (priv->critical_section)
    g_warning ("Driver was in a critical section at suspend time. It likely deadlocked!");

  task = g_steal_pointer (&priv->suspend_resume_task);

  if (priv->suspend_error)
    g_task_return_error (task, g_steal_pointer (&priv->suspend_error));
  else
    g_task_return_boolean (task, TRUE);
}

/**
 * fpi_device_suspend_complete:
 * @device: The #FpDevice
 * @error: The #GError or %NULL on success
 *
 * Finish a suspend request. Only return a %NULL error if suspend has been
 * correctly configured and the current action as returned by
 * fpi_device_get_current_action() will continue to run after resume.
 *
 * In all other cases an error must be returned. Should this happen, the
 * current action will be cancelled before the error is forwarded to the
 * application.
 *
 * It is recommended to set @error to #FP_ERROR_NOT_IMPLEMENTED.
 */
void
fpi_device_suspend_complete (FpDevice *device,
                             GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->suspend_resume_task);
  g_return_if_fail (priv->suspend_error == NULL);

  priv->suspend_error = g_steal_pointer (&error);
  priv->is_suspended = TRUE;

  /* If there is no error, we have no running task, return immediately. */
  if (!priv->suspend_error || !priv->current_task ||
      g_task_get_completed (priv->current_task))
    {
      fpi_device_suspend_completed (device);
      return;
    }

  /* Wait for completion of the current task. */
  g_signal_connect_object (priv->current_task,
                           "notify::completed",
                           G_CALLBACK (fpi_device_suspend_completed),
                           device,
                           G_CONNECT_SWAPPED);

  /* And cancel any action that might be long-running. */
  if (!priv->current_cancellation_reason)
    priv->current_cancellation_reason = fpi_device_error_new_msg (FP_DEVICE_ERROR_BUSY,
                                                                  "Cannot run while suspended.");

  g_cancellable_cancel (priv->current_cancellable);
}

/**
 * fpi_device_resume_complete:
 * @device: The #FpDevice
 * @error: The #GError or %NULL on success
 *
 * Finish a resume request.
 */
void
fpi_device_resume_complete (FpDevice *device,
                            GError   *error)
{
  g_autoptr(GTask) task = NULL;
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->suspend_resume_task);

  priv->is_suspended = FALSE;
  fpi_device_configure_wakeup (device, FALSE);

  task = g_steal_pointer (&priv->suspend_resume_task);

  if (error)
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
}

/**
 * fpi_device_clear_storage_complete:
 * @device: The #FpDevice
 * @error: The #GError or %NULL on success
 *
 * Finish an ongoing clear_storage operation.
 */
void
fpi_device_clear_storage_complete (FpDevice *device,
                                   GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_CLEAR_STORAGE);

  g_debug ("Device reported deletion completion");

  clear_device_cancel_action (device);
  fpi_device_report_finger_status (device, FP_FINGER_STATUS_NONE);

  if (!error)
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_BOOL,
                                    GUINT_TO_POINTER (TRUE));
  else
    fpi_device_return_task_in_idle (device, FP_DEVICE_TASK_RETURN_ERROR, error);
}

/**

 * fpi_device_enroll_progress:
 * @device: The #FpDevice
 * @completed_stages: The number of stages that are completed at this point
 * @print: (transfer floating): The #FpPrint for the newly completed stage or %NULL on failure
 * @error: (transfer full): The #GError or %NULL on success
 *
 * Notify about the progress of the enroll operation. This is important for UI interaction.
 * The passed error may be used if a scan needs to be retried, use fpi_device_retry_new().
 */
void
fpi_device_enroll_progress (FpDevice *device,
                            gint      completed_stages,
                            FpPrint  *print,
                            GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpEnrollData *data;

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_ENROLL);
  g_return_if_fail (error == NULL || error->domain == FP_DEVICE_RETRY);

  g_debug ("Device reported enroll progress, reported %i of %i have been completed", completed_stages, priv->nr_enroll_stages);

  if (print)
    g_object_ref_sink (print);

  if (error && print)
    {
      g_warning ("Driver passed an error and also provided a print, returning error!");
      g_clear_object (&print);
    }

  data = g_task_get_task_data (priv->current_task);

  if (data->enroll_progress_cb)
    {
      data->enroll_progress_cb (device,
                                completed_stages,
                                print,
                                data->enroll_progress_data,
                                error);
    }

  g_clear_error (&error);
  g_clear_object (&print);
}

/**
 * fpi_device_verify_report:
 * @device: The #FpDevice
 * @result: The #FpiMatchResult of the operation
 * @print: (transfer floating) The scanned #FpPrint
 * @error: A #GError if result is %FPI_MATCH_ERROR
 *
 * Report the result of a verify operation. Note that the passed @error must be
 * a retry error with the %FP_DEVICE_RETRY domain. For all other error cases,
 * the error should passed to fpi_device_verify_complete().
 */
void
fpi_device_verify_report (FpDevice      *device,
                          FpiMatchResult result,
                          FpPrint       *print,
                          GError        *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpMatchData *data = g_task_get_task_data (priv->current_task);
  gboolean call_cb = TRUE;

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_VERIFY);
  g_return_if_fail (data->result_reported == FALSE);

  data->result_reported = TRUE;

  g_debug ("Device reported verify result");

  if (print)
    print = g_object_ref_sink (print);

  if (error || result == FPI_MATCH_ERROR)
    {
      if (result != FPI_MATCH_ERROR)
        g_warning ("Driver reported an error code without setting match result to error!");

      if (error == NULL)
        {
          g_warning ("Driver reported an error without specifying a retry code, assuming general retry error!");
          error = fpi_device_retry_new (FP_DEVICE_RETRY_GENERAL);
        }

      if (print)
        {
          g_warning ("Driver reported a print together with an error!");
          g_clear_object (&print);
        }

      data->error = error;

      if (error->domain != FP_DEVICE_RETRY)
        {
          g_warning ("Driver reported a verify error that was not in the retry domain, delaying report!");
          call_cb = FALSE;
        }
    }
  else
    {
      if (result == FPI_MATCH_SUCCESS)
        {
          fpi_device_get_verify_data (device, &data->match);
          g_object_ref (data->match);
        }

      data->print = g_steal_pointer (&print);
    }

  if (call_cb && data->match_cb)
    data->match_cb (device, data->match, data->print, data->match_data, data->error);
}

/**
 * fpi_device_identify_report:
 * @device: The #FpDevice
 * @match: (transfer none): The #FpPrint from the gallery that matched
 * @print: (transfer floating): The scanned #FpPrint, set in the absence
 *   of an error.
 * @error: A #GError of %FP_DEVICE_RETRY type if @match and @print are unset.
 *
 * Report the results of an identify operation.
 *
 * In case of successful identification @match is expected to be set to a
 * #FpPrint that matches one from the provided gallery, while @print
 * represents the scanned print and will be different.
 *
 * If there are no errors, it's expected that the device always reports the
 * recognized @print even if there is no @match with the provided gallery (that
 * can be potentially empty). This is required for application logic further
 * up in the stack, such as for enroll-duplicate checking. @print needs to be
 * sufficiently filled to do a comparison.
 *
 * In case of error, both @match and @print are expected to be %NULL.
 * Note that the passed @error must be a retry error from the %FP_DEVICE_RETRY
 * domain. For all other error cases, the error should passed to
 * fpi_device_identify_complete().
 */
void
fpi_device_identify_report (FpDevice *device,
                            FpPrint  *match,
                            FpPrint  *print,
                            GError   *error)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpMatchData *data = g_task_get_task_data (priv->current_task);
  gboolean call_cb = TRUE;

  g_return_if_fail (FP_IS_DEVICE (device));
  g_return_if_fail (priv->current_action == FPI_DEVICE_ACTION_IDENTIFY);
  g_return_if_fail (data->result_reported == FALSE);

  data->result_reported = TRUE;

  if (match)
    g_object_ref (match);

  if (print)
    print = g_object_ref_sink (print);

  if (match && !g_ptr_array_find (data->gallery, match, NULL))
    {
      g_warning ("Driver reported a match to a print that was not in the gallery, ignoring match.");
      g_clear_object (&match);
    }

  g_debug ("Device reported identify result");

  if (error)
    {
      if (match != NULL)
        {
          g_warning ("Driver reported an error code but also provided a match!");
          g_clear_object (&match);
        }

      if (print)
        {
          g_warning ("Driver reported a print together with an error!");
          g_clear_object (&print);
        }

      data->error = error;

      if (error->domain != FP_DEVICE_RETRY)
        {
          g_warning ("Driver reported a verify error that was not in the retry domain, delaying report!");
          call_cb = FALSE;
        }
    }
  else
    {
      if (match)
        data->match = g_steal_pointer (&match);

      if (print)
        data->print = g_steal_pointer (&print);
    }

  if (call_cb && data->match_cb)
    data->match_cb (device, data->match, data->print, data->match_data, data->error);
}

/**
 * fpi_device_report_finger_status:
 * @device: The #FpDevice
 * @finger_status: The current #FpFingerStatusFlags to report
 *
 * Report the finger status for the @device.
 * This can be used by UI to give a feedback
 *
 * Returns: %TRUE if changed
 */
gboolean
fpi_device_report_finger_status (FpDevice           *device,
                                 FpFingerStatusFlags finger_status)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  g_autofree char *status_string = NULL;

  if (priv->finger_status == finger_status)
    return FALSE;

  status_string = g_flags_to_string (FP_TYPE_FINGER_STATUS_FLAGS, finger_status);
  fp_dbg ("Device reported finger status change: %s", status_string);

  priv->finger_status = finger_status;
  g_object_notify (G_OBJECT (device), "finger-status");

  return TRUE;
}

/**
 * fpi_device_report_finger_status_changes:
 * @device: The #FpDevice
 * @added_status: The #FpFingerStatusFlags to add
 * @removed_status: The #FpFingerStatusFlags to remove
 *
 * Report the finger status for the @device adding the @added_status flags
 * and removing the @removed_status flags.
 *
 * This can be used by UI to give a feedback
 *
 * Returns: %TRUE if changed
 */
gboolean
fpi_device_report_finger_status_changes (FpDevice           *device,
                                         FpFingerStatusFlags added_status,
                                         FpFingerStatusFlags removed_status)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  FpFingerStatusFlags finger_status = priv->finger_status;

  finger_status |= added_status;
  finger_status &= ~removed_status;

  return fpi_device_report_finger_status (device, finger_status);
}

static void
update_temp_timeout (FpDevice *device, gpointer user_data)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);

  fpi_device_update_temp (device, priv->temp_last_active);
}

/**
 * fpi_device_update_temp:
 * @device: The #FpDevice
 * @is_active: Whether the device is now active
 *
 * Purely internal function to update the temperature. Also ensure that the
 * state is updated once a threshold is reached.
 */
void
fpi_device_update_temp (FpDevice *device, gboolean is_active)
{
  FpDevicePrivate *priv = fp_device_get_instance_private (device);
  gint64 now = g_get_monotonic_time ();
  gdouble passed_seconds;
  gdouble alpha;
  gdouble next_threshold;
  gdouble old_ratio;
  FpTemperature old_temp;
  g_autofree char *old_temp_str = NULL;
  g_autofree char *new_temp_str = NULL;

  if (priv->temp_hot_seconds < 0)
    {
      g_debug ("Not updating temperature model, device can run continuously!");
      return;
    }

  passed_seconds = (now - priv->temp_last_update) / 1e6;
  old_ratio = priv->temp_current_ratio;

  if (priv->temp_last_active)
    {
      alpha = exp (-passed_seconds / priv->temp_hot_seconds);
      priv->temp_current_ratio = alpha * priv->temp_current_ratio + 1 - alpha;
    }
  else
    {
      alpha = exp (-passed_seconds / priv->temp_cold_seconds);
      priv->temp_current_ratio = alpha * priv->temp_current_ratio;
    }

  priv->temp_last_active = is_active;
  priv->temp_last_update = now;

  old_temp = priv->temp_current;
  if (priv->temp_current_ratio < TEMP_COLD_THRESH)
    {
      priv->temp_current = FP_TEMPERATURE_COLD;
      next_threshold = is_active ? TEMP_COLD_THRESH : -1.0;
    }
  else if (priv->temp_current_ratio < TEMP_HOT_WARM_THRESH)
    {
      priv->temp_current = FP_TEMPERATURE_WARM;
      next_threshold = is_active ? TEMP_WARM_HOT_THRESH : TEMP_COLD_THRESH;
    }
  else if (priv->temp_current_ratio < TEMP_WARM_HOT_THRESH)
    {
      /* Keep HOT until we reach TEMP_HOT_WARM_THRESH */
      if (priv->temp_current != FP_TEMPERATURE_HOT)
        priv->temp_current = FP_TEMPERATURE_WARM;

      next_threshold = is_active ? TEMP_WARM_HOT_THRESH : TEMP_HOT_WARM_THRESH;
    }
  else
    {
      priv->temp_current = FP_TEMPERATURE_HOT;
      next_threshold = is_active ? -1.0 : TEMP_HOT_WARM_THRESH;
    }

  old_temp_str = g_enum_to_string (FP_TYPE_TEMPERATURE, old_temp);
  new_temp_str = g_enum_to_string (FP_TYPE_TEMPERATURE, priv->temp_current);
  g_debug ("Updated temperature model after %0.2f seconds, ratio %0.2f -> %0.2f, active %d -> %d, %s -> %s",
           passed_seconds,
           old_ratio,
           priv->temp_current_ratio,
           priv->temp_last_active,
           is_active,
           old_temp_str,
           new_temp_str);

  if (priv->temp_current != old_temp)
    g_object_notify (G_OBJECT (device), "temperature");

  /* If the device is HOT, then do an internal cancellation of long running tasks. */
  if (priv->temp_current == FP_TEMPERATURE_HOT)
    {
      if (priv->current_action == FPI_DEVICE_ACTION_ENROLL ||
          priv->current_action == FPI_DEVICE_ACTION_VERIFY ||
          priv->current_action == FPI_DEVICE_ACTION_IDENTIFY ||
          priv->current_action == FPI_DEVICE_ACTION_CAPTURE)
        {
          if (!priv->current_cancellation_reason)
            priv->current_cancellation_reason = fpi_device_error_new (FP_DEVICE_ERROR_TOO_HOT);

          g_cancellable_cancel (priv->current_cancellable);
        }
    }

  g_clear_pointer (&priv->temp_timeout, g_source_destroy);

  if (next_threshold < 0)
    return;

  /* Set passed_seconds to the time until the next update is needed */
  if (is_active)
    passed_seconds = -priv->temp_hot_seconds * log ((next_threshold - 1.0) / (priv->temp_current_ratio - 1.0));
  else
    passed_seconds = -priv->temp_cold_seconds * log (next_threshold / priv->temp_current_ratio);

  passed_seconds += TEMP_DELAY_SECONDS;

  priv->temp_timeout = fpi_device_add_timeout (device,
                                               passed_seconds * 1000,
                                               update_temp_timeout,
                                               NULL, NULL);
}
