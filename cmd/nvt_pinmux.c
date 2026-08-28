// SPDX-License-Identifier: GPL-2.0+
/*
* Copyright (C) 2018, STMicroelectronics - All Rights Reserved
*/

#if (defined(CONFIG_TARGET_NS02302_A64))
#else
#include <asm/arch/pad.h>
#endif

#include <command.h>
#include <dm.h>
#include <errno.h>
#include <dm/pinctrl.h>
#include <dm/uclass-internal.h>
#include <asm/nvt_pinmux.h>
#include <malloc.h>
#include <asm/arch/IOAddress.h>

#define LIMIT_DEVNAME	30

static char *pinmux_table[] = {
	#if  defined(CONFIG_TARGET_NS02302_A64)
	"sdio",
	"nand",
	"eth",
	"i2c",
	"pwm",
	"pwmII",
	"pwmIII",
	"ccnt",
	"sensor",
	"sensor2",
	"sensor3",
	"sensormisc",
	"sensorsync",
	"audio",
	"uart",
	"uartII",
	"csi",
	"remote",
	"sdp",
	"spi",
	"sif",
	"misc",
	"lcd",
	"tv",
	"sel_lcd",

	#elif defined(CONFIG_TARGET_NS02501)
	"sdio",
	"nand",
	"eth",
	"i2c",
	"pwm",
	"pwmII",
	"pwmIII",
	"ccnt",
	"sensor",
	"sensor2",
	"sensormisc",
	"sensorsync",
	"audio",
	"uart",
	"csi",
	"remote",
	"sdp",
	"spi",
	"sif",
	"misc",
	"lcd",
	"tv",
	"sel_lcd",
	"weigand",

#else
	"sdio",
	"sdio2",
	"sdio3",
	"nand",
	"sensor",
	"sensor2",
	"mipi_lvds",
	"i2c",
	"sif",
	"uart",
	"spi",
	"sdp",
	"remote",
	"pwm",
	"pwm2",
	"ccnt",
	"audio",
	"lcd",
	"tv",
	"eth",
	"misc"
#endif
};

static int do_pinmux_set(struct cmd_tbl *cmdtp, int flag, int argc,
    char *const argv[])
{
    char *endp = NULL;
    unsigned long config = 0x0;
	char module[MAX_MODULE_NAME];
	PIN_GROUP_CONFIG pinmux_config[1] = {0};
	u8 count;


    if (strlen(argv[1]) <= MAX_MODULE_NAME) {
		strcpy(module, argv[1]);
	} else {
		pr_err("invalid length of module name:%ld\n", (unsigned long)strlen(argv[1]));
		return -EINVAL;
	}

    config = simple_strtoul(argv[2], &endp, 0);
	if (endp == argv[2] || *endp != '\0') {
		pr_err("invalid config:%s\n", argv[2]);
		return -EINVAL;
	}

    /*Find the module name and replace the configuration*/
	for (count = 0; count < PIN_FUNC_MAX; count++) {
		if (!strcmp(module, pinmux_table[count])) {
			pinmux_config->pin_function = count;
			pinmux_config->config = config;
			break;
		}
	}

	if (count == PIN_FUNC_MAX) {
		pr_err("module name invalid %s\n", module);
		return -EINVAL;
	}

    nvt_pinmux_update(pinmux_config, 1);
    return CMD_RET_SUCCESS;
}

static int do_pinmux_show(struct cmd_tbl *cmdtp, int flag, int argc,
    char *const argv[])
{
    gpio_func_show();
    return CMD_RET_SUCCESS;
}

static int do_pinmux_check(struct cmd_tbl *cmdtp, int flag, int argc,
    char *const argv[])
{
	#if (defined(CONFIG_TARGET_NS02302_A64))
		printf("not support\n");
	#else
		if (pinmux_conflict_check() == E_OK){
			printf("pinmux_conflict_check == E_OK\n");
		}
	#endif
    return CMD_RET_SUCCESS;
}

static int do_pinmux_summary_open(struct cmd_tbl *cmdtp, int flag, int argc,
    char *const argv[])
{
	u32 top_reg_addr = 0xf0010000;
	int ret = E_OK;
	struct nvt_pinctrl_info *info ;
    int i = 0, nr_pinmux = 0;
	u8 count;
	/* Enable it after dts parsing ready*/
    int nodeoffset;
    u32 *cell = NULL;
	char path[PIN_FUNC_MAX] = {0};

    info = malloc(sizeof(struct nvt_pinctrl_info));
	if (!info) {
		printf("nvt pinmux mem alloc fail\n");
		free(info);
		return -ENOMEM;
	}
	memset(info, 0, sizeof(*info));

	info->top_base = (void*) IOADDR_TOP_REG_BASE;
	top_reg_addr = (uintptr_t) info->top_base;

	for (nr_pinmux = 0; nr_pinmux < PIN_FUNC_MAX; nr_pinmux++)
    {
		if (!pinmux_table[nr_pinmux]){
			pr_err("pinmux_dts_name not found! \n");
			goto err_out;
		}

		if (nr_pinmux >= ARRAY_SIZE(info->top_pinmux)) {
            pr_err("top_pinmux array overflow (max %zu)\n",
                   ARRAY_SIZE(info->top_pinmux));
            ret = -EOVERFLOW;
            goto err_out;
        }

        sprintf(path,"/top/%s", pinmux_table[nr_pinmux]);
    	nodeoffset = fdt_path_offset((const void*)nvt_fdt_buffer, path);
		if (nodeoffset < 0) {
			pr_err("FDT: node path %s not found!\n", path);
		}
		info->top_pinmux[nr_pinmux].pin_function = nr_pinmux;

    	cell = (u32*)fdt_getprop((const void*)nvt_fdt_buffer, nodeoffset, "pinmux", NULL);
		if (cell == NULL) {
			pr_err("FDT: property pinmux not found in %s!\n", path);
			info->top_pinmux[nr_pinmux].config = 0x0;
		} else {
			info->top_pinmux[nr_pinmux].config = __be32_to_cpu(cell[0]);
		}
	}

	pr_err("\n&top {\n");
	for (count = 0; count < PIN_FUNC_MAX; count++) {
		pr_err("%s{pinmux=<0x%x>;};\n", pinmux_table[count], info->top_pinmux[count].config);
	}
	pr_err("};\n");

	free(info);
	return CMD_RET_SUCCESS;

err_out:
	free(info);
	return ret;
}

static int do_pad_show(struct cmd_tbl *cmdtp, int flag, int argc,
    char *const argv[])
{
	#if (defined(CONFIG_TARGET_NS02302_A64))
		printf("not support\n");
	#else
		pad_show();
	#endif
    return CMD_RET_SUCCESS;
}

static struct cmd_tbl pinmux_subcmd[] = {
	U_BOOT_CMD_MKENT(check, 1, 1, do_pinmux_check, "", ""),
	U_BOOT_CMD_MKENT(set, 3, 1, do_pinmux_set, "", ""),
	U_BOOT_CMD_MKENT(pinmux_show, 1, 1, do_pinmux_show, "", ""),
	U_BOOT_CMD_MKENT(pinmux_summary, 2, 1, do_pinmux_summary_open, "", ""),

	U_BOOT_CMD_MKENT(pad_show, 1, 1, do_pad_show, "", ""),
};

static int do_pinctrl(struct cmd_tbl *cmdtp, int flag, int argc,
    char *const argv[])
{
    struct cmd_tbl *cmd;

    argc--;
    argv++;

    cmd = find_cmd_tbl(argv[0], pinmux_subcmd, ARRAY_SIZE(pinmux_subcmd));
    if (!cmd || argc > cmd->maxargs)
    return CMD_RET_USAGE;

    return cmd->cmd(cmdtp, flag, argc, argv);
}


U_BOOT_CMD(pinctrl, CONFIG_SYS_MAXARGS, 1, do_pinctrl,
    "pinctrl usage",
    "set	:set GPIO func.	             example:pinctrl set spi 0x1\n"
	"check	        :check GPIO func conflict.   example:pinctrl check\n"
    "pinmux_show	:show GPIO func.	     example:pinctrl pinmux_show\n"
	"pinmux_summary	:show dtsi.		     example:pinctrl pinmux_summary\n"

	"\n"
	"pad_show	:show pull/dowm/ds status.   example:pinctrl pad_show\n"

)
