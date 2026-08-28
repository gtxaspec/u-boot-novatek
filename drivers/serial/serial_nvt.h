/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2024 Novatek, Inc
 *
 * Serial DM U-Boot driver for Novatek SoCs
 */

#ifndef _SERIAL_NVT_H_
#define _SERIAL_NVT_H_

int nvt_serial_plat_init(struct ns16550 *com_port, int baud_divisor);

#endif
