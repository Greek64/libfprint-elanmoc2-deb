/*
 * FpImageDevice - An image based fingerprint reader device
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

#define FP_COMPONENT "image_device"
#include "fpi-log.h"

#include "fp-image-device-private.h"

#define BOZORTH3_DEFAULT_THRESHOLD 40

/**
 * SECTION: fp-image-device
 * @title: FpImageDevice
 * @short_description: Image device subclass
 *
 * This is a helper class for the commonly found image based devices.
 */

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (FpImageDevice, fp_image_device, FP_TYPE_DEVICE)

enum {
  PROP_0,
  PROP_FPI_STATE,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

enum {
  FPI_STATE_CHANGED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/*******************************************************/

/* TODO:
 * - sanitize_image seems a bit odd, in particular the sizing stuff.
 **/

/* Static helper functions */

static gboolean
pending_activation_timeout (gpointer user_data)
{
  FpImageDevice *self = FP_IMAGE_DEVICE (user_data);
  FpDevice *device = FP_DEVICE (self);
  FpImageDevicePrivate *priv = fp_image_device_get_instance_private (self);
  FpiDeviceAction action = fpi_device_get_current_action (device);
  GError *error;

  priv->pending_activation_timeout_id = 0;

  if (priv->pending_activation_timeout_waiting_finger_off)
    error = fpi_device_retry_new_msg (FP_DEVICE_RETRY_REMOVE_FINGER,
                                      "Remove finger before requesting another scan operation");
  else
    error = fpi_device_retry_new (FP_DEVICE_RETRY_GENERAL);

  if (action == FPI_DEVICE_ACTION_VERIFY)
    {
      fpi_device_verify_report (device, FPI_MATCH_ERROR, NULL, error);
      fpi_device_verify_complete (device, NULL);
    }
  else if (action == FPI_DEVICE_ACTION_IDENTIFY)
    {
      fpi_device_identify_report (device, NULL, NULL, error);
      fpi_device_identify_complete (device, NULL);
    }
  else
    {
      /* Can this happen for enroll? */
      fpi_device_action_error (device, error);
    }

  return G_SOURCE_REMOVE;
}

/* Callbacks/vfuncs */
static void
fp_image_device_open (FpDevice *device)
{
  FpImageDeviceClass *cls = FP_IMAGE_DEVICE_GET_CLASS (device);

  /* Nothing special about opening an image device, just
   * forward the request. */
  cls->img_open (FP_IMAGE_DEVICE (device));
}

static void
fp_image_device_close (FpDevice *device)
{
  FpImageDevice *self = FP_IMAGE_DEVICE (device);
  FpImageDeviceClass *cls = FP_IMAGE_DEVICE_GET_CLASS (self);
  FpImageDevicePrivate *priv = fp_image_device_get_instance_private (self);

  /* In the close case we may need to wait/force deactivation first.
   * Three possible cases:
   *  1. We are inactive
   *     -> immediately close
   *  2. We are waiting for finger off
   *     -> immediately deactivate
   *  3. We are deactivating
   *     -> handled by deactivate_complete */

  if (!priv->active)
    cls->img_close (self);
  else if (priv->state != FPI_IMAGE_DEVICE_STATE_INACTIVE)
    fpi_image_device_deactivate (self);
}

static void
fp_image_device_cancel_action (FpDevice *device)
{
  FpImageDevice *self = FP_IMAGE_DEVICE (device);
  FpImageDevicePrivate *priv = fp_image_device_get_instance_private (self);
  FpiDeviceAction action;

  action = fpi_device_get_current_action (device);

  /* We can only cancel capture operations, in that case, deactivate and return
   * an error immediately. */
  if (action == FPI_DEVICE_ACTION_ENROLL ||
      action == FPI_DEVICE_ACTION_VERIFY ||
      action == FPI_DEVICE_ACTION_IDENTIFY ||
      action == FPI_DEVICE_ACTION_CAPTURE)
    {
      priv->cancelling = TRUE;
      fpi_image_device_deactivate (self);
      priv->cancelling = FALSE;

      /* XXX: Some nicer way of doing this would be good. */
      fpi_device_action_error (FP_DEVICE (self),
                               g_error_new (G_IO_ERROR,
                                            G_IO_ERROR_CANCELLED,
                                            "Device operation was cancelled"));
    }
}

static void
fp_image_device_start_capture_action (FpDevice *device)
{
  FpImageDevice *self = FP_IMAGE_DEVICE (device);
  FpImageDevicePrivate *priv = fp_image_device_get_instance_private (self);
  FpiDeviceAction action;

  /* There is just one action that we cannot support out
   * of the box, which is a capture without first waiting
   * for a finger to be on the device.
   */
  action = fpi_device_get_current_action (device);
  if (action == FPI_DEVICE_ACTION_CAPTURE)
    {
      gboolean wait_for_finger;

      fpi_device_get_capture_data (device, &wait_for_finger);

      if (!wait_for_finger)
        {
          fpi_device_action_error (device, fpi_device_error_new (FP_DEVICE_ERROR_NOT_SUPPORTED));
          return;
        }
    }
  else if (action == FPI_DEVICE_ACTION_ENROLL)
    {
      FpPrint *enroll_print = NULL;

      fpi_device_get_enroll_data (device, &enroll_print);
      fpi_print_set_type (enroll_print, FPI_PRINT_NBIS);
    }

  priv->enroll_stage = 0;
  priv->enroll_await_on_pending = FALSE;

  /* The device might still be deactivating from a previous call.
   * In that situation, try to wait for a bit before reporting back an
   * error (which will usually say that the user should remove the
   * finger).
   */
  if (priv->state != FPI_IMAGE_DEVICE_STATE_INACTIVE || priv->active)
    {
      g_debug ("Got a new request while the device was still active");
      g_assert (priv->pending_activation_timeout_id == 0);
      priv->pending_activation_timeout_id =
        g_timeout_add (100, pending_activation_timeout, device);

      if (priv->state == FPI_IMAGE_DEVICE_STATE_AWAIT_FINGER_OFF)
        priv->pending_activation_timeout_waiting_finger_off = TRUE;
      else
        priv->pending_activation_timeout_waiting_finger_off = FALSE;

      return;
    }

  /* And activate the device; we rely on fpi_image_device_activate_complete()
   * to be called when done (or immediately). */
  fpi_image_device_activate (self);
}


/*********************************************************/

static void
fp_image_device_finalize (GObject *object)
{
  FpImageDevice *self = (FpImageDevice *) object;
  FpImageDevicePrivate *priv = fp_image_device_get_instance_private (self);

  g_assert (priv->active == FALSE);
  g_clear_handle_id (&priv->pending_activation_timeout_id, g_source_remove);

  G_OBJECT_CLASS (fp_image_device_parent_class)->finalize (object);
}

static void
fp_image_device_default_activate (FpImageDevice *self)
{
  fpi_image_device_activate_complete (self, NULL);
}

static void
fp_image_device_default_deactivate (FpImageDevice *self)
{
  fpi_image_device_deactivate_complete (self, NULL);
}

static void
fp_image_device_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  FpImageDevice *self = FP_IMAGE_DEVICE (object);
  FpImageDevicePrivate *priv = fp_image_device_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_FPI_STATE:
      g_value_set_enum (value, priv->state);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
fp_image_device_constructed (GObject *obj)
{
  FpImageDevice *self = FP_IMAGE_DEVICE (obj);
  FpImageDevicePrivate *priv = fp_image_device_get_instance_private (self);
  FpImageDeviceClass *cls = FP_IMAGE_DEVICE_GET_CLASS (self);

  /* Set default values. */
  fpi_device_set_nr_enroll_stages (FP_DEVICE (self), IMG_ENROLL_STAGES);

  priv->bz3_threshold = BOZORTH3_DEFAULT_THRESHOLD;
  if (cls->bz3_threshold > 0)
    priv->bz3_threshold = cls->bz3_threshold;

  G_OBJECT_CLASS (fp_image_device_parent_class)->constructed (obj);
}

static void
fp_image_device_class_init (FpImageDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  FpDeviceClass *fp_device_class = FP_DEVICE_CLASS (klass);

  object_class->finalize = fp_image_device_finalize;
  object_class->get_property = fp_image_device_get_property;
  object_class->constructed = fp_image_device_constructed;

  fp_device_class->open = fp_image_device_open;
  fp_device_class->close = fp_image_device_close;
  fp_device_class->enroll = fp_image_device_start_capture_action;
  fp_device_class->verify = fp_image_device_start_capture_action;
  fp_device_class->identify = fp_image_device_start_capture_action;
  fp_device_class->capture = fp_image_device_start_capture_action;

  fp_device_class->cancel = fp_image_device_cancel_action;

  /* Default implementations */
  klass->activate = fp_image_device_default_activate;
  klass->deactivate = fp_image_device_default_deactivate;

  /**
   * FpImageDevice::fpi-image-device-state: (skip)
   *
   * This property is only for internal purposes.
   *
   * Stability: private
   */
  properties[PROP_FPI_STATE] =
    g_param_spec_enum ("fpi-image-device-state",
                       "Image Device State",
                       "Private: The state of the image device",
                       FPI_TYPE_IMAGE_DEVICE_STATE,
                       FPI_IMAGE_DEVICE_STATE_INACTIVE,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READABLE);

  /**
   * FpImageDevice::fpi-image-device-state-changed: (skip)
   * @image_device: A #FpImageDevice
   * @new_state: The new state of the device
   *
   * This signal is only for internal purposes.
   *
   * Stability: private
   */
  signals[FPI_STATE_CHANGED] =
    g_signal_new ("fpi-image-device-state-changed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (FpImageDeviceClass, change_state),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, FPI_TYPE_IMAGE_DEVICE_STATE);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
fp_image_device_init (FpImageDevice *self)
{
}
