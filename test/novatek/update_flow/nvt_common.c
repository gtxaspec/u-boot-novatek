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
#include <asm/nvt-common/nvt_common.h>

static int update_flow_get_part_and_flash_type(struct unit_test_state *uts)
{
	int id;
	loff_t part_off = 0, part_size = 0;
	nvt_flash_type_t FlashType;
	int failed_count= 0;

	EMB_PARTITION* pEmb = (EMB_PARTITION*)emb_partition_info_data_curr;
	for (id = 0; id < EMB_PARTITION_INFO_COUNT; id++) {

		if (pEmb[id].EmbType == EMBTYPE_FDT) 
		{
			get_part_and_flash_type("fdt", &part_off, &part_size, &FlashType);
			if ((pEmb[id].PartitionOffset != (u64)part_off ) & (pEmb[id].PartitionSize != (u64)part_size)
				& (pEmb[id].FlashType != FlashType ))
			{
				printf("==== fdt ====\n");
				printf("Golden: offset = 0x%llx, size = 0x%llx, storage_type: %d\n", 
					pEmb[id].PartitionOffset, pEmb[id].PartitionSize, pEmb[id].FlashType);
				printf("Res: offset = 0x%llx, size = 0x%llx, storage_type: %d\n", 
					part_off, part_size, FlashType);
				failed_count++;
			}

		}

		if (pEmb[id].EmbType == EMBTYPE_UBOOT) 
		{
			get_part_and_flash_type("uboot", &part_off, &part_size, &FlashType);
			if ((pEmb[id].PartitionOffset != (u64)part_off ) & (pEmb[id].PartitionSize != (u64)part_size)
				& (pEmb[id].FlashType != FlashType ))
			{
				printf("==== uboot ====\n");
				printf("Golden: offset = 0x%llx, size = 0x%llx, storage_type: %d\n", 
					pEmb[id].PartitionOffset, pEmb[id].PartitionSize, pEmb[id].FlashType);
				printf("Res: offset = 0x%llx, size = 0x%llx, storage_type: %d\n", 
					part_off, part_size, FlashType);
				failed_count++;
			}
			
		}

		if (pEmb[id].EmbType == EMBTYPE_LINUX) 
		{
			get_part_and_flash_type("linux", &part_off, &part_size, &FlashType);
			if ((pEmb[id].PartitionOffset != (u64)part_off ) & (pEmb[id].PartitionSize != (u64)part_size)
				& (pEmb[id].FlashType != FlashType ))
			{
				printf("==== uboot ====\n");
				printf("Golden: offset = 0x%llx, size = 0x%llx, storage_type: %d\n", 
					pEmb[id].PartitionOffset, pEmb[id].PartitionSize, pEmb[id].FlashType);
				printf("Res: offset = 0x%llx, size = 0x%llx, storage_type: %d\n", 
					part_off, part_size, FlashType);
					failed_count++;
				
			}
		}



		if (pEmb[id].EmbType == EMBTYPE_ROOTFS) 
		{
			if(pEmb[id].OrderIdx == 0)
			{
				get_part_and_flash_type("rootfs0", &part_off, &part_size, &FlashType);
				if ((pEmb[id].PartitionOffset != (u64)part_off ) & (pEmb[id].PartitionSize != (u64)part_size)
					& (pEmb[id].FlashType != FlashType ))
				{
					printf("==== rootfs0 ====\n");
					printf("Golden: offset = 0x%llx, size = 0x%llx, storage_type: %d\n", 
						pEmb[id].PartitionOffset, pEmb[id].PartitionSize, pEmb[id].FlashType);
					printf("Res: offset = 0x%llx, size = 0x%llx, storage_type: %d\n", 
						part_off, part_size, FlashType);
					failed_count++;
				
				}
			}
		}



	}

	ut_asserteq(0, failed_count);
	return 0;
}
NOVATEK_TEST(update_flow_get_part_and_flash_type, 0);

