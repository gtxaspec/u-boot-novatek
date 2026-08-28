// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 Novatek LLC
 * Written by Selo Lo <selo_lo@novatek.com.tw>
 */

#include <common.h>
#include <test/lib.h>
#include <test/test.h>
#include <test/ut.h>
#include <test/novatek.h>

static int example_fail_value(struct unit_test_state *uts)
{
	int total = 0;
	total = 6+6;

	if ( total > 6) {
		ut_failf(uts, __FILE__, __LINE__, __func__,
		"total should greater than 6, ",
		"total = %d\n", total);
	}

	return 0;
}
NOVATEK_TEST(example_fail_value, 0);


int failed_test(void){
	return -1;
}

static int example_fail_fuction(struct unit_test_state *uts)
{
	int ret = 0;
	ret = failed_test();
	if (ret) {
		ut_failf(uts, __FILE__, __LINE__, __func__,
		"failed_test() failed, ",
		"return: %d expect: 0\n", ret);
	}

	return 0;
}
NOVATEK_TEST(example_fail_fuction, 0);


