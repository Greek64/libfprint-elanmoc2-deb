/*
 * FpImageDevice - An image based fingerprint reader device
 * Copyright (C) 2021 Marco Trevisan <marco.trevisan@canonical.com>
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

#pragma once

#include "base-fpi-device.h"

typedef struct _FpImageDevice FpImageDevice;

typedef enum {
  FPI_IMAGE_DEVICE_STATE_TODV1_90_1_INACTIVE,
  FPI_IMAGE_DEVICE_STATE_TODV1_90_1_AWAIT_FINGER_ON,
  FPI_IMAGE_DEVICE_STATE_TODV1_90_1_CAPTURE,
  FPI_IMAGE_DEVICE_STATE_TODV1_90_1_AWAIT_FINGER_OFF,
} FpiImageDeviceStateTODV1_90_1;

typedef enum {
  FPI_IMAGE_DEVICE_STATE_TODV1_92_0_INACTIVE,
  FPI_IMAGE_DEVICE_STATE_TODV1_92_0_AWAIT_FINGER_ON,
  FPI_IMAGE_DEVICE_STATE_TODV1_92_0_CAPTURE,
  FPI_IMAGE_DEVICE_STATE_TODV1_92_0_AWAIT_FINGER_OFF,
  FPI_IMAGE_DEVICE_STATE_TODV1_92_0_ACTIVATING,
  FPI_IMAGE_DEVICE_STATE_TODV1_92_0_DEACTIVATING,
  FPI_IMAGE_DEVICE_STATE_TODV1_92_0_IDLE,
} FpiImageDeviceStateTODV1_90_4;

typedef struct _FpImageDeviceClassTODV1_90_1
{
  FpDeviceClassTODV1_90_1 parent_class;

  gint                    bz3_threshold;
  gint                    img_width;
  gint                    img_height;

  void (*img_open)(FpImageDevice *dev);
  void (*img_close)(FpImageDevice *dev);
  void (*activate)(FpImageDevice *dev);
  void (*change_state)(FpImageDevice                *dev,
                       FpiImageDeviceStateTODV1_90_1 state);
  void (*deactivate)(FpImageDevice *dev);

  /*< private >*/
  /* padding for future expansion */
  TOD_PADDING (32, 0);
} FpImageDeviceClassTODV1_90_1;
