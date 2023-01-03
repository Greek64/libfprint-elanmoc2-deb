/*
 * Shared library loader for libfprint
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

#include "tod-wrappers.h"
#include "fpi-ssm.h"

static gboolean
check_delayed_cancellable (FpiSsm       *machine,
                           GCancellable *cancellable)
{
  if (g_cancellable_is_cancelled (cancellable))
    {
      fpi_ssm_mark_failed (machine, g_error_new (G_IO_ERROR,
                                                 G_IO_ERROR_CANCELLED,
                                                 "Action cancelled"));
      return FALSE;
    }

  if (cancellable)
    fp_err ("GCancellable in SSM Delayed actions is ignored as per libfprint 1.92");

  return TRUE;
}

void
fpi_ssm_next_state_delayed_1_90 (FpiSsm       *machine,
                                 int           delay,
                                 GCancellable *cancellable)
{
  if (check_delayed_cancellable (machine, cancellable))
    fpi_ssm_next_state_delayed (machine, delay);
}

void
fpi_ssm_jump_to_state_delayed_1_90 (FpiSsm       *machine,
                                    int           state,
                                    int           delay,
                                    GCancellable *cancellable)
{
  if (check_delayed_cancellable (machine, cancellable))
    fpi_ssm_jump_to_state_delayed (machine, state, delay);
}

void
fpi_ssm_mark_completed_delayed_1_90 (FpiSsm       *machine,
                                     int           delay,
                                     GCancellable *cancellable)
{
  if (check_delayed_cancellable (machine, cancellable))
    fpi_ssm_mark_completed_delayed (machine, delay);
}
