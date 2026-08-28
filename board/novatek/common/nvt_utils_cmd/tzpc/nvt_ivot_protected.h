#ifndef _DMA_PROTECTED_H_
#define _DMA_PROTECTED_H_

#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/rcw_macro.h>


#ifdef __DBG_COLOR_MONO__
#define DBG_COLOR_GRAY ""
#define DBG_COLOR_RED ""
#define DBG_COLOR_YELLOW ""
#define DBG_COLOR_CYAN ""
#define DBG_COLOR_GREEN ""
#define DBG_COLOR_MAGENTA ""
#define DBG_COLOR_BLUE ""
#define DBG_COLOR_WHITE ""
#define DBG_COLOR_HI_GRAY ""
#define DBG_COLOR_HI_RED ""
#define DBG_COLOR_HI_YELLOW ""
#define DBG_COLOR_HI_CYAN ""
#define DBG_COLOR_HI_GREEN ""
#define DBG_COLOR_HI_MAGENTA ""
#define DBG_COLOR_HI_BLUE ""
#define DBG_COLOR_HI_WHITE ""
#define DBG_COLOR_FATAL ""
#define DBG_COLOR_ERR ""
#define DBG_COLOR_WRN ""
#define DBG_COLOR_UNIT ""
#define DBG_COLOR_FUNC ""
#define DBG_COLOR_IND ""
#define DBG_COLOR_MSG ""
#define DBG_COLOR_VALUE ""
#define DBG_COLOR_USER ""
#define DBG_COLOR_END ""
#else
#define DBG_COLOR_GRAY "\033[0;30m"
#define DBG_COLOR_RED "\033[0;31m"
#define DBG_COLOR_YELLOW "\033[0;33m"
#define DBG_COLOR_CYAN "\033[0;36m"
#define DBG_COLOR_GREEN "\033[0;32m"
#define DBG_COLOR_MAGENTA "\033[0;35m"
#define DBG_COLOR_BLUE "\033[0;34m"
#define DBG_COLOR_WHITE "\033[0;37m"
#define DBG_COLOR_HI_GRAY "\033[1;30m"
#define DBG_COLOR_HI_RED "\033[1;31m"
#define DBG_COLOR_HI_YELLOW "\033[1;33m"
#define DBG_COLOR_HI_CYAN "\033[1;36m"
#define DBG_COLOR_HI_GREEN "\033[1;32m"
#define DBG_COLOR_HI_MAGENTA "\033[1;35m"
#define DBG_COLOR_HI_BLUE "\033[1;34m"
#define DBG_COLOR_HI_WHITE "\033[1;37m"
#define DBG_COLOR_FATAL "\033[1;31m"
#define DBG_COLOR_ERR "\033[1;31m"
#define DBG_COLOR_WRN "\033[1;33m"
#define DBG_COLOR_UNIT "\033[0;32m"
#define DBG_COLOR_FUNC "\033[0;36m"
#define DBG_COLOR_IND ""
#define DBG_COLOR_MSG ""
#define DBG_COLOR_VALUE ""
#define DBG_COLOR_USER ""
#define DBG_COLOR_END "\033[0m"
#endif

#ifndef ENUM_DUMMY4WORD
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000
#endif
/*
    Translate DRAM address to physical address.

    Translate DRAM address to physical address.

    @param[in] addr     DRAM address

    @return physical DRAM address
*/
#define         dma_getPhyAddr(addr)            ((((UINT32)(addr))>=0x60000000UL)?((UINT32)(addr)-0x60000000UL):(UINT32)(addr))

/*
    Translate DRAM address to non-cacheable address.

    Translate DRAM address to non-cacheable address.

    @param[in] addr     DRAM address

    @return non-cacheable DRAM address
*/
#define         dma_getNonCacheAddr(addr)       ((((UINT32)(addr))<0x60000000UL)?((UINT32)(addr)+0x60000000UL):(UINT32)(addr))

#ifdef CONFIG_TARGET_NS02302_A64
typedef struct {
	UINT32 CPU: 1;   //CPU_S
	UINT32 CPU_NS: 1;
	UINT32 HVYLD: 1; 
	UINT32 reserved: 1;
	//M1
	UINT32 AGE: 1;
	UINT32 SSCA: 1;
	UINT32 OSG: 1;		
	UINT32 GPE: 1;		
	UINT32 GPE2: 1;			
	UINT32 JPG: 1;
	UINT32 SMC: 1;
	UINT32 SDIO: 1;
	UINT32 DAI3: 1;
	UINT32 DAI4: 1;
	UINT32 ETH: 1;
	UINT32 ETH2: 1;
	UINT32 USB: 1;
	UINT32 USB2: 1;
	UINT32 SATA: 1;
	UINT32 SATA2: 1;
	UINT32 LARB: 1;
	UINT32 SCE: 1;
	UINT32 HASH: 1;
	UINT32 HWCP: 1;
	UINT32 HVYLD2: 1;
	UINT32 reserved3: 1;
	UINT32 reserved4: 1;
	UINT32 reserved5: 1;

	//M2
	UINT32 CNN: 1;
	UINT32 NUE: 1;
	UINT32 NUE2: 1;
	UINT32 VPE_0: 1;
	UINT32 VPE_1: 1;
	UINT32 reserved6: 1;
	UINT32 reserved7: 1;
	UINT32 reserved8: 1;
	//M3
	UINT32 VENC_0_1: 1;
	UINT32 HVYLD3: 1;
	UINT32 reserved9: 1;
	UINT32 reserved10: 1;
	UINT32 reserved11: 1;
	UINT32 reserved12: 1;
	UINT32 reserved13: 1;
	UINT32 reserved14: 1;
	//M4
	UINT32 VDEC_0: 1;
	UINT32 VDEC_1_2_3: 1;
	UINT32 DEI: 1;
	UINT32 reserved15: 1;
	UINT32 reserved16: 1;
	UINT32 reserved17: 1;
	UINT32 reserved18: 1;
	UINT32 reserved19: 1;
	//M5
	UINT32 VCAP_0: 1;
	UINT32 VCAP_1: 1;
	UINT32 VCAP_2: 1;
	UINT32 LCD_0: 1;
	UINT32 LCD_1: 1;
	UINT32 GPD: 1;
	UINT32 DAI: 1;
	UINT32 DAI2: 1;
	UINT32 DAI5: 1;
	UINT32 reserved20: 1;
	UINT32 reserved21: 1;
	UINT32 reserved22: 1;
} DMA_CH_MSK, *PDMA_CH_MSK;

/**
    DDR Arbiter ID

*/
typedef enum _DDR_ARB {
        DDR_ARB_1,                           ///< DDR Arbiter
        DDR_ARB_2,                           ///< DDR Arbiter

        DDR_ARB_COUNT,                       //< Arbiter count

        ENUM_DUMMY4WORD(DDR_ARB)
} DDR_ARB;


typedef enum _DMA_WRITEPROT_SET {
        WPSET_0,            // Write protect function set 0
        WPSET_1,            // Write protect function set 1
        WPSET_2,            // Write protect function set 2
        WPSET_3,            // Write protect function set 3
        WPSET_4,            // Write protect function set 4
        OUT_WP,             // Out range write protect
        WPSET_COUNT,
        ENUM_DUMMY4WORD(DMA_WRITEPROT_SET)
} DMA_WRITEPROT_SET;

typedef enum _DMA_WRITEPROT_LEVEL {
	DMA_WPLEL_UNWRITE,      // Not only detect write action but also denial access.
	DMA_WPLEL_DETECT,       // Only detect write action but allow write access.
	DMA_RPLEL_UNREAD,       // Not only detect read action but also denial access.
	DMA_RWPLEL_UNRW,        // Not only detect read write action but also denial access.
	ENUM_DUMMY4WORD(DMA_WRITEPROT_LEVEL)
} DMA_WRITEPROT_LEVEL;

typedef struct _DMA_WRITEPROT_ATTR {
        DMA_CH_MSK          mask;       // DMA channel masks to be protected/detected
        DMA_WRITEPROT_LEVEL level;          // protect level
        uint64_t            starting_addr; // DDR3:must be 4 words alignment
        uint32_t            size;          // DDR3:must be 4 words alignment
} DMA_WRITEPROT_ATTR, *PDMA_WRITEPROT_ATTR;

typedef enum _DMA_CH_GROUP {
	DMA_CH_GROUP0 = 0x0,
	DMA_CH_GROUP1,

	DMA_CH_GROUP_CNT,
	ENUM_DUMMY4WORD(DMA_CH_GROUP)
} DMA_CH_GROUP;

#define MAU_INT_ENABLE_OFS 0x190
REGDEF_BEGIN(MAU_INT_ENABLE_REG)
REGDEF_BIT(ERR_MEM_INT_EN0, 1)
REGDEF_BIT(ERR_MEM_INT_EN1, 1)
REGDEF_BIT(ERR_MEM_INT_EN2, 1)
REGDEF_BIT(ERR_MEM_INT_EN3, 1)
REGDEF_BIT(ERR_MEM_INT_EN4, 1)
REGDEF_BIT(ERR_MEM_INT_EN5, 1)
REGDEF_BIT(ERR_MEM_INT_EN6, 1)
REGDEF_BIT(ERR_MEM_INT_EN7, 1)
REGDEF_BIT(ERR_WT_INT_EN0, 1)
REGDEF_BIT(ERR_WT_INT_EN1, 1)
REGDEF_BIT(ERR_WT_INT_EN2, 1)
REGDEF_BIT(ERR_WT_INT_EN3, 1)
REGDEF_BIT(ERR_WT_INT_EN4, 1)
REGDEF_BIT(ERR_WT_INT_EN5, 1)
REGDEF_BIT(ERR_WT_INT_EN6, 1)
REGDEF_BIT(ERR_WT_INT_EN7, 1)
REGDEF_BIT(READ_TIMEOUT_INT_EN, 1)
REGDEF_BIT(MON_UPDATE_INT_EN, 1)
REGDEF_BIT(, 14)
REGDEF_END(MAU_INT_ENABLE_REG)

#define MAU_INT_STATUS_OFS 0x194
REGDEF_BEGIN(MAU_INT_STATUS_REG)
REGDEF_BIT(ERR_MEM_INT0, 1)
REGDEF_BIT(ERR_MEM_INT1, 1)
REGDEF_BIT(ERR_MEM_INT2, 1)
REGDEF_BIT(ERR_MEM_INT3, 1)
REGDEF_BIT(ERR_MEM_INT4, 1)
REGDEF_BIT(ERR_MEM_INT5, 1)
REGDEF_BIT(ERR_MEM_INT6, 1)
REGDEF_BIT(ERR_MEM_INT7, 1)
REGDEF_BIT(ERR_WT_INT0, 1)
REGDEF_BIT(ERR_WT_INT1, 1)
REGDEF_BIT(ERR_WT_INT2, 1)
REGDEF_BIT(ERR_WT_INT3, 1)
REGDEF_BIT(ERR_WT_INT4, 1)
REGDEF_BIT(ERR_WT_INT5, 1)
REGDEF_BIT(ERR_WT_INT6, 1)
REGDEF_BIT(ERR_WT_INT7, 1)
REGDEF_BIT(READ_TIMEOUT_INT, 1)
REGDEF_BIT(MON_UPDATE_INT, 1)
REGDEF_BIT(, 14)
REGDEF_END(MAU_INT_STATUS_REG)

#define MAU_PROTECT_MODE_OFS 0x48C
REGDEF_BEGIN(MAU_PROTECT_MODE_REG)
REGDEF_BIT(WP0_MODE, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(WP1_MODE, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(WP2_MODE, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(WP3_MODE, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(WP4_MODE, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(OUT_WP_MODE, 2)
REGDEF_BIT(, 10)
REGDEF_END(MAU_PROTECT_MODE_REG)

// Protect
#define MAU_IN_RANGE_PROTECT0_ENABLE_OFS0 0x440
#define MAU_IN_RANGE_PROTECT1_ENABLE_OFS0 0x448
#define MAU_IN_RANGE_PROTECT2_ENABLE_OFS0 0x450
#define MAU_IN_RANGE_PROTECT3_ENABLE_OFS0 0x458
#define MAU_IN_RANGE_PROTECT4_ENABLE_OFS0 0x460
#define MAU_OUT_RANGE_PROTECT_ENABLE_OFS0 0x468

#define MAU_IN_RANGE_PROTECT0_ENABLE_OFS1 0x444
#define MAU_IN_RANGE_PROTECT1_ENABLE_OFS1 0x44C
#define MAU_IN_RANGE_PROTECT2_ENABLE_OFS1 0x454
#define MAU_IN_RANGE_PROTECT3_ENABLE_OFS1 0x45C
#define MAU_IN_RANGE_PROTECT4_ENABLE_OFS1 0x464
#define MAU_OUT_RANGE_PROTECT_ENABLE_OFS1 0x46C

#define MAU_PROTECT0_START_ADDR 	0x490
#define MAU_PROTECT0_END_ADDR 		0x494
#define MAU_PROTECT1_START_ADDR 	0x498
#define MAU_PROTECT1_END_ADDR 		0x49C
#define MAU_PROTECT2_START_ADDR 	0x4A0
#define MAU_PROTECT2_END_ADDR 		0x4A4
#define MAU_PROTECT3_START_ADDR 	0x4A8
#define MAU_PROTECT3_END_ADDR 		0x4AC
#define MAU_PROTECT4_START_ADDR 	0x4B0
#define MAU_PROTECT4_END_ADDR 		0x4B4
#define MAU_OUT_PROTECT_START_ADDR 	0x4B8
#define MAU_OUT_PROTECT_END_ADDR 	0x4BC
#else
typedef enum _DMA_CH_GROUP {
	DMA_CH_GROUP0 = 0x0,    // represent channel 00-31
	DMA_CH_GROUP1,          // represent channel 32-63
	DMA_CH_GROUP2,          // represent channel 64-95
	DMA_CH_GROUP3,          // represent channel 96-127
	DMA_CH_GROUP4,          // represent channel 128-159
	DMA_CH_GROUP5,          // represent channel 160-191
	DMA_CH_GROUP6,          // represent channel 192-223
	DMA_CH_GROUP7,          // represent channel 224-255

	DMA_CH_GROUP_CNT,
	ENUM_DUMMY4WORD(DMA_CH_GROUP)
} DMA_CH_GROUP;

/*
    DMA channel mask

    Indicate which DMA channels are required to protect/detect

    @note For DMA_WRITEPROT_ATTR
*/
typedef struct _DMA_CH_MSK {
	// ch 0
	UINT32 CPU_S: 1;
	UINT32 CPU_NS: 1;
	UINT32 reserved0: 29;
	UINT32 HVYLD: 1;

	// ch 1
	UINT32 SDIO: 1;
	UINT32 SDIO2: 1;
	UINT32 SDIO3: 1;
	UINT32 SMC: 1;
	UINT32 ETH: 1;
	UINT32 ETH2: 1;
	UINT32 USB2: 1;
	UINT32 USB3: 1;
	UINT32 USB3_2: 1;
	UINT32 SATA: 1;
	UINT32 SATA2: 1;
	UINT32 PCIE: 1;
	UINT32 PCIE2: 1;
	UINT32 MSI: 1;
	UINT32 MSI2: 1;
	UINT32 N25: 1;
	UINT32 LARB_1_4_SPI3: 1;
	UINT32 DAI_1_5: 1;
	UINT32 GPE_1_2: 1;
	UINT32 GRP_1_3: 1;
	UINT32 AGE: 1;
	UINT32 TSE: 1;
	UINT32 UVCP_1_2: 1;
	UINT32 XOR: 1;
	UINT32 HWCP: 1;
	UINT32 SCE: 1;
	UINT32 HASH: 1;
	UINT32 reserved1: 4;
	UINT32 HVYLD2: 1;

	// ch 2
	UINT32 VENC_0_1: 1;
	UINT32 VTRC: 1;
	UINT32 reserved2: 29;
	UINT32 HVYLD3: 1;

	// ch 3
	UINT32 IFE: 1;
	UINT32 IFE2: 1;
	UINT32 IPE: 1;
	UINT32 IPE2: 1;
	UINT32 ISE: 1;
	UINT32 ISE2: 1;
	UINT32 IME: 1;
	UINT32 IME2: 1;
	UINT32 DRE: 1;
	UINT32 reserved4: 1;
	UINT32 VDEC_0: 1;
	UINT32 VDEC_1_4: 1;
	UINT32 JPEG: 1;
	UINT32 JPEG2: 1;
	UINT32 JPEG3: 1;
	UINT32 reserved5: 1;
	UINT32 SIE_1: 1;
	UINT32 SIE2_1: 1;
	UINT32 SIE3_1: 1;
	UINT32 SIE4_1: 1;
	UINT32 SIE5_1: 1;
	UINT32 SIE6_1: 1;
	UINT32 SIE7_1: 1;
	UINT32 SIE8_1: 1;
	UINT32 SIE9_1: 1;
	UINT32 SIE10_1: 1;
	UINT32 SIE11_1: 1;
	UINT32 SIE12_1: 1;
	UINT32 reserved6: 4;


	// ch 4
	UINT32 VPE_0_1: 1;
	UINT32 VPE2_0_1: 1;
	UINT32 VPE3_0_1: 1;
	UINT32 reserved7: 29;

	// ch 5
	UINT32 LCD: 1;
	UINT32 LCD2: 1;
	UINT32 LCD3: 1;
	UINT32 CSITX: 1;
	UINT32 CSITX2: 1;
	UINT32 reserved8: 3;
	UINT32 VIE: 1;
	UINT32 VIE2: 1;
	UINT32 reserved9: 6;
	UINT32 SIE_0: 1;
	UINT32 SIE2_0: 1;
	UINT32 SIE3_0: 1;
	UINT32 SIE4_0: 1;
	UINT32 SIE5_0: 1;
	UINT32 SIE6_0: 1;
	UINT32 SIE7_0: 1;
	UINT32 SIE8_0: 1;
	UINT32 SIE9_0: 1;
	UINT32 SIE10_0: 1;
	UINT32 SIE11_0: 1;
	UINT32 SIE12_0: 1;
	UINT32 reserved10: 4;

	// ch 6
	UINT32 GPU: 1;
	UINT32 DSP: 1;
	UINT32 DSP2: 1;
	UINT32 reserved11: 13;
	UINT32 LME: 1;
	UINT32 SDE: 1;
	UINT32 IVE: 1;
	UINT32 MDBC: 1;
	UINT32 TRKE: 1;
	UINT32 NUE2: 1;
	UINT32 reserved12: 10;

	// ch 7
	UINT32 CONV: 1;
	UINT32 CONV2: 1;
	UINT32 CONV3: 1;
	UINT32 CONV4: 1;
	UINT32 reserved13: 12;
	UINT32 JM_JMISP: 1;
	UINT32 UTL: 1;
	UINT32 ROU: 1;
	UINT32 CAL: 1;
	UINT32 LSU: 1;
	UINT32 PPU: 1;
	// backward compatible
	UINT32 DCE: 1;
	UINT32 VENC_0: 1;
	UINT32 VENC_1: 1;
	UINT32 VPE_0: 1;
	UINT32 VPE_1: 1;
	UINT32 NUE: 1;
	UINT32 CNN: 1;
	UINT32 CNN2: 1;
	UINT32 DIS: 1;
	UINT32 reserved14: 1;

} DMA_CH_MSK, *PDMA_CH_MSK;

/**
    DDR Arbiter ID

*/
typedef enum _DDR_ARB {
	DDR_ARB_1,                           ///< DDR Arbiter
	DDR_ARB_2,                           ///< DDR Arbiter2

	DDR_ARB_COUNT,                       //< Arbiter count

	ENUM_DUMMY4WORD(DDR_ARB)
} DDR_ARB;

typedef enum _DMA_WRITEPROT_SET {
	WPSET_0,            // Write protect function set 0
	WPSET_1,            // Write protect function set 1
	WPSET_2,            // Write protect function set 2
	WPSET_3,            // Write protect function set 3
	WPSET_4,            // Write protect function set 4
	WPSET_5,            // Write protect function set 5
	WPSET_COUNT,
	ENUM_DUMMY4WORD(DMA_WRITEPROT_SET)
} DMA_WRITEPROT_SET;

typedef enum _DMA_WRITEPROT_LEVEL {
	DMA_WPLEL_UNWRITE,      // Not only detect write action but also denial access.
	DMA_WPLEL_DETECT,       // Only detect write action but allow write access.
	DMA_RPLEL_UNREAD,       // Not only detect read action but also denial access.
	DMA_RWPLEL_UNRW,        // Not only detect read write action but also denial access.
	ENUM_DUMMY4WORD(DMA_WRITEPROT_LEVEL)
} DMA_WRITEPROT_LEVEL;

typedef enum _DMA_PROT_MODE {
	DMA_PROT_IN,
	DMA_PROT_OUT,
	ENUM_DUMMY4WORD(DMA_PROT_MODE)
} DMA_PROT_MODE;

typedef struct _DMA_PROT_RGN_ATTR {
	BOOL                en;            // enable this region
	u64                 starting_addr; // DDR3:must be 4 words alignment
	u32                 size;          // DDR3:must be 4 words alignment
} DMA_PROT_RGN_ATTR, *PDMA_PROT_RGN_ATTR;

typedef struct _DMA_WRITEPROT_ATTR {
	DMA_CH_MSK          mask;       // DMA channel masks to be protected/detected
	DMA_WRITEPROT_LEVEL level;      // protect level
	DMA_PROT_MODE       protect_mode; // in or out region
	DMA_PROT_RGN_ATTR   protect_rgn_attr;
} DMA_WRITEPROT_ATTR, *PDMA_WRITEPROT_ATTR;

/*
    @name DMA outstanding (NO USE)

    DMA outstanding setting

    @note Used in dma_set_channel_outstanding()
*/
typedef enum {
	DMA_CH_OUTSTANDING_CNN_0,           // CNN input
	DMA_CH_OUTSTANDING_CNN_1,           // CNN input
	DMA_CH_OUTSTANDING_CNN_2,           // CNN input
	DMA_CH_OUTSTANDING_CNN_3,           // CNN input
	DMA_CH_OUTSTANDING_CNN_4,           // CNN output
	DMA_CH_OUTSTANDING_CNN_5,           // CNN output

	DMA_CH_OUTSTANDING_CNN2_0,
	DMA_CH_OUTSTANDING_CNN2_1,
	DMA_CH_OUTSTANDING_CNN2_2,
	DMA_CH_OUTSTANDING_CNN2_3,
	DMA_CH_OUTSTANDING_CNN2_4,
	DMA_CH_OUTSTANDING_CNN2_5,

	DMA_CH_OUTSTANDING_NUE_0,           // NUE input
	DMA_CH_OUTSTANDING_NUE_1,           // NUE input
	DMA_CH_OUTSTANDING_NUE_2,           // NUE output

	DMA_CH_OUTSTANDING_CNN_ALL,
	DMA_CH_OUTSTANDING_CNN2_ALL,
	DMA_CH_OUTSTANDING_NUE_ALL,
	ENUM_DUMMY4WORD(DMA_CH_OUTSTANDING)
} DMA_CH_OUTSTANDING;


typedef enum _DRAM_CONSUME_BW_DEGREE {
	DRAM_CONSUME_EASY_LOADING = 0x0,
	DRAM_CONSUME_NORMAL_LOADING,
	DRAM_CONSUME_HEAVY_LOADING,

	DRAM_CONSUME_CH_DISABLE,

	ENUM_DUMMY4WORD(DRAM_CONSUME_BW_DEGREE)
} DRAM_CONSUME_BW_DEGREE;


typedef struct _DRAM_CONSUME_ATTR {
	DDR_ARB             id;       // Dram id
	DRAM_CONSUME_BW_DEGREE  load_degree;
	u64                     addr;
	u32                     size;
	BOOL                    is_start;
} DRAM_CONSUME_ATTR, *PDRAM_CONSUME_ATTR;

// protect reg
// protect reg
#define DMA_DETECT_CH0_W_REG_OFS   0x00
#define DMA_DETECT_CH1_W_REG_OFS   0x04
#define DMA_DETECT_CH2_W_REG_OFS   0x08
#define DMA_DETECT_CH3_W_REG_OFS   0x0C
#define DMA_DETECT_CH4_W_REG_OFS   0x10
#define DMA_DETECT_CH5_W_REG_OFS   0x14
#define DMA_DETECT_CH6_W_REG_OFS   0x18
#define DMA_DETECT_CH7_W_REG_OFS   0x1C

#define DMA_DETECT_CH0_R_REG_OFS   0x20
#define DMA_DETECT_CH1_R_REG_OFS   0x24
#define DMA_DETECT_CH2_R_REG_OFS   0x28
#define DMA_DETECT_CH3_R_REG_OFS   0x2C
#define DMA_DETECT_CH4_R_REG_OFS   0x30
#define DMA_DETECT_CH5_R_REG_OFS   0x34
#define DMA_DETECT_CH6_R_REG_OFS   0x38
#define DMA_DETECT_CH7_R_REG_OFS   0x3C

#define DMA_PROTECT_CH0_W_REG_OFS   0x40
#define DMA_PROTECT_CH1_W_REG_OFS   0x44
#define DMA_PROTECT_CH2_W_REG_OFS   0x48
#define DMA_PROTECT_CH3_W_REG_OFS   0x4C
#define DMA_PROTECT_CH4_W_REG_OFS   0x50
#define DMA_PROTECT_CH5_W_REG_OFS   0x54
#define DMA_PROTECT_CH6_W_REG_OFS   0x58
#define DMA_PROTECT_CH7_W_REG_OFS   0x5C

#define DMA_PROTECT_CH0_R_REG_OFS   0x60
#define DMA_PROTECT_CH1_R_REG_OFS   0x64
#define DMA_PROTECT_CH2_R_REG_OFS   0x68
#define DMA_PROTECT_CH3_R_REG_OFS   0x6C
#define DMA_PROTECT_CH4_R_REG_OFS   0x70
#define DMA_PROTECT_CH5_R_REG_OFS   0x74
#define DMA_PROTECT_CH6_R_REG_OFS   0x78
#define DMA_PROTECT_CH7_R_REG_OFS   0x7C

#define DMA_RANGE_SEL_REG_OFS       0x80
REGDEF_BEGIN(DMA_RANGE_SEL_REG)
REGDEF_BIT(range_sel_w, 8)
REGDEF_BIT(, 8)
REGDEF_BIT(range_sel_r, 8)
REGDEF_BIT(, 8)
REGDEF_END(DMA_RANGE_SEL_REG)

#define DMA_PROTECT_START_ADDR_REG_OFS      0x88
#define DMA_PROTECT_END_ADDR_REG_OFS        0x8C
#endif

#endif /* _ASM_ARMV8_GIC_H_ */
