/*
 * Copyright Novatek Inc
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __ARCH_GPIO_H
#define __ARCH_GPIO_H

#define C_GPIO(pin)         (pin)
#define L_GPIO(pin)         (pin + 0x60)
#define P_GPIO(pin)         (pin + 0x20)
#define HSI_GPIO(pin)       (pin + 0xA0)
#define D_GPIO(pin)         (pin + 0x80)
#define S_GPIO(pin)         (pin + 0x40)
#define DSI_GPIO(pin)       (pin + 0xE0)
#define A_GPIO(pin)         (pin + 0xC0)

#define NVT_GPIO_STG_DATA_0     (0)
#define NVT_GPIO_STG_DIR_0      (0x20)
#define NVT_GPIO_STG_SET_0      (0x40)
#define NVT_GPIO_STG_CLR_0      (0x60)

/*
 * Empty file - cmd_gpio.c requires this. The implementation
 * is in drivers/gpio/ns02xxx_gpio.c instead of inlined here.
 */

#endif /* __ARCH_GPIO_H */
