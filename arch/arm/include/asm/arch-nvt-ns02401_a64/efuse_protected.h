/*
    Novatek protected header file of NT96660's driver.

    The header file for Novatek protected APIs of NT96660's driver.

    @file       efuse_protected.h
    @ingroup    mIDriver
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef __ARCH_EFUSE_PROTECTED_H
#define __ARCH_EFUSE_PROTECTED_H

#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/rcw_macro.h>

typedef enum {
	TRIM_1ST_FIELD  = 0x0,
	TRIM_2ND_FIELD,
} TRIM_FIELD_SET;

typedef enum {
	PWM0_CORE_POWER     = 0x0,
	PWM1_CPU_POWER,
} PWM_POWER_SET;

typedef enum {
	LEAKAGE_0,
	LEAKAGE_1,
	LEAKAGE_2,
	LEAKAGE_3,
	LEAKAGE_4,
	LEAKAGE_5,
	LEAKAGE_6,
	LEAKAGE_7,
	LEAKAGE_8,
	LEAKAGE_9,
	LEAKAGE_10,
	LEAKAGE_11,
	LEAKAGE_12,
	LEAKAGE_13,
	LEAKAGE_14,
	LEAKAGE_15,
	LEAKAGE_16,
	LEAKAGE_COUNT,

	LEAKAGE_MAX = 255,
} PWM_POWER_LEAKAGE;

typedef enum {
	NT98635_PKG = 0x00,                         // NT98635   package version
	NT98635_I_PKG,                              // NT98635-I (WT) package version
	NT98635_Q_PKG,                              // NT98635-Q (AECQ100) package version

	NT98635_E_PKG,                            	// NT98635_E package version
	NT98635_E_I_PKG,                            // NT98635_E-I (WT) package version
	NT98635_E_Q_PKG,                            // NT98635_E-Q (AECQ100) package version

	NT98XXX_PKG_CNT = 6,
	IC_PKG_CNT      = NT98XXX_PKG_CNT,
	UNKNOWN_PKG_VER,
} NVT_PKG_UID;

typedef enum {
	SCE_NO_0_KEY_SET_FIELD = 0x0,
	SCE_NO_1_KEY_SET_FIELD,
	SCE_NO_2_KEY_SET_FIELD,
	SCE_NO_3_KEY_SET_FIELD,
	SCE_NO_4_KEY_SET_FIELD,
	SCE_NO_5_KEY_SET_FIELD,
	SCE_NO_6_KEY_SET_FIELD,
	SCE_NO_7_KEY_SET_FIELD,


	SCE_NO_8_KEY_SET_FIELD,
	SCE_NO_9_KEY_SET_FIELD,
	SCE_NO_10_KEY_SET_FIELD,
	SCE_NO_11_KEY_SET_FIELD,

	SCE_NO_12_KEY_SET_FIELD,
	SCE_NO_13_KEY_SET_FIELD,
	SCE_NO_14_KEY_SET_FIELD,
	SCE_NO_15_KEY_SET_FIELD,

	EFUSE_TOTAL_KEY_SET_FIELD,

	EFUSE_CRYPTO_ENGINE_KEY_CNT = SCE_NO_8_KEY_SET_FIELD,
	EFUSE_RSA_ENGINE_KEY_CNT = EFUSE_TOTAL_KEY_SET_FIELD,
	EFUSE_HASH_ENGINE_KEY_CNT = EFUSE_TOTAL_KEY_SET_FIELD,

	ENUM_DUMMY4WORD(SCE_KEY_SET_TO_OTP_FIELD)
} SCE_KEY_SET_TO_OTP_FIELD;


typedef enum {
	EFUSE_OTP_1ST_KEY_SET_FIELD     = 0x0,        // This if for secure boot
	EFUSE_OTP_2ND_KEY_SET_FIELD     = 0x1,
	EFUSE_OTP_3RD_KEY_SET_FIELD     = 0x2,
	EFUSE_OTP_4TH_KEY_SET_FIELD     = 0x3,
	EFUSE_OTP_5TH_KEY_SET_FIELD     = 0x4,
	EFUSE_OTP_TOTAL_KEY_SET_FIELD   = 0x5,
} EFUSE_OTP_KEY_SET_FIELD;

STATIC_ASSERT(EFUSE_OTP_TOTAL_KEY_SET_FIELD <= 5);

#define EFUSE_OTP_KEY_FIELD_CNT         4

typedef enum {
	OTP_KEY_MANAGER_NONE = 0x0,               			// Not process key manager
	OTP_KEY_MANAGER_CRYPTO,                   			// Key manager, destination is crypto engine(SCE)
	OTP_KEY_MANAGER_RSA,                      			// Key manager, destination is RSA engine
	OTP_KEY_MANAGER_HASH,                     			// Key manager, destination is HASH engine
	OTP_KEY_MANAGER_CNT,
} OTP_KEY_DESTINATION;

#define KEY_MANAGER_TIMING_CONFIG_ADDRESS      			(IOADDR_EFUSE_REG_BASE + 0x10)
#define KEY_MANAGER_DESTINATION_ADDRESS        			(IOADDR_EFUSE_REG_BASE + 0x14)
#define KEY_MANAGER_CONTROL_ADDRESS            			(IOADDR_EFUSE_REG_BASE + 0x20)
#define KEY_MANAGER_STATUS_ADDRESS             			(IOADDR_EFUSE_REG_BASE + 0x30)
#define KEY_MANAGER_KEY_ENGINE_RIGHT_ADDRESS   			(IOADDR_EFUSE_REG_BASE + 0x40)
#define KEY_MANAGER_KEY_ENGINE_RIGHT2_ADDRESS  			(IOADDR_EFUSE_REG_BASE + 0x44)

#define KEY_MANAGER2_TIMING_CONFIG_ADDRESS     			(IOADDR_EFUSE2_REG_BASE + 0x10)
#define KEY_MANAGER2_DESTINATION_ADDRESS       			(IOADDR_EFUSE2_REG_BASE + 0x14)
#define KEY_MANAGER2_CONTROL_ADDRESS           			(IOADDR_EFUSE2_REG_BASE + 0x20)
#define KEY_MANAGER2_STATUS_ADDRESS            			(IOADDR_EFUSE2_REG_BASE + 0x30)
#define KEY_MANAGER2_KEY_ENGINE_RIGHT_ADDRESS  			(IOADDR_EFUSE2_REG_BASE + 0x40)
#define KEY_MANAGER2_KEY_ENGINE_RIGHT2_ADDRESS 			(IOADDR_EFUSE2_REG_BASE + 0x44)
#define KEY_MANAGER2_VERSION_ADDRESS					(IOADDR_EFUSE2_REG_BASE + 0x8C)	//0x70~0x8C = user usage register use efuse2's 0x8C as version control
																						//bit[ 3..0] =>   LDR version
																						//bit[10..4] =>   atf version
																						//bit[17..11]=> optee version
																						//bit[24..18]=> uboot version
																						//bit[31..25]=> linux version

#define OTP_ATF_VERSION_OFS								4
#define OTP_OPTEE_VERSION_OFS				    		11
#define OTP_UBOOT_VERSION_OFS				    		18
#define OTP_LINUX_VERSION_OFS				    		25

#define OTP_ATF_VERSION_MASK							0x3F
#define OTP_OPTEE_VERSION_MASK							0x3F
#define OTP_UBOOT_VERSION_MASK							0x3F

#define WIDE_TEMPERATURE_VALID_BIT          			12
#define AECQ100_VALID_BIT                   			13



#define SOC_WIDE_TEMPERATURE_ENABLE         			(1<<WIDE_TEMPERATURE_VALID_BIT)
#define SOC_WIDE_TEMPERATURE_DISABLE        			(0<<WIDE_TEMPERATURE_VALID_BIT)

#define SOC_AECQ100_ENABLE                  			(1<<AECQ100_VALID_BIT)
#define SOC_AECQ100_DISABLE                 			(0<<AECQ100_VALID_BIT)

#define EFUSE_TRIM_DATA_SATA1_PRI                    		0x0    		///<bit[ 4..0]=>SATA1 internal 12K resistor
																		///<bit[ 8..5]=>SATA1 TX term trim
																		///<bit[12..9]=>SATA1 RX term trim
#define EFUSE_TRIM_DATA_HDMI_2_0_H_PHY_VL_TRIM_PRI   		0x1			///<bit[11..0]=>HDMI2.0 H-phy VL trim
#define EFUSE_TRIM_DATA_DDR_DRIVING_PRI              		0x2			///<bit [4..0]=>rzq(H)
																		///<bit [9..5]=>rzq(V)
#define	EFUSE_TRIM_DATA_ADC_PRI								0x3
#define OTP_PKGUID_FIELD                             		0x4
#define EFUSE_TRIM_DATA_SATA3_USB3_1_PRI             		0x5     	///<bit[4..0]=>USB3_1(SATA3) internal 12K resistor
																		///<bit[8..5]=>USB3_1(SATA3) TX trim
																		///<bit[12..9]=>USB3_1(SATA3) RX trim
																		///<bit[13]=>USB3_1(SATA3) RX LDO_Output

#define EFUSE_TRIM_DATA_THERMAL_PRI                  		0x6     	///<bit[ 7..0]=>T-sensor trim
#define EFUSE_TRIM_DATA_SATA2_PRI                    		0x7     	///<bit[ 4..0]=>SATA2 internal 12K resistor
																		///<bit[ 8..5]=>SATA2 TX trim
																		///<bit[12..9]=>SATA2 RX trim
																		///<   bit[13]=>SATA2 RX LDO_Output

#define EFUSE_TRIM_PCIE_PHY0_ODC_PRI						0x24
#define EFUSE_TRIM_PCIE_PHY1_ODC_PRI						0x25
#define EFUSE_TRIM_PCIE_PHY2_ODC_PRI						0x26
#define EFUSE_TRIM_PCIE_PHY3_ODC_PRI						0x27

#define EFUSE_TRIM_HDMI2_1_DCC_LANE0_8b_10G_PRI				0x28
#define EFUSE_TRIM_HDMI2_1_DCC_LANE1_8b_10G_PRI				0x29
#define EFUSE_TRIM_HDMI2_1_DCC_LANE2_8b_10G_PRI				0x2A
#define EFUSE_TRIM_HDMI2_1_DCC_LANE3_8b_10G_PRI				0x2B

#define EFUSE_TRIM_PCIe0_ODT_PRI                     		0x2C    	///<PCIe0
#define EFUSE_TRIM_PCIe1_ODT_PRI                     		0x2D    	///<PCIe1
#define EFUSE_TRIM_PCIe2_ODT_PRI                     		0x2E    	///<PCIe2
#define EFUSE_TRIM_DATA_HDMI_1_4_VL_TRIM_PRI         		0x2F    	///<bit[11..0]=>HDMI1.4 VL trim
#define EFUSE_TRIM_PCIe3_ODT_PRI                     		0x30    	//PCIe3

#define EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE0_8G_PRI			0x31		///<HDMI_2_1_TERM_DCC_LANE0
#define EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE1_8G_PRI			0x32		///<HDMI_2_1_TERM_DCC_LANE1
#define EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE2_8G_PRI			0x33		///<HDMI_2_1_TERM_DCC_LANE2
#define EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE3_8G_PRI			0x34		///<HDMI_2_1_TERM_DCC_LANE3

#define EFUSE_TRIM_HDMI_2_1_EACH_LANE_FINE_TUNE_VL_1_4_PRI	0x35
#define EFUSE_TRIM_HDMI_2_1_EACH_LANE_FINE_TUNE_VL_2_0_PRI	0x36


#define EFUSE_TRIM_DATA_HDMI_2_RESISTOR_PRI					0x37		///<2nd HDMI embeded resistor
																		///< bit[4..0]:2nd Embedded Resistor trim for HDMI 1.4
																		///< bit[9..5]:2nd Embedded Resistor trim for HDMI 2.0


#define EFUSE_TRIM_DATA_HDMI_2_0_VLANE_1_4_PRI				0x38		///< bit[11..0] : Each 2.0 HDMI's (1.4) lane fine tune VL trim

#define EFUSE_TRIM_DATA_HDMI_2_0_VLANE_2_0_PRI				0x39		///< bit[11..0] : Each 2.0 HDMI's (2.0) lane fine tune VL trim

#define EFUSE_TRIM_DATA_HDMI_RESISTOR_2_1_PRI				0x3A		///<HDMI embeded resistor
																		///< bit[4..0]:Embedded Resistor trim for HDMI 2.1(TMDS)
																		///< bit[9..5]:Embedded Resistor trim for HDMI 2.0(FRL)

#define EFUSE_TRIM_DATA_SATA3_SGMII_2_PRI					0x3B		///<0x13B
																		///< SATA3_SGMII_2
																		///<	bit[4..0] : SATA3.0_SGMII_2_internal 12K trimming
																		///< 	bit[8..5] : SATA3.0_SGMII_2_TX_TRIM
																		///<  bit[12..9]: SATA3.0_SGMII_2_RX_TRIM
																		///<  bit[13]	  : SATA3.0_SGMII_2_RX_LDO_Output

#define EFUSE_TRIM_DATA_SATA3_SGMII_1_PRI					0x3C		//0x13C
																		///< SATA3_SGMII_1
																		///< 	bit[4..0] : SATA3.0_SGMII_1_internal 12K trimming
																		///< 	bit[8..5] : SATA3.0_SGMII_1_TX_TRIM
																		///<  bit[12..9]: SATA3.0_SGMII_1_RX_TRIM
																		///<  bit[13]	  : SATA3.0_SGMII_1_RX_LDO_Output

#define EFUSE_TRIM_DATA_SATA_3_USB3_2_PRI					0x3D		//0x13D
																		///< SATA3.0_USB3_2
																		///< 	bit[4..0] : SATA3_USB3_2_internal 12K trimming
																		///< 	bit[8..5] : SATA3_USB3_2_TX_TRIM
																		///<  bit[12..9]: SATA3_USB3_2_RX_TRIM
																		///<  bit[13]	  : SATA3_USB3_2_RX LDO_Output




#define EFUSE_TRIM_DATA_HDMI_1_RESISTOR_PRI					0x3E		//1st HDMI embeded resistor
																		///<  bit[4..0]:1st Embedded Resistor trim for HDMI 1.4
																		///<  bit[9..5]:1st Embedded Resistor trim for HDMI 2.0


#define OTP_635_TRIM                            			0x3C1

/**
    Crypto engine check
*/
typedef enum {
	SECUREBOOT_SECURE_EN = 0x00,        ///< Quary if secure enable or not
	SECUREBOOT_DATA_AREA_ENCRYPT,       ///< Quary if data area encrypt to cypher text or not
	SECUREBOOT_SIGN_RSA,                ///< Quary if Signature methed is RSA or not(AES)
	SECUREBOOT_SIGN_RSA_CHK,            ///< Quary if Signature hash checksum RSA key correct or not
	SECUREBOOT_JTAG_DISABLE_EN,         ///< Quary if JTAG is disable or not(TRUE : disable)
	SECUREBOOT_SIGN_ECDSA,              ///< Quary if Signature methed is ECDSA (once RSA ECDSA enable at the same time=> ECDSA priority high)

	SECUREBOOT_1ST_KEY_SET_PROGRAMMED,  ///< Quary if 1st key set programmed or not
	SECUREBOOT_2ND_KEY_SET_PROGRAMMED,  ///< Quary if 2nd key set programmed or not
	SECUREBOOT_3RD_KEY_SET_PROGRAMMED,  ///< Quary if 3rd key set programmed or not
	SECUREBOOT_4TH_KEY_SET_PROGRAMMED,  ///< Quary if 4th key set programmed or not
	SECUREBOOT_5TH_KEY_SET_PROGRAMMED,  ///< Quary if 5th key set programmed or not

	SECUREBOOT_READ_LOCK_KEY_SET_START,
	SECUREBOOT_1ST_KEY_SET_READ_LOCK = SECUREBOOT_READ_LOCK_KEY_SET_START,  ///< Quary if 1st key read lock already or not
	SECUREBOOT_2ND_KEY_SET_READ_LOCK,                                       ///< Quary if 2nd key read lock already or not
	SECUREBOOT_3RD_KEY_SET_READ_LOCK,                                       ///< Quary if 3rd key read lock already or not
	SECUREBOOT_4TH_KEY_SET_READ_LOCK,                                       ///< Quary if 4th key read lock already or not
	SECUREBOOT_5TH_KEY_SET_READ_LOCK,                                       ///< Quary if 5th key read lock already or not
	SECUREBOOT_READ_LOCK_KEY_SET_END = SECUREBOOT_5TH_KEY_SET_READ_LOCK,

	SECUREBOOT_HDMI_AVAILABLE,			///< Quary HDMI available
	SECUREBOOT_GPU_DISABLE,
	SECUREBOOT_STATUS_NUM,

	SECUREBOOT_CPU_CA55_OVER_CLOCKING,  ///< Quary if CPU can over clocking or not
	SECUREBOOT_CPU_CA76_OVER_CLOCKING,  ///< Quary if CPU can over clocking or not
	SECUREBOOT_CPU_L3_OVER_CLOCKING,	///< Quary if CPU can over clocking or not

	SECUREBOOT_CONV_OVER_CLOCKING,      ///< Quary if CONV can over clocking or not
	SECUREBOOT_DDR_OVER_CLOCKING,       ///< Quary if CPU can over clocking or not
	SECUREBOOT_VENC_OVER_CLOCKING,      ///< Quary if CONV can over clocking or not
	SECUREBOOT_VDEC_OVER_CLOCKING,      ///< Quary if VDEC can over clocking or not


	SECUREBOOT_SIGN_RSA_PSS,            ///RSAPKCS2.1
	SECUREBOOT_SIGN_RSA_PKCS1P5,        ///RSAPKCS1.5
	SECUREBOOT_HEADER_SIGNATURE,
} SECUREBOOT_STATUS;


/*
    @name eFuse enum - base address
*/
//@{
typedef enum {
	EFUSE_ID_0 = 0,     ///< efuse ID 0
	EFUSE_ID_1 = 1,     ///< efuse ID 1
	ENUM_DUMMY4WORD(EFUSE_ENGINE)
} EFUSE_ENGINE;
//@}
#ifndef TRUE
#define TRUE                                1
#endif
#ifndef FALSE
#define FALSE                               0
#endif
#define EFUSE_SUCCESS                 		E_OK
#define EFUSE_FREEZE_ERR              		-1001          // Programmed already, only can read
#define EFUSE_INACTIVE_ERR            		-1002          // This field is empty(not programmed yet)
#define EFUSE_INVALIDATE_ERR          		-1003          // This field force invalidate already
#define EFUSE_UNKNOW_PARAM_ERR        		-1004          // efuse param field not defined
#define EFUSE_OPS_ERR                 		-1005          // efuse operation error
#define EFUSE_SECURITY_ERR            		-1006          // efuse under security mode => can not read back
#define EFUSE_PARAM_ERR               		-1007          // efuse param error
#define EFUSE_CONTENT_ERR               	-1008          // efuse operation error
#define EFUSE_POWER_ON_OFF_CALLBACK_ERR		-1009			// efuse program power callback not define
#define OTP_HW_SECURE_EN                	(1 << 0)
#define OTP_FW_SECURE_EN              		(1 << 5)
#define OTP_DATA_ENCRYPT_EN           		(1 << 7)
#define OTP_SIGNATURE_RSA             		(1 << 1)
#define OTP_SIGNATURE_RSA_CHK_EN      		(1 << 3)
#define OTP_SIGNATURE_ECDSA_CHKSUM_EN 		OTP_SIGNATURE_RSA_CHK_EN
#define OTP_JTAG_DISABLE_EN           		(1 << 2)
#define EFUSE_HDMI_USAGE_BIT          		11
#define EFUSE_HDMI_USAGE_BIT_MASK     		(0x7<<11)

#define OTP_1ST_KEY_PROGRAMMED_BIT    		27
#define OTP_2ND_KEY_PROGRAMMED_BIT    		28
#define OTP_3RD_KEY_PROGRAMMED_BIT    		29
#define OTP_4TH_KEY_PROGRAMMED_BIT    		30
#define OTP_5TH_KEY_PROGRAMMED_BIT    		31

#define OTP_GPU_DISABLE_BIT           		16		//[9][17][16]

#define GPU_1ST_AVAILABLE					0
#define GPU_1ST_DISABLE						0x10000
#define GPU_2ND_AVAILABLE					0x30000
#define GPU_2ND_DISABLE						0x30200
#define EFUSE_GPU_USAGE_BIT_MASK			0x30200

//2nd
#define OTP_CPU_OVER_CLOCKING_BIT     		10
#define OTP_VDEC_OVER_CLOCKING_BIT    		8
#define OTP_CONV_OVER_CLOCKING_BIT    		7

#define OTP_SIGNATURE_RSAPKCS1P5_BIT  		16
#define OTP_HEADER_SIGNATURE_BIT      		17
#define OTP_SIGNATURE_RSA_OTHERS_BIT  		19
#define OTP_SIGNATURE_ECDSA_BIT       		20

#define HDMI_1ST_AVAILABLE         			(0)
#define HDMI_1ST_DISABLE           			(0x1<<EFUSE_HDMI_USAGE_BIT)
#define HDMI_2ND_AVAILABLE         			(0x3<<EFUSE_HDMI_USAGE_BIT)
#define HDMI_2ND_DISABLE           			(0x7<<EFUSE_HDMI_USAGE_BIT)

#define OTP_1ST_KEY_PROGRAMMED     			(1 << OTP_1ST_KEY_PROGRAMMED_BIT)
#define OTP_2ND_KEY_PROGRAMMED     			(1 << OTP_2ND_KEY_PROGRAMMED_BIT)
#define OTP_3RD_KEY_PROGRAMMED     			(1 << OTP_3RD_KEY_PROGRAMMED_BIT)
#define OTP_4TH_KEY_PROGRAMMED     			(1 << OTP_4TH_KEY_PROGRAMMED_BIT)
#define OTP_5TH_KEY_PROGRAMMED     			(1 << OTP_5TH_KEY_PROGRAMMED_BIT)

//2nd => addr[9]
#define OTP_SIGNATURE_RSAPKCS1P5        	(1 << OTP_SIGNATURE_RSAPKCS1P5_BIT) //bit[16]
#define OTP_HEADER_SIGNATURE_EN         	(1 << OTP_HEADER_SIGNATURE_BIT)     //bit[17]
#define OTP_SIGNATURE_RSA_OTHERS        	(1 << OTP_SIGNATURE_RSA_OTHERS_BIT) //bit[19]
#define OTP_SIGNATURE_ECDSA             	(1 << OTP_SIGNATURE_ECDSA_BIT)      //bit[20]

#define OTP_CPU_OVER_CLOCKING           	(1 << OTP_CPU_OVER_CLOCKING_BIT)	//bit[10]
#define OTP_VDEC_OVER_CLOCKING_AVL      	(1 << OTP_VDEC_OVER_CLOCKING_BIT)	//bit[8]
#define OTP_CONV_OVER_CLOCKING          	(1 << OTP_CONV_OVER_CLOCKING_BIT)	//bit[7]

#define OTP_1ST_KEY_READ_LOCK_BIT       	22
#define OTP_2ND_KEY_READ_LOCK_BIT       	23
#define OTP_3RD_KEY_READ_LOCK_BIT       	24
#define OTP_4TH_KEY_READ_LOCK_BIT       	25
#define OTP_5TH_KEY_READ_LOCK_BIT       	26

#define OTP_1ST_KEY_READ_LOCK           	(1 << OTP_1ST_KEY_READ_LOCK_BIT)
#define OTP_2ND_KEY_READ_LOCK           	(1 << OTP_2ND_KEY_READ_LOCK_BIT)
#define OTP_3RD_KEY_READ_LOCK           	(1 << OTP_3RD_KEY_READ_LOCK_BIT)
#define OTP_4TH_KEY_READ_LOCK           	(1 << OTP_4TH_KEY_READ_LOCK_BIT)
#define OTP_5TH_KEY_READ_LOCK           	(1 << OTP_5TH_KEY_READ_LOCK_BIT)



#define OTP_1ST_KEY_BIT_START           	(0xF << 16)
#define OTP_2ND_KEY_BIT_START           	(0xF << 20)
#define OTP_3RD_KEY_BIT_START           	(0xF << 24)
#define OTP_4TH_KEY_BIT_START           	(0xF << 28)
#define OTP_5TH_KEY_BIT_START           	(0xF << 12)

#define OTP_GPU_DISABLE						(1 << OTP_GPU_DISABLE_BIT)

#define is_secure_enable()                  quary_secure_boot(SECUREBOOT_SECURE_EN)
#define is_data_area_encrypted()            quary_secure_boot(SECUREBOOT_DATA_AREA_ENCRYPT)

#define is_signature_rsa()                  quary_secure_boot(SECUREBOOT_SIGN_RSA)
#define is_signature_rsapss()               quary_secure_boot(SECUREBOOT_SIGN_RSA_PSS)
#define is_signature_rsa_chsum_enable()     quary_secure_boot(SECUREBOOT_SIGN_RSA_CHK)
#define is_signature_rsapkcs1p5()           quary_secure_boot(SECUREBOOT_SIGN_RSA_PKCS1P5)

#define is_signature_aes()                  !quary_secure_boot(SECUREBOOT_SIGN_RSA)
#define is_JTAG_DISABLE_en()                quary_secure_boot(SECUREBOOT_JTAG_DISABLE_EN)




#define is_signature_ecdsa()                quary_secure_boot(SECUREBOOT_SIGN_ECDSA)
#define is_signature_chsum_enable()         quary_secure_boot(SECUREBOOT_SIGN_RSA_CHK)
#define is_header_signature_en()            quary_secure_boot(SECUREBOOT_HEADER_SIGNATURE)




#define is_1st_key_programmed()             quary_secure_boot(SECUREBOOT_1ST_KEY_SET_PROGRAMMED)
#define is_2nd_key_programmed()             quary_secure_boot(SECUREBOOT_2ND_KEY_SET_PROGRAMMED)
#define is_3rd_key_programmed()             quary_secure_boot(SECUREBOOT_3RD_KEY_SET_PROGRAMMED)
#define is_4th_key_programmed()             quary_secure_boot(SECUREBOOT_4TH_KEY_SET_PROGRAMMED)
#define is_5th_key_programmed()             quary_secure_boot(SECUREBOOT_5TH_KEY_SET_PROGRAMMED)


#define is_1st_key_read_lock()              quary_secure_boot(SECUREBOOT_1ST_KEY_SET_READ_LOCK)
#define is_2nd_key_read_lock()              quary_secure_boot(SECUREBOOT_2ND_KEY_SET_READ_LOCK)
#define is_3rd_key_read_lock()              quary_secure_boot(SECUREBOOT_3RD_KEY_SET_READ_LOCK)
#define is_4th_key_read_lock()              quary_secure_boot(SECUREBOOT_4TH_KEY_SET_READ_LOCK)
#define is_5th_key_read_lock()              quary_secure_boot(SECUREBOOT_5TH_KEY_SET_READ_LOCK)
#define is_hdmi_available()                 quary_secure_boot(SECUREBOOT_HDMI_AVAILABLE)
#define is_DSP1_available()                 !quary_secure_boot(SECUREBOOT_DSP1_DISABLE)
#define is_DSP2_available()                 !quary_secure_boot(SECUREBOOT_DSP2_DISABLE)
#define is_GPU_available()                  !quary_secure_boot(SECUREBOOT_GPU_DISABLE)
#define is_MCU_available()                  !quary_secure_boot(SECUREBOOT_MCU_DISABLE)

#define is_hdmi_disable()                   !quary_secure_boot(SECUREBOOT_HDMI_AVAILABLE)
#define is_DSP1_disable()                   quary_secure_boot(SECUREBOOT_DSP1_DISABLE)
#define is_DSP2_disable()                   quary_secure_boot(SECUREBOOT_DSP2_DISABLE)
#define is_GPU_disable()                    quary_secure_boot(SECUREBOOT_GPU_DISABLE)
#define is_MCU_disable()                    quary_secure_boot(SECUREBOOT_MCU_DISABLE)

extern s32  otp_set_key_read_lock(EFUSE_OTP_KEY_SET_FIELD key_set_index);
extern s32  otp_set_key_destination(EFUSE_OTP_KEY_SET_FIELD key_set_index);
extern s32  otp_set_key_engine_access_right(EFUSE_OTP_KEY_SET_FIELD key_set_index);
extern s32  otp_write_key(EFUSE_OTP_KEY_SET_FIELD key_set_index, UINT8 *uc_key);
extern BOOL quary_secure_boot(SECUREBOOT_STATUS scu_status);
extern BOOL enable_secure_boot(SECUREBOOT_STATUS scu_status);
extern u32  otp_key_manager(EFUSE_ENGINE id, u32 rowAddress);
extern BOOL extract_trim_valid(u32 code, u32 *pData);

extern ER   efuse_get_unique_id(UINT32 *id_L, UINT32 *id_H);
extern void otp_init(void);

extern u32  extract_trim_to_pwm_duty(PWM_POWER_SET power_set, UINT32 data);
extern int  prepare_power_trim(int mode);
extern INT32 otp_get_pkg_version(void);
/**
    efuse_secure_en

    Enable secure boot
*/
#define efuse_secure_en()               	enable_secure_boot(SECUREBOOT_SECURE_EN)
/**
    efuse_jtag_dis

    Disable JTAG

    @Note: Can not re enable once disabled
*/
#define efuse_jtag_dis()                	enable_secure_boot(SECUREBOOT_JTAG_DISABLE_EN)
#define efuse_data_area_encrypt_en()    	enable_secure_boot(SECUREBOOT_DATA_AREA_ENCRYPT)
#define efuse_signature_rsa_en()        	enable_secure_boot(SECUREBOOT_SIGN_RSA)
#define efuse_signature_rsa_chksum_en() 	enable_secure_boot(SECUREBOOT_SIGN_RSA_CHK)

#define efuse_header_signature_en()     	enable_secure_boot(SECUREBOOT_HEADER_SIGNATURE)
#define efuse_signature_rsapss_en()     	enable_secure_boot(SECUREBOOT_SIGN_RSA_PSS)
#define efuse_signature_rsapkcs1p5_en() 	enable_secure_boot(SECUREBOOT_SIGN_RSA_PKCS1P5)

/**
    otp_signature_ecdsa_en

    ROM treat loader by use ECDSA as loader's signature
*/
#define efuse_signature_ecdsa_en()          enable_secure_boot(SECUREBOOT_SIGN_ECDSA)

/**
    otp_signature_ecdsa_chksum_en

    Once use ECDSA as signature, enable ECDSA public checksum(use SHA256) enable
*/
#define efuse_signature_chksum_en()         enable_secure_boot(SECUREBOOT_SIGN_RSA_CHK)

#endif /* __ARCH_EFUSE_PROTECTED_H */