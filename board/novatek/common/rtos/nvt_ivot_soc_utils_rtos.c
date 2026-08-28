/**
    NVT utilities for command customization

    @file       nvt_ivot_soc_utils_rtos.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2022.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <cpu_func.h>
#include <command.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/modelext/emb_partition_info.h>
#include <asm/nvt-common/modelext/bin_info.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/nvt-common/shm_info.h>
#include <asm/arch/IOAddress.h>
#include <asm/cache.h>
#include <lzma/LzmaTools.h>
#include <mmc.h>
#include "../nvt_ivot_soc_utils.h"
#include "../nvt_ivot_pack.h"

#if defined(__aarch64__)
#define FW_PART1_SIZE_OFFSET (RTOS_CODEINFO_OFFSET + 0x28) //ref to CodeInfo.S on rtos (addr of _section_01_size)
#else
#define FW_PART1_SIZE_OFFSET (RTOS_CODEINFO_OFFSET + 0x1C) //ref to CodeInfo.S on rtos (addr of _section_01_size)
#endif

#define UINT32_SWAP(data)           (((((u32)(data)) & 0x000000FF) << 24) | \
									 ((((u32)(data)) & 0x0000FF00) << 8) | \
									 ((((u32)(data)) & 0x00FF0000) >> 8) | \
									 ((((u32)(data)) & 0xFF000000) >> 24))   ///< Swap [31:24] with [7:0] and [23:16] with [15:8].

extern HEADINFO gHeadInfo;

int nvt_chk_rtos(ulong addr, unsigned int size, unsigned int pat_id)
{
	if(MemCheck_CalcCheckSum16Bit(addr, size)!=0)
	{
		printf("ecos pat%d, res check sum fail.\r\n",pat_id);
		return -1;
	}
	return 0;
}

int nvt_on_rtos_partition_enum_copy_to_dest(unsigned int id, NVTPACK_MEM *p_mem, void *p_user_data)
{
	int ret = 0;
	ulong rtos_addr, rtos_size;
	ulong dram_addr, dram_size;
	EMB_PARTITION *pEmb = (EMB_PARTITION *)p_user_data;
	NVTPACK_BFC_HDR *pBFC = (NVTPACK_BFC_HDR *)p_mem->p_data;

	rtos_addr = nvt_memory_cfg[MEMTYPE_RTOS].addr;
	rtos_size = nvt_memory_cfg[MEMTYPE_RTOS].size;

	/* Copy to dest address */
	switch (pEmb[id].EmbType) {
	case EMBTYPE_RTOS:
		/* Boot compressed rtos or non-compressed rtos */
		if (pBFC->uiFourCC == MAKEFOURCC('B', 'C', 'L', '1')) {
			printf("RUNFW: Boot compressed rtos at 0x%08x with length %d bytes\r\n",
				   (ulong)p_mem->p_data, p_mem->len);
			if ((cpu_to_be32(pBFC->uiAlgorithm) & 0xFF) == 11) {
				/* lzma compressed image*/
				printf("%sDecompressed lzma rtos%s \r\n", ANSI_COLOR_YELLOW, ANSI_COLOR_RESET);
				size_t Compsz = cpu_to_be32(pBFC->uiSizeComp);
				size_t Uncompsz = cpu_to_be32(pBFC->uiSizeUnComp);
				ret = lzmaBuffToBuffDecompress((unsigned char *)rtos_addr, &Uncompsz, (unsigned char *)(p_mem->p_data + sizeof(NVTPACK_BFC_HDR)), Compsz);
				if (ret != 0) {
					printf("Decompressed lzma fail\n");
					return ret;
				}  else {
					flush_dcache_range((ulong)rtos_addr, (ulong)(rtos_addr + Uncompsz));
				}
			} else {
				/* lz77 compressed image*/
				lz_uncompress((unsigned char *)(p_mem->p_data + sizeof(NVTPACK_BFC_HDR)), (unsigned char *)rtos_addr, cpu_to_be32(pBFC->uiSizeComp));
				flush_dcache_range((ulong)rtos_addr,
								   (ulong)(rtos_addr + rtos_size));
			}
		} else {
			dram_addr = nvt_memory_cfg[MEMTYPE_DRAM].addr;
			dram_size = nvt_memory_cfg[MEMTYPE_DRAM].size;

			printf("RUNFW: Copy rtos from 0x%08x to 0x%08x with length %d bytes\r\n",
				   (ulong)p_mem->p_data, (ulong)rtos_addr, p_mem->len);
			memcpy((void *)rtos_addr, (void *)p_mem->p_data, p_mem->len);
			flush_dcache_range((ulong)rtos_addr, (ulong)(rtos_addr + p_mem->len));

			//check partial compress load
			BININFO *pbininfo = (BININFO *)(rtos_addr + BIN_INFO_OFFSET_RTOS);
			SHMINFO *p_shminfo = (SHMINFO *)nvt_memory_cfg[MEMTYPE_SHMEM].addr;

			//Pass HEADINFO_RESV_IDX_DRAM_SIZE information to freeRTOS (MMU mapping necessary)
			//Once fail => dram_size = 0
			pbininfo->head.Resv1[HEADINFO_RESV_IDX_DRAM_SIZE] = dram_size;

			if (pbininfo->head.Resv1[HEADINFO_RESV_IDX_BOOT_FLAG] & BOOT_FLAG_PARTLOAD_EN) {
				u32 part1_size = *(u32 *)(rtos_addr + FW_PART1_SIZE_OFFSET);
				//align to block size
				part1_size = ALIGN_CEIL(part1_size, _EMBMEM_BLK_SIZE_);
				printf("part1_size_aligned=%08X\r\n", part1_size);
				pBFC = (NVTPACK_BFC_HDR *)((unsigned long)p_mem->p_data + part1_size);
				if (pBFC->uiFourCC == MAKEFOURCC('B', 'C', 'L', '1')) {
					printf("RUNFW: decompress gzip.\r\n");
					UINT32 uiLength = UINT32_SWAP(pBFC->uiSizeComp);
					UINT32 uiSizeComp = UINT32_SWAP(pBFC->uiSizeComp);
					UINT32 uiSizeUnComp = UINT32_SWAP(pBFC->uiSizeUnComp);
					UINT32 uiAddrCompress = ((unsigned long)pBFC) + sizeof(NVTPACK_BFC_HDR);
					int decoded_size = gz_uncompress((unsigned char *)(unsigned long)(uiAddrCompress)
													 , (unsigned char *)(rtos_addr + part1_size)
													 , uiLength
													 , uiSizeUnComp);
					if (decoded_size == 0) {
						printf("Decompressed gzip fail\n");
						return -1;
					}
					flush_dcache_range((ulong)rtos_addr, (ulong)(rtos_addr + pbininfo->head.BinLength));
				}
			}
			//update LdLoadSize
			p_shminfo->boot.LdLoadSize = pbininfo->head.BinLength;
			flush_dcache_range((ulong)p_shminfo, (ulong)p_shminfo + sizeof(SHMINFO));
		}
		break;
	default:
		break;
	}

	return 0;
}

int nvt_runfw_bin_unpack_to_dest_rtos(ulong addr)
{
	u32 i;
	EMB_PARTITION *pEmb = emb_partition_info_data_curr;
	int ret = 0;
	unsigned int size = gHeadInfo.Resv1[0];
	NVTPACK_GET_PARTITION_INPUT np_get_input;
	NVTPACK_ENUM_PARTITION_INPUT np_enum_input;
	NVTPACK_MEM mem_in = {(void *)addr, size};
	NVTPACK_MEM mem_out = {0};

	memset(&np_get_input, 0, sizeof(np_get_input));
	memset(&np_enum_input, 0, sizeof(np_enum_input));

	ret = nvt_getfdt_emb((ulong)nvt_fdt_buffer, pEmb);
	if (ret < 0) {
		printf("failed to get current partition resource.\r\n");
		return -1;
	}

	/* Check for all partition existed */
	for (i = 1; i < EMB_PARTITION_INFO_COUNT; i++) {
		if (pEmb[i].PartitionSize != 0) {
			switch (pEmb[i].EmbType) {
			case EMBTYPE_FDT:
			case EMBTYPE_RTOS:
			case EMBTYPE_UBOOT:
				//check this type exist in all-in-one
				np_get_input.id = i;
				np_get_input.mem = mem_in;
				if (nvtpack_check_partition(pEmb[i], &np_get_input, &mem_out) != NVTPACK_ER_SUCCESS) {
					printf("RUNFW boot: need partition[%d]\n", i);
					return -1;
				}
				break;
			}
		}
	}

	/* Enum all partition to do necessary handling */
	np_enum_input.mem = mem_in;
	np_enum_input.p_user_data = pEmb;
	/* Loading images */
	np_enum_input.fp_enum = nvt_on_rtos_partition_enum_copy_to_dest;

	if (nvtpack_enum_partition(&np_enum_input) != NVTPACK_ER_SUCCESS) {
		printf("failed sanity.\r\n");
		return -1;
	}

	/* Make sure images had been already copied */
	flush_dcache_all();

	return 0;
}

u32 nvt_get_32bits_data(ulong addr, bool is_little_endian)
{
	u_int32_t value = 0;
	u_int8_t *pb = (u_int8_t *)addr;
	if (is_little_endian) {
		if (addr & 0x3) { //NOT word aligned
			value = (*pb);
			value |= (*(pb + 1)) << 8;
			value |= (*(pb + 2)) << 16;
			value |= (*(pb + 3)) << 24;
		} else {
			value = *(u_int32_t *)addr;
		}
	} else {
		value = (*pb) << 24;
		value |= (*(pb + 1)) << 16;
		value |= (*(pb + 2)) << 8;
		value |= (*(pb + 3));
	}
	return value;
}

int nvt_boot_rtos_bin_auto(void)
{
	ulong rtos_addr = 0, rtos_size = 0;
	ulong dram_addr, dram_size;
	char cmd[256] = {0};
	int ret = 0;
	unsigned int tmp_addr = 0;
	u_int32_t codesize = 0;
	u32 align_size = 0;
	char *pCodeInfo = NULL;
	BININFO *pbininfo = NULL;
	SHMINFO *p_shminfo = (SHMINFO *)nvt_memory_cfg[MEMTYPE_SHMEM].addr;
#if defined(CONFIG_NVT_SPI_NOR) || defined(CONFIG_NVT_IVOT_EMMC) || defined(CONFIG_NVT_SPI_NOR_NAND)
	loff_t part_off = 0, part_size = 0;
#endif

#if defined(CONFIG_NVT_IVOT_DUAL_RTOS)
	unsigned int boot_bank = nvt_decide_dual_rtos_idx();
#else
	unsigned int boot_bank = 0;
#endif

	unsigned int is_secure = 0;
#if defined(CONFIG_NVT_IVOT_SECBOOT_SUPPORT) && defined(CONFIG_ENCRYPT_RTOS)
	is_secure = is_secure_enable();
#endif


	char rtos_name[8] = {0};
	switch (boot_bank) {
	case 0:
		strcpy(rtos_name, "rtos");
		break;
	case 1:
		strcpy(rtos_name, "rtos1");
		break;
	default:
		printf("%s no healthy rtos can be used. %s\r\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
		while (1);
	}

	rtos_addr = nvt_memory_cfg[MEMTYPE_RTOS].addr;
	rtos_size = nvt_memory_cfg[MEMTYPE_RTOS].size;

	if (nvt_detect_fw_tbin()) {
		/* Check fw is valid */
		nvt_runfw_bin_chk_valid((ulong)nvt_memory_cfg[MEMTYPE_ALL_IN_ONE].addr);
		/* Copy rtos binary to destination address */
		ret = nvt_runfw_bin_unpack_to_dest_rtos((ulong)nvt_memory_cfg[MEMTYPE_ALL_IN_ONE].addr);;
		if (ret < 0) {
			return ret;
		}
	} else {
		// get rtos image size firstly
#if defined(CONFIG_NVT_LINUX_SPINAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NAND)) \
        || defined(CONFIG_NVT_LINUX_SPINORNAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR_NAND))
		sprintf(cmd, "nand read 0x%x %s 0x500", rtos_addr, rtos_name);
#elif defined(CONFIG_NVT_LINUX_SPINOR_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR))
		get_part(rtos_name, &part_off, &part_size);
		sprintf(cmd, "sf read 0x%x 0x%llx 0x500 ", rtos_addr, part_off);
#elif defined(CONFIG_NVT_LINUX_EMMC_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_IVOT_EMMC))
		get_part(rtos_name, &part_off, &part_size);
		sprintf(cmd, "mmc read 0x%x 0x%llx 4", rtos_addr, part_off / MMC_MAX_BLOCK_LEN);
#endif
		printf("=>run cmd : %s\n", cmd);
		run_command(cmd, 0);
		// Reading rtos image
		NVTPACK_BFC_HDR *pbfc = (NVTPACK_BFC_HDR *)rtos_addr;
		if (pbfc->uiFourCC == MAKEFOURCC('B', 'C', 'L', '1')) {
			printf("Compressed rtos\n");
			/* rtos image size will be put in the end of rtos region */
			size_t firmware_size = cpu_to_be32(pbfc->uiSizeComp) + sizeof(NVTPACK_BFC_HDR);
			tmp_addr = rtos_addr + rtos_size - firmware_size;
			if ((firmware_size + cpu_to_be32(pbfc->uiSizeUnComp)) > rtos_size) {
				printf("%s Attention!!! rtos starting address at 0x%08x with size 0x%08x (uncompressed) is larger than temp buffer address!! 0x%08x %s\r\n",
					   ANSI_COLOR_RED, rtos_addr, cpu_to_be32(pbfc->uiSizeUnComp), tmp_addr, ANSI_COLOR_RESET);
				printf("%s It can't decompress %s\r\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
				while (1);
			}
#if defined(CONFIG_NVT_LINUX_SPINAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NAND)) \
            || defined(CONFIG_NVT_LINUX_SPINORNAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR_NAND))
			sprintf(cmd, "nand read 0x%x %s 0x%x ", tmp_addr, rtos_name, firmware_size);
#elif defined(CONFIG_NVT_LINUX_SPINOR_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR))
			sprintf(cmd, "sf read 0x%x 0x%llx 0x%x ", tmp_addr, part_off, firmware_size);
#elif defined(CONFIG_NVT_LINUX_EMMC_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_IVOT_EMMC))
			align_size = ALIGN_CEIL(firmware_size, MMC_MAX_BLOCK_LEN) / MMC_MAX_BLOCK_LEN;
			sprintf(cmd, "mmc read 0x%x 0x%llx 0x%x", tmp_addr, part_off / MMC_MAX_BLOCK_LEN, align_size);
#endif
			printf("=>run cmd : %s\n", cmd);
			run_command(cmd, 0);

			debug("auto boot bfc UnCompsz:%d Compsz:%d Algorithm: %x \n", cpu_to_be32(pbfc->uiSizeUnComp), cpu_to_be32(pbfc->uiSizeComp), cpu_to_be32(pbfc->uiAlgorithm));
			if ((cpu_to_be32(pbfc->uiAlgorithm) & 0xFF) == 11) {
				/* lzma compressed image*/
				size_t Compsz = cpu_to_be32(pbfc->uiSizeComp);
				size_t Uncompsz = cpu_to_be32(pbfc->uiSizeUnComp);
				printf("Decompressed lzma rtos\n");
				flush_dcache_range((ulong)tmp_addr,
								   (ulong)(tmp_addr + firmware_size));
				ret = lzmaBuffToBuffDecompress((unsigned char *)rtos_addr, &Uncompsz, (unsigned char *)(tmp_addr + sizeof(NVTPACK_BFC_HDR)), Compsz);
				if (ret != 0) {
					printf("Decompressed lzma fail\n");
				}
				flush_dcache_range((ulong)rtos_addr, (ulong)(rtos_addr + Uncompsz));
			} else {
				/* lz77 compressed image*/
				lz_uncompress((unsigned char *)(tmp_addr + sizeof(NVTPACK_BFC_HDR)), (unsigned char *)rtos_addr, cpu_to_be32(pbfc->uiSizeComp));
				flush_dcache_range((ulong)rtos_addr, (ulong)(rtos_addr + rtos_size));
			}
		} else {
			int i;
			int partload;
			unsigned long size;
			dram_addr = nvt_memory_cfg[MEMTYPE_DRAM].addr;
			dram_size = nvt_memory_cfg[MEMTYPE_DRAM].size;
			if (is_secure == 0) {
				pbininfo = (BININFO *)(rtos_addr + BIN_INFO_OFFSET_RTOS);
				partload = pbininfo->head.Resv1[HEADINFO_RESV_IDX_BOOT_FLAG] & BOOT_FLAG_PARTLOAD_EN;
				size = pbininfo->head.BinLength;
			} else {
				HEADINFO *info = (HEADINFO *)(rtos_addr);
				partload = info->Resv1[HEADINFO_RESV_IDX_BOOT_FLAG] & BOOT_FLAG_PARTLOAD_EN;
				size = info->BinLength;
			}
			printf("Non-compressed rtos\n");

			if (partload) {
				// fllush whole rtos memory to invalidate cache on partial load
				flush_dcache_range((ulong)rtos_addr, (ulong)(rtos_addr + rtos_size));
				// preload some to get part-1 size for partial load and partial compressed load
				u32 preload_size = ALIGN_CEIL(FW_PART1_SIZE_OFFSET, _EMBMEM_BLK_SIZE_);
#if defined(CONFIG_NVT_LINUX_SPINAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NAND)) \
                || defined(CONFIG_NVT_LINUX_SPINORNAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR_NAND))
				sprintf(cmd, "nand read 0x%x %s 0x%x ", rtos_addr, rtos_name, preload_size);
#elif defined(CONFIG_NVT_LINUX_SPINOR_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR))
				align_size = ALIGN_CEIL(preload_size, ARCH_DMA_MINALIGN);
				sprintf(cmd, "sf read 0x%x 0x%llx 0x%x", rtos_addr, part_off, align_size);
#elif defined(CONFIG_NVT_LINUX_EMMC_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_IVOT_EMMC))
				align_size = ALIGN_CEIL(preload_size, MMC_MAX_BLOCK_LEN) / MMC_MAX_BLOCK_LEN;
				sprintf(cmd, "mmc read 0x%x 0x%llx 0x%x", rtos_addr, part_off / MMC_MAX_BLOCK_LEN, align_size);
#endif
				run_command(cmd, 0);
				//partial-load or partial-compressed-load
				u32 part1_size = *(u32 *)(rtos_addr + FW_PART1_SIZE_OFFSET);
				printf("part1_size=%08X,preload_size=%08X\r\n", part1_size, preload_size);
				//align to block size
				part1_size = ALIGN_CEIL(part1_size, _EMBMEM_BLK_SIZE_);
				printf("part1_size_aligned=%08X\r\n", part1_size);
				//sprintf(cmd, "nand read 0x%x 0x%x 0x%x", (unsigned int)dst_addr, base_ofs+ofs, size);
#if defined(CONFIG_NVT_LINUX_SPINAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NAND)) \
                || defined(CONFIG_NVT_LINUX_SPINORNAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR_NAND))
				align_size = part1_size;
				sprintf(cmd, "nand read 0x%x %s 0x%x ", rtos_addr, rtos_name, align_size);
				//update LdLoadSize
				p_shminfo->boot.LdLoadSize = align_size;
#elif defined(CONFIG_NVT_LINUX_SPINOR_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR))
				align_size = ALIGN_CEIL(part1_size, ARCH_DMA_MINALIGN);
				sprintf(cmd, "sf read 0x%x 0x%llx 0x%x", rtos_addr, part_off, align_size);
				//update LdLoadSize
				p_shminfo->boot.LdLoadSize = align_size;
#elif defined(CONFIG_NVT_LINUX_EMMC_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_IVOT_EMMC))
				align_size = ALIGN_CEIL(part1_size, MMC_MAX_BLOCK_LEN) / MMC_MAX_BLOCK_LEN;
				sprintf(cmd, "mmc read 0x%x 0x%llx 0x%x", rtos_addr, part_off / MMC_MAX_BLOCK_LEN, align_size);
				//update LdLoadSize
				p_shminfo->boot.LdLoadSize = align_size * MMC_MAX_BLOCK_LEN;
#endif
				flush_dcache_range((ulong)p_shminfo, (ulong)p_shminfo + sizeof(SHMINFO));
			} else {
				//full load
#if defined(CONFIG_NVT_LINUX_SPINAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NAND)) \
                || defined(CONFIG_NVT_LINUX_SPINORNAND_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR_NAND))
				sprintf(cmd, "nand read 0x%x %s 0x%x ", rtos_addr, rtos_name, size);
#elif defined(CONFIG_NVT_LINUX_SPINOR_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_SPI_NOR))
				sprintf(cmd, "sf read 0x%x 0x%llx 0x%x ", rtos_addr, part_off, size);
#elif defined(CONFIG_NVT_LINUX_EMMC_BOOT) || (defined(NVT_LINUX_RAMDISK_SUPPORT) && defined(CONFIG_NVT_IVOT_EMMC))
				sprintf(cmd, "mmc read 0x%x 0x%llx 0x%x", rtos_addr, part_off / MMC_MAX_BLOCK_LEN, size / MMC_MAX_BLOCK_LEN);
#endif
				//update LdLoadSize
				p_shminfo->boot.LdLoadSize = size;
				flush_dcache_range((ulong)p_shminfo, (ulong)p_shminfo + sizeof(SHMINFO));
			}
			printf("=>run cmd : %s\n", cmd);
			run_command(cmd, 0);
			pbininfo->head.Resv1[HEADINFO_RESV_IDX_DRAM_SIZE] = dram_size;
			//printf("dram size %x\n", dram_size);

			// fllush whole rtos memory to invalidate cache
			flush_dcache_range((ulong)rtos_addr, (ulong)(rtos_addr + rtos_size));
		}
	}

#ifdef CONFIG_NVT_IVOT_SECBOOT_SUPPORT
	if (is_secure) {
		/* Decrypt Date */
		nvt_decrypt_aes_on_uboot(rtos_addr);
	}
#endif

	pCodeInfo = (char *)(rtos_addr + RTOS_CODEINFO_OFFSET);
	pbininfo = (BININFO *)(rtos_addr + BIN_INFO_OFFSET_RTOS);

	if (strncmp(pCodeInfo, "CODEINFO", 8) != 0) {
		printf("invalid CODEINFO\r\n");
		return -1;
	}

	/* dram partition rtos address should be the same with headinfo address */
	if (rtos_addr != pbininfo->head.CodeEntry) {
		printf("dram partition rtos addr (%08X) != headinfo(%08X)\r\n"
			   , rtos_addr
			   , pbininfo->head.CodeEntry);
		return -1;
	}

	/* To check if code size is larger than rtos_addr + rtos_size (Not image size) */
	codesize = nvt_get_32bits_data((ulong)(&pCodeInfo[RTOS_CODEINFO_SUB_ZI_LIMIT]), true) - rtos_addr;
	if (codesize > rtos_size) {
		printf("uBoot uiCodeSize(%08X) > dram partition rtos size(%08X)\r\n"
			   , codesize
			   , rtos_size);
		return -1;
	}

#if defined(CONFIG_NVT_IVOT_DUAL_RTOS)
	if (is_secure == 0) {
		nvt_update_dual_rtos_info(pbininfo, boot_bank);
	} else {
		printf("Dual rtos on secure boot is not yet supported!!\n");
	}
#endif

	printf("rtos starting.... 0x%08x\n", rtos_addr);
	{
		typedef void (*BRANCH_CB)(void);
		BRANCH_CB p_func = (BRANCH_CB)rtos_addr;
		p_func();
	}

	return 0;
}

