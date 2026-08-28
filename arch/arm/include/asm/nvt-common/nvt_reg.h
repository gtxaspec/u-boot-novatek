#ifndef __NVT_REG_H__
#define __NVT_REG_H__

#if defined(CONFIG_TARGET_NA51055)
    #include <asm/arch/na51055_regs.h>
#elif defined(CONFIG_TARGET_NA51089)
    #include <asm/arch/na51089_regs.h>
#elif defined(CONFIG_TARGET_NA51090) || defined(CONFIG_TARGET_NA51090_A64)
    #include <asm/arch/na51090_regs.h>
#elif defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64)
    #include <asm/arch/na51102_regs.h>
#elif defined(CONFIG_TARGET_NA51103) || defined(CONFIG_TARGET_NA51103_A64)
    #include <asm/arch/na51103_regs.h>
#elif defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64)
    #include <asm/arch/ns02201_regs.h>
#elif defined(CONFIG_TARGET_NS02301)
    #include <asm/arch/ns02301_regs.h>
#elif defined(CONFIG_TARGET_NS02302_A64)
    #include <asm/arch/ns02302_regs.h>
#elif defined(CONFIG_TARGET_NS02401_A64) 
    #include <asm/arch/ns02401_regs.h>
#elif defined(CONFIG_TARGET_NS02401_V0_A64)
    #include <asm/arch/ns02401_v0_regs.h>
#elif defined(CONFIG_TARGET_NS02501)
    #include <asm/arch/ns02501_regs.h>
#endif

#endif /* __NVT_REG_H__ */
