#ifndef __NVT_PINMUX_H__
#define __NVT_PINMUX_H__

#if defined(CONFIG_TARGET_NA51055)
    #include <asm/arch/na51055_pinmux.h>
#elif defined(CONFIG_TARGET_NA51089)
    #include <asm/arch/na51089_pinmux.h>
#elif defined(CONFIG_TARGET_NA51090) || defined(CONFIG_TARGET_NA51090_A64)
    #include <asm/arch/na51090_pinmux.h>
#elif defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64)
    #include <asm/arch/na51102_pinmux.h>
#elif defined(CONFIG_TARGET_NA51103) || defined(CONFIG_TARGET_NA51103_A64)
    #include <asm/arch/na51103_pinmux.h>
#elif defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64)
    #include <asm/arch/ns02201_pinmux.h>
#elif defined(CONFIG_TARGET_NS02301)
    #include <asm/arch/ns02301_pinmux.h>
#elif defined(CONFIG_TARGET_NS02302_A64)
    #include <asm/arch/ns02302_pinmux.h>
#elif defined(CONFIG_TARGET_NS02401_A64)
    #include <asm/arch/ns02401_pinmux.h>
#elif defined(CONFIG_TARGET_NS02401_V0_A64)
    #include <asm/arch/ns02401_v0_pinmux.h>
#elif defined(CONFIG_TARGET_NS02501)
    #include <asm/arch/ns02501_pinmux.h>
#endif


#endif /* __NVT_PINMUX_H__ */
