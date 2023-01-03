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

#include <glib.h>
#include <glib-object.h>

#include "base-fp-device.h"

typedef struct _GUsbDevice          GUsbDevice;

typedef struct _FpIdEntryTODV1_90_1 FpIdEntryTODV1_90_1;

struct _FpIdEntryTODV1_90_1
{
  union
  {
    struct
    {
      guint pid;
      guint vid;
    };
    const gchar *virtual_envvar;
  };
  guint64 driver_data;

  /*< private >*/
  /* padding for future expansion */
  TOD_PADDING (16, 0);
};

struct _FpDeviceClassTODV1_90_1
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  /* Static information about the driver. */
  const gchar               *id;
  const gchar               *full_name;
  FpDeviceTypeTODV1_90_1     type;
  const FpIdEntryTODV1_90_1 *id_table;

  /* Defaults for device properties */
  gint                 nr_enroll_stages;
  FpScanTypeTODV1_90_1 scan_type;

  /* Callbacks */
  gint (*usb_discover) (GUsbDevice *usb_device);
  void (*probe)    (FpDevice *device);
  void (*open)     (FpDevice *device);
  void (*close)    (FpDevice *device);
  void (*enroll)   (FpDevice *device);
  void (*verify)   (FpDevice *device);
  void (*identify) (FpDevice *device);
  void (*capture)  (FpDevice *device);
  void (*list)     (FpDevice *device);
  void (*delete)   (FpDevice * device);

  void (*cancel)   (FpDevice *device);

  /*< private >*/
  /* padding for future expansion */
  TOD_PADDING (32, 0);
};

typedef struct _FpDeviceClassTODV1_90_1 FpDeviceClassTODV1_90_1;

typedef enum {
  FPI_DEVICE_ACTION_TODV1_90_1_NONE = 0,
  FPI_DEVICE_ACTION_TODV1_90_1_PROBE,
  FPI_DEVICE_ACTION_TODV1_90_1_OPEN,
  FPI_DEVICE_ACTION_TODV1_90_1_CLOSE,
  FPI_DEVICE_ACTION_TODV1_90_1_ENROLL,
  FPI_DEVICE_ACTION_TODV1_90_1_VERIFY,
  FPI_DEVICE_ACTION_TODV1_90_1_IDENTIFY,
  FPI_DEVICE_ACTION_TODV1_90_1_CAPTURE,
  FPI_DEVICE_ACTION_TODV1_90_1_LIST,
  FPI_DEVICE_ACTION_TODV1_90_1_DELETE,
} FpiDeviceActionTODV1_90_1;

typedef enum {
  FPI_DEVICE_ACTION_TODV1_92_0_NONE = 0,
  FPI_DEVICE_ACTION_TODV1_92_0_PROBE,
  FPI_DEVICE_ACTION_TODV1_92_0_OPEN,
  FPI_DEVICE_ACTION_TODV1_92_0_CLOSE,
  FPI_DEVICE_ACTION_TODV1_92_0_ENROLL,
  FPI_DEVICE_ACTION_TODV1_92_0_VERIFY,
  FPI_DEVICE_ACTION_TODV1_92_0_IDENTIFY,
  FPI_DEVICE_ACTION_TODV1_92_0_CAPTURE,
  FPI_DEVICE_ACTION_TODV1_92_0_LIST,
  FPI_DEVICE_ACTION_TODV1_92_0_DELETE,
  FPI_DEVICE_ACTION_TODV1_92_0_CLEAR_STORAGE
} FpiDeviceActionTODV1_92_0;

typedef enum {
  FPI_DEVICE_UDEV_SUBTYPE_TODV1_92_0_SPIDEV = 1 << 0,
  FPI_DEVICE_UDEV_SUBTYPE_TODV1_92_0_HIDRAW = 1 << 1,
} FpiDeviceUdevSubtypeFlagsTODV1_92_0;

typedef struct _FpIdEntryTODV1_92_0 FpIdEntryTODV1_92_0;

struct _FpIdEntryTODV1_92_0
{
  union
  {
    struct
    {
      guint pid;
      guint vid;
    };
    const gchar *virtual_envvar;
  };
  guint64 driver_data;

  /* Elements added after TODv1 */
  union
  {
    struct
    {
      FpiDeviceUdevSubtypeFlagsTODV1_92_0 udev_types;
      const gchar                        *spi_acpi_id;
      struct
      {
        guint pid;
        guint vid;
      } hid_id;
    };
  };

  /*< private >*/
  /* padding for future expansion */
  TOD_PADDING_ALIGNED (16, sizeof (guint) * 2 +
                       sizeof (FpiDeviceUdevSubtypeFlagsTODV1_92_0) +
                       sizeof (gpointer));
};

typedef struct _FpIdEntryTODV1_92_0 FpIdEntryTODV1_92_0;

struct _FpDeviceClassTODV1_92_0
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  /* Static information about the driver. */
  const gchar               *id;
  const gchar               *full_name;
  FpDeviceTypeTODV1_92_0     type;
  const FpIdEntryTODV1_92_0 *id_table;

  /* Defaults for device properties */
  gint                 nr_enroll_stages;
  FpScanTypeTODV1_90_1 scan_type;

  /* Callbacks */
  gint (*usb_discover) (GUsbDevice *usb_device);
  void (*probe)    (FpDevice *device);
  void (*open)     (FpDevice *device);
  void (*close)    (FpDevice *device);
  void (*enroll)   (FpDevice *device);
  void (*verify)   (FpDevice *device);
  void (*identify) (FpDevice *device);
  void (*capture)  (FpDevice *device);
  void (*list)     (FpDevice *device);
  void (*delete)   (FpDevice * device);

  void                      (*cancel)   (FpDevice *device);

  FpDeviceFeatureTODV1_92_0 features;

  /*< private >*/
  /* padding for future expansion */
  TOD_PADDING (32, sizeof (FpDeviceFeatureTODV1_92_0));
};

typedef struct _FpDeviceClassTODV1_92_0 FpDeviceClassTODV1_92_0;


struct _FpDeviceClassTODV1_94_0
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  /* Static information about the driver. */
  const gchar               *id;
  const gchar               *full_name;
  FpDeviceTypeTODV1_92_0     type;
  const FpIdEntryTODV1_92_0 *id_table;

  /* Defaults for device properties */
  gint                 nr_enroll_stages;
  FpScanTypeTODV1_90_1 scan_type;

  /* Callbacks */
  gint (*usb_discover) (GUsbDevice *usb_device);
  void (*probe)    (FpDevice *device);
  void (*open)     (FpDevice *device);
  void (*close)    (FpDevice *device);
  void (*enroll)   (FpDevice *device);
  void (*verify)   (FpDevice *device);
  void (*identify) (FpDevice *device);
  void (*capture)  (FpDevice *device);
  void (*list)     (FpDevice *device);
  void (*delete)   (FpDevice * device);

  void (*cancel)   (FpDevice *device);

  /* Class elements added after tod-v1 */
  FpDeviceFeatureTODV1_94_0 features;

  /* Simple device temperature model constants */
  gint32 temp_hot_seconds;
  gint32 temp_cold_seconds;

  void   (*clear_storage)  (FpDevice * device);
  void   (*suspend)  (FpDevice *device);
  void   (*resume)   (FpDevice *device);

  /*< private >*/
  /* padding for future expansion */
  TOD_PADDING_ALIGNED8 (32,
                        sizeof (FpDeviceFeatureTODV1_94_0) +
                        sizeof (gint32) * 2 +
                        sizeof (gpointer) * 3)
};

typedef struct _FpDeviceClassTODV1_94_0 FpDeviceClassTODV1_94_0;
