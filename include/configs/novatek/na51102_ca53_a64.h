/* NVT board configuration */
/* FLASH        FileSystem */
#if defined(CONFIG_NVT_SPI_NAND) || defined(CONFIG_NVT_SPI_NONE)
#define CONFIG_SYS_MAX_NAND_DEVICE              1
#define CONFIG_SYS_NAND_MAX_CHIPS               1
#endif

#ifdef CONFIG_NVT_FPGA_EMULATION
#define CONFIG_SYS_HZ_CLOCK			24000000
#else /* !CONFIG_NVT_FPGA_EMULATION */
#define CONFIG_SYS_HZ_CLOCK			12000000
#endif /* CONFIG_NVT_FPGA_EMULATION */


/*
 * Serial driver init.
 *
 */
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_MEM32
#define CONFIG_SYS_NS16550_REG_SIZE			-1
#define CONFIG_SYS_NS16550_COM1                         0x2f0290000
#define CONFIG_SYS_NS16550_COM2                         0x2f0300000
#define CONFIG_SYS_NS16550_COM3                         0x2f0310000
#define CONFIG_SYS_NS16550_COM4                         0x2f0380000
#define CONFIG_SYS_NS16550_COM5                         0x2f03E0000
#ifdef CONFIG_NVT_FPGA_EMULATION
#define CONFIG_SYS_NS16550_CLK                          24000000        /* COM1: 24M, other COM port 48M */
#else
#define CONFIG_SYS_NS16550_CLK                          48000000        /* COM1: 24M, other COM port 48M */
#endif
