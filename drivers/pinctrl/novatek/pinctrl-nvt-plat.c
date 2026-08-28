/*
 * Driver for the Novatek pinmux
 *
 * Copyright (c) 2019, NOVATEK MICROELECTRONIC CORPORATION.  All rights reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#include <asm/nvt-common/rcw_macro.h>
#include <asm/arch/IOAddress.h>
#include <linux/libfdt.h>
#include <linux/compat.h>

#include <asm/io.h>
#include "common.h"
#include <errno.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <command.h>
#include <asm/gpio.h>
#include <malloc.h>
#include <asm/arch/hardware.h>
#include <asm/nvt-common/nvt_common.h>
// #include <asm/arch/top.h>

#include <asm/nvt-common/nvt_reg.h>
#include <asm/nvt_pinmux.h>


uintptr_t top_reg_addr = 0xf0010000;
extern uintptr_t top_reg_addr;
uintptr_t pad_reg_addr = 0xf0030000;
extern uintptr_t pad_reg_addr;
uintptr_t gpio_reg_addr = 0xf0030000;
extern uintptr_t gpio_reg_addr;


const char *pinmux_dts_name[PIN_FUNC_MAX] =
{
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

static PINMUX_FUNC_ID id_restore = 0x0;
static u32 pinmux_restore = 0x0;


static void nvt_pinmux_show_conflict(struct nvt_pinctrl_info *info)
{
	int i;

	pinmux_parsing(info);
	for (i = 0; i < PIN_FUNC_MAX; i++) {
        pr_err("pinmux %-2d config 0x%x\n", i, info->top_pinmux[i].config);
	}

	panic("###### Conflicted Pinmux Setting ######\n");
}


int nvt_pinmux_capture(PIN_GROUP_CONFIG *pinmux_config, int count)
{
	struct nvt_pinctrl_info *info;
	int i, j, ret = E_OK;
/*
	if (in_interrupt() || in_atomic() || irqs_disabled())
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_ATOMIC);
	else
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}
	*/

	info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}

	if (top_reg_addr) {
		info->top_base = (void*) top_reg_addr;
		pinmux_parsing(info);

		for (j = 0; j < count; j++) {
			for (i = 0; i < PIN_FUNC_MAX; i++) {
				if (i == pinmux_config[j].pin_function)
					pinmux_config[j].config = info->top_pinmux[i].config;
			}
		}
	} else {
		pr_err("invalid pinmux address\n");
		ret = -ENOMEM;
	}

	kfree(info);

	return ret;
}
EXPORT_SYMBOL(nvt_pinmux_capture);

int nvt_pinmux_update(PIN_GROUP_CONFIG *pinmux_config, int count)
{
	struct nvt_pinctrl_info *info;
	int i, j, ret = E_OK;
/*
	if (in_interrupt() || in_atomic() || irqs_disabled())
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_ATOMIC);
	else
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}
*/
	info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}

	if (top_reg_addr) {
		info->top_base = (void*) top_reg_addr;
		pinmux_parsing(info);

		for (j = 0; j < count; j++) {
			for (i = 0; i < PIN_FUNC_MAX; i++) {
				if (i == pinmux_config[j].pin_function)
					info->top_pinmux[i].config = pinmux_config[j].config;
			}
		}

		ret = pinmux_init(info);
#if !(defined(CONFIG_TARGET_NS02302_A64))
		if (ret == E_OK){
			ret = pinmux_set_config(id_restore, pinmux_restore);
		} else {
			nvt_pinmux_show_conflict(info);
		}
#endif

	} else {
		pr_err("invalid pinmux address\n");
		ret = -ENOMEM;
	}

	kfree(info);

	return ret;
}
EXPORT_SYMBOL(nvt_pinmux_update);

#ifndef OUTW
#define OUTW(x, y)              writel(y, x)
#endif

#ifndef INW
#define INW(x)                  readl(x)
#endif
int pinmux_set_config(PINMUX_FUNC_ID id, u32 pinmux)
{
#if (defined(CONFIG_TARGET_NS02302_A64))
	PIN_GROUP_CONFIG pinmux_cfg[1] = {0};
	int ret = E_OK;

	switch (id) {
	case PINMUX_FUNC_ID_LCD:
		pinmux_cfg[0].pin_function = PIN_FUNC_SEL_LCD;
		pinmux_cfg[0].config = pinmux;
		ret = nvt_pinmux_update(pinmux_cfg, 1);
		break;

	/*case PINMUX_FUNC_ID_LCD2:
		pinmux_cfg[0].pin_function = PIN_FUNC_SEL_LCD2;
		pinmux_cfg[0].config = pinmux;
		ret = nvt_pinmux_update(pinmux_cfg, 1);
		break;*/

	case PINMUX_FUNC_ID_TV:
		break;

	case PINMUX_FUNC_ID_HDMI:
		break;

	default:
		break;
	}

	return ret;
#else
	struct nvt_pinctrl_info *info;
	int ret;
/*
	if (in_interrupt() || in_atomic() || irqs_disabled())
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_ATOMIC);
	else
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}
*/
    info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);

	if (top_reg_addr) {
		info->top_base = (void*) top_reg_addr;
		ret = pinmux_set_host(info, id, pinmux);

		if (id <= PINMUX_FUNC_ID_LCD2) {
			id_restore = id;
			pinmux_restore = pinmux;
		}
	} else {
		pr_err("invalid pinmux address\n");
		ret = -ENOMEM;
	}

	kfree(info);

	return ret;
#endif
}
//EXPORT_SYMBOL(pinmux_set_config);


int nvt_pinmux_probe(struct udevice *dev)
{
    struct nvt_pinctrl_info *info ;
	int ret = E_OK;
    int i = 0, nr_pinmux = 0, nr_pad = 0;
	const fdt32_t *pad_cfg;
	/* Enable it after dts parsing ready*/
    int nodeoffset = 0, len = 0, subnode = 0;
    u32 *cell = NULL;
    char func_node[PIN_FUNC_MAX] = {0};

	/* find top node name. (e.g. top@10,f0010000)*/
	ofnode root = ofnode_path("/");
	ofnode top_node = ofnode_by_compatible(root, "nvt,nvt_top");
	const char *top_node_name = ofnode_get_name(top_node);
	if (!ofnode_valid(top_node)){
		pr_err("Failed to find top_node");
		return -ENODEV;
	}
	printf("%s: version[%s] \n", __func__, DRV_VERSION);

    info = malloc(sizeof(struct nvt_pinctrl_info));
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}
	memset(info, 0, sizeof(*info));

	info->top_base = (void*) IOADDR_TOP_REG_BASE;
	info->pad_base = (void*) IOADDR_PAD_REG_BASE;
	info->gpio_base = (void*) IOADDR_GPIO_REG_BASE;
	top_reg_addr = (uintptr_t) info->top_base;
	pad_reg_addr = (uintptr_t) info->pad_base;
	gpio_reg_addr = (uintptr_t) info->gpio_base;

	/* find top.dtsi module node */
	for (nr_pinmux = 0; nr_pinmux < PIN_FUNC_MAX; nr_pinmux++)
    {
		if (!pinmux_dts_name[nr_pinmux]){
			pr_err("pinmux_dts_name not found! \n");
			goto err_out;
		}

		if (nr_pinmux >= ARRAY_SIZE(info->top_pinmux)) {
            pr_err("top_pinmux array overflow (max %zu)\n",
                   ARRAY_SIZE(info->top_pinmux));
            ret = -EOVERFLOW;
            goto err_out;
        }

		sprintf(func_node,"/top/%s",pinmux_dts_name[nr_pinmux]);
		nodeoffset = fdt_path_offset((const void*)nvt_fdt_buffer, func_node);
		if (nodeoffset < 0) {
			pr_err("FDT: func_node %s not found!\n", func_node);
		}
		info->top_pinmux[nr_pinmux].pin_function = nr_pinmux;

		cell = (u32*)fdt_getprop((const void*)nvt_fdt_buffer, nodeoffset, "pinmux", NULL);
		if (cell == NULL) {
			pr_err("FDT: property pinmux not found in %s!\n", func_node);
			info->top_pinmux[nr_pinmux].config = 0x0;
		} else {
			info->top_pinmux[nr_pinmux].config = __be32_to_cpu(cell[0]);
		}
	}

	/* find pad config*/
	snprintf(func_node, sizeof(func_node), "/%s", top_node_name);
	nodeoffset = fdt_path_offset((const void*)nvt_fdt_buffer, func_node);
	if (nodeoffset < 0) {
		pr_err("FDT: top node not found!\n");
		goto err_out;
	}

	for (subnode = fdt_first_subnode(nvt_fdt_buffer, nodeoffset);
		subnode >= 0;
		subnode = fdt_next_subnode(nvt_fdt_buffer, subnode)) {

		if (nr_pad >= ARRAY_SIZE(info->pad)) {
			pr_err("pad array overflow (max %zu)\n",
					ARRAY_SIZE(info->pad));
			ret = -EOVERFLOW;
			goto err_out;
		}

		pad_cfg = (const fdt32_t *)fdt_getprop(nvt_fdt_buffer, subnode, "pad_config", &len);
		if (pad_cfg && len >= 4 * sizeof(fdt32_t)) {
			info->pad[nr_pad].pad_ds_pin   = fdt32_to_cpu(pad_cfg[0]);
			info->pad[nr_pad].driving      = fdt32_to_cpu(pad_cfg[1]);
			info->pad[nr_pad].pad_gpio_pin = fdt32_to_cpu(pad_cfg[2]);
			info->pad[nr_pad].direction    = fdt32_to_cpu(pad_cfg[3]);
			nr_pad++;
		}
	}
#if (defined(CONFIG_TARGET_NS02302_A64))

#else
	pad_preset(info);
	ret = pad_init(info, nr_pad);
	if (ret != E_OK){
		goto err_out;
	}
#endif
	ret = pinmux_init(info);
	if (ret != E_OK){
		nvt_pinmux_show_conflict(info);
		goto err_out;
	}

	ret = pinmux_set_config(id_restore, pinmux_restore);
	if (ret != E_OK){
		goto err_out;
	}

	free(info);
	return 0;

err_out:
	free(info);
	return ret;
}
EXPORT_SYMBOL(nvt_pinmux_probe);

/*
static const struct udevice_id nvt_pinctrl_ids[] = {
	{ .compatible = "nvt,nvt_top", },
	{ },
};

U_BOOT_DRIVER(nvt_pinmux_drv) = {
	.name = "nvt_pinctrl",
    .id		= UCLASS_PINCTRL,
	.of_match = nvt_pinctrl_ids,
	.probe = nvt_pinmux_probe,
	.remove = nvt_pinctrl_remove,
	.priv_auto_alloc_size   = sizeof(struct nvt_pinctrl_info),
};

*/

const struct pinctrl_ops ntk_pinctrl_ops = {

};

static const struct udevice_id nvt_pinctrl_of_match[] = {
	{ .compatible = "nvt,nvt_top" },
	{ },
};

U_BOOT_DRIVER(nvt_pinctrl) = {
	.name = "nvt_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = nvt_pinctrl_of_match,
	.ops = &ntk_pinctrl_ops,
	.probe = nvt_pinmux_probe,
	.priv_auto	= sizeof(struct nvt_pinctrl_info),
};