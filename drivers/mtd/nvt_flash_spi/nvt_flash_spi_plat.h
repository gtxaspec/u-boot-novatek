/*
    Copyright	Novatek Microelectronics Corp. 2025.  All rights reserved.

    @file	nvt_flash_spi_plat.h

    @note	Novatek NAND flash controller platform parameter definitions

 */
#ifndef _NVT_FLASH_SPI_PLAT_H_
#define _NVT_FLASH_SPI_PLAT_H_

/*
	NVT Flash controller platform parameters
*/
#if (defined(CONFIG_TARGET_NA51055) || defined(CONFIG_TARGET_NA51055_A64) || \
	 defined(CONFIG_TARGET_NA51084) || defined(CONFIG_TARGET_NA51084_A64) || \
	 defined(CONFIG_TARGET_NA51089) || defined(CONFIG_TARGET_NA51089_A64))
	#define NVT_INDLY_STEP              (64)
	#define SPINAND_FIFO_SIZE           (64)
	#define NVT_BCH_ECC_SUPPORT         (0)
	#define NVT_OCTAL_IO_SUPPORT        (0)
	#define NVT_SPARE_32_SEL_2BIT       (0)
#elif  (defined(CONFIG_TARGET_NS02301) || defined(CONFIG_TARGET_NS02301_A64) || \
		defined(CONFIG_TARGET_NS02302) || defined(CONFIG_TARGET_NS02302_A64) || \
		defined(CONFIG_TARGET_NS02401) || defined(CONFIG_TARGET_NS02401_A64) || \
		defined(CONFIG_TARGET_NS02501) )
	#define NVT_INDLY_STEP              (128)
	#define SPINAND_FIFO_SIZE           (128)
	#define NVT_BCH_ECC_SUPPORT         (1)
	#define NVT_OCTAL_IO_SUPPORT        (1)
	#define NVT_SPARE_32_SEL_2BIT       (1)
#elif  (defined(CONFIG_TARGET_NA51103) || defined(CONFIG_TARGET_NA51103_A64))
	#define NVT_INDLY_STEP              (128)
	#define SPINAND_FIFO_SIZE           (128)
	#define NVT_BCH_ECC_SUPPORT         (0)
	#define NVT_OCTAL_IO_SUPPORT        (0)
	#define NVT_SPARE_32_SEL_2BIT       (0)
#else	// (NA51102, NS02201)
	#define NVT_INDLY_STEP              (128)
	#define SPINAND_FIFO_SIZE           (128)
	#define NVT_BCH_ECC_SUPPORT         (1)
	#define NVT_OCTAL_IO_SUPPORT        (0)
	#define NVT_SPARE_32_SEL_2BIT       (0)
#endif


#endif	// _NVT_FLASH_SPI_PLAT_H_ END