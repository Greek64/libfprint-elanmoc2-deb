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

#define FP_COMPONENT "tod"

#include "tod-wrappers.h"

FpiSsm *
fpi_ssm_new_full_1_90 (FpDevice             *dev,
                       FpiSsmHandlerCallback handler,
                       int                   nr_states,
                       const char           *machine_name)
{
  return fpi_ssm_new_full (dev, handler, nr_states, nr_states, machine_name);
}
