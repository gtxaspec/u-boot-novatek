/**
    NVT utilities for pice customization

    @file       nvt_pcie_boot_utils.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2023.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/libfdt.h>
#include <asm/nvt-common/nvt_common.h>
#include "nvt_ivot_pack.h"
#include <malloc.h>
#include <asm/nvt-common/modelext/bin_info.h>
#include <fdt_support.h>
#include <env.h>
#include <mapmem.h>
#include <image.h>

#ifdef CONFIG_NVT_IVOT_OPTEE_SECBOOT_SUPPORT
#include <asm/nvt-common/nvt_ivot_aes_smc.h>
#include <asm/nvt-common/nvt_ivot_efuse_smc.h>
#endif

#define CMD_BUFFER_SIZE 255

/**************************************************************************
 *
 *
 * Novatek common function
 *
 *
 **************************************************************************/

#ifdef CONFIG_NVT_IVOT_OPTEE_SECBOOT_SUPPORT
static void get_bininfo_size_offset(unsigned char* bininfo,unsigned int* size,unsigned int *offset)
{

	        *size = bininfo[4] | bininfo[5]<<8 | bininfo[6] << 16 | bininfo[7] << 24;
		        *offset = bininfo[0] | bininfo[1] << 8 | bininfo[2] << 16 | bininfo[3] <<24;
}
#endif

/**************************************************************************
 *
 *
 * Novatek EP Boot Flow
 *
 *
 **************************************************************************/
#ifdef CONFIG_NVT_IVOT_OPTEE_SECBOOT_SUPPORT
static int nvt_ep_decrypt_linux(unsigned long addr)
{
	int ret = 0;
	unsigned char test_aes_key[16]={0x04,0x03,0x02,0x01,0x08,0x07,0x06,0x05,0x12,0x11,0x10,0x09,0x16,0x15,0x14,0x013};
	NVT_SMC_EFUSE_DATA efuse_data = {0};
	efuse_data.cmd = NVT_SMC_EFUSE_IS_DATA_ENCRYPTED;
	if (1 != nvt_ivot_optee_efuse_operation(&efuse_data)) {
		printf("data encrypt not enable\n");
		return 0;
	}

	HEADINFO *headinfo = (HEADINFO *)addr;
	unsigned int crypto_offset = 0;
	unsigned int crypto_size = 0;
	unsigned char *encrypt_buf = NULL;
	unsigned char * bininfo = headinfo->BinInfo_3;
	get_bininfo_size_offset(headinfo->BinInfo_3, &crypto_size, &crypto_offset);

	encrypt_buf = (unsigned char *)(addr + crypto_offset);

	unsigned int total_size = headinfo->BinLength;

	unsigned int decrypt_size = total_size - crypto_offset;
	NVT_SMC_AES_DATA aes_data={0};
	aes_data.crypto_type = NVT_SMC_AES_CRYPTO_DECRYPTION;
	aes_data.aes_mode = NVT_SMC_AES_MODE_CBC;
	aes_data.key_buf = (unsigned char *)test_aes_key;
	memset(aes_data.IV,0,16);
	aes_data.key_size = 16;
	aes_data.input_data = (unsigned char *)encrypt_buf;
	aes_data.input_size = crypto_size;
	aes_data.output_data = (unsigned char *)addr;
	ret = nvt_ivot_optee_aes_operation(&aes_data);
	if(ret != 0)
	{
		printf("nvt_ivot_optee_aes_operation fail\r\n",ret);
		return -1;
	}
	if(decrypt_size > crypto_size) {
		//there are some data not encrypt, need copy to buf
		unsigned char * tmp = (unsigned char *)addr;
		memcpy(&tmp[crypto_size], &encrypt_buf[crypto_size], (decrypt_size - crypto_size));

	}

	if(ret != 0) {
		printf("%s fail ret:%d\n", __func__, ret);
		return -1;
	}

	return 0;
}
#endif

static int nvt_ep_boot_prepare_env_and_boot(u64 linux_addr, u64 linux_size)
{
	char buf[CMD_BUFFER_SIZE], cmd[CMD_BUFFER_SIZE];
	u64 kernel_size;
	unsigned int nvt_ep_boot_ramdisk_addr, nvt_ep_boot_ramdisk_size, size;
	image_header_t *hdr;
	int ret;

	/* Get Ramdisk size and address */
	hdr = (image_header_t *)(unsigned long)nvt_memory_cfg[MEMTYPE_LINUXTMP].addr;
	size = image_get_data_size(hdr) + sizeof(image_header_t);
	nvt_ep_boot_ramdisk_size = ALIGN_CEIL(size, 4096);
	nvt_ep_boot_ramdisk_addr = nvt_memory_cfg[MEMTYPE_LINUX].addr + nvt_memory_cfg[MEMTYPE_LINUX].size - nvt_ep_boot_ramdisk_size - 0x10000;

	/* Copy Ramdisk to targe address */
	memmove((void *)(unsigned long)nvt_ep_boot_ramdisk_addr, (void *)(unsigned long)nvt_memory_cfg[MEMTYPE_LINUXTMP].addr, nvt_ep_boot_ramdisk_size);

	sprintf(buf, "0x%08x ", nvt_ep_boot_ramdisk_addr + nvt_ep_boot_ramdisk_size);
	env_set("initrd_high", buf);

	/* To assign relocated fdt address */
	sprintf(buf, "0x%08x ", linux_addr + linux_size);
	env_set("fdt_high", buf);

	/* The following will setup the lmb memory parameters for bootm cmd */
	sprintf(buf, "0x%08x ", linux_addr + linux_size);
	env_set("bootm_size", buf);
	env_set("bootm_mapsize", buf);

	sprintf(buf, "0x%08x ", linux_addr);
	env_set("bootm_low", buf);
	env_set("kernel_comp_addr_r", buf);

#ifdef CONFIG_NVT_IVOT_OPTEE_SECBOOT_SUPPORT
	HEADINFO *headinfo = (HEADINFO *)(unsigned long)nvt_memory_cfg[MEMTYPE_KERNEL_IMG].addr;
	kernel_size = headinfo->BinLength;
	ret = nvt_ep_decrypt_linux(nvt_memory_cfg[MEMTYPE_KERNEL_IMG].addr);
	if (ret < 0){
		return ret;
	}
#else
	hdr = (image_header_t *)(unsigned long)nvt_memory_cfg[MEMTYPE_KERNEL_IMG].addr;
	kernel_size = image_get_data_size(hdr);

#endif
	sprintf(buf, "0x%x", kernel_size);
	env_set("kernel_comp_size", buf);
	run_command("pri", 0);

	sprintf(cmd, "booti");

	sprintf(cmd, "%s %x %lx %lx", cmd, nvt_memory_cfg[MEMTYPE_KERNEL_IMG].addr + sizeof(image_header_t), (unsigned long)nvt_ep_boot_ramdisk_addr, (unsigned long)gd->fdt_blob);
	printf("%s\n", cmd);
	run_command(cmd, 0);

	return 0;
}

static int nvt_ep_boot_flow(void)
{
	int ret;

#ifdef CONFIG_OF_CONTROL
	env_set_hex("fdtcontroladdr",
			(unsigned long)map_to_sysmem(gd->fdt_blob));
#endif

	/* Prepare env for boot to linux and boot */
	ret = nvt_ep_boot_prepare_env_and_boot(nvt_memory_cfg[MEMTYPE_LINUX].addr, nvt_memory_cfg[MEMTYPE_LINUX].size);

	return ret;
}

/**************************************************************************
 *
 *
 * Novatek RC Boot EP Entry
 *
 *
 **************************************************************************/

int do_nvt_pcie(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = -1;
	ret = nvt_ep_boot_flow();
	return ret;
}

U_BOOT_CMD(
	nvt_pcie, 2,    1,  do_nvt_pcie,
	"nvt RC boot EP",
	""
);
