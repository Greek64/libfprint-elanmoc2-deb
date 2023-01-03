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

#include "base-fpi-device.h"

typedef struct _FpiSpiTransferTODV1_92_0 FpiSpiTransferTODV1_92_0;
typedef struct _FpiSsm                   FpiSsm;

typedef void (*FpiSpiTransferCallbackTODV1_92_0)(FpiSpiTransferTODV1_92_0 *transfer,
                                                 FpDevice                 *dev,
                                                 gpointer                  user_data,
                                                 GError                   *error);

struct _FpiSpiTransferTODV1_92_0
{
  /*< public >*/
  FpDevice *device;

  FpiSsm   *ssm;

  gssize    length_wr;
  gssize    length_rd;

  guchar   *buffer_wr;
  guchar   *buffer_rd;

  /*< private >*/
  guint ref_count;

  int   spidev_fd;

  /* Callbacks */
  gpointer                         user_data;
  FpiSpiTransferCallbackTODV1_92_0 callback;

  /* Data free function */
  GDestroyNotify free_buffer_wr;
  GDestroyNotify free_buffer_rd;

  /* padding for future expansion */
  TOD_PADDING (32, 0);
};
