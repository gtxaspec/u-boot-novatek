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

static int example_nornal_env(struct unit_test_state *uts)
{
	char buf[64] = {0};
	env_set("example_env", "cmd");
	sprintf(buf, "%s", env_get("example_env"));

	ut_asserteq_str("cmd", buf);
	return 0;
}
NOVATEK_TEST(example_nornal_env, 0);

void* checkValue(int value) {
    if (value == 0) {
        return NULL;
    } else {
        int *ptr = (int *)malloc(sizeof(int));
        if (ptr != NULL) {
            *ptr = value;
        }
        return ptr; 
    }
}

static int example_nornal_null(struct unit_test_state *uts)
{
	void* result = checkValue(0);
	ut_assertnull(result);
	return 0;
}
NOVATEK_TEST(example_nornal_null, 0);

static int example_nornal_not_null(struct unit_test_state *uts)
{
	void* result = checkValue(1);
	ut_assertnonnull(result);
	return 0;
}
NOVATEK_TEST(example_nornal_not_null, 0);

