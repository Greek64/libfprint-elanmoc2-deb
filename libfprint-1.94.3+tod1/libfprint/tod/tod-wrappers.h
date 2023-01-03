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

#pragma once

#include "drivers_api.h"
#include "tod-symbols.h"

extern FpiSsm *fpi_ssm_new_full_1_90 (FpDevice             *dev,
                                      FpiSsmHandlerCallback handler,
                                      int                   nr_states,
                                      const char           *machine_name);

extern void fpi_ssm_next_state_delayed_1_90 (FpiSsm       *machine,
                                             int           delay,
                                             GCancellable *cancellable);
extern void fpi_ssm_jump_to_state_delayed_1_90 (FpiSsm       *machine,
                                                int           state,
                                                int           delay,
                                                GCancellable *cancellable);
extern void fpi_ssm_mark_completed_delayed_1_90 (FpiSsm       *machine,
                                                 int           delay,
                                                 GCancellable *cancellable);
