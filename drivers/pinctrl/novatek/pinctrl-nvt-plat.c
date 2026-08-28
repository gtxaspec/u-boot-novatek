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


#if defined(CONFIG_TARGET_NA51102_A64) || defined(CONFIG_TARGET_NS02201_A64) || defined(CONFIG_TARGET_NS02302_A64) || defined(CONFIG_TARGET_NA51090_A64)
u64 top_reg_addr = 0x2f0010000;
extern u64 top_reg_addr;

#else
u32 top_reg_addr = 0xf0010000;
extern u32 top_reg_addr;
u32 pad_reg_addr = 0xf0030000;
extern u32 pad_reg_addr;
u32 gpio_reg_addr = 0xf0030000;
extern u32 gpio_reg_addr;
#endif

#define TOP_DTS_MAX 25

const char *pinmux_dts_name[] =
{
#if defined(CONFIG_TARGET_NA51102_A64)
	"sdio",
	"nand",
	"eth",
	"i2c",
	"i2cII",
	"pwm",
	"pwmII",
	"ccnt",
	"sensor",
	"sensor2",
	"sensor3",
	"sensormisc",
	"sensorsync",
	"mipi_lvds",
	"audio",
	"uart",
	"uartII",
	"remote",
	"sdp",
	"spi",
	"sif",
	"misc",
	"lcd",
	"lcd2",
	"tv",
	"sel_lcd",
	"sel_lcd2",

#elif defined(CONFIG_TARGET_NS02302_A64)
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

#elif defined(CONFIG_TARGET_NS02201_A64)

	"sdio",
	"nand",
	"eth",
	"i2c",
	"i2cII",
	"i2cIII",
	"i2cIV",
	"pwm",
	"pwmII",
	"ccnt",
	"sensor",
	"sensor2",
	"sensormisc",
	"sensormiscii",
	"sensorsync",
	"sensorsyncII",
	"sensorsyncIII",
	"sensorsyncIV",
	"audio",
	"uart",
	"uartII",
	"remote",
	"sdp",
	"spi",
	"sif",
	"misc",
	"lcd",
	"lcd2",
	"lcd3",
	"tv",
	"lcd_pinout",
	"sel_lcd",
	"sel_lcd2",
	"pcie",
	"sel_pcie",

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
#if !(defined(CONFIG_TARGET_NA51102_A64) || defined(CONFIG_TARGET_NS02201_A64) || defined(CONFIG_TARGET_NS02302_A64))
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
#if (defined(CONFIG_TARGET_NA51102_A64) || defined(CONFIG_TARGET_NS02201_A64) || defined(CONFIG_TARGET_NS02302_A64))
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
	//struct nvt_gpio_info *gpio;
	//struct resource *nvt_mem_base;
	//struct device_node *top_np;
	//u32 value;
	// u32 pad_config[4] = {0, 0, 0, 0};
	//u32 gpio_config[2] = {0, 0};
	//int nr_pinmux = 0, nr_pad = 0, nr_gpio = 0 ;
    int i = 0, nr_pinmux = 0, nr_pad = 0;
	const fdt32_t *pad_cfg;
	/* Enable it after dts parsing ready*/
    int nodeoffset, len, subnode;
    u32 *cell = NULL;
    char path[TOP_DTS_MAX] = {0};

	printf("%s: version[%s] \n", __func__, DRV_VERSION);

    info = malloc(sizeof(struct nvt_pinctrl_info));
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}
#if 0
	for_each_child_of_node(pdev->dev.of_node,  top_np) {
		if (!of_get_property(top_np, "gpio_config", NULL))
			continue;

		nr_gpio++;
	}

	gpio = devm_kzalloc(&pdev->dev, nr_gpio * sizeof(struct nvt_gpio_info), GFP_KERNEL);
	if (!gpio) {
		dev_err(&pdev->dev, "nvt gpio mem alloc fail\n");
		return -ENOMEM;
	}

	nr_gpio = 0;
#endif

/*
	nvt_mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->top_base = devm_ioremap_resource(&pdev->dev, nvt_mem_base);
	if (IS_ERR(info->top_base)) {
		dev_err(&pdev->dev, "fail to get pinmux mem base\n");
		return -ENOMEM;
	}
	*/
#if defined(CONFIG_TARGET_NA51102_A64) || defined(CONFIG_TARGET_NS02201_A64) || defined(CONFIG_TARGET_NS02302_A64)
    info->top_base = (void*) IOADDR_TOP_REG_BASE;
	top_reg_addr = (u64) info->top_base;
    pr_debug("%s(pinmux= top_base=%p, length=%p):\n", __func__ , info->top_base,
	      (void*)IOADDR_TOP_REG_BASE);
#else
	info->top_base = (void*) IOADDR_TOP_REG_BASE;
	top_reg_addr = (u32) info->top_base;

#if defined(CONFIG_TARGET_NS02501)
	info->pad_base = (void*) IOADDR_PAD_REG_BASE;
	info->gpio_base = (void*) IOADDR_GPIO_REG_BASE;
	pad_reg_addr = (u32) info->pad_base;
	gpio_reg_addr = (u32) info->gpio_base;
#endif
    pr_debug("%s(pinmux= top_base=%p, length=%x):\n", __func__ , info->top_base,
	      IOADDR_TOP_REG_BASE);
#endif
#if 0
	nvt_mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	info->pad_base = devm_ioremap_resource(&pdev->dev, nvt_mem_base);
	if (IS_ERR(info->pad_base)) {
		dev_err(&pdev->dev, "fail to get pad mem base\n");
		return -ENOMEM;
	}

	nvt_mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	info->gpio_base = devm_ioremap_resource(&pdev->dev, nvt_mem_base);
	if (IS_ERR(info->gpio_base)) {
		dev_err(&pdev->dev, "fail to get gpio mem base\n");
		return -ENOMEM;
	}
#endif



	for (i = 0; i < PIN_FUNC_MAX; i++)
    {
#if defined(CONFIG_TARGET_NA51102_A64) || defined(CONFIG_TARGET_NS02201_A64) || defined(CONFIG_TARGET_NS02302_A64)
        sprintf(path,"/top/%s",pinmux_dts_name[i]);
#else
        sprintf(path,"/top@%x/%s",IOADDR_TOP_REG_BASE, pinmux_dts_name[i]);
#endif

    	nodeoffset = fdt_path_offset((const void*)nvt_fdt_buffer, path);
		if (nodeoffset < 0) {
			pr_err("FDT: node path %s not found!\n", path);
			nr_pinmux++;
			continue;
		}

    	cell = (u32*)fdt_getprop((const void*)nvt_fdt_buffer, nodeoffset, "pinmux", NULL);
		if (cell == NULL) {
			pr_err("FDT: property pinmux not found in %s!\n", path);
			nr_pinmux++;
			continue;
		}
        info->top_pinmux[nr_pinmux].config = __be32_to_cpu(cell[0]);
        info->top_pinmux[nr_pinmux].pin_function = nr_pinmux;

        nr_pinmux++;
	}

	if (nr_pinmux == 0)
		return -ENOMEM;


#if defined(CONFIG_TARGET_NS02501)
	sprintf(path,"/top@%x",IOADDR_TOP_REG_BASE);
	nodeoffset = fdt_path_offset((const void*)nvt_fdt_buffer, path);
	if (nodeoffset < 0) {
		pr_err("FDT: top node not found!\n");
	}

	for (subnode = fdt_first_subnode(nvt_fdt_buffer, nodeoffset);
		subnode >= 0;
		subnode = fdt_next_subnode(nvt_fdt_buffer, subnode)) {

		pad_cfg = (const fdt32_t *)fdt_getprop(nvt_fdt_buffer, subnode, "pad_config", &len);
		if (pad_cfg && len >= 4 * sizeof(fdt32_t)) {
			info->pad[nr_pad].pad_ds_pin   = fdt32_to_cpu(pad_cfg[0]);
			info->pad[nr_pad].driving      = fdt32_to_cpu(pad_cfg[1]);
			info->pad[nr_pad].pad_gpio_pin = fdt32_to_cpu(pad_cfg[2]);
			info->pad[nr_pad].direction    = fdt32_to_cpu(pad_cfg[3]);
			nr_pad++;
		}
	}
	pad_preset(info);
	ret = pad_init(info, nr_pad);

	if (ret != E_OK)
		return ret;
#endif

	// pinmux_preset(info);
	//pad_preset(info);

	ret = pinmux_init(info);

	if (ret != E_OK)
		return ret;

	#if !(defined(CONFIG_TARGET_NA51102_A64) || defined(CONFIG_TARGET_NS02201_A64) || defined(CONFIG_TARGET_NS02302_A64))
			if (ret == E_OK){
				ret = pinmux_set_config(id_restore, pinmux_restore);
			} else {
				nvt_pinmux_show_conflict(info);
			}
	#endif

	return 0;
}
EXPORT_SYMBOL(nvt_pinmux_probe);

#if !defined(CONFIG_TARGET_NS02501)
static int nvt_pinctrl_remove(struct udevice *pdev)
{

	return 0;
}
#endif

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