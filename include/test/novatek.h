// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 Novatek LLC
 * Written by Selo Lo <selo_lo@novatek.com.tw>
 */

#ifndef __TEST_NOVATEK_H__
#define __TEST_NOVATEK_H__

#include <test/test.h>

/* Declare a new Novatek SOC test */
#define NOVATEK_TEST(_name, _flags)	UNIT_TEST(_name, _flags, novatek_test)

#endif /* __TEST_NOVATEK_H__ */
