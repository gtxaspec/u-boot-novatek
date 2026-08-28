/**
    NVT update handling api

    @file       nvt_ivot_common.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <common.h>
#include <cli.h>
#include <command.h>
#include <malloc.h>
#include <fs.h>
#include <jffs2/jffs2.h>
#include <mmc.h>
#include <nand.h>
#include <memalign.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <linux/ctype.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/modelext/bin_info.h>
#include <asm/nvt-common/modelext/emb_partition_info.h>
#include <asm/nvt-common/shm_info.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/nvt-common/nvt_ivot_optee.h>
#include <asm/nvt-common/nvt_ivot_efuse_smc.h>
#include <asm/arch/hardware.h>
#include <u-boot/md5.h> 

unsigned long nvt_shminfo_comm_uboot_boot_func = 0;
unsigned long nvt_shminfo_comm_core1_start = 0;
unsigned long nvt_shminfo_comm_core2_start = 0;
unsigned long nvt_shminfo_comm_itron_comp_addr = 0;
unsigned long nvt_shminfo_comm_itron_comp_len = 0;
unsigned long nvt_shminfo_comm_fw_update_addr = 0;
unsigned long nvt_shminfo_comm_fw_update_len = 0;
unsigned int nvt_shminfo_boot_fdt_addr = 0;
unsigned long nvt_tm0_cnt_beg = 0;
unsigned long nvt_tm0_cnt_end = 0;
uint8_t *nvt_fdt_buffer = NULL;
u32 uart_disable_anchor = 0;
u32 rtos_disable_anchor = 0;

void nvt_shminfo_init(void)
{
	SHMINFO *p_shminfo;

	p_shminfo = (SHMINFO *)CONFIG_SMEM_SDRAM_BASE;

	debug("%s \n", p_shminfo->boot.LdInfo_1);
	debug("0x%x \n", (unsigned int)p_shminfo->boot.fdt_addr);

	if (strncmp(p_shminfo->boot.LdInfo_1, "LD_NVT", 6) != 0) {
		printf("%sAttention!!!! Please update to latest version loader%s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
		cli_loop();
	}

	/* If the loader does not provide FDT, we will use uboot FDT */
	if ((fdt_check_header((void *)(unsigned long)p_shminfo->boot.fdt_addr)) != 0) {
		if ((fdt_check_header((void *)gd->fdt_blob)) != 0) {
			printf("%sinvalid fdt header, can not found fdt on uboot%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
			cli_loop();
		} else {
			p_shminfo->boot.fdt_addr = (unsigned long)gd->fdt_blob;
		}
	}

	nvt_shminfo_comm_uboot_boot_func = (unsigned long)&p_shminfo->comm.Resv[0];
	nvt_shminfo_comm_core1_start = (unsigned long)&p_shminfo->comm.Resv[1];
	nvt_shminfo_comm_core2_start = (unsigned long)&p_shminfo->comm.Resv[2];
	nvt_shminfo_comm_itron_comp_addr = (unsigned long)&p_shminfo->comm.Resv[3];
	nvt_shminfo_comm_itron_comp_len = (unsigned long)&p_shminfo->comm.Resv[4];
	nvt_shminfo_comm_fw_update_addr = (unsigned long)&p_shminfo->comm.Resv[5];
	nvt_shminfo_comm_fw_update_len = (unsigned long)&p_shminfo->comm.Resv[6];
	nvt_shminfo_boot_fdt_addr = (unsigned long)&p_shminfo->boot.fdt_addr;
	ulong fdt_addr = nvt_readl((ulong)nvt_shminfo_boot_fdt_addr);
	int fdt_addr1 = fdt_totalsize((void *)fdt_addr);
	int fdt_size = ALIGN_CEIL(fdt_addr1, _EMBMEM_BLK_SIZE_);
	nvt_fdt_buffer = malloc(fdt_size);
	/* We will copy fdt into this allocated space. */
	if (nvt_fdt_buffer == NULL) {
		nvt_dbg(ERR, "%sfailed to alloc (%08X) bytes for fdt %s", ANSI_COLOR_RED, fdt_size, ANSI_COLOR_RESET);
		cli_loop();
	}
	nvt_dbg(IND, "%s The fdt buffer addr: 0x%08lx%s\n", ANSI_COLOR_YELLOW, (ulong)nvt_fdt_buffer, ANSI_COLOR_RESET);
}

int nvt_fdt_init(bool reload)
{
	int ret;

	if (reload) {
	#ifdef CONFIG_NVT_IVOT_SOC_FW_UPDATE_SUPPORT
		ulong fdt_real_size, alloc_size = SZ_512K;
		ulong preload_size = ALIGN_CEIL(sizeof(struct fdt_header), _EMBMEM_BLK_SIZE_);
		ulong tmp_addr  = (ulong)memalign(CONFIG_SYS_CACHELINE_SIZE, alloc_size);
		EMB_PARTITION* pEmb = (EMB_PARTITION*)emb_partition_info_data_new;
		int id;

		/* Get FDT partition offset */
		ret = nvt_getfdt_emb((ulong)nvt_fdt_buffer, pEmb);
		if(ret < 0) {
			printf("failed to get current partition resource.\r\n");
			free((void*)tmp_addr);
			return -1;
		}

		for (id = 0; id < EMB_PARTITION_INFO_COUNT; id++) {
			if (pEmb[id].EmbType == EMBTYPE_FDT) {
				break;
			}
		}

		if(id == EMB_PARTITION_INFO_COUNT){
			nvt_dbg(ERR, "Failed to find fdt partition.\r\n");
			free((void*)tmp_addr);
			return -1;
		}

		if (!tmp_addr) {
			printf("fdt malloc fail\n");
			free((void*)tmp_addr);
			return -1;
		}
		if (preload_size > alloc_size) {
			printf("preload_size 0x%lx > alloc_size 0x%lx\n", preload_size, alloc_size);
			free((void*)tmp_addr);
			return -1;
		}
		memset((unsigned char *)tmp_addr, 0, alloc_size);
		//read first block to get fdt size
		char command[128];
		#if defined(CONFIG_NVT_LINUX_SPINOR_BOOT) || defined(CONFIG_NVT_LINUX_SPINORNAND_BOOT) || (defined(CONFIG_NVT_SPI_NOR) && defined(CONFIG_NVT_LINUX_RAMDISK_SUPPORT))
		sprintf(command, "sf read 0x%lx 0x%llx 0x%lx", tmp_addr, pEmb[id].PartitionOffset, preload_size);
		ret = run_command(command, 0);
		#elif defined(CONFIG_NVT_LINUX_SPINAND_BOOT) || (defined(CONFIG_NVT_SPI_NAND) && defined(CONFIG_NVT_LINUX_RAMDISK_SUPPORT))
		sprintf(command, "nand read 0x%lx 0x%llx 0x%lx", tmp_addr, pEmb[id].PartitionOffset, preload_size);
		ret = run_command(command, 0);
		#elif defined(CONFIG_NVT_LINUX_EMMC_BOOT) || (defined(CONFIG_NVT_IVOT_EMMC) && defined(CONFIG_NVT_LINUX_RAMDISK_SUPPORT))
		/* MMC read should use block number unit */
		sprintf(command, "mmc read 0x%lx 0x%llx 0x%lx", tmp_addr, ALIGN_CEIL(pEmb[id].PartitionOffset, MMC_MAX_BLOCK_LEN) / MMC_MAX_BLOCK_LEN, ALIGN_CEIL(preload_size,MMC_MAX_BLOCK_LEN) / MMC_MAX_BLOCK_LEN);
		ret = run_command(command, 0);
		#elif defined(CONFIG_NVT_LINUX_SD_BOOT) || defined(CONFIG_NVT_LINUX_RAMDISK_SUPPORT)
		sprintf(command, "fatload mmc 0:1 0x%lx %s", tmp_addr, get_nvt_bin_name(NVT_BIN_NAME_TYPE_MODELEXT));
		ret = run_command(command, 0);
		#else
		/* All-in-one SD boot */
		#endif /* CONFIG_NVT_LINUX_SPINAND_BOOT */
		nvt_dbg(IND, "%s: cmd:%s\n",__func__, command);
		if (ret) {
			printf("fdt init fail return %d\n", ret);
			free((void*)tmp_addr);
			return ret;
		}

#if defined(CONFIG_ENCRYPT_CFG)
		HEADINFO *headinfo = (HEADINFO *)tmp_addr;
		fdt_real_size =  headinfo->BinLength;
#else  /* !CONFIG_ENCRYPT_CFG */
		if ((ret = fdt_check_header((void *)tmp_addr)) != 0) {
			printf("invalid fdt header, addr=0x%08X er = %d \n", (unsigned int)tmp_addr, ret);
			free((void*)tmp_addr);
			return ret;
		}
		fdt_real_size = (ulong)fdt_totalsize(tmp_addr);
		debug("fdt size = %d\n", (unsigned int)fdt_real_size);
#endif /* CONFIG_ENCRYPT_CFG */

		if (fdt_real_size > alloc_size) {
			printf("fdt_real_size 0x%lx > alloc_size 0x%lx\n", fdt_real_size, alloc_size);
			free((void*)tmp_addr);
			return -1;
		}

		//read remain size
		if (fdt_real_size > preload_size) {
			#if defined(CONFIG_NVT_LINUX_SPINOR_BOOT) || defined(CONFIG_NVT_LINUX_SPINORNAND_BOOT) || (defined(CONFIG_NVT_SPI_NOR) && defined(CONFIG_NVT_LINUX_RAMDISK_SUPPORT))
			sprintf(command, "sf read 0x%lx 0x%llx 0x%lx", tmp_addr+preload_size, pEmb[id].PartitionOffset+preload_size, fdt_real_size-preload_size);
			ret = run_command(command, 0);
			#elif defined(CONFIG_NVT_LINUX_SPINAND_BOOT) || (defined(CONFIG_NVT_SPI_NAND) && defined(CONFIG_NVT_LINUX_RAMDISK_SUPPORT))
			sprintf(command, "nand read 0x%lx 0x%llx 0x%lx", tmp_addr+preload_size, pEmb[id].PartitionOffset+preload_size, fdt_real_size-preload_size);
			ret = run_command(command, 0);
			#elif defined(CONFIG_NVT_LINUX_EMMC_BOOT) || (defined(CONFIG_NVT_IVOT_EMMC) && defined(CONFIG_NVT_LINUX_RAMDISK_SUPPORT))
			/* MMC read should use block number unit */
			ulong num = ALIGN_CEIL(fdt_real_size, MMC_MAX_BLOCK_LEN)/MMC_MAX_BLOCK_LEN;
			if ((num * MMC_MAX_BLOCK_LEN) > alloc_size) {
				printf("fdt_real_size 0x%lx > 0x%lx\n", num*MMC_MAX_BLOCK_LEN, alloc_size);
				free((void*)tmp_addr);
				return -1;
			}
			sprintf(command, "mmc read 0x%lx 0x%llx 0x%x", tmp_addr, ALIGN_CEIL(pEmb[id].PartitionOffset, MMC_MAX_BLOCK_LEN) / MMC_MAX_BLOCK_LEN, num);
			ret = run_command(command, 0);
			#elif defined(CONFIG_NVT_LINUX_SD_BOOT) || defined(CONFIG_NVT_LINUX_RAMDISK_SUPPORT)
			sprintf(command, "fatload mmc 0:1 0x%lx %s", tmp_addr+preload_size, get_nvt_bin_name(NVT_BIN_NAME_TYPE_MODELEXT));
			ret = run_command(command, 0);
			#else
			/* All-in-one SD boot */
			#endif /* CONFIG_NVT_LINUX_SPINAND_BOOT */
		}
#if defined(CONFIG_ENCRYPT_CFG)
	#if defined(CONFIG_NVT_IVOT_SECBOOT_SUPPORT)
		if (is_secure_enable() != 0) {
			if(nvt_chk_signature_on_uboot((ulong)tmp_addr, fdt_real_size))
			{
				nvt_dbg(ERR, "check fdt signature fail\n");
				free((void*)tmp_addr);
				return -1;

			}
			ret = nvt_decrypt_aes_on_uboot((unsigned long)tmp_addr);
			if (ret < 0) {
				nvt_dbg(ERR, "Decrypt fdt fail, ret = %d\r\n", ret);
				free((void*)tmp_addr);
				return ret;
			}
		}
	#elif defined(CONFIG_NVT_IVOT_OPTEE_SECBOOT_SUPPORT)
		NVT_SMC_EFUSE_DATA efuse_data = {0};
		efuse_data.cmd = NVT_SMC_EFUSE_IS_SECURE;
		if (nvt_ivot_optee_efuse_operation(&efuse_data) != 0) {
			if(nvt_chk_signature((ulong)tmp_addr, fdt_real_size))
			{
				nvt_dbg(ERR, "check fdt signature fail\n");
				free((void*)tmp_addr);
				return -1;

			}
			ret = nvt_decrypt_aes((unsigned long)tmp_addr);
			if (ret < 0) {
				nvt_dbg(ERR, "Decrypt fdt fail, ret = %d\r\n", ret);
				free((void*)tmp_addr);
				return ret;
			}
		}
	#endif /* CONFIG_NVT_IVOT_SECBOOT_SUPPORT */
		nvt_dbg(IND, "Verify encrypted dts PASS\r\n");
#endif /* CONFIG_ENCRYPT_CFG */

		// fdt in flash, maybe its size larger than loader passed one
		if (fdt_totalsize(nvt_fdt_buffer) < fdt_real_size) {
			free(nvt_fdt_buffer);
			nvt_fdt_buffer = malloc(fdt_real_size);
			if (nvt_fdt_buffer == NULL) {
				nvt_dbg(ERR, "%sfailed to alloc (%08lx) bytes for fdt %s", ANSI_COLOR_RED, fdt_real_size, ANSI_COLOR_RESET);
				free((void*)tmp_addr);
				cli_loop();
			}
		}
		memcpy(nvt_fdt_buffer, (void*)tmp_addr, fdt_real_size);
		free((void*)tmp_addr);
		if (ret) {
			printf("fdt init fail return %d\n", ret);
			return ret;
		}
	#else /* !CONFIG_NVT_IVOT_SOC_FW_UPDATE_SUPPORT */
			/* FIXME: To do customized boot */

	#endif /* CONFIG_NVT_IVOT_SOC_FW_UPDATE_SUPPORT */
	} else {
		ulong fdt_addr = nvt_readl((ulong)nvt_shminfo_boot_fdt_addr);
		ulong fdt_real_size;
		if ((ret = fdt_check_header((void *)fdt_addr)) != 0) {
			printf("invalid fdt header, addr=0x%08X er = %d \n", (unsigned int)fdt_addr, ret);
			return ret;
		}

		fdt_real_size = (ulong)fdt_totalsize((void*)fdt_addr);
		if (fdt_real_size <= SZ_512K) {
			/* Copy fdt data from loader fdt area to uboot fdt area */
			memcpy(nvt_fdt_buffer, (void*)fdt_addr, fdt_real_size);
		} else {
			printf("%s: fdt_real_size 0x%lx > 512K\n", __func__, fdt_real_size);
			cli_loop();
		}
	}

	return 0;
}

int nvt_check_isfdt(ulong addr)
{
	int  nodeoffset, ret;
	ret = fdt_check_header((void *)addr);
	if (ret < 0) {
		debug("fdt address %lx is not valid with error code: %d\n", addr, ret);
		return -1;
	}

	return 0;
}

void nvt_print_system_info(void)
{
	nvt_dbg(IND, "\r%s \
			\r CONFIG_MEM_SIZE                =      0x%08x \n \
			\r CONFIG_NVT_UIMAGE_SIZE         =      0x%08x \n \
			\r CONFIG_NVT_ALL_IN_ONE_IMG_SIZE =      0x%08x \n \
			\r CONFIG_UBOOT_SDRAM_BASE        =      0x%08x \n \
			\r CONFIG_UBOOT_SDRAM_SIZE        =      0x%08x \n \
			\r CONFIG_LINUX_SDRAM_BASE        =      0x%08x \n \
			\r CONFIG_LINUX_SDRAM_SIZE        =      0x%08x \n \
			\r CONFIG_LINUX_SDRAM_START       =      0x%08x %s \n", ANSI_COLOR_YELLOW, CONFIG_MEM_SIZE,
										CONFIG_NVT_UIMAGE_SIZE, CONFIG_NVT_ALL_IN_ONE_IMG_SIZE,
										CONFIG_UBOOT_SDRAM_BASE, CONFIG_UBOOT_SDRAM_SIZE,
										CONFIG_LINUX_SDRAM_BASE, CONFIG_LINUX_SDRAM_SIZE,
										CONFIG_LINUX_SDRAM_START, ANSI_COLOR_RESET);
}

unsigned long get_nvt_timer0_cnt(void)
{
	return nvt_readl((ulong)NVT_TIMER0_CNT);
}

int nvt_board_init(void)
{
	int ret = 0;

#ifndef CONFIG_NVT_IVOT_QEMU_SIM
	ret = nvt_ivot_hw_init();
#endif
	if (ret < 0)
		return -1;

#ifndef CONFIG_NVT_IVOT_SOC_FW_UPDATE_SUPPORT
	ret = nvt_dts_config_parsing();
	if (ret < 0)
		return -1;
#endif /* CONFIG_NVT_IVOT_SOC_FW_UPDATE_SUPPORT */

	return 0;
}

int nvt_board_init_early(void)
{
	int ret = 0;

	ret = nvt_ivot_hw_init_early();
	if (ret < 0)
		return -1;

	return 0;
}

int nvt_ivot_set_cpuclk(void)
{
	int  nodeoffset = -1, subnode = -1, ret = -1, ret_apb = -1;
	u32 *cell = NULL;
	u32 cpu_freq = 0;
	u32 apb_freq = 0;
	char cmd[512];
	char buf[80];

	memset(cmd, 0, 512);
	memset(buf, 0, 80);

#ifdef CONFIG_TARGET_NA51103
	writel(0x7F, 0xf0024104);
#endif

	/* We will find the cpu clock node firstly */
	sprintf(cmd, "/cpus");
	nodeoffset = fdt_path_offset((void*)nvt_fdt_buffer, cmd);
	subnode = fdt_first_subnode((void*)nvt_fdt_buffer, nodeoffset);
	cell = (u32*)fdt_getprop((const void*)nvt_fdt_buffer, subnode, "clock-frequency", NULL);
	if (cell == NULL) {
		return 0;
	} else {
		cpu_freq = __be32_to_cpu(cell[0]) / 1000000;
		printf("DTS find cpu freq clock %dMHz\n", cpu_freq);
	}

	sprintf(cmd, "nvt_cpu_freq %d", cpu_freq);
	ret = run_command(cmd, 0);

	if (ret) {
		printf("run do_nvt_cpu_freq fail\n");
	}

	cell = (u32*)fdt_getprop((const void*)nvt_fdt_buffer, subnode, "apb-frequency", NULL);
	if (cell == NULL) {
		printf("no apb clock dtsi\n");
		return ret;
	} else {
		apb_freq = __be32_to_cpu(cell[0]) / 1000000;
		printf("DTS find apb freq clock %dMHz\n", apb_freq);
	}

	sprintf(cmd, "nvt_apb_freq %d", apb_freq);
	ret_apb = run_command(cmd, 0);

	if (ret_apb) {
		printf("run do_nvt_apb_freq fail\n");
	}

	return (ret + ret_apb);
}

/*
parsing non secure share memory address and size

*/

#ifdef CONFIG_NVT_IVOT_OPTEE_SUPPORT
int nvt_dts_optee_nsmem(unsigned long *addr, unsigned long* size)
{
	char cmd[512];
	int  nodeoffset = -1;
	int subnode = -1;
	const unsigned long *val = NULL;
//	u64 addr, size;
	memset(cmd, 0, 512);
	sprintf(cmd, "/nvt_memory_cfg");

	nodeoffset = fdt_path_offset((void*)nvt_fdt_buffer, cmd);
	if(nodeoffset < 0)
	{
		printf("can not find nvt_memory_cfg in dts\r\n");
		return -1;
	}
	subnode = fdt_subnode_offset(nvt_fdt_buffer,nodeoffset,"nsmem");
	if(subnode < 0)
	{
		printf("can not find nsmem in dts\r\n");
		return -1;
	}
	val = (const unsigned long *)fdt_getprop(nvt_fdt_buffer, subnode, "reg", NULL);
	*addr = be32_to_cpu(val[0]);
	*size = be32_to_cpu(val[1]);
	return 0;
}
#endif
/*
 * to parsing necessary dts info
 * format: phandle is /nand, /nor and
 *
 */
int nvt_dts_config_parsing(void)
{
	int  nodeoffset = -1, nextoffset, subnode, ret;
	const char *ptr = NULL;
	char *endptr = NULL;
	u64 addr, size;
	const unsigned long long *val = NULL;
	unsigned int idx = 0;
	char cmd[512];
	char buf[80];

	memset(cmd, 0, 512);
	memset(buf, 0, 80);
	/* We will find the partition table node firstly */
	sprintf(cmd, "/nand");
	nodeoffset = fdt_path_offset((void*)nvt_fdt_buffer, cmd);
	subnode = fdt_first_subnode((void*)nvt_fdt_buffer, nodeoffset);
	ptr = fdt_get_name((const void*)nvt_fdt_buffer, subnode, NULL);
	if (ptr != NULL && strncmp(ptr, "partition_", 10) == 0) {
		sprintf(cmd, "nand0=spi_nand.0");
		ret = env_set("mtdids", cmd);
		if (ret) {
			nvt_dbg(ERR, "%s: error set\n", __func__);
			return ret;
		}

		sprintf(cmd, "mtdparts=spi_nand.0:");
		goto nvt_dts_cfg_getnode;
	}

	sprintf(cmd, "/nor");
	nodeoffset = fdt_path_offset((void*)nvt_fdt_buffer, cmd);
	subnode = fdt_first_subnode((void*)nvt_fdt_buffer, nodeoffset);
	ptr = fdt_get_name((const void*)nvt_fdt_buffer, subnode, NULL);
	if (ptr != NULL && strncmp(ptr, "partition_", 10) == 0) {
		sprintf(cmd, "nor0=spi_nor.0");
		ret = env_set("mtdids", cmd);
		if (ret) {
			nvt_dbg(ERR, "%s: error set\n", __func__);
			return ret;
		}

		sprintf(cmd, "mtdparts=spi_nor.0:");
		goto nvt_dts_cfg_getnode;
	}

	sprintf(cmd, "/emmc");
	nodeoffset = fdt_path_offset((void*)nvt_fdt_buffer, cmd);
	subnode = fdt_first_subnode((void*)nvt_fdt_buffer, nodeoffset);
	ptr = fdt_get_name((const void*)nvt_fdt_buffer, subnode, NULL);
	if (nodeoffset < 0 || strncmp(ptr, "partition_", 10) != 0)
		return -1;
	else
		sprintf(cmd, "mtdparts=emmc:");

nvt_dts_cfg_getnode:
	fdt_for_each_subnode(subnode, nvt_fdt_buffer, nodeoffset) {
		/* Got every subnode */
		ptr = fdt_getprop(nvt_fdt_buffer, subnode, "label", NULL);
		val = (const unsigned long long *)fdt_getprop(nvt_fdt_buffer, subnode, "reg", NULL);
		if (val == NULL) {
			nvt_dbg(FUNC, "Label: %s get reg value failed\n", ptr);
			return -1;
		}
		addr = be64_to_cpu(val[0]);
		size = be64_to_cpu(val[1]);
		if (ptr != NULL && strncmp(ptr, "all", 3) != 0) {
			nvt_dbg(FUNC, "Label: %s PartitionOffset=%llx PartitionSize=%llx\n", ptr, addr, size);
			sprintf(buf, "0x%llx@0x%llx(%s),", size, addr, ptr);
			strcat(cmd, buf);
		}
	}

	/* To handle uboot mtd env config */
	cmd[strlen(cmd) - 1] = '\0';
	ret = env_set("mtdparts", cmd);

	return 0;
}

#ifdef CONFIG_NVT_IVOT_EMMC
lbaint_t nvt_mmc_sparse_write(struct sparse_storage *info, lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
	char command[128];

	sprintf(command, "mmc write 0x%lx 0x%lx 0x%lx", (unsigned long)buffer, blk, blkcnt);
	run_command(command, 0);
	return blkcnt;
}

lbaint_t nvt_mmc_sparse_read(struct sparse_storage *info, lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
	char command[128];

	sprintf(command, "mmc read 0x%lx 0x%lx 0x%lx", (unsigned long)buffer, blk, blkcnt);
	run_command(command, 0);
	return blkcnt;
}

lbaint_t nvt_mmc_sparse_reserve(struct sparse_storage *info, lbaint_t blk, lbaint_t blkcnt)
{
	return blkcnt;
}

void nvt_mmc_sparse_msg_print(const char *str, char *response)
{
	nvt_dbg(IND, "%s\n", str);
	return;
}

int nvt_sparse_image_update(u64 addr, u64 off, u64 size, u64 part_size)
{
	char command[128];
	u64 align_off = ALIGN_CEIL(off, MMC_MAX_BLOCK_LEN);
	/* Using block unit */
	align_off /= MMC_MAX_BLOCK_LEN;
	u64 align_size = ALIGN_CEIL(size, MMC_MAX_BLOCK_LEN);
	/* Using block unit */
	align_size /= MMC_MAX_BLOCK_LEN;
	if (is_sparse_image((void*)(unsigned long)addr)) {
		struct sparse_storage sparse;
		sparse.blksz = MMC_MAX_BLOCK_LEN;
		sparse.start = (lbaint_t)align_off;
		/* It's used to check if update size is larger than partition size */
		sparse.size = (lbaint_t)part_size/MMC_MAX_BLOCK_LEN;
		sparse.read = NULL;
		sparse.write = nvt_mmc_sparse_write;
		sparse.reserve = nvt_mmc_sparse_reserve;
		sparse.mssg = nvt_mmc_sparse_msg_print;
		void		(*mssg)(const char *str, char *response);
		printf("Flashing sparse ext4 image at offset 0x%lx\n", sparse.start);
		write_sparse_image(&sparse, "Rootfs.ext4", (void *)(unsigned long)addr, NULL);
	} else {
		printf("Flashing raw ext4 or fat image at offset 0x%llx\n", align_off);
		sprintf(command, "mmc write 0x%llx 0x%llx 0x%x", addr, align_off, align_size);
		run_command(command, 0);
	}

	return 0;
}

int nvt_sparse_image_readback(u64 addr, u64 off, u64 size, u64 part_size)
{
	char command[128];
	int ret = 0;
	u64 align_off = ALIGN_CEIL(off, MMC_MAX_BLOCK_LEN);
	/* Using block unit */
	align_off /= MMC_MAX_BLOCK_LEN;
	u64 align_size = ALIGN_CEIL(size, MMC_MAX_BLOCK_LEN);
	/* Using block unit */
	align_size /= MMC_MAX_BLOCK_LEN;
	if (is_sparse_image((void*)(unsigned long)addr)) {
			struct sparse_storage sparse;
			sparse.blksz = MMC_MAX_BLOCK_LEN;
			sparse.start = (lbaint_t)align_off;
			sparse.size = (lbaint_t)part_size/MMC_MAX_BLOCK_LEN;
			sparse.write = NULL;
			sparse.read = nvt_mmc_sparse_read;
			sparse.reserve = nvt_mmc_sparse_reserve;
			printf("Read sparse ext4 image at offset 0x%lx\n", sparse.start);
			ret = read_sparse_image(&sparse, "Rootfs.ext4", (void *)(unsigned long)addr, NULL);
			if (ret < 0) {
					nvt_dbg(ERR, "Read spares image failed with addr: 0x%08x emmc offset: 0x%08x\n", addr, off);
					return -1;
			}
	} else {
			printf("Read raw ext4 image at offset 0x%llx\n", off);
			sprintf(command, "mmc read 0x%llx 0x%llx 0x%llx", addr, align_off, align_size);
			run_command(command, 0);
	}

	return 0;
}
#endif /* CONFIG_NVT_LINUX_EMMC_BOOT */
