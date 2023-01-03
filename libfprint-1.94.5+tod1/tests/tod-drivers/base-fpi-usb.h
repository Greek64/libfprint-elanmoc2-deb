/*
 * FpDevice - A fingerprint reader device
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

#include <gusb.h>

#include "base-fpi-device.h"

typedef struct _FpiUsbTransferTODV1_90_1 FpiUsbTransferTODV1_90_1;
typedef struct _FpiSsm                   FpiSsm;

typedef void (*FpiUsbTransferCallbackTODV1_90_1)(FpiUsbTransferTODV1_90_1 *transfer,
                                                 FpDevice                 *dev,
                                                 gpointer                  user_data,
                                                 GError                   *error);

typedef enum  {
  FP_TRANSFER_TODV1_90_1_NONE = -1,
  FP_TRANSFER_TODV1_90_1_CONTROL = 0,
  FP_TRANSFER_TODV1_90_1_BULK = 2,
  FP_TRANSFER_TODV1_90_1_INTERRUPT = 3,
} FpiTransferTypeTODV1_90_3;

struct _FpiUsbTransferTODV1_90_1
{
  /*< public >*/
  FpDevice *device;

  FpiSsm   *ssm;

  gssize    length;
  gssize    actual_length;

  guchar   *buffer;

  /*< private >*/
  guint ref_count;

  /* USB Transfer information */
  FpiTransferTypeTODV1_90_3 type;
  guint8                    endpoint;

  /* Control Transfer options */
  GUsbDeviceDirection   direction;
  GUsbDeviceRequestType request_type;
  GUsbDeviceRecipient   recipient;
  guint8                request;
  guint16               value;
  guint16               idx;

  /* Flags */
  gboolean short_is_error;

  /* Callbacks */
  gpointer                         user_data;
  FpiUsbTransferCallbackTODV1_90_1 callback;

  /* Data free function */
  GDestroyNotify free_buffer;

  /* padding for future expansion */
  TOD_PADDING (32, 0);
};
