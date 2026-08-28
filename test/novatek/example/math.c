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


static int example_math_add(struct unit_test_state *uts)
{
	ut_asserteq(2, 1+1);
	return 0;
}
NOVATEK_TEST(example_math_add, 0);

static int example_math_sub(struct unit_test_state *uts)
{
	ut_asserteq(0, 1-1);
	return 0;
}
NOVATEK_TEST(example_math_sub, 0);


