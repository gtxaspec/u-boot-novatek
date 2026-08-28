// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 Novatek LLC
 * Written by Selo Lo <selo_lo@novatek.com.tw>
 */

#include <common.h>
#include <command.h>
#include <test/novatek.h>
#include <test/suites.h>
#include <test/ut.h>

int do_ut_novatek(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct unit_test *tests = UNIT_TEST_SUITE_START(novatek_test);
	const int n_ents = UNIT_TEST_SUITE_COUNT(novatek_test);

	return cmd_ut_category("novatek", "novatek_test_", tests, n_ents, argc, argv);
}
